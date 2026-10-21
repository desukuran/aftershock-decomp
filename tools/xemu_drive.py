#!/usr/bin/env python3
"""Boot xemu with a virtual gamepad and play a script of inputs and screenshots.

  xemu_drive.py <xemu.toml> <script> [outdir]

The script is whitespace-separated steps:
  2.5            wait 2.5 seconds
  A B X Y START BACK UP DOWN LEFT RIGHT WHITE BLACK LT RT L3 R3
                 tap a button (0.15 s)
  A*5            tap it 5 times
  hold:A:1.0     hold a button for 1 second
  LX:-1:0.5      push a stick axis (LX LY RX RY, -1..1, +LY is down) for 0.5 s
  run:a,b,c      run a Python script (args comma-separated) with the VM up
  shot:name      save the front buffer to <outdir>/name.png
  mem:name:a:n   save n bytes of guest virtual memory at a to <outdir>/name.bin
  ram:name       save all 64 MB of guest RAM to <outdir>/name.ram and CR3 to name.cr3

The pad is a uinput device (needs write access to /dev/uinput) that xemu sees
as an Xbox 360 pad. Its SDL GUID is printed; bind it to port 1 in the config:
  [input.bindings] port1 = '<guid>'
xemu is started with -qmp on port 44445 and killed at the end. The script stops
early if xemu exits (a crash prints its assertion).
"""
import os
import re
import struct
import subprocess
import sys
import time

import evdev
from evdev import UInput, AbsInfo, ecodes as e

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xemu_qmp  # noqa: E402

NAME = 'Decomp Test Pad'
# Not a real pad's ids: SDL leaves devices it could drive through HIDAPI to
# HIDAPI, so a fake 360 pad is ignored while a real one is plugged in.
VENDOR, PRODUCT, VERSION = 0x1209, 0x0001, 0x0001
PORT = 44445
XEMU = os.path.expanduser('~/xemu/xemu.AppImage')

BUTTONS = {
    'A': e.BTN_A, 'B': e.BTN_B, 'X': e.BTN_X, 'Y': e.BTN_Y,
    'WHITE': e.BTN_TL, 'BLACK': e.BTN_TR, 'BACK': e.BTN_SELECT, 'START': e.BTN_START,
    'L3': e.BTN_THUMBL, 'R3': e.BTN_THUMBR,
}
DPAD = {'UP': (e.ABS_HAT0Y, -1), 'DOWN': (e.ABS_HAT0Y, 1), 'LEFT': (e.ABS_HAT0X, -1), 'RIGHT': (e.ABS_HAT0X, 1)}
TRIGGERS = {'LT': e.ABS_Z, 'RT': e.ABS_RZ}
STICKS = {'LX': e.ABS_X, 'LY': e.ABS_Y, 'RX': e.ABS_RX, 'RY': e.ABS_RY}


def crc16(data, crc=0):
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return crc


def sdl_guid():
    return struct.pack('<8H', 3, crc16(NAME.encode()), VENDOR, 0, PRODUCT, 0, VERSION, 0).hex()


def sdl_mapping():
    # evdev numbers buttons and axes in code order: A B X Y TL TR SELECT START
    # MODE THUMBL THUMBR, and X Y Z RX RY RZ.
    return (sdl_guid() + ',' + NAME + ',a:b0,b:b1,x:b2,y:b3,leftshoulder:b4,rightshoulder:b5,back:b6,'
            'start:b7,guide:b8,leftstick:b9,rightstick:b10,leftx:a0,lefty:a1,lefttrigger:a2,rightx:a3,'
            'righty:a4,righttrigger:a5,dpup:h0.1,dpright:h0.2,dpdown:h0.4,dpleft:h0.8,platform:Linux,')


def make_pad():
    stick = AbsInfo(0, -32768, 32767, 16, 128, 0)
    trig = AbsInfo(0, 0, 255, 0, 0, 0)
    hat = AbsInfo(0, -1, 1, 0, 0, 0)
    caps = {
        e.EV_KEY: [e.BTN_A, e.BTN_B, e.BTN_X, e.BTN_Y, e.BTN_TL, e.BTN_TR, e.BTN_SELECT, e.BTN_START,
                   e.BTN_MODE, e.BTN_THUMBL, e.BTN_THUMBR],
        e.EV_ABS: [(e.ABS_X, stick), (e.ABS_Y, stick), (e.ABS_Z, trig), (e.ABS_RX, stick), (e.ABS_RY, stick),
                   (e.ABS_RZ, trig), (e.ABS_HAT0X, hat), (e.ABS_HAT0Y, hat)],
    }
    return UInput(caps, name=NAME, vendor=VENDOR, product=PRODUCT, version=VERSION, bustype=e.BUS_USB)


