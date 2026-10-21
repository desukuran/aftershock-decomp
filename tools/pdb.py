#!/usr/bin/env python3
"""Minimal PDB 7.0 reader: the data symbols of each module, including the
static ones the linker map leaves out (file statics, function-local statics).

  pdb.py <file.pdb> [filter]     print "segment:offset name (function)" lines

As a module: statics(path) -> list of (segment, offset, name, function or None).
Segments are 1-based section numbers of the .exe.
"""
import struct
import sys

S_END = 0x0006
PROCS = {0x100A, 0x100B, 0x1110, 0x1111}            # S_[LG]PROC32(_ST)
SCOPES = {0x0206, 0x1004, 0x1103, 0x0207}           # thunks, S_BLOCK32(_ST), S_WITH32: closed by S_END too
DATA = {0x1007: True, 0x1008: True, 0x110C: False, 0x110D: False}   # kind -> name is a Pascal string


class Msf:
    def __init__(self, path):
        self.data = open(path, 'rb').read()
        if not self.data.startswith(b'Microsoft C/C++ MSF 7.00'):
            raise SystemExit('not a PDB 7.0 file')
        self.bs, _, _, dir_size, _, map_block = struct.unpack_from('<6I', self.data, 32)
        nblocks = (dir_size + self.bs - 1) // self.bs
        dir_blocks = struct.unpack_from('<%dI' % nblocks, self.data, map_block * self.bs)
        d = b''.join(self._block(b) for b in dir_blocks)[:dir_size]
        n = struct.unpack_from('<I', d)[0]
        sizes = struct.unpack_from('<%dI' % n, d, 4)
        pos = 4 + 4 * n
        self.streams = []
        for size in sizes:
            if size == 0xFFFFFFFF:
                self.streams.append((0, ()))
                continue
            k = (size + self.bs - 1) // self.bs
            self.streams.append((size, struct.unpack_from('<%dI' % k, d, pos)))
            pos += 4 * k

    def _block(self, b):
        return self.data[b * self.bs:(b + 1) * self.bs]

    def stream(self, i):
        size, blocks = self.streams[i]
        return b''.join(self._block(b) for b in blocks)[:size]


def _cstr(b, pos):
    end = b.index(b'\0', pos)
    return b[pos:end].decode('latin-1'), end + 1


def modules(msf):
    """(module name, symbol stream, symbol bytes) for each module in the DBI."""
    dbi = msf.stream(3)
    mod_size = struct.unpack_from('<i', dbi, 24)[0]
    pos, end = 64, 64 + mod_size
    out = []
    while pos < end:
        stream, sym_bytes = struct.unpack_from('<HI', dbi, pos + 34)
        name, p = _cstr(dbi, pos + 64)
        _, p = _cstr(dbi, p)
        out.append((name, stream, sym_bytes))
        pos = (p + 3) & ~3
    return out


def statics(path):
    msf = Msf(path)
    out = []
    for _, stream, sym_bytes in modules(msf):
        if stream == 0xFFFF:
            continue
        s = msf.stream(stream)[:sym_bytes]
        pos, scope = 4, []
        while pos + 4 <= len(s):
            reclen, kind = struct.unpack_from('<HH', s, pos)
            body = s[pos + 4:pos + 2 + reclen]
            if kind in PROCS:
                pascal = kind in (0x100A, 0x100B)
                nm = body[35:]
                scope.append(nm[1:1 + nm[0]].decode('latin-1') if pascal else nm.split(b'\0')[0].decode('latin-1'))
            elif kind == S_END and scope:
                scope.pop()
            elif kind in DATA:
                _, off, seg = struct.unpack_from('<IIH', body)
                nm = body[10:]
                name = nm[1:1 + nm[0]].decode('latin-1') if DATA[kind] else nm.split(b'\0')[0].decode('latin-1')
                out.append((seg, off, name, scope[-1] if scope else None))
            elif kind in SCOPES:
                scope.append(scope[-1] if scope else None)
            pos += 2 + reclen
    return out


def main():
    flt = sys.argv[2] if len(sys.argv) > 2 else ''
    for seg, off, name, fn in statics(sys.argv[1]):
        if flt in name or (fn and flt in fn):
            print('%04x:%08x %s%s' % (seg, off, name, ' (%s)' % fn if fn else ''))


if __name__ == '__main__':
    main()
