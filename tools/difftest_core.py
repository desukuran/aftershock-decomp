#!/usr/bin/env python3
"""Differential testing, the shared part: run a function of the original
default.xbe and the same function of our rebuild side by side in Unicorn,
from the same game state, and compare what they do.

A game's tools/difftest.py uses this module (games/pool has an older,
standalone version of the same idea). The pieces:

* Ram: a whole-RAM snapshot of the original running in xemu
  (xemu_drive.py "ram:name" saves name.ram and name.cr3), read through the
  guest's page tables.
* Build: one XBE in an emulator. The original's image comes from the
  snapshot itself (its code and its live globals, its libraries' too); ours
  from build/xbox/default.xbe, with each global the original names copied
  over. Any other memory either touches (the heap) is paged in from the
  snapshot, each build its own copy; for ours, pointers in it to the
  original's globals, vtables and functions become pointers to ours.
* install_os_stubs: the same fake OS in both (files from original/, T:\\ in a
  scratch folder, a fixed rand, a tick count the caller advances).
* Tracer: every D3D (and XFONT) function both builds have, stubbed out and
  logged with its arguments read out, so the call streams can be compared.
* compare_memory: the globals and every heap page both builds touched,
  byte for byte (after relocating the original's pointers).
"""
import bisect
import glob
import hashlib
import os
import re
import struct
import sys
import tempfile

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_UNMAPPED, UcError
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EIP,
                               UC_X86_REG_ESP)

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402

import pdb as pdblib  # noqa: E402
import xbe as xbelib  # noqa: E402
from gameconf import ROOT, ORIGINAL  # noqa: E402
from msvcname import demangle  # noqa: E402

BASE = 0x10000
RETURN = 0x7FF00000      # fake return address: emulation stops here
STACK = 0x7FE00000       # our stack (top) and heap, away from the guest's
HEAP = 0x7E000000
FILES = 0x7D000000       # fake FILE pointers
OURS = os.path.join(ROOT, 'build', 'xbox', 'default.xbe')
MAP = os.path.join(ROOT, 'build', 'xbox', 'default.map')


# ---- symbols ---------------------------------------------------------------

def src_functions():
    """original address -> qualified name, from the `// 0xADDR` comments right
    above a definition in src/."""
    out = {}
    pat = re.compile(r'([A-Za-z_]\w*(?:::~?\w+)*)\s*\(')
    for path in sorted(glob.glob(os.path.join(ROOT, 'src', '*.cpp'))):
        lines = open(path, errors='replace').read().split('\n')
        for i, line in enumerate(lines):
            m = re.match(r'\s*// 0x([0-9A-Fa-f]{8})\b', line)
            if not m:
                continue
            for nxt in lines[i + 1:i + 12]:
                if nxt.startswith('//') or not nxt.strip():
                    continue
                d = pat.search(nxt)
                if d and not nxt.rstrip().endswith(';'):
                    out[int(m.group(1), 16)] = d.group(1)
                break
    return out


def orig_symbols():
    """name -> address in the original: symbols/*.txt, src/'s functions and
    the globals defined in src/ with their address in a comment."""
    syms = {}
    for fn in ('names.txt', 'xdk.txt', 'xdklib.txt'):
        path = os.path.join(ROOT, 'symbols', fn)
        if os.path.exists(path):
            for line in open(path):
                m = re.match(r'([0-9a-f]{8}) (\S+)', line)
                if m:
                    syms.setdefault(m.group(2), int(m.group(1), 16))
    for a, n in src_functions().items():
        syms[n] = a
    for path in glob.glob(os.path.join(ROOT, 'src', '*.cpp')):
        for line in open(path, errors='replace'):
            m = re.match(r'[A-Za-z_][\w:<> ]*?[\s*&]+(g_\w+)(?:\[[^]]*\])*(?:\s*=[^;]*)?;\s*// 0x([0-9A-Fa-f]{8})',
                         line)
            if m:
                syms[m.group(1)] = int(m.group(2), 16)
    return syms


