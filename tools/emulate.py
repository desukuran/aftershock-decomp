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


if __name__ == '__main__':
    {'quadbuffers': quadbuffers}[sys.argv[1]]()
