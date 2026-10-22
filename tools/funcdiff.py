#!/usr/bin/env python3
"""Compare each function of the rebuild with the original's, instruction by
instruction, to find where the source says something else than the original.

  funcdiff.py [-v] [-o] [-x] [ADDR|name ...]      (from the game's folder)

The original's functions are the `// 0xADDR` comments in src/ right above a
definition; ours are found by that definition's name in build/xbox/default.map.
Both are disassembled and normalized: calls and jumps lose their targets,
globals become G, constants read from .rdata (floats, strings) become their
contents, and a compare with its jump becomes the test it makes (signed or
unsigned x < N, or x == N, whichever way the jump goes), so what is left
differs mostly where the code does: what remains is register allocation and
instruction order, and a count that differs can be one side duplicating a
branch. Prints one
line per function that isn't the same, with the calls and constants only one
side has (-o: also the struct and stack offsets, which register allocation
makes noisy), and with -v or named functions the differing instructions.
-x: only functions the same instruction for instruction count as the same.
"""
import difflib
import os
import re
import struct
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402

from gameconf import ROOT, ORIGINAL, RDATA  # noqa: E402
from msvcname import demangle  # noqa: E402
from xbe import Xbe  # noqa: E402

OURS = os.path.join(ROOT, 'build', 'xbox', 'default.xbe')
MAP = os.path.join(ROOT, 'build', 'xbox', 'default.map')
HEX = re.compile(r'0x[0-9a-f]+')
# Conditional jumps: signed or unsigned compare, and what to add to the
# constant to make the test x < constant.
JCC = {'jl': ('s', 0), 'jge': ('s', 0), 'jle': ('s', 1), 'jg': ('s', 1),
       'jb': ('u', 0), 'jae': ('u', 0), 'jbe': ('u', 1), 'ja': ('u', 1),
       'je': ('==', 0), 'jne': ('==', 0)}
KEEPS_FLAGS = {'mov', 'lea', 'push', 'pop', 'movss', 'movaps', 'movzx', 'movsx', 'fld', 'fild', 'fst', 'fstp'}
DEF = re.compile(r'([A-Za-z_]\w*(?:::~?\w+)*)\s*\(')


def src_functions():
    """original address -> qualified name, from the comments in src/."""
    out = {}
    src = os.path.join(ROOT, 'src')
    for fn in sorted(os.listdir(src)):
        if not fn.endswith('.cpp'):
            continue
        lines = open(os.path.join(src, fn), errors='replace').read().split('\n')
        for i, line in enumerate(lines):
            m = re.match(r'\s*// 0x([0-9A-Fa-f]{8})\b', line)
            if not m:
                continue
            for nxt in lines[i + 1:i + 12]:
                if nxt.startswith('//') or not nxt.strip():
                    continue
                d = DEF.search(nxt)
                if d and not nxt.rstrip().endswith(';'):
                    out[int(m.group(1), 16)] = d.group(1)
                break
    return out


def our_functions(x):
    """demangled name -> [(xbe address, size)] for our code sections."""
    exe = open(MAP.replace('.map', '.exe'), 'rb').read()
    pe = struct.unpack_from('<I', exe, 0x3C)[0]
    nsec = struct.unpack_from('<H', exe, pe + 6)[0]
    opt = struct.unpack_from('<H', exe, pe + 20)[0]
    moves = []
    xbe_secs = {s.name: s.va for s in x.sections}
    for i in range(nsec):
        o = pe + 24 + opt + 40 * i
        name = exe[o:o + 8].rstrip(b'\0').decode()
        size, va = struct.unpack_from('<II', exe, o + 8)
        if name in xbe_secs:
            moves.append((va, va + size, xbe_secs[name] - va))
    addrs = []
    for line in open(MAP, errors='replace'):
        m = re.match(r'\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{8})', line)
        if not m:
            continue
        rva = int(m.group(2), 16) - 0x400000
        for lo, hi, d in moves:
            if lo <= rva < hi:
                addrs.append((rva + d, demangle(m.group(1))))
                break
    addrs.sort()
    out = {}
    for (a, n), (b, _) in zip(addrs, addrs[1:] + [(addrs[-1][0] + 0x1000, None)]):
        out.setdefault(n, []).append((a, b - a))
    return out, dict(addrs)