def our_symbols():
    """(name -> XBE address, name -> (convention, argument bytes)) for our
    build. The map is in .exe addresses; imagebld moves each section."""
    exe = open(MAP.replace('.map', '.exe'), 'rb').read()
    pe = struct.unpack_from('<I', exe, 0x3C)[0]
    nsec = struct.unpack_from('<H', exe, pe + 6)[0]
    opt = struct.unpack_from('<H', exe, pe + 20)[0]
    exe_secs = []
    for i in range(nsec):
        o = pe + 24 + opt + 40 * i
        size, va = struct.unpack_from('<II', exe, o + 8)
        exe_secs.append((exe[o:o + 8].rstrip(b'\0').decode(), va, size))
    xbe_secs = {s.name: s.va for s in xbelib.Xbe(OURS).sections}
    moves = [(va, va + size, xbe_secs[n] - va) for n, va, size in exe_secs if n in xbe_secs]

    def move(rva):
        for lo, hi, d in moves:
            if lo <= rva < hi:
                return rva + d
        return None
    syms, args = {}, {}
    pdb_path = MAP.replace('.map', '.pdb')
    if os.path.exists(pdb_path):
        for seg, off, name, fn in pdblib.statics(pdb_path):
            if 1 <= seg <= len(exe_secs):
                a = move(exe_secs[seg - 1][1] + off)
                if a is not None:
                    syms.setdefault('%s::%s' % (fn, name) if fn else name, a)
    for line in open(MAP, errors='replace'):
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if not m:
            continue
        a = move(int(m.group(2), 16) - 0x400000)
        if a is None:
            continue
        raw = m.group(1)
        n = demangle(raw)
        syms.setdefault(n, a)
        dm = re.match(r'([_@])[^@]+@(\d+)$', raw)
        if dm:
            args.setdefault(n, ('fastcall' if dm.group(1) == '@' else 'stdcall', int(dm.group(2))))
    return syms, args


def vtables(xbe, ctors, rdata):
    """class -> its vtable address, from what each constructor stores at
    [this]: the last vtable it writes is the class's own."""
    img = xbe.flat_image()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    out = {}
    for cls, a in ctors.items():
        code = bytes(img[a - xbe.base:a - xbe.base + 0x400])
        last = None
        for i in md.disasm(code, a):
            if i.mnemonic == 'mov' and len(i.operands) == 2 and i.operands[0].type == 3 and \
                    i.operands[1].type == 2 and i.operands[0].mem.disp == 0 and i.operands[0].mem.index == 0:
                v = i.operands[1].imm & 0xFFFFFFFF
                if rdata[0] <= v < rdata[1]:
                    last = v
            if i.mnemonic == 'ret':
                break
        if last:
            out[cls] = last
    return out


def vtables_by_content(ox, osyms, rdata, mx, msyms, mrdata):
    """{original vtable: ours} for each of our `X::vftable`s whose slots name
    (mostly) the same functions as a run of function pointers in the
    original's .rdata; this finds the classes whose constructors the
    compiler generated."""
    oimg, mimg = ox.flat_image(), mx.flat_image()
    oname = {a: n for n, a in osyms.items()}
    mname = {a: n for n, a in msyms.items()}

    def run(img, base, a, names, end):
        out = []
        while a + 4 <= end:
            v = struct.unpack_from('<I', img, a - base)[0]
            if v not in names or not (BASE <= v < rdata[0] if names is oname else BASE <= v < mrdata[0]):
                break
            out.append(names[v])
            a += 4
        return out
    out = {}
    for n, m in msyms.items():
        if not n.endswith('::vftable') or not mrdata[0] <= m < mrdata[1]:
            continue
        mine = run(mimg, mx.base, m, mname, mrdata[1])
        if not mine:
            continue
        # Vtables lie back to back: score every position, not runs.
        scores = []
        for a in range(rdata[0], rdata[1] - 4 * len(mine) + 1, 4):
            vals = struct.unpack_from('<%dI' % len(mine), oimg, a - ox.base)
            scores.append((sum(oname.get(v) == x for v, x in zip(vals, mine)), a))
        scores.sort(reverse=True)
        if scores[0][0] * 2 > len(mine) and (len(scores) < 2 or scores[1][0] < scores[0][0]):
            out[scores[0][1]] = m
    return out


# ---- memory ----------------------------------------------------------------

class Ram:
    """A whole-RAM snapshot and the guest page tables (2-level, 4 MB pages)."""
    def __init__(self, path):
        self.ram = open(path, 'rb').read()
        self.cr3 = int(open(path[:-4] + '.cr3').read(), 16)

    def _u32(self, pa):
        return struct.unpack_from('<I', self.ram, pa)[0] if pa + 4 <= len(self.ram) else 0

    def translate(self, va):
        pde = self._u32((self.cr3 & ~0xFFF) + (va >> 22) * 4)
        if not pde & 1:
            return None
        if pde & 0x80:
            pa = (pde & 0xFFC00000) | (va & 0x3FFFFF)
        else:
            pte = self._u32((pde & ~0xFFF) + ((va >> 12) & 0x3FF) * 4)
            if not pte & 1:
                return None
            pa = (pte & ~0xFFF) | (va & 0xFFF)
        return pa if pa < len(self.ram) else None

    def page(self, va):
        pa = self.translate(va & ~0xFFF)
        return None if pa is None else self.ram[pa:pa + 0x1000]

    def read(self, va, n):
        out = b''
        while n > 0:
            chunk = min(n, 0x1000 - (va & 0xFFF))
            pg = self.page(va)
            out += pg[va & 0xFFF:(va & 0xFFF) + chunk] if pg else bytes(chunk)
            va, n = va + chunk, n - chunk
        return out


