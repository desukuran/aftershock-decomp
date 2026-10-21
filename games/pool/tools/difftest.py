#!/usr/bin/env python3
"""Differential test: run a function of the original default.xbe and of our
rebuilt one (build/xbox/default.xbe) side by side in Unicorn, from the same
game state, and compare the state afterwards.

  difftest.py physics <snapshot> [frames] [frameTime] [shots] [seed]
  difftest.py render <snapshot> [function] [maxdiffs]
  difftest.py update <snapshot> <function> "<pad script>" [render function]
  difftest.py fuzz <snapshot> [frames] [seed]     random input into Update+Render

<snapshot> is either the original's .data section (0xB6140..) saved from a
running game ("mem:snap:0xB6140:0xD3A0" in xemu_drive.py), or a whole-RAM
snapshot ("ram:snap", give the .ram path); with RAM, any other memory the code
touches (the heap) is paged in from it through the guest page tables. The
original's globals are copied into our build by name (symbols/names.txt and
`// 0xADDR` comments in src/ for the original, build/xbox/default.map for
ours), so both start equal; heap pointers stay valid in both.

physics: takes the shot the cue is aimed for and steps UpdatePhysics at a
fixed frame time, comparing every ball after every frame. Sounds and the tick
count are stubbed (the same in both).

update: feeds scripted pad input (see parse_script) frame by frame to an
update function (UpdateMenu, UpdateGame, UpdateEditor...) in both builds,
optionally rendering each frame too, and compares g_Game, the fake OS calls
and the draw calls after every frame.

render: calls a render function (default Render) with every D3D device call
stubbed out and logged, arguments symbolized (addresses in either image by
symbol name) and matrices, materials, viewports and vertex data read out, and
compares the two call streams.
"""
import os
import re
import struct
import sys

from unicorn import UC_HOOK_MEM_UNMAPPED, UcError
from unicorn.x86_const import UC_X86_REG_EIP, UC_X86_REG_ESP

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../tools'))
import emulate  # noqa: E402
import pdb as pdblib  # noqa: E402
import xbe as xbelib  # noqa: E402

ROOT = emulate.ROOT
ORIG_DATA = 0x000B6140
ORIG_DATA_SIZE = 0xD3A0
# Our stack, heap and return address, away from the guest's heap.
emulate.RETURN = 0x7FF00000
emulate.STACK = 0x7FE00000
emulate.HEAP = 0x7E000000
ARGS = {}   # our function name -> (convention, bytes of stack arguments) from the map
# The original's names for statics that ours has inside functions (from the PDB).
ALIASES = {
    'g_GhostBalls': 'DrawAimGuide::s_Ghost',
    'g_GhostBallsInit': 'DrawAimGuide::s_Ghost+0xE0',   # its constructor guard, right after it
    # Separate globals in the original, the tail of g_BallTextures in ours.
    'g_RedBallTexture': 'g_BallTextures+0x40',
    'g_BlueBallTexture': 'g_BallTextures+0x44',
    'g_BlackBallTexture': 'g_BallTextures+0x48',
}


def orig_symbols():
    syms = {}
    for line in open(os.path.join(ROOT, 'symbols/names.txt')):
        m = re.match(r'([0-9a-f]{8}) (\S+)', line)
        if m:
            syms[m.group(2)] = int(m.group(1), 16)
    # Globals defined in src/ with their address in a comment.
    import glob
    for path in glob.glob(os.path.join(ROOT, 'src/*.cpp')):
        for line in open(path, errors='replace'):
            m = re.match(r'[A-Za-z_][\w:<> ]*?[\s*&]+(g_\w+)(?:\[[^]]*\])*(?:\s*=[^;]*)?;\s*// 0x([0-9A-Fa-f]{8})', line)
            if m:
                syms.setdefault(m.group(1), int(m.group(2), 16))
    for line in open(os.path.join(ROOT, 'symbols/xdklib.txt')):
        m = re.match(r'([0-9a-f]{8}) (\S+)', line)
        if m:
            syms.setdefault(m.group(2), int(m.group(1), 16))
    return syms


OPERATORS = {'??2@YAPAXI@Z': 'operator_new', '??3@YAXPAX@Z': 'operator_delete',
             '??_U@YAPAXI@Z': 'operator_new[]', '??_V@YAXPAX@Z': 'operator_delete[]'}


def demangle(name):
    """Just enough MSVC demangling for our own names."""
    if name in OPERATORS:
        return OPERATORS[name]
    if name.startswith('_') or name.startswith('@'):
        return name[1:].split('@')[0]
    m = re.match(r'\?([A-Za-z0-9_]+)@([A-Za-z0-9_]+)@@[QAIU]', name)
    if m and not name.startswith('?' + m.group(1) + '@@'):
        return m.group(2) + '::' + m.group(1)
    m = re.match(r'\?([A-Za-z0-9_]+)@@', name)
    return m.group(1) if m else None


