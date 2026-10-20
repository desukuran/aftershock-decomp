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
import re
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
        self.watches = {}
        self.uc.hook_add(UC_HOOK_CODE, self._hook)

    def alloc(self, n):
        p = self.heap
        self.heap += (n + 15) & ~15
        return p

    def stub(self, addr, nargs, fn, stdcall=True):
        self.stubs[addr] = (nargs, fn, stdcall)

    def watch(self, addr, nargs, fn):
        """Calls fn(emu, args) when the function at addr is entered; it still runs."""
        self.watches[addr] = (nargs, fn)

    def _hook(self, uc, addr, size, _):
        if addr in self.watches:
            nargs, fn = self.watches[addr]
            esp = uc.reg_read(UC_X86_REG_ESP)
            fn(self, struct.unpack('<%dI' % nargs, uc.mem_read(esp + 4, 4 * nargs)))
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


def wstr(emu, a, n=200):
    raw = bytes(emu.uc.mem_read(a, n * 2))
    return raw.decode('utf-16le', 'replace').split('\x00')[0]


def f32(v):
    return struct.unpack('<f', struct.pack('<I', v))[0]


def vec(emu, a):
    return '(%g, %g, %g)' % emu.read(a, 'fff')


def trace_calls(emu, log):
    """Stubs the device and drawing calls a render function makes, logging
    them; the D3DX math runs as it is (LookAt is logged too)."""
    def add(addr, name, nargs, fmt=None, stdcall=True, ret=0):
        def fn(e, a):
            log.append(f'{name}({fmt(e, a) if fmt else ", ".join(hex(x) for x in a)})')
            return ret
        emu.stub(addr, nargs, fn, stdcall)

    add(0x00011860, 'DrawText', 6, lambda e, a: f'"{wstr(e, a[0])}", {f32(a[1]):g}, {f32(a[2]):g}, {a[3]}, {a[4]}, {a[5]}')
    add(0x000167E0, 'DrawRect', 5, lambda e, a: f'{f32(a[0]):g}, {f32(a[1]):g}, {f32(a[2]):g}, {f32(a[3]):g}, {a[4]:#x}', stdcall=False)
    add(0x00012EA0, 'STablePiece::Render', 2)
    add(0x00036770, 'CMesh::Render', 2)
    add(0x00016EE0, 'RenderRoom', 0, stdcall=False)
    add(0x00013000, 'DrawAimGuide', 0, stdcall=False)
    add(0x0004B670, 'SetTransform', 2, lambda e, a: f'{a[0]}, [{", ".join("%g" % v for v in e.read(a[1], "16f"))}]')
    add(0x0004BCC0, 'SetViewport', 1, lambda e, a: 'x=%d y=%d w=%d h=%d' % e.read(a[0], 'IIII'))
    add(0x0004C1B0, 'LightEnable', 2)
    add(0x0004C310, 'SetTexture', 2)
    add(0x0004CEA0, 'SetRenderState_Simple', 0)
    add(0x0004D360, 'SetRenderState_TextureFactor', 1)
    add(0x0004E0F0, 'SetRenderState_ZEnable', 1)
    add(0x0004D270, 'SetRenderState_CullMode', 1)
    add(0x0004F250, 'Clear', 6, lambda e, a: f'{a[2]:#x}, {a[3]:#x}, {f32(a[4]):g}, {a[5]}')
    add(0x0004F840, 'DrawVertices', 3)
    add(0x0004F6D0, 'DrawVerticesUP', 4)
    add(0x0004F920, 'SetVertexData4f', 5, lambda e, a: f'{a[0]}, {f32(a[1]):g}, {f32(a[2]):g}, {f32(a[3]):g}, {f32(a[4]):g}')
    add(0x0004F980, 'Begin', 1)
    add(0x0004F9C0, 'End', 0)
    add(0x0004FC70, 'SetStreamSource', 3)
    add(0x00050050, 'SetVertexShader', 1)
    add(0x0004B790, 'SetMaterial', 1)

    def swprintf(e, a):
        fmt = wstr(e, a[1])
        args = list(a[2:])
        out = re.sub(r'%(\d*)([dSsif])', lambda m: '{%s}' % m.group(2), fmt)
        vals = []
        for m in re.finditer(r'%(\d*)([dSsif])', fmt):
            v = args.pop(0)
            if m.group(2) in 'di':
                vals.append(str(struct.unpack('<i', struct.pack('<I', v))[0]))
            elif m.group(2) == 'S':
                vals.append(bytes(e.uc.mem_read(v, 64)).split(b'\0')[0].decode('latin-1'))
            elif m.group(2) == 's':
                vals.append(wstr(e, v))
            else:
                lo = v
                hi = args.pop(0)
                vals.append('%f' % struct.unpack('<d', struct.pack('<II', lo, hi))[0])
        text = fmt
        for v in vals:
            text = re.sub(r'%\d*[dSsif]', lambda m: v.replace('\\', '\\\\'), text, count=1)
        e.uc.mem_write(a[0], text.encode('utf-16le') + b'\0\0')
        log.append(f'swprintf -> "{text}"')
        return len(text)
    emu.stub(0x0004180E, 8, swprintf, stdcall=False)

    def lookat(e, a):
        log.append(f'LookAt(eye={vec(e, a[1])}, at={vec(e, a[2])}, up={vec(e, a[3])})')
    emu.watch(0x0005E459, 4, lookat)


GAME = 0x000BBC20


def game_emu():
    """An emulator with g_Game's strings constructed (empty SSO strings)."""
    emu = Emu()
    strings = [GAME + 0x110 + i * 0x24 for i in range(3)] + \
              [GAME + 0xA84 + i * 0x1C for i in range(10)] + [GAME + 0xB9C + i * 0x1C for i in range(10)]
    for a in strings:
        emu.uc.mem_write(a + 4, b'slot' + bytes(12))
        emu.uc.mem_write(a + 0x14, struct.pack('<II', 4, 15))
    emu.uc.mem_write(GAME + 0xA04, struct.pack('<I', 0x00C00000))   # device (never dereferenced)
    return emu


def poke(emu, addr, fmt, *values):
    emu.uc.mem_write(addr, struct.pack('<' + fmt, *values))


def editor(screen, mode=0, *extra):
    """Trace RenderEditor on one editor screen: editor.py editor <screen> [mode] [addr=value...]."""
    emu = game_emu()
    poke(emu, GAME + 0x19C, 'i', 5)                 # g_Game.state = STATE_EDITOR
    poke(emu, 0x000BA284, 'i', int(screen))         # the editor screen
    poke(emu, GAME + 0x108, 'i', int(mode))         # g_Game.editorMode
    for kv in extra:
        k, v = kv.split('=')
        poke(emu, int(k, 16), 'f' if '.' in v else 'i', float(v) if '.' in v else int(v, 0))
    log = []
    trace_calls(emu, log)
    emu.call(0x0002AC30)
    print('\n'.join(log))


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
    {'quadbuffers': quadbuffers, 'pockets': pockets, 'barriers': barriers, 'editor': editor}[sys.argv[1]](*sys.argv[2:])
