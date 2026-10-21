#!/usr/bin/env python3
"""Print the call graph of a code range: each function with its callers and callees.

  callgraph.py <start>-<end>
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from capstone import Cs, CS_ARCH_X86, CS_MODE_32  # noqa: E402


def main():
    lo, hi = (int(v, 16) for v in sys.argv[1].split('-'))
    funcs = {}
    with open(os.path.join(ROOT, 'build/export/functions.tsv')) as f:
        next(f)
        for line in f:
            a, size, block, name = line.split('\t')[:4]
            funcs[int(a, 16)] = (int(size), name)
    img = open(os.path.join(ROOT, 'build/default.flat'), 'rb').read()
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    callees, callers = {}, {}
    for a, (size, name) in funcs.items():
        if not lo <= a < hi:
            continue
        out = []
        for ins in md.disasm(img[a - 0x10000:a - 0x10000 + size], a):
            if ins.mnemonic in ('call', 'jmp') and ins.op_str.startswith('0x') and int(ins.op_str, 16) in funcs:
                t = int(ins.op_str, 16)
                out.append(t)
                callers.setdefault(t, []).append(a)
        callees[a] = out
    nm = lambda t: funcs.get(t, (0, f'{t:x}'))[1]
    for a in sorted(callees):
        game = sorted({nm(t) for t in callees[a] if lo <= t < hi})
        lib = sorted({nm(t) for t in callees[a] if not lo <= t < hi})
        print(f'{a:08x} {nm(a)} [{funcs[a][0]}]  <- {", ".join(sorted({nm(c) for c in callers.get(a, [])}))}')
        if game:
            print(f'    calls: {", ".join(game)}')
        if lib:
            print(f'    lib:   {", ".join(lib)}')


if __name__ == '__main__':
    main()