def our_symbols(xbe_path, map_path):
    """name -> XBE virtual address. The map is in .exe addresses; imagebld
    moves each section, so translate section by section."""
    exe = open(map_path.replace('.map', '.exe'), 'rb').read()
    pe = struct.unpack_from('<I', exe, 0x3C)[0]
    nsec = struct.unpack_from('<H', exe, pe + 6)[0]
    opt = struct.unpack_from('<H', exe, pe + 20)[0]
    exe_secs = {}
    for i in range(nsec):
        o = pe + 24 + opt + 40 * i
        exe_secs[exe[o:o + 8].rstrip(b'\0').decode()] = struct.unpack_from('<II', exe, o + 8)[::-1]
    xbe_secs = {s.name: s.va for s in xbelib.Xbe(xbe_path).sections}
    moves = [(va, va + size, xbe_secs[name] - va) for name, (va, size) in exe_secs.items() if name in xbe_secs]
    syms = {}
    # Statics the map leaves out, from the PDB (needs /Z7).
    sec_va = [va for va, _ in exe_secs.values()]
    pdb_path = map_path.replace('.map', '.pdb')
    if os.path.exists(pdb_path):
        for seg, off, name, fn in pdblib.statics(pdb_path):
            if not 1 <= seg <= len(sec_va):
                continue
            rva = sec_va[seg - 1] + off
            for lo, hi, d in moves:
                if lo <= rva < hi:
                    syms.setdefault('%s::%s' % (fn, name) if fn else name, rva + d)
                    break
    for line in open(map_path):
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if not m:
            continue
        rva = int(m.group(2), 16) - 0x400000
        for lo, hi, d in moves:
            if lo <= rva < hi:
                raw = m.group(1)
                n = demangle(raw)
                if n:
                    syms.setdefault(n, rva + d)
                    dm = re.match(r'([_@])[^@]+@(\d+)$', raw)
                    if dm:
                        ARGS.setdefault(n, ('fastcall' if dm.group(1) == '@' else 'stdcall', int(dm.group(2))))
                break
    for orig_name, ours in ALIASES.items():
        base, _, delta = ours.partition('+')
        if base in syms:
            syms[orig_name] = syms[base] + int(delta or '0', 16)
    return syms


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


class Build:
    def __init__(self, name, flat, syms):
        self.name = name
        emulate_root = os.path.join(ROOT, flat)
        self.emu = emulate.Emu.__new__(emulate.Emu)
        img = open(emulate_root, 'rb').read()
        from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
        uc = self.emu.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(emulate.BASE, (len(img) + 0x20000 + 0xFFF) & ~0xFFF)
        uc.mem_write(emulate.BASE, img)
        uc.mem_map(emulate.RETURN, 0x1000)
        uc.mem_map(emulate.STACK - 0x100000, 0x100000)
        uc.mem_map(emulate.HEAP, 0x1000000)
        uc.mem_map(0, 0x1000)   # fs:[0] (SEH frames) with a zero FS base
        self.emu.heap = emulate.HEAP
        self.emu.stubs, self.emu.watches = {}, {}
        uc.hook_add(UC_HOOK_CODE, self.emu._hook)
        uc.hook_add(UC_HOOK_MEM_UNMAPPED, self._fault)
        self.syms = syms
        self.sounds = []
        self.emu.stub(syms['CSound::Play'], 1, lambda e, a: self.sounds.append(a[0]))
        self.emu.stub(syms['GetTickCount'], 0, lambda e, a: 100000)

    ram = None
    relocate = None

    def _fault(self, uc, access, addr, size, value, _):
        page = self.ram.page(addr) if self.ram else None
        if page is not None and self.relocate:
            page = self.relocate(page)
        if page is not None:
            uc.mem_map(addr & ~0xFFF, 0x1000)
            uc.mem_write(addr & ~0xFFF, page)
            return True
        print('%s: fault at %#x (eip %#x)' % (self.name, addr, uc.reg_read(UC_X86_REG_EIP)))
        return False

    def addr(self, name):
        return self.syms[name]

    def read(self, name, fmt, off=0):
        return self.emu.read(self.addr(name) + off, fmt)

    def write(self, name, fmt, *v, off=0):
        self.emu.uc.mem_write(self.addr(name) + off, struct.pack('<' + fmt, *v))

    def call(self, name, limit=200000000):
        uc = self.emu.uc
        esp = emulate.STACK - 0x1000 - 4
        uc.mem_write(esp, struct.pack('<I', emulate.RETURN))
        uc.reg_write(UC_X86_REG_ESP, esp)
        try:
            uc.emu_start(self.addr(name), emulate.RETURN, count=limit)
        except UcError as ex:
            eip = uc.reg_read(UC_X86_REG_EIP)
            code = bytes(uc.mem_read(eip, 8)).hex() if eip else ''
            esp = uc.reg_read(UC_X86_REG_ESP)
            stack = struct.unpack('<64I', uc.mem_read(esp, 256))
            by = sorted((a, n) for n, a in self.syms.items())
            import bisect
            keys = [a for a, _ in by]
            calls = []
            for v in stack:
                if 0x11000 <= v < 0xC0000:
                    a, n = by[bisect.bisect_right(keys, v) - 1]
                    calls.append('%s+%#x' % (n, v - a))
            raise SystemExit('%s: %s in %s at eip %#x (%s)\n  stack: %s' % (self.name, ex, name, eip, code,
                                                                          ' < '.join(calls[:10])))
        if uc.reg_read(UC_X86_REG_EIP) != emulate.RETURN:
            raise SystemExit('%s: %s still running after %d instructions (eip %#x)'
                             % (self.name, name, limit, uc.reg_read(UC_X86_REG_EIP)))


