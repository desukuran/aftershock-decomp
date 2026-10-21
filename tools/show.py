#!/usr/bin/env python3
"""Print raw Ghidra decompiler output from build/export/decomp.

  show.py <addr|name> [...]        single functions
  show.py <start>-<end>            every function whose address is in [start, end)
"""
import glob
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gameconf import ROOT, REPO, D3D_TEXTURESTATE, D3D_RENDERSTATE  # noqa: E402


def constants():
    """flt_/dbl_ names from symbols/consts.txt -> C literals of their values."""
    path = os.path.join(ROOT, 'symbols/consts.txt')
    img = open(os.path.join(ROOT, 'build/default.flat'), 'rb').read()
    out = {}
    for line in open(path):
        if line.startswith('#'):
            continue
        a, name, kind = line.split()
        a = int(a, 16)
        if kind == 'data:float':
            v = struct.unpack_from('<f', img, a - 0x10000)[0]
            lit = f'{v:.7g}'
            lit = (lit if any(c in lit for c in '.e') else lit + '.0') + 'f'
        else:
            v = struct.unpack_from('<d', img, a - 0x10000)[0]
            lit = f'{v:.15g}'
            lit = lit if any(c in lit for c in '.e') else lit + '.0'
        out[name] = lit
    return out




def d3d_states():
    """DAT_ names of the Xbox D3D state arrays -> D3D__RenderState[D3DRS_x] etc.
    The XDK's inline SetRenderState/SetTextureStageState write them directly."""
    path = os.path.join(REPO, 'reference/5558/xbox/include/D3D8Types.h')
    if not os.path.exists(path):
        return {}
    rs, tss = {}, {}
    for m in re.finditer(r'^\s*(D3D(RS|TSS)_\w+)\s*=\s*(\d+)\s*,', open(path, errors='replace').read(), re.M):
        name, kind, val = m.group(1), m.group(2), int(m.group(3))
        if name.endswith('_MAX') or 'FORCE' in name:
            continue
        (rs if kind == 'RS' else tss).setdefault(val, name)
    out = {}
    for i, name in rs.items():
        out[f'DAT_{D3D_RENDERSTATE + 4 * i:08x}'] = f'D3D__RenderState[{name}]'
    for stage in range(4):
        for i, name in tss.items():
            out[f'DAT_{D3D_TEXTURESTATE + 0x80 * stage + 4 * i:08x}'] = f'D3D__TextureState[{stage}][{name}]'
    return out


def main():
    consts = constants()
    consts.update(d3d_states())
    pat = re.compile(r'\b_*(?:(?:flt|dbl)_000[0-9a-f]{5}|DAT_000[0-9a-f]{5})\b')
    funcs = []
    for path in sorted(glob.glob(os.path.join(ROOT, 'build/export/decomp/*.c'))):
        for part in re.split(r'(?m)^// ==== ', open(path).read())[1:]:
            name, addr = re.match(r'(.+?) @ ([0-9a-f]+)', part).groups()
            text = pat.sub(lambda m: consts.get(m.group(0).lstrip('_'), m.group(0)), part.strip())
            funcs.append((int(addr, 16), name, '// ==== ' + text))
    for arg in sys.argv[1:]:
        if '-' in arg:
            lo, hi = (int(v, 16) for v in arg.split('-'))
            sel = [f for f in funcs if lo <= f[0] < hi]
        else:
            sel = [f for f in funcs if f[1] == arg or (re.fullmatch(r'[0-9a-fA-F]+', arg) and f[0] == int(arg, 16))]
        for f in sorted(sel):
            print(f[2] + '\n')


if __name__ == '__main__':
    main()