def wstr(uc, a, n=200):
    raw = bytes(uc.mem_read(a, 2 * n))
    out = []
    for i in range(0, len(raw), 2):
        c = raw[i] | raw[i + 1] << 8
        if not c:
            break
        out.append(chr(c))
    return ''.join(out)


def cstr(uc, a, n=256):
    return bytes(uc.mem_read(a, n)).split(b'\0')[0].decode('latin-1')


class Build:
    """One XBE in an emulator, with stubs (stdcall: the stub pops its
    arguments; fn(build, args) returns eax)."""
    def __init__(self, name, image, image_end, syms, args, ram):
        self.name, self.syms, self.args, self.ram = name, syms, args, ram
        self.image_end = image_end
        uc = self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(BASE, (image_end - BASE + 0xFFF) & ~0xFFF)
        uc.mem_write(BASE, image)
        uc.mem_map(RETURN, 0x1000)
        uc.mem_map(STACK - 0x100000, 0x100000)
        uc.mem_map(HEAP, 0x1000000)
        uc.mem_map(FILES, 0x10000)
        uc.mem_map(0, 0x1000)        # fs:[0] (SEH frames) with a zero FS base
        self.heap = HEAP
        self.stubs = {}
        self.paged = set()           # guest pages faulted in from the snapshot
        self.relocate = None         # ours: the original's pointers -> ours
        uc.hook_add(UC_HOOK_CODE, self._hook)
        uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._fault)
        # The kernel is in the snapshot too, but nothing should run it: stop
        # at once and say who called it (a library call left unstubbed).
        self.kernel_call = None
        uc.hook_add(UC_HOOK_CODE, self._kernel, begin=0x80000000, end=0xFFFFFFFF)
        self.by_addr = sorted((a, n) for n, a in syms.items() if BASE <= a < image_end)
        self.keys = [a for a, _ in self.by_addr]

    def sym(self, v):
        if BASE <= v < self.image_end and self.keys:
            i = bisect.bisect_right(self.keys, v) - 1
            if i >= 0:
                a, n = self.by_addr[i]
                return n if v == a else '%s+%#x' % (n, v - a)
        if STACK - 0x100000 <= v < STACK:
            return 'stack'
        return '%#x' % v

    def alloc(self, n):
        p = self.heap
        self.heap += (max(n, 1) + 15) & ~15
        return p

    def stub(self, name, nargs, fn, stdcall=True):
        """By name, if this build has it; nargs: stack arguments."""
        if name in self.syms:
            self.stubs[self.syms[name]] = (nargs, fn, stdcall)
            return True
        return False

    def stub_decorated(self, name, fn):
        """A stdcall or fastcall function whose argument size our map's
        decoration gives; fn(build, args) with the register arguments first."""
        conv, nbytes = self.args[name]
        nstack = nbytes // 4 if conv == 'stdcall' else max(0, nbytes - 8) // 4

        def wrap(b, a):
            if conv == 'fastcall':
                a = [b.uc.reg_read(UC_X86_REG_ECX), b.uc.reg_read(UC_X86_REG_EDX)] + list(a)
            return fn(b, list(a))
        return self.stub(name, nstack, wrap)

    def _hook(self, uc, addr, size, _):
        s = self.stubs.get(addr)
        if s:
            nargs, fn, stdcall = s
            esp = uc.reg_read(UC_X86_REG_ESP)
            ret, *args = struct.unpack('<%dI' % (nargs + 1), uc.mem_read(esp, 4 * (nargs + 1)))
            self.caller = ret
            uc.reg_write(UC_X86_REG_EAX, (fn(self, args) or 0) & 0xFFFFFFFF)
            uc.reg_write(UC_X86_REG_ESP, esp + 4 + (4 * nargs if stdcall else 0))
            uc.reg_write(UC_X86_REG_EIP, ret)

    def _kernel(self, uc, addr, size, _):
        esp = uc.reg_read(UC_X86_REG_ESP)
        stack = struct.unpack('<32I', uc.mem_read(esp, 128))
        self.kernel_call = ' < '.join(self.sym(v) for v in stack if BASE <= v < self.image_end)[:400]
        uc.emu_stop()

    def _fault(self, uc, access, addr, size, value, _):
        page = self.ram.page(addr) if self.ram else None
        if page is None:
            print('%s: fault at %#x (eip %#x %s)' % (self.name, addr, uc.reg_read(UC_X86_REG_EIP),
                                                    self.sym(uc.reg_read(UC_X86_REG_EIP))))
            return False
        if self.relocate:
            page = self.relocate(page)
        uc.mem_map(addr & ~0xFFF, 0x1000)
        uc.mem_write(addr & ~0xFFF, page)
        self.paged.add(addr & ~0xFFF)
        return True

    def ensure(self, addr, n):
        """Page in [addr, addr + n) from the snapshot, as the code would."""
        mapped = [(lo, hi) for lo, hi, _ in self.uc.mem_regions()]
        for pg in range(addr & ~0xFFF, addr + n, 0x1000):
            if not any(lo <= pg <= hi for lo, hi in mapped):
                self._fault(self.uc, 0, pg, 4, 0, None)

    def read(self, addr, fmt):
        if isinstance(addr, str):
            addr = self.syms[addr]
        n = struct.calcsize('<' + fmt)
        self.ensure(addr, n)
        return struct.unpack('<' + fmt, self.uc.mem_read(addr, n))

    def write(self, addr, fmt, *v):
        if isinstance(addr, str):
            addr = self.syms[addr]
        data = struct.pack('<' + fmt, *v)
        self.ensure(addr, len(data))
        self.uc.mem_write(addr, data)

    def call(self, name, *args, ecx=0, limit=100000000):
        uc = self.uc
        esp = STACK - 0x1000 - 4 * (len(args) + 1)
        uc.mem_write(esp, struct.pack('<%dI' % (len(args) + 1), RETURN, *args))
        uc.reg_write(UC_X86_REG_ESP, esp)
        uc.reg_write(UC_X86_REG_ECX, ecx)
        addr = self.syms[name] if isinstance(name, str) else name
        try:
            uc.emu_start(addr, RETURN, count=limit)
        except UcError as ex:
            eip = uc.reg_read(UC_X86_REG_EIP)
            esp = uc.reg_read(UC_X86_REG_ESP)
            stack = struct.unpack('<64I', uc.mem_read(esp, 256))
            calls = [self.sym(v) for v in stack if BASE <= v < self.image_end and '+' in self.sym(v)]
            raise SystemExit('%s: %s in %s at %s\n  stack: %s' % (self.name, ex, name, self.sym(eip),
                                                                   ' < '.join(calls[:10])))
        if self.kernel_call:
            raise SystemExit('%s: %s ran into the kernel; called from %s' % (self.name, name, self.kernel_call))
        if uc.reg_read(UC_X86_REG_EIP) != RETURN:
            raise SystemExit('%s: %s still running after %d instructions (at %s)'
                             % (self.name, name, limit, self.sym(uc.reg_read(UC_X86_REG_EIP))))
        return uc.reg_read(UC_X86_REG_EAX)