def load_builds(snapshot):
    orig_syms = orig_symbols()
    ours = os.path.join(ROOT, 'build/xbox/default.xbe')
    flat = os.path.join(ROOT, 'build/xbox/default.flat')
    open(flat, 'wb').write(xbelib.Xbe(ours).flat_image())
    our_syms = our_symbols(ours, os.path.join(ROOT, 'build/xbox/default.map'))
    orig = Build('orig', 'build/default.flat', orig_syms)
    mine = Build('mine', 'build/xbox/default.flat', our_syms)

    if snapshot.endswith('.ram'):
        orig.ram = mine.ram = Ram(snapshot)
        data = orig.ram.read(ORIG_DATA, ORIG_DATA_SIZE)
    else:
        data = open(snapshot, 'rb').read()
    orig.emu.uc.mem_write(ORIG_DATA, data)
    # Copy each data global the original names into ours. Size: the gap to
    # the next symbol in either build (never more than the object).
    olist = sorted(a for a in orig_syms.values() if ORIG_DATA <= a < ORIG_DATA + len(data))
    # Pointers into the original's globals become pointers to ours.
    import bisect
    onames = sorted((a, n) for n, a in orig_syms.items()
                    if ORIG_DATA <= a < ORIG_DATA + len(data) and n in our_syms)
    okeys = [a for a, _ in onames]

    def relocate(buf):
        if len(buf) < 4:
            return buf
        out = bytearray(buf)
        for i in range(0, len(buf) - 3, 4):
            v = struct.unpack_from('<I', buf, i)[0]
            if ORIG_DATA <= v < ORIG_DATA + len(data):
                k = bisect.bisect_right(okeys, v) - 1
                if k >= 0:
                    a, n = onames[k]
                    struct.pack_into('<I', out, i, our_syms[n] + (v - a))
        return bytes(out)
    mine.relocate = relocate
    mine.sizes = {}
    mlist = sorted(our_syms.values())
    copied = []
    for name, a in orig_syms.items():
        if not (ORIG_DATA <= a < ORIG_DATA + len(data)) or name not in our_syms:
            continue
        m = our_syms[name]
        nxt_o = next((x for x in olist if x > a), ORIG_DATA + len(data))
        nxt_m = next((x for x in mlist if x > m), m + 0x10000)
        size = min(nxt_o - a, nxt_m - m)
        mine.emu.uc.mem_write(m, relocate(data[a - ORIG_DATA:a - ORIG_DATA + size]))
        copied.append(name)
        mine.sizes[name] = size
    return orig, mine, copied


BALL_FMT = 'i3f3f16fiif??2xi'   # SBall, 0x70 bytes
BALL_FIELDS = ['type', 'pos', 'dir', 'rot', 'inPocket', 'pocket', 'speed', 'onTable', 'needsReplace', 'number']


SCALARS = [('g_bBallsStopped', '?'), ('g_ShotPower', 'f'), ('g_CueDistance', 'f'), ('g_bCueMode', '?'),
           ('g_unknownBA1F4', 'i'), ('g_NumBalls', 'i')]


def balls(b, n):
    out = []
    for i in range(n):
        v = b.read('g_Balls', BALL_FMT, off=i * 0x70)
        out.append(v)
    return out


def next_shot(builds, n, rng, log):
    """Aims the cue ball at a random ball still on the table and shoots,
    the same way in every build. Ball in hand goes back to the middle."""
    b0 = builds[0]
    cue = b0.read('g_Balls', BALL_FMT)
    for b in builds:
        if cue[27] or not cue[26]:   # needsReplace / off the table
            b.write('g_Balls', '3f', 0.0, cue[2], 0.0, off=4)
            b.write('g_Balls', '??', True, False, off=0x68)
            b.write('g_Balls', 'ii', 0, 0, off=0x5C)
    cx, _, cz = b0.read('g_Balls', '3f', off=4)
    targets = [i for i in range(1, n) if b0.read('g_Balls', '?', off=i * 0x70 + 0x68)[0]]
    if not targets:
        return False
    t = targets[rng.randrange(len(targets))]
    tx, _, tz = b0.read('g_Balls', '3f', off=t * 0x70 + 4)
    dx, dz = cx - tx, cz - tz
    d = (dx * dx + dz * dz) ** 0.5 or 1.0
    log.append(t)
    for b in builds:
        b.write('g_AimDir', '3f', dx / d, 0.0, dz / d)
        b.write('g_CueDistance', 'f', 0.0)
    return True


def physics(snapshot, frames=600, frame_time=1.0 / 60, shots=1, seed=1):
    orig, mine, copied = load_builds(snapshot)
    n = orig.read('g_NumBalls', 'i')[0]
    print('copied %d globals; %d balls; aim %s' % (len(copied), n, orig.read('g_AimDir', '3f')))
    for b in (orig, mine):
        b.write('g_CueDistance', 'f', 0.0)
        b.write('g_bBallsStopped', '?', True)
        b.write('g_Game', 'f', frame_time, off=0xA30)   # frameTime
    import random
    rng = random.Random(seed)
    first = None
    shot_log = []
    settled = 0
    for f in range(frames):
        if orig.read('g_bBallsStopped', '?')[0] and f > 0:
            settled += 1
            if settled > 5 and len(shot_log) < shots - 1:
                state = orig.read('g_Game', 'i', off=0x19C)[0]
                player = orig.read('g_Game', 'i', off=0x370)[0]
                if state != 3 or not next_shot((orig, mine), n, rng, shot_log):
                    print('frame %d: state %d, stopping' % (f, state))
                    break
                print('frame %d: player %d shoots at ball %d' % (f, player, shot_log[-1]))
                settled = 0
        else:
            settled = 0
        for b in (orig, mine):
            b.call('UpdatePhysics')
        bo, bm = balls(orig, n), balls(mine, n)
        go, gm = mine.relocate(orig.read('g_Game', '3316s')[0]), mine.read('g_Game', '3316s')[0]
        if go != gm and first is None:
            diffs = [i for i in range(len(go)) if go[i] != gm[i]]
            print('frame %d: g_Game differs at offsets %s' % (f, ', '.join('%#x' % i for i in diffs[:16])))
            first = (f, -1)
        for name, fmt in SCALARS:
            if orig.read(name, fmt) != mine.read(name, fmt) and first is None:
                print('frame %d: %s orig %r mine %r' % (f, name, orig.read(name, fmt), mine.read(name, fmt)))
                first = (f, -1)
        for i in range(n):
            if bo[i] != bm[i] and first is None:
                first = (f, i)
                print('frame %d ball %d differs' % (f, i))
                for k, (x, y) in enumerate(zip(bo[i], bm[i])):
                    if x != y:
                        print('   field %2d orig %r mine %r' % (k, x, y))
        if first:
            break
    if not first:
        moving = sum(1 for v in bo if v[25] > 0)
        print('%d frames identical (%d balls still moving); sounds orig %d mine %d %s'
              % (frames, moving, len(orig.sounds), len(mine.sounds),
                 'same' if orig.sounds == mine.sounds else 'DIFFERENT'))
        for i, v in enumerate(bo):
            print('  ball %2d pos (%8.3f %8.3f %8.3f) onTable %d pocket %d' % (i, v[1], v[2], v[3], v[26], v[24]))