def press(ui, step, secs):
    if step in BUTTONS:
        ev, down, up = e.EV_KEY, 1, 0
        code = BUTTONS[step]
    elif step in DPAD:
        code, down = DPAD[step]
        ev, up = e.EV_ABS, 0
    elif step in TRIGGERS:
        ev, code, down, up = e.EV_ABS, TRIGGERS[step], 255, 0
    else:
        raise SystemExit('unknown step ' + step)
    ui.write(ev, code, down)
    ui.syn()
    time.sleep(secs)
    ui.write(ev, code, up)
    ui.syn()
    time.sleep(0.15)


def main():
    config, script = sys.argv[1], sys.argv[2].split()
    outdir = sys.argv[3] if len(sys.argv) > 3 else '.'
    ui = make_pad()
    print('pad guid', sdl_guid())
    time.sleep(1)
    log = open(os.path.join(outdir, 'xemu.log'), 'w')
    env = dict(os.environ, SDL_VIDEODRIVER='offscreen', SDL_GAMECONTROLLERCONFIG=sdl_mapping())
    xemu = subprocess.Popen([XEMU, '-config_path', os.path.abspath(config),
                             '-qmp', 'tcp:127.0.0.1:%d,server,nowait' % PORT],
                            stdout=log, stderr=subprocess.STDOUT, env=env)
    try:
        for step in script:
            if xemu.poll() is not None:
                print('xemu exited before', step)
                break
            if step[0].isdigit():
                time.sleep(float(step))
            elif step.startswith('shot:'):
                path = os.path.join(outdir, step[5:] + '.png')
                sys.argv = ['xemu_qmp', 'shot', path, str(PORT)]
                try:
                    xemu_qmp.main()
                    print('shot', path)
                except Exception as ex:  # noqa: BLE001
                    print('shot failed', step, ex)
            elif step.startswith('mem:'):
                _, name, addr, size = step.split(':')
                path = os.path.abspath(os.path.join(outdir, name + '.bin'))
                xemu_qmp.hmp(PORT, 'stop')
                xemu_qmp.hmp(PORT, 'memsave %s %s "%s"' % (addr, size, path))
                xemu_qmp.hmp(PORT, 'cont')
                print('mem', path)
            elif step.startswith('ram:'):
                path = os.path.abspath(os.path.join(outdir, step[4:]))
                xemu_qmp.hmp(PORT, 'stop')
                regs = xemu_qmp.hmp(PORT, 'info registers')
                open(path + '.cr3', 'w').write(re.search(r'CR3=([0-9a-f]+)', regs).group(1) + '\n')
                xemu_qmp.hmp(PORT, 'pmemsave 0 0x4000000 "%s.ram"' % path)
                xemu_qmp.hmp(PORT, 'cont')
                print('ram', path)
            elif step.startswith('run:'):
                subprocess.run([sys.executable, '-I'] + step[4:].split(','), cwd=os.getcwd())
            elif step.startswith('hold:'):
                _, name, secs = step.split(':')
                press(ui, name, float(secs))
            elif step.split(':')[0] in STICKS:
                name, val, secs = step.split(':')
                ui.write(e.EV_ABS, STICKS[name], int(max(-1.0, min(1.0, float(val))) * 32767))
                ui.syn()
                time.sleep(float(secs))
                ui.write(e.EV_ABS, STICKS[name], 0)
                ui.syn()
                time.sleep(0.15)
            else:
                name, _, count = step.partition('*')
                for _ in range(int(count or 1)):
                    press(ui, name, 0.15)
    finally:
        alive = xemu.poll() is None
        if alive:
            xemu.kill()
        xemu.wait()
        ui.close()
        log.close()
        for line in open(os.path.join(outdir, 'xemu.log')):
            if 'Assertion' in line or 'rror' in line:
                print(line.rstrip())
        print('xemu was', 'alive' if alive else 'DEAD', 'at the end')


if __name__ == '__main__':
    main()