def load_builds(snapshot, orig_data, aliases=None, extra=(), orig_extra=None):
    """The original (from the snapshot) and ours (its globals copied from the
    original's). orig_data: the original's (lo, hi) range of game globals;
    aliases: original name -> our 'name[+offset]' for globals we name
    differently; extra: library globals copied as they are (D3D state);
    orig_extra: name -> address of the original's statics src/ has no
    definition comment for."""
    ram = Ram(snapshot)
    ox = xbelib.Xbe(ORIGINAL)
    oimg = bytearray(ox.flat_image())
    # The original's image as it is in the snapshot: code and live globals.
    for va in range(BASE, BASE + len(oimg), 0x1000):
        pg = ram.page(va)
        if pg is not None:
            n = min(0x1000, len(oimg) - (va - BASE))
            oimg[va - BASE:va - BASE + n] = pg[:n]
    osyms = orig_symbols()
    osyms.update(orig_extra or {})
    oend = BASE + len(oimg)
    orig = Build('orig', bytes(oimg), oend, osyms, {}, ram)

    mx = xbelib.Xbe(OURS)
    msyms, margs = our_symbols()
    for oname, ours in (aliases or {}).items():
        base, _, delta = ours.partition('+')
        if base in msyms:
            msyms[oname] = msyms[base] + int(delta or '0', 16)
    mimg = mx.flat_image()
    mine = Build('mine', bytes(mimg), BASE + len(mimg), msyms, margs, ram)
    orig.args = margs

    # Pointers to the original's globals, vtables and functions -> ours.
    targets = {}
    for n, a in osyms.items():
        if n in msyms and (orig_data[0] <= a < orig_data[1] or n in extra):
            targets[a] = n
    rd = [s for s in ox.sections if s.name == '.rdata'][0]
    mrd = [s for s in mx.sections if s.name == '.rdata'][0]
    funcs = src_functions()
    ctors = {n.split('::')[0]: a for a, n in funcs.items() if n.count('::') == 1 and
             n.split('::')[0] == n.split('::')[1]}
    mctors = {c: msyms[c + '::' + c] for c in ctors if c + '::' + c in msyms}
    ovt = vtables(ox, ctors, (rd.va, rd.va + rd.vsize))
    mvt = vtables(mx, mctors, (mrd.va, mrd.va + mrd.vsize))
    exact = vtables_by_content(ox, osyms, (rd.va, rd.va + rd.vsize), mx, msyms, (mrd.va, mrd.va + mrd.vsize))
    for c, a in ovt.items():
        if c in mvt:
            exact[a] = mvt[c]
    # Not function addresses: plain numbers (tick counts) look like them too
    # often; the heap holds vtables, not function pointers.
    okeys = sorted(targets)

    def reloc(v):
        if v in exact:
            return exact[v]
        if orig_data[0] <= v < orig_data[1] or v in targets:
            k = bisect.bisect_right(okeys, v) - 1
            if k >= 0:
                a = okeys[k]
                return msyms[targets[a]] + (v - a)
        return v

    def relocate(buf):
        n = len(buf) // 4
        vals = struct.unpack('<%dI' % n, buf[:4 * n])
        new = [reloc(v) for v in vals]
        if new == list(vals):
            return buf
        return struct.pack('<%dI' % n, *new) + buf[4 * n:]
    mine.relocate = relocate
    mine.reloc = reloc

    # Our globals: each one the original names, sized by the gap to the next
    # symbol in either build.
    olist = sorted(set(osyms.values()))
    mlist = sorted(set(msyms.values()))
    sizes = {}
    for a, n in sorted((a, n) for a, n in targets.items()):
        m = msyms[n]
        nxt_o = olist[bisect.bisect_right(olist, a)] if bisect.bisect_right(olist, a) < len(olist) else a + 0x1000
        nxt_m = mlist[bisect.bisect_right(mlist, m)] if bisect.bisect_right(mlist, m) < len(mlist) else m + 0x1000
        size = min(nxt_o - a, nxt_m - m)
        data = bytes(orig.uc.mem_read(a, size))
        mine.uc.mem_write(m, relocate(data))
        sizes[n] = size
    mine.sizes = sizes
    return orig, mine