D3D_PREFIXES = ('D3DDevice_', 'D3DResource_', 'D3DBaseTexture_', 'D3DTexture_', 'D3DVertexBuffer_',
                'D3DIndexBuffer_', 'D3DSurface_', 'D3DCubeTexture_', 'D3DVolumeTexture_', 'Direct3D_',
                'XFONT_')   # XFONT draws text with the CPU into the locked back buffer
STRUCTS = {   # name -> {argument index: struct format to read out}
    'D3DDevice_SetTransform': {1: '16f'},
    'D3DDevice_SetMaterial': {0: '17f'},
    'D3DDevice_SetViewport': {0: '4I2f'},
    'D3DDevice_SetLight': {1: 'I25f'},
}


class Tracer:
    """Stubs every D3D function both builds have, logging calls with their
    arguments made comparable."""
    def __init__(self, build, names, image_end):
        self.b, self.log, self.raw = build, [], []
        self.image_end = image_end
        self.by_addr = sorted((a, n) for n, a in build.syms.items() if 0x10000 <= a < image_end)
        import bisect
        self.addrs = [a for a, _ in self.by_addr]
        self.bisect = bisect.bisect_right
        for n in names:
            conv, nbytes = ARGS[n]
            nstack = nbytes // 4 if conv == 'stdcall' else max(0, nbytes - 8) // 4
            build.emu.stub(build.syms[n], nstack, self._make(n, conv, nstack), stdcall=True)

    def sym(self, v):
        if 0x10000 <= v < self.image_end:
            i = self.bisect(self.addrs, v) - 1
            a, n = self.by_addr[i]
            return n if v == a else '%s+%#x' % (n, v - a)
        if emulate.STACK - 0x100000 <= v < emulate.STACK:
            return 'stack'
        return '%#x' % v

    def _make(self, name, conv, nstack):
        from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_EDX

        def fn(e, args):
            args = list(args)
            if conv == 'fastcall':
                args = [e.uc.reg_read(UC_X86_REG_ECX), e.uc.reg_read(UC_X86_REG_EDX)] + args
            parts, raw = [], []
            for i, v in enumerate(args):
                fmt = STRUCTS.get(name, {}).get(i)
                if name in ('XFONT_TextOut', 'XFONT_GetTextExtent') and i == 2:
                    text = emulate.wstr(e, v)
                    if len(text) == 2 and ord(text[1]) >= 0x3000 and args[4] - 280 in range(0, 15 * 13, 13):
                        text = text[0]   # the name editor's stack-garbage character (MenuRender.cpp)
                    parts.append(repr(text))
                    raw.append(parts[-1])
                elif fmt and v:
                    parts.append('[%s]' % ' '.join('%.6g' % x if isinstance(x, float) else '%#x' % x
                                                    for x in e.read(v, fmt)))
                    raw.append(parts[-1])
                else:
                    parts.append(self.sym(v))
                    raw.append('stack' if parts[-1] == 'stack' else '%#x' % v)
            if name in ('D3DDevice_DrawVerticesUP', 'D3DDevice_DrawIndexedVerticesUP'):
                import hashlib
                count, data, stride = args[1], args[-2], args[-1]
                nbytes = min(count * stride, 0x100000)
                parts.append('data#' + hashlib.sha1(bytes(e.uc.mem_read(data, nbytes))).hexdigest()[:12])
                raw.append(parts[-1])
            self.log.append('%s(%s)' % (name, ', '.join(parts)))
            self.raw.append('%s(%s)' % (name, ', '.join(raw)))
            return 0
        return fn