class Image:
    def __init__(self, x, rdata, names):
        self.x, self.img, self.rdata, self.names = x, x.flat_image(), rdata, names
        self.md = Cs(CS_ARCH_X86, CS_MODE_32)
        self.md.detail = True

    def content(self, a):
        """What a constant address holds, if it is read-only data."""
        lo, hi = self.rdata
        if not lo <= a < hi:
            return None
        raw = bytes(self.img[a - self.x.base:a - self.x.base + 64])
        s = raw.split(b'\0')[0]
        if len(s) >= 3 and all(32 <= c < 127 or c in (9, 10) for c in s):
            return '"%s"' % s.decode()
        w = raw[::2].split(b'\0')[0]
        if len(w) >= 2 and raw[1:2 * len(w):2] == bytes(len(w)) and all(32 <= c < 127 for c in w):
            return 'L"%s"' % w.decode()
        f = struct.unpack_from('<f', raw)[0]
        if f == f and 1e-6 < abs(f) < 1e9:
            return '%.9gf' % f
        return '=%08x' % struct.unpack_from('<I', raw)[0]

    def constant(self, v):
        if v < 0x10000:
            return hex(v)
        c = self.content(v)
        if c:
            return c
        if self.x.base <= v < self.x.base + len(self.img):
            return 'G'
        f = struct.unpack('<f', struct.pack('<I', v))[0]
        if f == f and 1e-6 < abs(f) < 1e9 and v & 0xFF == 0:
            return '%.9gf' % f
        return hex(v)

    def disasm(self, a, size):
        """The normalized instructions, and the facts: what is called, the
        constants, the struct offsets."""
        code = bytes(self.img[a - self.x.base:a - self.x.base + size])
        out, facts = [], []
        cmp = None      # (index in facts, constant) of a cmp with a constant
        reach = a       # the furthest jump target inside the function so far
        for i in self.md.disasm(code, a):
            if i.mnemonic == 'int3':
                break
            ops = i.op_str
            if cmp and i.mnemonic in JCC:
                # A compare and its jump as the test it makes, so that x > 4
                # and x >= 5 (or the jump the other way) come out the same.
                kind, bump = JCC[i.mnemonic]
                v = cmp[1] if kind == '==' else (cmp[1] + bump) & 0xFFFFFFFF
                if kind == 's' and v >= 0x80000000:
                    v -= 1 << 32
                facts[cmp[0]] = '%s%s' % ({'s': '<', 'u': 'u<', '==': '=='}[kind], hex(v) if abs(v) > 9 else v)
            if i.mnemonic not in KEEPS_FLAGS:
                cmp = None
            if i.mnemonic == 'cmp' and len(i.operands) == 2 and i.operands[1].type == 2 and \
                    self.constant(i.operands[1].imm & 0xFFFFFFFF).startswith('0x'):
                cmp = ('after', i.operands[1].imm & 0xFFFFFFFF)   # its fact is the last one added below
            elif i.mnemonic == 'test' and len(i.operands) == 2 and i.operands[0].type == 1 and \
                    i.operands[0].reg == i.operands[1].reg:
                facts.append('0x0')         # test r, r: a compare with 0
                cmp = (len(facts) - 1, 0)
            if (i.mnemonic == 'call' or i.mnemonic.startswith('j')) and HEX.fullmatch(ops):
                t = int(ops, 16)
                if a <= t < a + size:
                    reach = max(reach, t)
                if i.mnemonic == 'call' or not a <= t < a + size:
                    facts.append('call ' + self.names.get(t, '?'))
                ops = 'T'
            else:
                def sub(m):
                    v = int(m.group(0), 16)
                    if v < 0x10000:
                        return m.group(0)
                    c = self.content(v)
                    return c if c else 'G'
                ops = HEX.sub(sub, ops)
                regs = [i.reg_name(r) for r in i.regs_access()[1]] if i.mnemonic != 'push' else []
                stack = bool({'esp', 'ebp'} & set(regs)) or i.mnemonic in ('ret', 'lea')
                for op in i.operands:
                    if op.type == 2 and not stack:                       # immediate
                        facts.append(self.constant(op.imm & 0xFFFFFFFF))
                    elif op.type == 3:                                   # memory
                        base = i.reg_name(op.mem.base) if op.mem.base else None
                        if base is None and not op.mem.index:
                            c = self.content(op.mem.disp & 0xFFFFFFFF)
                            if c:
                                facts.append(c)
                        elif self.x.base <= op.mem.disp & 0xFFFFFFFF < self.x.base + len(self.img):
                            facts.append('G[]')                         # a global array, indexed
                        elif base not in ('esp', 'ebp') and op.mem.disp:
                            facts.append('+' + hex(op.mem.disp))
            if cmp and cmp[0] == 'after':
                cmp = (len(facts) - 1, cmp[1])
            out.append(i.mnemonic + ' ' + ops)
            # The end: a ret that no jump goes past (the sizes include padding,
            # and code after a ret can still be the function's).
            if i.mnemonic == 'ret' and i.address + i.size > reach:
                break
        return out, facts


