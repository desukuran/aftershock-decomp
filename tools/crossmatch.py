#!/usr/bin/env python3
"""Find game functions shared with another of the games here.

  crossmatch.py OTHER_GAME [--near R]     (OTHER_GAME: a folder under games/)

Both games' functions are disassembled and compared by instruction shape:
mnemonics and operands with absolute addresses and call/jump targets masked.
A function of this XBE whose shape matches exactly one named function of the
other game is printed as "<address> <name>  # <other address>" for
symbols/names.txt (to be checked by hand; members can move between games).

With --near R, a function with no exact match is also compared with every
named function of the other game, and the closest one is printed as a
comment when their instruction sequences are at least R alike (0..1, e.g.
0.8): "# <address> near <name> 0.93  # <other address>". These are the same
function changed a little between the games, or a different one with a
similar shape; read both.

  crossmatch.py OTHER_GAME --diff ADDR OTHER_ADDR

prints the two functions' instructions side by side where they differ
(this game's on the left), addresses masked as above.
"""
import os
import difflib
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xbe import Xbe  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402

import gameconf  # noqa: E402
from gameconf import ROOT, ORIGINAL, APP_RANGES  # noqa: E402
ADDR = re.compile(r'0x[0-9a-f]{5,}')


def shapes(xbe, funcs):
    img = xbe.flat_image()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    out = {}
    for a, size in funcs:
        code = img[a - xbe.base:a - xbe.base + size]
        out[a] = tuple(i.mnemonic + ' ' + ADDR.sub('A', i.op_str) for i in md.disasm(code, a))
    return out


def text_functions(path, lo, hi):
    out = []
    with open(path) as f:
        next(f)
        for line in f:
            a, size, block = line.split('\t')[:3]
            a = int(a, 16)
            if block == '.text' and lo <= a < hi:
                out.append((a, int(size)))
    return out


def disasm_one(xbe, funcs_tsv, a):
    for f, size in text_functions(funcs_tsv, 0, 1 << 32):
        if f == a:
            img = xbe.flat_image()
            code = img[a - xbe.base:a - xbe.base + size]
            return [i.mnemonic + ' ' + ADDR.sub('A', i.op_str) for i in Cs(CS_ARCH_X86, CS_MODE_32).disasm(code, a)]
    sys.exit(f'no function at {a:08x}')


def diff(other_root, other_xbe, a, b):
    mine = disasm_one(Xbe(ORIGINAL), os.path.join(ROOT, 'build/export/functions.tsv'), a)
    theirs = disasm_one(Xbe(other_xbe), os.path.join(other_root, 'build/export/functions.tsv'), b)
    m = difflib.SequenceMatcher(None, mine, theirs, autojunk=False)
    print(f'{len(mine)} vs {len(theirs)} instructions, {m.ratio():.2f} alike')
    for op, i1, i2, j1, j2 in m.get_opcodes():
        if op == 'equal':
            print(f'    ... {i2 - i1} the same')
            continue
        for k in range(max(i2 - i1, j2 - j1)):
            left = mine[i1 + k] if i1 + k < i2 else ''
            right = theirs[j1 + k] if j1 + k < j2 else ''
            print(f'  {left:<40} | {right}')


def main():
    other_root = os.path.join(gameconf.GAMES, sys.argv[1])
    other_xbe = os.path.join(other_root, 'original', 'default.xbe')
    other_ranges = gameconf.ranges(gameconf.config(other_root)['app_ranges'])
    if '--diff' in sys.argv:
        i = sys.argv.index('--diff')
        return diff(other_root, other_xbe, int(sys.argv[i + 1], 16), int(sys.argv[i + 2], 16))
    mine = shapes(Xbe(ORIGINAL),
                  text_functions(os.path.join(ROOT, 'build/export/functions.tsv'), *APP_RANGES[0]))
    names = {}
    for line in open(os.path.join(other_root, 'symbols/names.txt')):
        f = line.split(None, 2)
        if len(f) >= 2 and not line.startswith('#'):
            names[int(f[0], 16)] = f[1]
    theirs = shapes(Xbe(other_xbe), text_functions(os.path.join(other_root, 'build/export/functions.tsv'), *other_ranges[0]))
    by_shape = {}
    for a, s in theirs.items():
        by_shape.setdefault(s, []).append(a)
    near = float(sys.argv[sys.argv.index('--near') + 1]) if '--near' in sys.argv else None
    named = [(h, t) for h, t in theirs.items() if h in names and len(t) >= 3]
    for a, s in sorted(mine.items()):
        hits = by_shape.get(s, [])
        if len(s) >= 3 and len(hits) == 1 and hits[0] in names:
            print(f'{a:08x} {names[hits[0]]}  # {hits[0]:08x}')
        elif len(s) >= 3 and hits:
            print(f'# {a:08x} ambiguous: ' + ' '.join(f'{h:08x}={names.get(h, "?")}' for h in hits))
        elif near is not None and len(s) >= 3:
            best, ratio = None, near
            for h, t in named:
                if not 0.5 <= len(t) / len(s) <= 2:
                    continue
                m = difflib.SequenceMatcher(None, s, t, autojunk=False)
                if m.real_quick_ratio() >= ratio and m.quick_ratio() >= ratio and m.ratio() >= ratio:
                    best, ratio = h, m.ratio()
            if best is not None:
                print(f'# {a:08x} near {names[best]} {ratio:.2f}  # {best:08x}')


if __name__ == '__main__':
    main()