def render(snapshot, fn='Render', maxdiffs=10):
    orig, mine, copied = load_builds(snapshot)
    names = sorted(n for n in mine.syms if n.startswith(D3D_PREFIXES) and n in orig.syms and n in ARGS)
    for b in (orig, mine):
        install_os_stubs(b, [])
    xo = Tracer(orig, names, 0xCA660)
    xm = Tracer(mine, names, max(mine.syms.values()) + 1)
    for b, x in ((orig, xo), (mine, xm)):
        try:
            b.call(fn)
        except SystemExit:
            print('\n'.join(x.log[-5:]))
            raise
    print('%s: %d calls orig, %d mine (%d D3D functions stubbed)' % (fn, len(xo.log), len(xm.log), len(names)))
    import difflib
    diffs = 0
    # Equal if the raw values match (e.g. pointers both copied from the
    # snapshot) or the symbolized ones do (addresses of each build's own globals).
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, xo.raw, xm.raw, autojunk=False).get_opcodes():
        if tag == 'equal':
            continue
        if tag == 'replace' and i2 - i1 == j2 - j1 and xo.log[i1:i2] == xm.log[j1:j2]:
            continue
        diffs += 1
        if diffs > maxdiffs:
            print('...')
            break
        print('--- %s at call %d (orig) / %d (mine)' % (tag, i1, j1))
        for l in xo.log[max(0, i1 - 2):i1]:
            print('    ' + l[:200])
        for l in xo.log[i1:i2][:6]:
            print('  - ' + l[:200])
        for l in xm.log[j1:j2][:6]:
            print('  + ' + l[:200])
    # Deferred render and texture states are only stored, not called.
    for name, fmt in (('D3D__RenderState', '%dI' % 0x6C), ('D3D__TextureState', '%dI' % (4 * 32))):
        if name in orig.syms and name in mine.syms:
            a, b = orig.read(name, fmt), mine.read(name, fmt)
            bad = [i for i in range(len(a)) if a[i] != b[i]]
            print('%s: %s' % (name, 'same' if not bad else 'differ at %s' % ['%d: %#x/%#x' % (i, a[i], b[i]) for i in bad[:8]]))
    if diffs == 0:
        print('identical call streams')
    return xo.log, xm.log


PAD0 = 0x62C + 0x60          # g_Game.input.m_Gamepads[0]
MENUSEL = 0xCC
PCURPIECE = 0x50    # pCurPiece, pSelPiece
ANALOG = ['A', 'B', 'X', 'Y', 'BLACK', 'WHITE', 'LT', 'RT']
DIGITAL = {'UP': 1, 'DOWN': 2, 'LEFT': 4, 'RIGHT': 8, 'START': 0x10, 'BACK': 0x20, 'L3': 0x40, 'R3': 0x80}


GAME_DIR = os.path.join(ROOT, 'original')
FILES = 0x7D000000          # fake FILE pointers


class FakeFs:
    """D:\\ is the game folder (read only); T:\\ (title data) is a scratch
    folder of the build's own. Handles are fake FILE pointers."""
    def __init__(self, build):
        import tempfile
        self.tdir = tempfile.mkdtemp(prefix='difftest-%s-' % build.name)
        self.files = {}
        self.next = FILES
        build.emu.uc.mem_map(FILES, 0x10000)

    def path(self, p):
        p = p.replace('/', '\\')
        if p[:3].upper() == 'D:\\':
            return os.path.join(GAME_DIR, *p[3:].split('\\'))
        if p[:3].upper() == 'T:\\':
            return os.path.join(self.tdir, *p[3:].split('\\'))
        return None

    def open(self, name, mode):
        real = self.path(name)
        if real is None or ('w' not in mode and 'a' not in mode and not os.path.isfile(real)):
            return 0
        if real.startswith(GAME_DIR) and ('w' in mode or 'a' in mode):
            return 0
        if 'w' in mode or 'a' in mode:
            os.makedirs(os.path.dirname(real), exist_ok=True)
        h = self.next
        self.next += 0x40
        self.files[h] = open(real, mode.replace('t', '') + ('b' if 'b' not in mode else ''))
        return h


def _cstr(em, a, n=256):
    return bytes(em.uc.mem_read(a, n)).split(b'\0')[0].decode('latin-1')


def _stack_args(em, n=32):
    esp = em.uc.reg_read(UC_X86_REG_ESP)
    return list(struct.unpack('<%dI' % n, em.uc.mem_read(esp + 4, 4 * n)))


def scanf(em, f, fmt, args, wide=False):
    """fscanf for the directives the game uses: %d %f %s and whitespace
    (fwscanf: %s stores WCHARs, %S chars)."""
    import re as _re
    count = 0
    for m in _re.finditer(r'(\s+)|%(\d*)([dfsSc])|(.)', fmt):
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
            em.uc.mem_write(dst, struct.pack('<i', int(tok)))
        elif kind == 'f':
            em.uc.mem_write(dst, struct.pack('<f', float(tok)))
        elif kind == 's' and wide:
            em.uc.mem_write(dst, tok.decode('latin-1').encode('utf-16le') + b'\0\0')
        elif kind in 'sS':
            em.uc.mem_write(dst, tok + b'\0')
        else:
            em.uc.mem_write(dst, tok)
        count += 1
    return count


def printf_format(em, fmt, args, wide=False):
    import re as _re

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
        elif kind == 'S' if wide else kind == 's':
            v, kind = _cstr(em, v, 512), 's'
        elif kind in 'sS':
            v, kind = emulate.wstr(em, v, 400), 's'
        elif kind == 'c':
            v = chr(v & 0xFFFF)
        return ('%' + flags + width + prec + kind) % v
    return _re.sub(r'%([-+ 0#]*)(\d*)((?:\.\d+)?)([diouxXeEfgGcsS%])', conv, fmt)