# ---- the fake OS -------------------------------------------------------------

class FakeFs:
    """D:\\ is the game folder (read only); T:\\ (title data) is a scratch
    folder of the build's own. Handles are fake FILE pointers."""
    def __init__(self, build):
        self.tdir = tempfile.mkdtemp(prefix='difftest-%s-' % build.name)
        self.files = {}
        self.next = FILES

    def path(self, p):
        p = p.replace('/', '\\')
        if p[:3].upper() == 'D:\\':
            return os.path.join(ROOT, 'original', *p[3:].split('\\'))
        if p[:3].upper() == 'T:\\':
            return os.path.join(self.tdir, *p[3:].split('\\'))
        return None

    def open(self, name, mode):
        real = self.path(name)
        writing = 'w' in mode or 'a' in mode
        if real is None or (not writing and not os.path.isfile(real)):
            return 0
        if writing and not real.startswith(self.tdir):
            return 0
        if writing:
            os.makedirs(os.path.dirname(real), exist_ok=True)
        h = self.next
        self.next += 0x40
        self.files[h] = open(real, mode.replace('t', '').replace('b', '') + 'b')
        return h


def _stack_args(b, n=32):
    esp = b.uc.reg_read(UC_X86_REG_ESP)
    return list(struct.unpack('<%dI' % n, b.uc.mem_read(esp + 4, 4 * n)))