def orig_names(funcs):
    """address -> name for the original: src/, then symbols/, then Ghidra's."""
    names = {}
    with open(os.path.join(ROOT, 'build/export/functions.tsv')) as f:
        next(f)
        for line in f:
            a, _, _, name = line.rstrip('\n').split('\t')[:4]
            names[int(a, 16)] = name
    for fn in ('xdklib.txt', 'xdk.txt', 'names.txt'):
        path = os.path.join(ROOT, 'symbols', fn)
        if os.path.exists(path):
            for line in open(path):
                p = line.split()
                if len(p) >= 2 and not line.startswith('#') and re.fullmatch(r'[0-9a-fA-F]{8}', p[0]):
                    names[int(p[0], 16)] = p[1]
    names.update(funcs)
    return names


def main():
    args = sys.argv[1:]
    verbose = '-v' in args
    offsets = '-o' in args
    strict = '-x' in args
    args = [a for a in args if a not in ('-v', '-o', '-x')]
    funcs = src_functions()
    orig_x, our_x = Xbe(ORIGINAL), Xbe(OURS)
    mine, our_names = our_functions(our_x)
    orig = Image(orig_x, RDATA, orig_names(funcs))
    rd = [s for s in our_x.sections if s.name == '.rdata'][0]
    ours = Image(our_x, (rd.va, rd.va + rd.vsize), our_names)
    sizes = {}
    with open(os.path.join(ROOT, 'build/export/functions.tsv')) as f:
        next(f)
        for line in f:
            a, size = line.split('\t')[:2]
            sizes[int(a, 16)] = int(size)
    want = set()
    for a in args:
        if re.fullmatch(r'(0x)?[0-9a-fA-F]+', a):
            want.add(int(a, 16))
        else:
            want |= {k for k, v in funcs.items() if v == a or v.endswith('::' + a)}
    same = differ = exact = 0
    for a, name in sorted(funcs.items()):
        if want and a not in want:
            continue
        cands = mine.get(name, [])
        if not cands:
            print('%08x %-40s not in the map' % (a, name))
            continue
        theirs, tf = orig.disasm(a, sizes.get(a, 0x400))
        if theirs and not theirs[-1].startswith(('ret', 'jmp')):
            # Ghidra ended it at a call it took for one that doesn't return
            # (the stack cookie check): read on to the ret.
            nxt = min([b for b in sizes if b > a] + [a + sizes.get(a, 0x400) + 0x40])
            theirs, tf = orig.disasm(a, min(nxt - a, sizes.get(a, 0x400) + 0x40))
        best = None
        for b, size in cands:
            m, mf = ours.disasm(b, size)
            miss = Counter(tf) - Counter(mf)
            extra = Counter(mf) - Counter(tf)
            score = sum(miss.values()) + sum(extra.values())
            if best is None or score < best[0]:
                best = (score, b, m, miss, extra)
        score, b, m, miss, extra = best
        if not score and len(theirs) == len(m) and (theirs == m or not strict):
            same += 1
            exact += theirs == m
            if not want:
                continue
        else:
            differ += 1
        print('%08x %-36s %4d vs %4d instructions (ours %08x)' % (a, name, len(theirs), len(m), b))
        for label, c in (('only the original', miss), ('only ours', extra)):
            for kind, keep in (('', lambda k: not k.startswith('+')), (' offsets', lambda k: k.startswith('+'))):
                items = sorted(k + ('*%d' % v if v > 1 else '') for k, v in c.items() if keep(k))
                if items and (not kind or offsets):
                    print('    %-26s %s' % (label + kind + ':', ', '.join(items)))
        if verbose or want:
            sm = difflib.SequenceMatcher(None, theirs, m, autojunk=False)
            for op, i1, i2, j1, j2 in sm.get_opcodes():
                if op == 'equal':
                    continue
                for k in range(max(i2 - i1, j2 - j1)):
                    t = theirs[i1 + k] if i1 + k < i2 else ''
                    o = m[j1 + k] if j1 + k < j2 else ''
                    print('      %-44s | %s' % (t, o))
                print('      ' + '-' * 44)
    print('%d functions the same (constants, offsets, calls and length), %d differ' % (same, differ))
    print('%d of them instruction for instruction (registers and order too)' % exact)


if __name__ == '__main__':
    main()