def install_os_stubs(b, log):
    """The same fake OS in both builds: files from the game folder, a bump
    heap, a fixed rand."""
    e = b.emu
    state = {'rand': 12345}
    fs = b.fs = FakeFs(b)

    def stub(name, nargs, fn, stdcall=True):
        if name in b.syms:
            e.stub(b.syms[name], nargs, fn, stdcall)

    def note(name, ret=0):
        return lambda em, a: (log.append(name), ret)[1]
    def fopen(em, a):
        name, mode = _cstr(em, a[0]), _cstr(em, a[1], 8)
        h = fs.open(name, mode)
        log.append('fopen %s %s -> %s' % (name, mode, 'ok' if h else 'NULL'))
        return h

    def fclose(em, a):
        f = fs.files.pop(a[0], None)
        if f:
            f.close()
        return 0

    def fscanf(em, _):
        args = _stack_args(em)
        f, fmt = fs.files[args[0]], _cstr(em, args[1])
        return scanf(em, f, fmt, args[2:]) & 0xFFFFFFFF

    def fprintf(em, _):
        args = _stack_args(em)
        f, fmt = fs.files[args[0]], _cstr(em, args[1])
        text = printf_format(em, fmt, args[2:])
        f.write(text.encode('latin-1'))
        return len(text)

    def fwprintf(em, _):
        args = _stack_args(em)
        f, fmt = fs.files[args[0]], emulate.wstr(em, args[1], 400)
        text = printf_format(em, fmt, args[2:], wide=True)
        f.write(text.encode('latin-1', 'replace'))
        return len(text)

    def fwscanf(em, _):
        args = _stack_args(em)
        f, fmt = fs.files[args[0]], emulate.wstr(em, args[1], 400)
        return scanf(em, f, fmt, args[2:], wide=True) & 0xFFFFFFFF

    def fread(em, a):
        buf, size, n, h = a
        data = fs.files[h].read(size * n)
        em.uc.mem_write(buf, data)
        return len(data) // size if size else 0

    stub('fopen', 2, fopen, False)
    stub('fclose', 1, fclose, False)
    stub('fscanf', 0, fscanf, False)
    stub('fprintf', 0, fprintf, False)
    stub('fread', 4, fread, False)
    stub('fwprintf', 0, fwprintf, False)
    stub('fwscanf', 0, fwscanf, False)
    stub('FindFirstFileA', 2, lambda em, a: (log.append('FindFirstFile %s' % bytes(em.uc.mem_read(a[0], 64)).split(b'\0')[0].decode()), 0xFFFFFFFF)[1])
    stub('FindNextFileA', 2, note('FindNextFile'))
    stub('OutputDebugStringA', 1, lambda em, a: log.append('debug %s' % bytes(em.uc.mem_read(a[0], 64)).split(b'\0')[0].decode()))
    # The heap at every level: the original's CRT heap is live in the
    # snapshot, ours never ran its startup.
    sizes = {}

    def alloc(n):
        p = e.alloc(max(n, 1))
        sizes[p] = n
        return p

    def realloc(ptr, n):
        p = alloc(n)
        if ptr:
            e.uc.mem_write(p, bytes(e.uc.mem_read(ptr, min(n, sizes.get(ptr, n)))))
        return p
    for name in ('malloc', '_nh_malloc', '_heap_alloc', 'operator_new', 'operator_new[]'):
        stub(name, 1, lambda em, a: alloc(a[0]), False)
    stub('calloc', 2, lambda em, a: alloc(a[0] * a[1]), False)
    stub('realloc', 2, lambda em, a: realloc(a[0], a[1]), False)
    for name in ('free', 'operator_delete', 'operator_delete[]'):
        stub(name, 1, note(name), False)
    stub('RtlAllocateHeap', 3, lambda em, a: alloc(a[2]))
    stub('RtlFreeHeap', 3, lambda em, a: 1)
    stub('RtlReAllocateHeap', 4, lambda em, a: realloc(a[2], a[3]))
    stub('RtlSizeHeap', 3, lambda em, a: sizes.get(a[2], 0))

    def rand(em, a):
        state['rand'] = (state['rand'] * 214013 + 2531011) & 0xFFFFFFFF
        return (state['rand'] >> 16) & 0x7FFF
    stub('rand', 0, rand, False)
    stub('srand', 1, note('srand'), False)
    stub('XACTEngineDoWork', 0, note('XACTEngineDoWork'))
    for name in ('CMusicManager::Process', 'CMusicManager::Play', 'CMusicManager::Stop'):
        # thiscall with one stack argument at most; caller-cleaned is wrong for
        # thiscall, so pop by the original's signature: Play/Stop take one.
        stub(name, 0 if name.endswith('Process') else 1, note(name))
    stub('swprintf', 0, swprintf, False)

    def sprintf(em, _):
        args = _stack_args(em)
        buf, fmt = args.pop(0), _cstr(em, args.pop(0))
        text = printf_format(em, fmt, args)
        em.uc.mem_write(buf, text.encode('latin-1') + b'\0')
        return len(text)
    stub('sprintf', 0, sprintf, False)
    tick = {'t': 100000}
    stub('GetTickCount', 0, lambda em, a: tick['t'])
    return tick


def swprintf(em, _):
    """swprintf(buf, fmt, ...) in Python: the CRT's %S needs the locale its
    startup code sets up, which never ran in our build here."""
    args = _stack_args(em)
    buf, fmt = args.pop(0), emulate.wstr(em, args.pop(0), 400)
    text = printf_format(em, fmt, args, wide=True)
    em.uc.mem_write(buf, text.encode('utf-16le') + b'\0\0')
    return len(text)