def scanf(b, f, fmt, args, wide=False):
    """fscanf for %d %f %s %c and whitespace (fwscanf: %s stores WCHARs)."""
    count = 0
    for m in re.finditer(r'(\s+)|%(\d*)([dfsSc])|(.)', fmt):
        ws, width, kind, lit = m.groups()
        if ws:
            while True:
                c = f.read(1)
                if not c:
                    break
                if not c.isspace():
                    f.seek(-1, 1)
                    break
            continue
        if lit:
            c = f.read(1)
            if c != lit.encode('latin-1'):
                if c:
                    f.seek(-1, 1)
                return count
            continue
        if kind != 'c':
            while True:
                c = f.read(1)
                if not c:
                    return count if count else -1
                if not c.isspace():
                    f.seek(-1, 1)
                    break
        tok = b''
        limit = int(width) if width else 1 << 30
        while len(tok) < limit:
            c = f.read(1)
            if not c:
                break
            if kind == 'c':
                tok = c
                break
            if c.isspace() or (kind == 'd' and not (c.isdigit() or (not tok and c in b'+-'))) or \
                    (kind == 'f' and not (c.isdigit() or c in b'+-.eE')):
                f.seek(-1, 1)
                break
            tok += c
        if not tok:
            return count if count else -1
        dst = args.pop(0)
        if kind == 'd':
            b.uc.mem_write(dst, struct.pack('<i', int(tok)))
        elif kind == 'f':
            b.uc.mem_write(dst, struct.pack('<f', float(tok)))
        elif kind == 's' and wide:
            b.uc.mem_write(dst, tok.decode('latin-1').encode('utf-16le') + b'\0\0')
        elif kind in 'sS':
            b.uc.mem_write(dst, tok + b'\0')
        else:
            b.uc.mem_write(dst, tok)
        count += 1
    return count


def printf_format(b, fmt, args, wide=False):
    def conv(m):
        flags, width, prec, kind = m.groups()
        if kind == '%':
            return '%'
        if kind in 'eEfgG':
            lo, hi = args.pop(0), args.pop(0)
            v = struct.unpack('<d', struct.pack('<II', lo, hi))[0]
        else:
            v = args.pop(0)
        if kind in 'di':
            v = struct.unpack('<i', struct.pack('<I', v))[0]
        elif kind == ('S' if wide else 's'):
            v, kind = cstr(b.uc, v, 512), 's'
        elif kind in 'sS':
            v, kind = wstr(b.uc, v, 400), 's'
        elif kind == 'c':
            v = chr(v & 0xFFFF)
        return ('%' + flags + width + prec + kind) % v
    return re.sub(r'%([-+ 0#]*)(\d*)((?:\.\d+)?)([diouxXeEfgGcsS%])', conv, fmt)


def install_os_stubs(b, log):
    """Files, a bump heap, a fixed rand, printf and scanf in Python (the CRT's
    locale was never set up in ours), the tick count: returns a dict whose
    't' the caller advances."""
    fs = b.fs = FakeFs(b)
    state = {'rand': 12345}

    def note(name, ret=0):
        return lambda bb, a: (log.append(name), ret)[1]

    def fopen(bb, a):
        name, mode = cstr(bb.uc, a[0]), cstr(bb.uc, a[1], 8)
        h = fs.open(name, mode)
        log.append('fopen %s %s -> %s' % (name, mode, 'ok' if h else 'NULL'))
        return h

    def fclose(bb, a):
        f = fs.files.pop(a[0], None)
        if f:
            f.close()
        return 0

    def fscanf(bb, _):
        args = _stack_args(bb)
        return scanf(bb, fs.files[args[0]], cstr(bb.uc, args[1]), args[2:]) & 0xFFFFFFFF

    def fprintf(bb, _):
        args = _stack_args(bb)
        text = printf_format(bb, cstr(bb.uc, args[1]), args[2:])
        fs.files[args[0]].write(text.encode('latin-1'))
        return len(text)

    def fread(bb, a):
        buf, size, n, h = a
        data = fs.files[h].read(size * n)
        bb.uc.mem_write(buf, data)
        return len(data) // size if size else 0

    def fwprintf(bb, _):
        args = _stack_args(bb)
        text = printf_format(bb, wstr(bb.uc, args[1], 400), args[2:], wide=True)
        fs.files[args[0]].write(text.encode('latin-1', 'replace'))
        return len(text)

    def fwscanf(bb, _):
        args = _stack_args(bb)
        return scanf(bb, fs.files[args[0]], wstr(bb.uc, args[1], 400), args[2:], wide=True) & 0xFFFFFFFF

    b.stub('fwprintf', 0, fwprintf, False)
    b.stub('fwscanf', 0, fwscanf, False)
    b.stub('fopen', 2, fopen, False)
    b.stub('fclose', 1, fclose, False)
    b.stub('fscanf', 0, fscanf, False)
    b.stub('fprintf', 0, fprintf, False)
    b.stub('fread', 4, fread, False)
    b.stub('OutputDebugStringA', 1, lambda bb, a: log.append('debug ' + cstr(bb.uc, a[0], 64)))
    sizes = {}

    def alloc(n):
        p = b.alloc(n)
        sizes[p] = n
        return p
    for name in ('malloc', 'operator_new', 'operator_new[]'):
        b.stub(name, 1, lambda bb, a: alloc(a[0]), False)
    b.stub('calloc', 2, lambda bb, a: alloc(a[0] * a[1]), False)
    for name in ('free', 'operator_delete', 'operator_delete[]'):
        b.stub(name, 1, note(name), False)

    def rand(bb, a):
        state['rand'] = (state['rand'] * 214013 + 2531011) & 0xFFFFFFFF
        return (state['rand'] >> 16) & 0x7FFF
    b.stub('rand', 0, rand, False)
    b.stub('srand', 1, note('srand'), False)

    def swprintf(bb, _):
        args = _stack_args(bb)
        buf, fmt = args.pop(0), wstr(bb.uc, args.pop(0), 400)
        text = printf_format(bb, fmt, args, wide=True)
        bb.uc.mem_write(buf, text.encode('utf-16le') + b'\0\0')
        return len(text)

    def sprintf(bb, _):
        args = _stack_args(bb)
        buf, fmt = args.pop(0), cstr(bb.uc, args.pop(0))
        text = printf_format(bb, fmt, args)
        bb.uc.mem_write(buf, text.encode('latin-1') + b'\0')
        return len(text)
    b.stub('swprintf', 0, swprintf, False)
    b.stub('sprintf', 0, sprintf, False)
    tick = {'t': 0x20000000}     # above every image address, so never relocated
    b.stub('GetTickCount', 0, lambda bb, a: tick['t'])
    return tick


