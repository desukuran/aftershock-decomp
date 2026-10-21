#!/usr/bin/env python3
"""Patch the original default.xbe so it draws in xemu, for side-by-side tests.

  xemu_patch.py <original default.xbe> <out.xbe>

InitDevice (0x12200) builds its projection with a near plane of 0, which puts
every vertex's depth at exactly 1.0; real hardware draws that but xemu clips
it all away. The rebuild's XEMU=1 build uses 1.0 instead, and this does the
same to the original: the `push edi` (0.0) at 0x12352 can't hold a float
constant in place, so 0x1234D jumps to the 20 spare bytes at the end of .text,
which push 1000.0 and 1.0 and jump back. .text grows by those 20 bytes.
"""
import struct
import sys

PATCH_AT = 0x1234D          # push 0x447A0000 (1000.0) ; push edi (0.0)
RESUME = 0x12353            # push 0x3FAAAAAB (4/3)
OLD = bytes.fromhex('6800007a4457')


def main():
    src, dst = sys.argv[1], sys.argv[2]
    d = bytearray(open(src, 'rb').read())
    base, = struct.unpack_from('<I', d, 0x104)
    sec_addr, = struct.unpack_from('<I', d, 0x120)
    so = sec_addr - base                        # the first section: .text
    flags, va, vs, raw, rs = struct.unpack_from('<5I', d, so)
    _, nva = struct.unpack_from('<2I', d, so + 56)
    cave = va + vs
    code = (b'\x68' + struct.pack('<f', 1000.0) + b'\x68' + struct.pack('<f', 1.0)
            + b'\xe9' + struct.pack('<i', RESUME - (cave + 15)))
    if nva - cave < len(code) or d[raw + vs:raw + vs + len(code)].strip(b'\0'):
        sys.exit('no room after .text')
    po = raw + PATCH_AT - va
    if d[po:po + 6] != OLD:
        sys.exit('unexpected bytes at %#x' % PATCH_AT)
    d[po:po + 6] = b'\xe9' + struct.pack('<i', cave - (PATCH_AT + 5)) + b'\x90'
    d[raw + vs:raw + vs + len(code)] = code
    struct.pack_into('<I', d, so + 8, vs + len(code))
    struct.pack_into('<I', d, so + 16, rs + len(code))
    open(dst, 'wb').write(d)
    print('patched', dst)


if __name__ == '__main__':
    main()
