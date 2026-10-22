#!/usr/bin/env python3
"""Map the absolute data accesses of the game code in an address range.

  globalmap.py LO HI          e.g. globalmap.py a6128 a9e34

For each address: the access sizes, whether it's indexed ([disp + reg]),
whether it's taken as an address (push/mov imm), and the functions using it.
"""
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402
from capstone.x86 import X86_OP_MEM, X86_OP_IMM  # noqa: E402
from xbe import Xbe  # noqa: E402

from gameconf import ROOT, ORIGINAL, APP_RANGES  # noqa: E402


def main():
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    x = Xbe(ORIGINAL)
    img = x.flat_image()
    funcs = []
    with open(os.path.join(ROOT, 'build/export/functions.tsv')) as f:
        next(f)
        for line in f:
            a, size, block, name = line.split('\t')[:4]
            a = int(a, 16)
            if block == '.text' and any(lo_ <= a < hi_ for lo_, hi_ in APP_RANGES):
                funcs.append((a, int(size), name))
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    info = defaultdict(lambda: {'sizes': set(), 'indexed': False, 'addr': False, 'funcs': set()})
    for a, size, name in funcs:
        for ins in md.disasm(img[a - x.base:a - x.base + size], a):
            for op in ins.operands:
                if op.type == X86_OP_MEM and op.mem.base == 0 or (op.type == X86_OP_MEM and op.mem.index != 0 and op.mem.base == 0):
                    d = op.mem.disp & 0xFFFFFFFF
                    if lo <= d < hi:
                        e = info[d]
                        e['sizes'].add(op.size)
                        e['indexed'] |= op.mem.index != 0
                        e['funcs'].add(f'{a:05x}')
                elif op.type == X86_OP_MEM and op.mem.base != 0:
                    d = op.mem.disp & 0xFFFFFFFF
                    if lo <= d < hi:
                        e = info[d]
                        e['sizes'].add(op.size)
                        e['indexed'] = True
                        e['funcs'].add(f'{a:05x}')
                elif op.type == X86_OP_IMM:
                    d = op.imm & 0xFFFFFFFF
                    if lo <= d < hi:
                        e = info[d]
                        e['addr'] = True
                        e['funcs'].add(f'{a:05x}')
    for d in sorted(info):
        e = info[d]
        print(f'{d:08x} +{d - lo:04x} size={",".join(map(str, sorted(e["sizes"]))) or "-"}'
              f'{" idx" if e["indexed"] else ""}{" &" if e["addr"] else ""}  {" ".join(sorted(e["funcs"]))}')


if __name__ == '__main__':
    main()
