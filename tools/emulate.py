#!/usr/bin/env python3
"""Run a function of default.xbe in an x86 emulator (Unicorn) with library
calls stubbed out, to read data the code builds (vertex buffers, tables)
exactly instead of through the decompiler.

  emulate.py quadbuffers      the vertices CreateQuadBuffers writes

As a module: Emu(...).call(addr, *args) runs until the function returns;
emu.stub(addr, nargs, fn) replaces a function (stdcall: the stub pops its
arguments; fn(emu, args) returns eax).
"""
import os
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = 0x10000
STACK = 0x00F00000
HEAP = 0x01000000
RETURN = 0x00D00000   # fake return address: emulation stops here


class Emu:
    def __init__(self):
        img = open(os.path.join(ROOT, 'build/default.flat'), 'rb').read()
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        size = (len(img) + 0x20000 + 0xFFF) & ~0xFFF
        self.uc.mem_map(BASE, size)
        self.uc.mem_write(BASE, img)
        self.uc.mem_map(RETURN, 0x1000)
        self.uc.mem_map(STACK - 0x100000, 0x100000)
        self.uc.mem_map(HEAP, 0x1000000)
        self.heap = HEAP
        self.stubs = {}
        self.uc.hook_add(UC_HOOK_CODE, self._hook)

    def alloc(self, n):
        p = self.heap
        self.heap += (n + 15) & ~15
        return p

    def stub(self, addr, nargs, fn, stdcall=True):
        self.stubs[addr] = (nargs, fn, stdcall)

    def _hook(self, uc, addr, size, _):
        if addr in self.stubs:
            nargs, fn, stdcall = self.stubs[addr]
            esp = uc.reg_read(UC_X86_REG_ESP)
            ret, *args = struct.unpack('<%dI' % (nargs + 1), uc.mem_read(esp, 4 * (nargs + 1)))
            uc.reg_write(UC_X86_REG_EAX, fn(self, args) or 0)
            uc.reg_write(UC_X86_REG_ESP, esp + 4 + (4 * nargs if stdcall else 0))
            uc.reg_write(UC_X86_REG_EIP, ret)

    def call(self, addr, *args, ecx=0):
        esp = STACK - 0x1000 - 4 * (len(args) + 1)
        self.uc.mem_write(esp, struct.pack('<%dI' % (len(args) + 1), RETURN, *args))
        self.uc.reg_write(UC_X86_REG_ESP, esp)
        self.uc.reg_write(UC_X86_REG_ECX, ecx)
        self.uc.emu_start(addr, RETURN)
        return self.uc.reg_read(UC_X86_REG_EAX)

    def read(self, addr, fmt):
        return struct.unpack('<' + fmt, self.uc.mem_read(addr, struct.calcsize('<' + fmt)))


def quadbuffers():
    """CreateQuadBuffers: four 4-vertex buffers (MESHVERTEX: xyz, normal, color, uv)."""
    emu = Emu()
    buffers = []

    def create(e, args):
        buf = e.alloc(args[0])
        e.uc.mem_write(buf, b'\xCD' * args[0])   # uninitialized
        buffers.append(buf)
        return buf

    emu.stub(0x0004F180, 1, create)              # D3DDevice_CreateVertexBuffer2(length)
    emu.stub(0x0004F1D0, 2, lambda e, a: a[0])   # D3DVertexBuffer_Lock2(vb, flags) -> data
    emu.call(0x000254E0)
    for i, buf in enumerate(buffers):
        print(f'buffer {i}:')
        for v in range(4):
            x, y, z, nx, ny, nz, color, tu, tv = emu.read(buf + v * 36, 'fffIIIIff')
            print(f'  pos ({x:g}, {y:g}, {z:g}) normal {nx:08x} {ny:08x} {nz:08x} color {color:08x} uv ({tu:g}, {tv:g})')



TABLE_HEAD = 0x000BBC38     # g_Game.table
NUM_POCKETS = 0x000BA294
POCKETS = 0x000BB6B0
NUM_BARRIERS = 0x000BA208
BARRIERS = 0x000BA3F0


def run_piece(fn, ptype, mesh, rot, pos=(0.0, 0.0, 0.0)):
    """Runs a table builder on a table of one piece; returns the emulator."""
    emu = Emu()
    emu.uc.mem_write(TABLE_HEAD, struct.pack('<fffiifI', pos[0], pos[1], pos[2], ptype, mesh, rot, 0))
    emu.call(fn)
    return emu


def rotate(ox, oz, rot):
    c, s = {0: (1, 0), 1: (0, -1), 2: (-1, 0), 3: (0, 1)}[rot]
    return ox * c - oz * s, oz * c + ox * s


def piece_points(fn, count_addr, array_addr, stride):
    """{(type, mesh): [(x, z), ...]} at rotation 0, checked at rotations 1-3."""
    out = {}
    for ptype in range(3):
        for mesh in range(32):
            emu = run_piece(fn, ptype, mesh, 0.0)
            n = emu.read(count_addr, 'i')[0]
            if n <= 0:
                continue
            pts = [emu.read(array_addr + i * stride, 'fff') for i in range(min(n, 400))]
            out[(ptype, mesh)] = (n, pts)
            for rot in (1, 2, 3):
                e2 = run_piece(fn, ptype, mesh, float(rot), pos=(100.0, 7.0, -50.0))
                n2 = e2.read(count_addr, 'i')[0]
                assert n2 == n, (ptype, mesh, rot, n, n2)
                for i, (x, y, z) in enumerate(pts):
                    x2, y2, z2 = e2.read(array_addr + i * stride, 'fff')
                    if (x, y, z) == (0.0, 0.0, 0.0) and (x2, y2, z2) == (0.0, 0.0, 0.0):
                        continue    # a separator
                    rx, rz = rotate(x, z, rot)
                    assert abs(x2 - (100 + rx)) < 1e-3 and abs(z2 - (-50 + rz)) < 1e-3 and abs(y2 - (7 + y)) < 1e-3, \
                        (ptype, mesh, rot, i, (x, y, z), (x2, y2, z2))
    return out


def barriers():
    """Each piece's barrier points (S marks a (0,0,0) separator)."""
    for ptype in range(3):
        for mesh in range(32):
            emu = run_piece(0x00017800, ptype, mesh, 0.0)
            n = emu.read(NUM_BARRIERS, 'i')[0]
            if n <= 0:
                continue
            e2 = run_piece(0x00017800, ptype, mesh, 1.0, pos=(100.0, 7.0, -50.0))
            pts = []
            for i in range(n + 1):
                x, y, z = emu.read(BARRIERS + i * 12, 'fff')
                x2, y2, z2 = e2.read(BARRIERS + i * 12, 'fff')
                if (x2, y2, z2) == (0.0, 0.0, 0.0):
                    assert (x, y, z) == (0.0, 0.0, 0.0)
                    pts.append('S')
                    continue
                rx, rz = rotate(x, z, 1)
                assert abs(x2 - (100 + rx)) < 1e-3 and abs(z2 - (-50 + rz)) < 1e-3 and y == 0.0 and y2 == 7.0, (ptype, mesh, i)
                pts.append(f'{x:.9g},{z:.9g}')
            print(ptype, mesh, n, ' '.join(pts))


def pockets():
    for (ptype, mesh), (n, pts) in sorted(piece_points(0x00023650, NUM_POCKETS, POCKETS, 16).items()):
        print(ptype, mesh, n, ' '.join(f'({x:g},{z:g})' for x, y, z in pts))


if __name__ == '__main__':
    {'quadbuffers': quadbuffers, 'pockets': pockets, 'barriers': barriers}[sys.argv[1]]()