# ---- D3D calls ------------------------------------------------------------------

D3D_PREFIXES = ('D3DDevice_', 'D3DResource_', 'D3DBaseTexture_', 'D3DTexture_', 'D3DVertexBuffer_',
                'D3DIndexBuffer_', 'D3DSurface_', 'D3DCubeTexture_', 'D3DVolumeTexture_', 'Direct3D_',
                'XFONT_')    # XFONT draws text with the CPU into a surface
STRUCTS = {   # name -> {argument index: struct format to read out}
    'D3DDevice_SetTransform': {1: '16f'},
    'D3DDevice_SetMaterial': {0: '17f'},
    'D3DDevice_SetViewport': {0: '4I2f'},
    'D3DDevice_SetLight': {1: 'I25f'},
}


IDENTITY = struct.pack('<16f', 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)


class Tracer:
    """Stubs every D3D function both builds have, logging each call with its
    arguments made comparable: `log` symbolized, `raw` as values."""
    def __init__(self, build, names):
        self.b, self.log, self.raw = build, [], []
        self.data = {}      # call index -> (vertex bytes, stride) of the UP draws
        self.transforms = {}    # what GetTransform gives back: the last SetTransform
        for n in names:
            build.stub_decorated(n, self._make(n))

    def _make(self, name):
        def fn(b, args):
            parts, raw = [], []
            for i, v in enumerate(args):
                fmt = STRUCTS.get(name, {}).get(i)
                if name == 'XFONT_TextOut' and i == 2:
                    parts.append(repr(wstr(b.uc, v)))
                    raw.append(parts[-1])
                elif fmt and v:
                    parts.append('[%s]' % ' '.join('%.6g' % x if isinstance(x, float) else '%#x' % x
                                                    for x in b.read(v, fmt)))
                    raw.append(parts[-1])
                else:
                    parts.append(b.sym(v))
                    raw.append('stack' if parts[-1] == 'stack' else '%#x' % v)
            if name in ('D3DDevice_DrawVerticesUP', 'D3DDevice_DrawIndexedVerticesUP'):
                count, data, stride = args[1], args[-2], args[-1]
                nbytes = min(count * stride, 0x100000)
                verts = bytes(b.uc.mem_read(data, nbytes))
                parts.append('data#' + hashlib.sha1(verts).hexdigest()[:12])
                raw.append(parts[-1])
                self.data[len(self.log)] = (verts, stride)
            if name == 'D3DDevice_SetTransform':
                self.transforms[args[0]] = bytes(b.uc.mem_read(args[1], 64))
            elif name == 'D3DDevice_GetTransform':
                b.uc.mem_write(args[1], self.transforms.get(args[0], IDENTITY))
            self.log.append('%s(%s)  <- %s' % (name, ', '.join(parts), b.sym(b.caller).split('+')[0]))
            self.raw.append('%s(%s)' % (name, ', '.join(raw)))
            return 0
        return fn

    def clear(self):
        self.log.clear()
        self.raw.clear()
        self.data.clear()


def d3d_names(orig, mine):
    return sorted(n for n in mine.syms if n.startswith(D3D_PREFIXES) and n in orig.syms and n in mine.args)


