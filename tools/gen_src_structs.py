#!/usr/bin/env python3
"""Generate Ghidra struct declarations from the class layouts in src/*.h.

  gen_src_structs.py            writes build/src_structs.h and build/layout_check.cpp

A class or struct whose data members carry offset comments,

    class CGame
    {
        SPieceSet    pieceSets[3];          // +0x000
        ...
    };                                      // 0xCF4 bytes

becomes a C struct for Ghidra (tools/apply.sh appends it to the types), with
padding where members are missing and its size checked against the size
comment. The source headers stay the one place a layout is written down.
Pointers to types this script knows keep their type, other pointers become
void *; Xbox/STL types are mapped to their symbols/types.h equivalents.

A class derived from another one here (class CTie : public CShip) gets
its base's members first, and a class that declares a virtual function
without a base gets a vftable pointer at +0x00.

build/layout_check.cpp asserts every offset and size at compile time
(the build compiles it), so the C++ layouts can't drift from the binary's.

  gen_src_structs.py --pads     lists the gaps in the C++ classes, where the
                                compiler would pack later members too early
  gen_src_structs.py --pads fix inserts the BYTE padXXXX[n] members for them

With DISCOVER=1 in the environment, gaps become DWORD slots named u_XXX
(bytes where unaligned) instead of byte arrays, so Ghidra's decompiler names
every unknown access by its offset.
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gameconf import ROOT  # noqa: E402
DISCOVER = os.environ.get('DISCOVER') == '1'


def gap(pos, end):
    """Declarations filling [pos, end)."""
    if not DISCOVER:
        return [f'\tBYTE _pad{pos:04x}[{end - pos:#x}];']
    out = []
    while pos < end:
        if pos % 4 == 0 and end - pos >= 4:
            out.append(f'\tDWORD u_{pos:03x};')
            pos += 4
        else:
            out.append(f'\tBYTE u_{pos:03x};')
            pos += 1
    return out

# Types in symbols/types.h or Ghidra's builtins, with their sizes.
BASIC = {
    'char': ('char', 1), 'BYTE': ('BYTE', 1), 'bool': ('bool', 1), 'WORD': ('WORD', 2), 'short': ('short', 2),
    'SHORT': ('short', 2), 'WCHAR': ('wchar_t', 2), 'wchar_t': ('wchar_t', 2), 'int': ('int', 4), 'UINT': ('uint', 4),
    'DWORD': ('DWORD', 4), 'BOOL': ('BOOL', 4), 'LONG': ('long', 4), 'long': ('long', 4), 'float': ('float', 4),
    'FLOAT': ('float', 4), 'HANDLE': ('HANDLE', 4), 'HRESULT': ('HRESULT', 4), 'D3DCOLOR': ('DWORD', 4),
    'double': ('double', 8), 'std::string': ('std_string', 0x1C),
    'D3DXVECTOR2': ('D3DXVECTOR2', 8), 'D3DXVECTOR3': ('D3DXVECTOR3', 12), 'D3DXVECTOR4': ('D3DXVECTOR4', 16),
    'D3DXQUATERNION': ('D3DXQUATERNION', 16), 'D3DXMATRIX': ('D3DXMATRIX', 64), 'D3DLIGHT8': ('D3DLIGHT8', 0x68),
    'XINPUT_STATE': ('XINPUT_STATE', 0x16), 'XBGAMEPAD': ('XBGAMEPAD', 0xDC),
}

# XDK structs that symbols/types.h doesn't declare: opaque bytes for Ghidra.
OPAQUE = {'WAVEFORMATEX': 0x12, 'WMAXMOFileContDesc': 0x1E, 'WMAXMOFileHeader': 0x14}

CLASS = re.compile(r'^(?:class|struct)\s+(\w+)\s*(?::\s*(?:public\s+)?(\w+)[^{]*)?$')
MEMBER = re.compile(r'^\s*(?:struct\s+)?([\w:]+(?:\s*\*+)?)\s+(\w+)\s*(?:\[([^\]]+)\])?\s*;\s*//\s*\+0x([0-9A-Fa-f]+)')
END = re.compile(r'^\};\s*(?://\s*0x([0-9A-Fa-f]+) bytes)?')


def defines(paths):
    out = {}
    for path in paths:
        text = open(path).read()
        for m in re.finditer(r'^#define\s+(\w+)\s+(.+?)\s*(?://.*)?$', text, re.M):
            out[m.group(1)] = m.group(2)
        # enum constants with explicit values ("NAME = 6,")
        for m in re.finditer(r'^\s*([A-Z][A-Z0-9_]*)\s*=\s*(\d+)\s*,', text, re.M):
            out.setdefault(m.group(1), m.group(2))
    return out


def evaluate(expr, macros):
    for _ in range(5):
        expr = re.sub(r'\b[A-Z_][A-Z0-9_]*\b', lambda m: macros.get(m.group(0), m.group(0)), expr)
    return int(eval(expr, {}, {}))


def parse(path, macros):
    """Yields (name, size or None, [(offset, type, member, count)], base, virtual)."""
    name, members = None, []
    for line in open(path):
        line = line.rstrip()
        m = CLASS.match(line)
        if m:
            name, base, virtual, members = m.group(1), m.group(2), False, []
            continue
        if name is None:
            continue
        if re.match(r'^\s*virtual\b', line):
            virtual = True
        m = MEMBER.match(line)
        if m:
            typ, member, count, off = m.groups()
            members.append((int(off, 16), re.sub(r'\s+', '', typ), member,
                            evaluate(count, macros) if count else None))
            continue
        m = END.match(line)
        if m:
            if members or base:
                yield name, int(m.group(1), 16) if m.group(1) else None, members, base, virtual
            name = None


def main():
    headers = sorted(glob.glob(os.path.join(ROOT, 'src/*.h')))
    macros = defines(headers)
    structs = {}
    parsed = {}
    for path in headers:
        for name, size, members, base, virtual in parse(path, macros):
            parsed[name] = (size, members, os.path.basename(path), base, virtual)

    def inherited(name):
        """A class's own members after its base's (and the vftable of a root class with virtuals)."""
        size, members, src, base, virtual = parsed[name]
        if base in parsed:
            return inherited(base) + members
        if virtual:
            return [(0, 'void*', 'vftable', None)] + members
        return members

    structs = {name: (v[0], inherited(name), v[2]) for name, v in parsed.items()}
    own = {name: v[1] for name, v in parsed.items()}

    sizes = {k: v[1] for k, v in BASIC.items()}
    out_types = {}

    def ctype(typ):
        """(C type for Ghidra, size, by-value dependency)"""
        if typ.endswith('*'):
            base = typ.rstrip('*')
            if base in structs:
                return f'struct {base} *', 4, None
            if base in BASIC:
                return f'{BASIC[base][0]} *', 4, None
            return 'void *', 4, None
        if typ.startswith(('LP', 'PX')):
            return 'void *', 4, None
        if typ in BASIC:
            return BASIC[typ][0], BASIC[typ][1], None
        if typ in OPAQUE:
            return 'BYTE', OPAQUE[typ], None
        if typ in structs:
            return f'struct {typ}', None, typ
        raise ValueError(f'unknown type {typ!r}')

    done, order = set(), []

    def visit(name, stack=()):
        if name in done:
            return
        if name in stack:
            raise ValueError(f'cycle at {name}')
        for _, typ, _, _ in structs[name][1]:
            dep = ctype(typ)[2]
            if dep:
                visit(dep, stack + (name,))
        done.add(name)
        order.append(name)

    for name in sorted(structs):
        visit(name)

    out = ['/* Generated by tools/gen_src_structs.py from src/*.h -- do not edit */', '']
    out += [f'typedef struct {name} {name};' for name in order] + ['']
    for name in order:
        size, members, src = structs[name]
        body, pos = [], 0
        for off, typ, member, count in sorted(members):
            if re.fullmatch(r'pad[0-9A-Fa-f]+', member):
                continue    # explicit C++ padding: a gap
            c, n, dep = ctype(typ)
            if dep:
                n = sizes[dep]
            if off < pos:
                sys.exit(f'{src}: {name}::{member} at {off:#x} overlaps the previous member (ends {pos:#x})')
            if off > pos:
                body += gap(pos, off)
            if typ in OPAQUE:
                count = n * (count or 1)
                n = 1
            body.append(f'\t{c} {member}{f"[{count}]" if count else ""}; /* +{off:#x} */')
            pos = off + n * (count or 1)
        if size is None:
            size = pos
        if pos > size:
            sys.exit(f'{src}: {name} members end at {pos:#x}, past its size {size:#x}')
        if pos < size:
            body += gap(pos, size)
        sizes[name] = size
        out += [f'struct {name} {{', *body, '};', '']
    if '--pads' in sys.argv:
        return pads(parsed, structs, sizes, ctype, '--pads' != sys.argv[-1] and sys.argv[-1] == 'fix')
    os.makedirs(os.path.join(ROOT, 'build'), exist_ok=True)
    with open(os.path.join(ROOT, 'build/src_structs.h'), 'w') as f:
        f.write('\n'.join(out))

    check = ['// Generated by tools/gen_src_structs.py -- do not edit', '#include <stddef.h>']
    check += [f'#include "{os.path.basename(p)}"' for p in headers]
    for name in order:
        size = structs[name][0]
        for off, _, member, _ in own[name]:
            if re.fullmatch(r'pad[0-9A-Fa-f]+', member):
                continue
            check.append(f'C_ASSERT(offsetof({name}, {member}) == {off:#x});')
        if size is not None:
            check.append(f'C_ASSERT(sizeof({name}) == {size:#x});')
    with open(os.path.join(ROOT, 'build/layout_check.cpp'), 'w') as f:
        f.write('\n'.join(check) + '\n')
    print(f'{len(order)} structs from src/')


ALIGN1 = ('char', 'BYTE', 'bool') + tuple(OPAQUE)
ALIGN2 = ('WORD', 'short', 'SHORT', 'WCHAR', 'wchar_t')


def pads(parsed, structs, sizes, ctype, fix):
    """Where a class's members leave a gap the compiler won't (it only pads
    to each member's alignment), list it, or with fix insert a BYTE padXXXX
    member into the header."""
    def align(typ):
        if typ in ALIGN1:
            return 1
        if typ in ALIGN2:
            return 2
        return 4

    edits = {}
    for name, (size, members, src, base, virtual) in parsed.items():
        pos = sizes[base] if base in parsed else (4 if virtual else 0)
        gaps = []
        for off, typ, member, count in sorted(members):
            c, n, dep = ctype(typ)
            if dep:
                n = sizes[dep]
            a = align(typ)
            natural = (pos + a - 1) // a * a
            if off > natural and not re.fullmatch(r'pad[0-9A-Fa-f]+', member):
                gaps.append((pos, off, member))
            pos = max(pos, off + n * (count or 1))
        if size is not None and size > (pos + 3) // 4 * 4:
            gaps.append((pos, size, None))
        for lo, hi, member in gaps:
            print(f'{src}: {name} +{lo:#x}..+{hi:#x} ({hi - lo:#x} bytes) before {member or "the end"}')
            edits.setdefault(src, []).append((name, lo, hi, member))
    if not fix:
        return
    for src, items in edits.items():
        path = os.path.join(ROOT, 'src', src)
        lines = open(path).read().split('\n')
        for name, lo, hi, member in items:
            start = next(i for i, l in enumerate(lines) if CLASS.match(l) and CLASS.match(l).group(1) == name)
            for i in range(start + 1, len(lines)):
                if (member and re.search(r'\b%s\s*(\[[^\]]*\])?\s*;\s*//' % member, lines[i])) or \
                        (not member and END.match(lines[i])):
                    width = len(f'{sizes.get(name, hi):x}') if hi > 0xFF else 2
                    pad = f'    BYTE         pad{lo:0{width}X}[0x{hi - lo:X}];'
                    lines.insert(i, f'{pad:<40}// +0x{lo:0{max(width, 2)}X}')
                    break
        open(path, 'w').write('\n'.join(lines))


if __name__ == '__main__':
    main()