def set_pad(b, held, last, sticks):
    base = b.addr('g_Game') + PAD0
    w = sum(v for k, v in DIGITAL.items() if k in held)
    analog = [255 if k in held else 0 for k in ANALOG]
    lastw = sum(v for k, v in DIGITAL.items() if k in last)
    pressed_w = w & ~lastw
    pressed_a = [1 if (k in held and k not in last) else 0 for k in ANALOG]
    # Triggers count as pressed while held (XBInput's quirk the game relies on).
    pressed_a[6] = 1 if 'LT' in held else 0
    pressed_a[7] = 1 if 'RT' in held else 0
    lx, ly, rx, ry = (sticks.get(k, 0.0) for k in ('LX', 'LY', 'RX', 'RY'))
    uc = b.emu.uc
    uc.mem_write(base, struct.pack('<H8B4h', w, *analog, *(int(v * 32767) for v in (lx, ly, rx, ry))))
    uc.mem_write(base + 0x14, struct.pack('<4f', lx, ly, rx, ry))
    uc.mem_write(base + 0x24, struct.pack('<H', lastw))
    uc.mem_write(base + 0x28, struct.pack('<8I', *[1 if k in last else 0 for k in ANALOG]))
    uc.mem_write(base + 0x48, struct.pack('<H', pressed_w))
    uc.mem_write(base + 0x4C, struct.pack('<8I', *pressed_a))


def parse_script(script):
    """Frames of (held buttons, sticks): N idle frames, BUTTON (one frame),
    hold:BUTTON:N, LX/LY/RX/RY:value:N (+LY is up, as the game reads it)."""
    frames = []
    for step in script.split():
        if step.isdigit():
            frames += [(set(), {})] * int(step)
        elif step.startswith('hold:'):
            _, name, n = step.split(':')
            frames += [({name}, {})] * int(n)
        elif step.split(':')[0] in ('LX', 'LY', 'RX', 'RY'):
            name, v, n = step.split(':')
            frames += [(set(), {name: float(v)})] * int(n)
        else:
            frames += [({step}, {}), (set(), {})]
    return frames


def update(snapshot, fn, script, render_fn=None):
    orig, mine, copied = load_builds(snapshot)
    logs = ([], [])
    ticks = [install_os_stubs(b, l) for b, l in zip((orig, mine), logs)]
    if render_fn:
        names = sorted(n for n in mine.syms if n.startswith(D3D_PREFIXES) and n in orig.syms and n in ARGS)
        tracers = (Tracer(orig, names, 0xCA660), Tracer(mine, names, max(mine.syms.values()) + 1))
    last, prev = set(), None
    frames = parse_script(script)
    for f, (held, sticks) in enumerate(frames):
        for b, t in zip((orig, mine), ticks):
            set_pad(b, held, last, sticks)
            b.write('g_Game', 'f', 0.016, off=0xA30)
            t['t'] += 16
            b.call(fn)
            if render_fn:
                b.call(render_fn)
        last = held
        go, gm = mine.relocate(orig.read('g_Game', '3316s')[0]), mine.read('g_Game', '3316s')[0]
        diffs = [i for i in range(len(go)) if go[i] != gm[i]]
        others = [n for n, size in mine.sizes.items() if n != 'g_Game' and n.startswith('g_') and
                  mine.relocate(orig.read(n, '%ds' % size)[0]) != mine.read(n, '%ds' % size)[0]]
        if others and not diffs:
            n = others[0]
            size = mine.sizes[n]
            a, b2 = mine.relocate(orig.read(n, '%ds' % size)[0]), mine.read(n, '%ds' % size)[0]
            k = next(i for i in range(size) if a[i] != b2[i]) & ~3
            print('frame %d (%s): %s differs at +%#x: orig %s mine %s (also %s)' % (
                f, ' '.join(sorted(held)) or '-', n, k, a[k:k + 12].hex(), b2[k:k + 12].hex(), others[1:6]))
            print('  state %d editor screen %d' % (orig.read('g_Game', 'i', off=0x19C)[0], orig.read('g_EditorScreen', 'i')[0]))
            return
        if diffs or logs[0] != logs[1]:
            print('frame %d (%s): differs' % (f, ' '.join(sorted(held)) or '-'))
            if diffs:
                print('  g_Game offsets %s' % ', '.join('%#x: %02x/%02x' % (i, go[i], gm[i]) for i in diffs[:16]))
                print('  calls this run: %s' % logs[0][-6:])
            if logs[0] != logs[1]:
                print('  orig calls %s' % logs[0][-5:])
                print('  mine calls %s' % logs[1][-5:])
            return
        if render_fn and tracers[0].raw != tracers[1].raw:
            for i, (a, b2) in enumerate(zip(tracers[0].raw, tracers[1].raw)):
                if a != b2 and tracers[0].log[i] != tracers[1].log[i]:
                    print('frame %d: render call %d differs' % (f, i))
                    for k in range(max(0, i - 4), i + 4):
                        a2 = tracers[0].log[k][:150] if k < len(tracers[0].log) else ''
                        b3 = tracers[1].log[k][:150] if k < len(tracers[1].log) else ''
                        print('  %s %-75s | %s' % ('>' if k == i else ' ', a2, b3))
                    print('  state %d editor screen %d' % (orig.read('g_Game', 'i', off=0x19C)[0], orig.read('g_EditorScreen', 'i')[0]))
                    if 'g_Game' in orig.syms:
                        for b in (orig, mine):
                            print('  %s: pCurPiece %#x pSelPiece %#x' % (b.name, *b.read('g_Game', 'II', off=PCURPIECE)))
                    return
            if len(tracers[0].raw) != len(tracers[1].raw):
                print('frame %d: render call counts differ %d / %d' % (f, len(tracers[0].raw), len(tracers[1].raw)))
                return
        if render_fn:
            for tr in tracers:
                tr.raw.clear()
                tr.log.clear()
        st = (orig.read('g_Game', 'i', off=0x19C)[0], orig.read('g_Game', 'i', off=MENUSEL)[0],
              orig.read('g_EditorScreen', 'i')[0])
        if st != prev:
            print('frame %d: state %d menuSel %d editor screen %d' % ((f,) + st))
            prev = st
    print('all frames identical; calls: %s' % (logs[0][-8:],))
    compare_saves(orig, mine)