def compare_calls(xo, xm, maxdiffs=10, out=print):
    """The two call streams; equal when the raw values match (pointers both
    copied from the snapshot) or the symbolized ones do (each build's own
    globals). Returns the number of differing stretches."""
    import difflib
    diffs = 0
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, xo.raw, xm.raw, autojunk=False).get_opcodes():
        if tag == 'equal':
            continue
        if tag == 'replace' and i2 - i1 == j2 - j1 and all(
                a == b or _same_call(a, b) for a, b in zip(xo.log[i1:i2], xm.log[j1:j2])):
            continue
        diffs += 1
        if diffs > maxdiffs:
            out('...')
            break
        out('--- %s at call %d (orig) / %d (mine)' % (tag, i1, j1))
        for line in xo.log[max(0, i1 - 2):i1]:
            out('    ' + line[:220])
        for line in xo.log[i1:i2][:6]:
            out('  - ' + line[:220])
        for line in xm.log[j1:j2][:6]:
            out('  + ' + line[:220])
        for i, j in zip(range(i1, i2), range(j1, j2)):
            if i in xo.data and j in xm.data and xo.data[i] != xm.data[j]:
                (a, stride), (m, _) = xo.data[i], xm.data[j]
                for v in range(len(a) // stride):
                    va, vm = a[v * stride:(v + 1) * stride], m[v * stride:(v + 1) * stride]
                    if va != vm:
                        fa = struct.unpack('<%dI' % (stride // 4), va)
                        fm = struct.unpack('<%dI' % (stride // 4), vm)
                        out('    vertex %d: orig %s' % (v, ' '.join(_word(x) for x in fa)))
                        out('              mine %s' % ' '.join(_word(x) for x in fm))
                break
    return diffs


def _word(v):
    f = struct.unpack('<f', struct.pack('<I', v))[0]
    return '%.9g' % f if v and 1e-6 < abs(f) < 1e7 else '%#x' % v


def _same_call(a, b):
    return a == b


# ---- memory comparison -------------------------------------------------------------

def compare_memory(orig, mine, skip=(), per_region=8):
    """[(what, offset, orig bytes, our bytes)] for each differing dword of the
    globals and the heap pages (up to per_region of each)."""
    out = []

    def diff(what, base, a, m):
        n = 0
        for k in range(0, len(a) - len(a) % 4, 4):
            if a[k:k + 4] != m[k:k + 4]:
                out.append((what(base + k) if callable(what) else what, k, a[k:k + 4], m[k:k + 4]))
                n += 1
                if n >= per_region:
                    break
    for n, size in sorted(mine.sizes.items()):
        if n in skip:
            continue
        a = mine.relocate(bytes(orig.uc.mem_read(orig.syms[n], size)))
        m = bytes(mine.uc.mem_read(mine.syms[n], size))
        if a != m:
            diff(lambda x, n=n: '%s+%#x' % (n, x), 0, a, m)
    for pg in sorted(orig.paged & mine.paged):
        a = mine.relocate(bytes(orig.uc.mem_read(pg, 0x1000)))
        m = bytes(mine.uc.mem_read(pg, 0x1000))
        if a != m:
            diff(lambda x: 'heap %#x' % x, pg, a, m)
    # What both allocated since the snapshot (the same sizes in the same order,
    # so at the same addresses while they behave the same).
    if orig.heap != mine.heap:
        out.append(('allocations', 0, struct.pack('<I', orig.heap - HEAP), struct.pack('<I', mine.heap - HEAP)))
    end = min(orig.heap, mine.heap)
    for pg in range(HEAP, end, 0x1000):
        n = min(0x1000, end - pg)
        a = mine.relocate(bytes(orig.uc.mem_read(pg, n)))
        m = bytes(mine.uc.mem_read(pg, n))
        if a != m:
            diff(lambda x: 'new %#x' % x, pg, a, m)
    return out


def compare_saves(orig, mine, out=print):
    """The files each build wrote to T:\\. Returns True when they're the same."""
    names = set()
    for b in (orig, mine):
        for d, _, files in os.walk(b.fs.tdir):
            names |= {os.path.relpath(os.path.join(d, f), b.fs.tdir) for f in files}
    same = True
    for n in sorted(names):
        data = []
        for b in (orig, mine):
            p = os.path.join(b.fs.tdir, n)
            data.append(open(p, 'rb').read() if os.path.exists(p) else None)
        if data[0] == data[1]:
            out('T:\\%s: same (%d bytes)' % (n, len(data[0])))
            continue
        same = False
        a, m = data[0] or b'', data[1] or b''
        i = next((k for k in range(min(len(a), len(m))) if a[k] != m[k]), min(len(a), len(m)))
        out('T:\\%s: DIFFERENT at byte %d (%d / %d bytes)\n  orig %r\n  mine %r'
            % (n, i, len(a), len(m), a[max(0, i - 40):i + 80], m[max(0, i - 40):i + 80]))
    return same
