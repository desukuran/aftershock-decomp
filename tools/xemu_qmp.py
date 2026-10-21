#!/usr/bin/env python3
"""Talk to a running xemu's QMP monitor (started with
-qmp tcp:127.0.0.1:44444,server,nowait).

  xemu_qmp.py shot <out.png> [port]     screenshot of the scanout buffer in guest RAM
  xemu_qmp.py hmp "<command>" [port]    run a human-monitor command
  xemu_qmp.py quit [port]
"""
import json
import socket
import sys


def qmp(port, cmd, args=None):
    s = socket.create_connection(('127.0.0.1', port), timeout=30)
    f = s.makefile('rw')
    json.loads(f.readline())                        # greeting
    for c, a in (('qmp_capabilities', None), (cmd, args)):
        msg = {'execute': c}
        if a:
            msg['arguments'] = a
        f.write(json.dumps(msg) + '\n')
        f.flush()
        while True:
            reply = json.loads(f.readline())
            if 'return' in reply or 'error' in reply:
                break
    s.close()
    return reply


def hmp(port, line):
    return qmp(port, 'human-monitor-command', {'command-line': line}).get('return', '')


def bgrx_to_png(src, dst, w, h):
    import struct
    import zlib
    d = open(src, 'rb').read()
    rgb = bytearray(w * h * 3)
    rgb[0::3] = d[2::4]
    rgb[1::3] = d[1::4]
    rgb[2::3] = d[0::4]
    raw = b''.join(b'\0' + bytes(rgb[y * w * 3:(y + 1) * w * 3]) for y in range(h))
    chunk = lambda t, d: struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0)) + \
        chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b'')
    open(dst, 'wb').write(png)


def main():
    cmd = sys.argv[1]
    port = int(sys.argv[3]) if len(sys.argv) > 3 else 44444
    if cmd == 'shot':
        # xemu has no screendump; read NV_PCRTC_START and save that memory
        # (a 640x480 X8R8G8B8 front buffer, which xemu keeps up to date).
        import os
        import re
        out = os.path.abspath(sys.argv[2])
        start = re.search(r'0x([0-9a-f]+)\s*$', hmp(port, 'xp /1wx 0xfd600800')).group(1)
        raw = out + '.raw'
        hmp(port, 'pmemsave 0x%s %d "%s"' % (start, 640 * 480 * 4, raw))
        bgrx_to_png(raw, out, 640, 480)
        os.remove(raw)
    elif cmd == 'hmp':
        print(hmp(port, sys.argv[2]))
    elif cmd == 'quit':
        print(qmp(port, 'quit'))


if __name__ == '__main__':
    main()