def compare_saves(orig, mine):
    """The files each build wrote to T:."""
    names = set()
    for b in (orig, mine):
        for d, _, files in os.walk(b.fs.tdir):
            names |= {os.path.relpath(os.path.join(d, f), b.fs.tdir) for f in files}
    for n in sorted(names):
        data = []
        for b in (orig, mine):
            p = os.path.join(b.fs.tdir, n)
            data.append(open(p, 'rb').read() if os.path.exists(p) else None)
        if data[0] == data[1]:
            print('T:\\%s: same (%d bytes)' % (n, len(data[0])))
            continue
        a, b = data[0] or b'', data[1] or b''
        i = next((k for k in range(min(len(a), len(b))) if a[k] != b[k]), min(len(a), len(b)))
        print('T:\\%s: DIFFERENT at byte %d (%d / %d bytes)\n  orig %r\n  mine %r'
              % (n, i, len(a), len(b), a[max(0, i - 80):i + 120], b[max(0, i - 80):i + 120]))


def shapes(snapshot, seed=1):
    """BuildTableBarriers / BuildTablePockets for every piece shape in
    src/TableBuild.cpp at every rotation, compared float for float."""
    import random
    rng = random.Random(seed)
    orig, mine, _ = load_builds(snapshot)
    src = open(os.path.join(ROOT, 'src/TableBuild.cpp')).read()
    found = sorted(set((int(t), int(m), k) for t, m, k in
                       re.findall(r'\{\s*(\d+),\s*(\d+),\s*k(Barriers|Pockets)\d+_\d+', src)))
    bad = 0
    for t, m, kind in found:
        for rot in range(4):
            pos = (rng.uniform(-150, 150), rng.uniform(0, 10), rng.uniform(-150, 150))
            for b in (orig, mine):
                b.write('g_Game', '3fiifI', *pos, t, m, float(rot), 0, off=0x18)   # g_Game.table, alone
                b.write('g_NumPockets', 'i', 0)
                b.call('BuildTableBarriers' if kind == 'Barriers' else 'BuildTablePockets')
            if kind == 'Barriers':
                n = orig.read('g_NumBarriers', 'i')[0] + 1
                a, c = orig.read('g_Barriers', '%df' % (3 * n)), mine.read('g_Barriers', '%df' % (3 * n))
                stride = 3
            else:
                n = orig.read('g_NumPockets', 'i')[0]
                a, c = orig.read('g_Pockets', '%df' % (4 * n)), mine.read('g_Pockets', '%df' % (4 * n))
                stride = 4
            if n != (mine.read('g_NumBarriers', 'i')[0] + 1 if kind == 'Barriers' else mine.read('g_NumPockets', 'i')[0]):
                print('%s type %d mesh %d rot %d: counts differ' % (kind, t, m, rot))
                bad += 1
                continue
            diffs = sorted(set(i // stride for i in range(len(a))
                               if struct.pack('<f', a[i]) != struct.pack('<f', c[i])))
            if diffs:
                bad += 1
                print('%s type %d mesh %d rot %d: points %s of %d differ, e.g. %r / %r' % (
                    kind, t, m, rot, diffs, n, a[diffs[0] * stride:diffs[0] * stride + 3],
                    c[diffs[0] * stride:diffs[0] * stride + 3]))
    print('%d shape/rotation cases, %d differ' % (len(found) * 4, bad))


def fuzz_script(frames, seed):
    """Random taps, holds and stick pushes, about `frames` frames long."""
    import random
    rng = random.Random(seed)
    buttons = ANALOG[:6] + list(DIGITAL) + ['LT', 'RT']
    steps, n = [], 0
    while n < frames:
        r = rng.random()
        if r < 0.25:
            k = rng.randint(1, 30)
            steps.append(str(k))
        elif r < 0.55:
            steps.append(rng.choice(buttons))
            k = 2
        elif r < 0.7:
            k = rng.randint(5, 40)
            steps.append('hold:%s:%d' % (rng.choice(buttons), k))
        else:
            k = rng.randint(3, 30)
            steps.append('%s:%s:%d' % (rng.choice(['LX', 'LY', 'RX', 'RY']), rng.choice(['-1', '1', '-0.5', '0.5']), k))
        n += k
    return ' '.join(steps)


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == 'shapes':
        shapes(sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 1)
        return
    if len(sys.argv) >= 3 and sys.argv[1] == 'fuzz':
        frames = int(sys.argv[3]) if len(sys.argv) > 3 else 2000
        seed = int(sys.argv[4]) if len(sys.argv) > 4 else 1
        script = fuzz_script(frames, seed)
        print('script:', script)
        update(sys.argv[2], 'Update', script, 'Render')
        return
    if len(sys.argv) >= 5 and sys.argv[1] == 'update':
        update(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] if len(sys.argv) > 5 else None)
        return
    if len(sys.argv) >= 3 and sys.argv[1] == 'render':
        render(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else 'Render',
               int(sys.argv[4]) if len(sys.argv) > 4 else 10)
        return
    if len(sys.argv) < 3 or sys.argv[1] != 'physics':
        raise SystemExit(__doc__)
    frames = int(sys.argv[3]) if len(sys.argv) > 3 else 600
    ft = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0 / 60
    shots = int(sys.argv[5]) if len(sys.argv) > 5 else 1
    seed = int(sys.argv[6]) if len(sys.argv) > 6 else 1
    physics(sys.argv[2], frames, ft, shots, seed)


if __name__ == '__main__':
    main()
