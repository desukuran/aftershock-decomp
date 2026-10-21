#!/usr/bin/env python3
"""Find game functions shared with another SHiZNO game (AfterShock Pool SX).

  crossmatch.py OTHER_XBE OTHER_NAMES [--min N]

Both games' functions are disassembled and compared by instruction shape:
mnemonics and operands with absolute addresses and call/jump targets masked.
A function of this XBE whose shape matches exactly one named function of the
other game is printed as "<address> <name>  # <other address>" for
symbols/names.txt (to be checked by hand; members can move between games).
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xbe import Xbe  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
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


def main():
    other_xbe, other_root = sys.argv[1], sys.argv[2]
    mine = shapes(Xbe(os.path.join(ROOT, 'madness_interactive_sx/default.xbe')),
                  text_functions(os.path.join(ROOT, 'build/export/functions.tsv'), 0x11000, 0x279AC))
    names = {}
    for line in open(os.path.join(other_root, 'symbols/names.txt')):
        f = line.split(None, 2)
        if len(f) >= 2 and not line.startswith('#'):
            names[int(f[0], 16)] = f[1]
    theirs = shapes(Xbe(other_xbe), text_functions(os.path.join(other_root, 'build/export/functions.tsv'), 0x11000, 0x3867D))
    by_shape = {}
    for a, s in theirs.items():
        by_shape.setdefault(s, []).append(a)
    for a, s in sorted(mine.items()):
        hits = by_shape.get(s, [])
        if len(s) >= 3 and len(hits) == 1 and hits[0] in names:
            print(f'{a:08x} {names[hits[0]]}  # {hits[0]:08x}')
        elif len(s) >= 3 and hits:
            print(f'# {a:08x} ambiguous: ' + ' '.join(f'{h:08x}={names.get(h, "?")}' for h in hits))


if __name__ == '__main__':
    main()
