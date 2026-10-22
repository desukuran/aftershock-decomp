#!/usr/bin/env python3
"""Differential test: run frames of the original default.xbe and of our
rebuild side by side in Unicorn, from the same snapshot of the original
running in xemu, with the same pad input, and compare everything after every
frame: the globals, every heap page either touched, the D3D calls (with
their matrices and vertex data), the text drawn and the sound calls.

  difftest.py run <snapshot.ram> "<pad script>" [frames]
  difftest.py fuzz <snapshot.ram> [frames] [seed] ["<script to start with>"]

The snapshot is a whole-RAM dump from xemu_drive.py ("ram:name" saves
name.ram and name.cr3). The script is frames of input on pad 1: N idle
frames, BUTTON (one frame down, one up), hold:BUTTON:N, LX/LY/RX/RY:value:N
(+LY is up as the game reads it), and set:SYMBOL+OFFSET:FORMAT:VALUE to write
the same value into both builds before the next frame. Each frame is 16 ms:
CGame::Update, then RenderFrame, as main() does (the music player isn't
run).
"""
import os
import random
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../tools'))
import difftest_core as core  # noqa: E402

ORIG_DATA = (0x000F7260, 0x0010D89C)     # the original's .data
D3D_SECTION = (0x00093920, 0x000A5624)  # the original's D3D section: its data is the device state


def d3d_state():
    """The D3D library's globals, copied to ours as they are."""
    out = []
    for line in open(os.path.join(core.ROOT, 'symbols', 'xdklib.txt')):
        p = line.split()
        if len(p) == 3 and p[2] == 'data' and D3D_SECTION[0] <= int(p[0], 16) < D3D_SECTION[1]:
            out.append(p[1])
    return out


PAD0 = 0x58                              # g_Input.m_Gamepads[0]
ANALOG = ['A', 'B', 'X', 'Y', 'BLACK', 'WHITE', 'LT', 'RT']
DIGITAL = {'UP': 1, 'DOWN': 2, 'LEFT': 4, 'RIGHT': 8, 'START': 0x10, 'BACK': 0x20, 'L3': 0x40, 'R3': 0x80}
# Statics inside functions: the original's addresses, and where ours are
# (our PDB names the variable but not its guard).
ORIG_STATICS = {'CGame::UpdateInput::s_dwInputTime': 0x000FF758,
                'CGame::UpdateInput::s_dwInputTime$guard': 0x000FF75C}
ALIASES = {'CGame::UpdateInput::s_dwInputTime$guard': 'CGame::UpdateInput::s_dwInputTime+4'}
# Sound and music: what the game calls, logged instead of run.
SOUND_PREFIXES = ('IDirectSound', 'DirectSound', 'XAudio', 'XInputSetState', 'IDirectMusic', 'DirectMusic')
MUSIC = {'CMusicManager::Process': 0, 'CMusicManager::Play': 0, 'CMusicManager::Stop': 0,
         'CMusicManager::Pause': 0, 'CMusicManager::NextSong': 0, 'CMusicManager::PrevSong': 0,
         'CMusicManager::NextSoundtrack': 0, 'CMusicManager::RandomSong': 1, 'CMusicManager::SetVolume': 1,
         'CMusicManager::SetRandom': 1, 'CSoundEffect::Play': 0}
STATE = 0x709C                           # g_Game.m_state
MENU = 0x6F48                            # g_Game.m_menu


def set_pad(b, held, last, sticks):
    base = b.syms['g_Input'] + PAD0
    w = sum(v for k, v in DIGITAL.items() if k in held)
    analog = [255 if k in held else 0 for k in ANALOG]
    lastw = sum(v for k, v in DIGITAL.items() if k in last)
    pressed_w = w & ~lastw
    pressed_a = [1 if (k in held and k not in last) else 0 for k in ANALOG]
    lx, ly, rx, ry = (sticks.get(k, 0.0) for k in ('LX', 'LY', 'RX', 'RY'))
    b.write(base, 'H8B4h', w, *analog, *(int(v * 32767) for v in (lx, ly, rx, ry)))
    b.write(base + 0x14, '4f', lx, ly, rx, ry)
    b.write(base + 0x24, 'H', lastw)
    b.write(base + 0x28, '8I', *[1 if k in last else 0 for k in ANALOG])
    b.write(base + 0x48, 'H', pressed_w)
    b.write(base + 0x4C, '8I', *pressed_a)


def address(b, where):
    """SYMBOL+OFF, or [SYMBOL+OFF]+OFF2 for OFF2 into what it points to."""
    import re
    m = re.fullmatch(r'\[(\w+)(?:\+(\w+))?\](?:\+(\w+))?', where)
    if m:
        p, = b.read(b.syms[m.group(1)] + int(m.group(2) or '0', 0), 'I')
        return p + int(m.group(3) or '0', 0)
    sym, _, off = where.partition('+')
    return b.syms[sym] + int(off or '0', 0)


def parse_script(script):
    """[(held, sticks, pokes)]."""
    frames, pokes = [], []
    for step in script.split():
        if step.startswith('set:'):
            where, fmt, value = step[4:].rsplit(':', 2)
            pokes.append((where, fmt, float(value) if 'f' in fmt else int(value, 0)))
            continue
        start = len(frames)
        if step.isdigit():
            frames += [(set(), {})] * int(step)
        elif step.startswith('hold:'):
            _, name, n = step.split(':')
            frames += [({name}, {})] * int(n)
        elif step.split(':')[0] in ('LX', 'LY', 'RX', 'RY'):
            name, v, n = step.split(':')
            frames += [(set(), {name: float(v)})] * int(n)
        else:
            frames += [({step}, {}), (set(), {})]
        if pokes and len(frames) > start:
            frames[start] = frames[start] + (pokes,)
            pokes = []
    return [f if len(f) == 3 else f + ([],) for f in frames]


def fuzz_script(frames, seed):
    rng = random.Random(seed)
    buttons = ANALOG + list(DIGITAL)
    steps, n = [], 0
    while n < frames:
        r = rng.random()
        if r < 0.25:
            k = rng.randint(1, 30)
            steps.append(str(k))
        elif r < 0.55:
            steps.append(rng.choice(buttons))
            k = 2
        elif r < 0.75:
            k = rng.randint(5, 40)
            steps.append('hold:%s:%d' % (rng.choice(buttons), k))
        else:
            k = rng.randint(3, 40)
            steps.append('%s:%s:%d' % (rng.choice(['LX', 'LY', 'RX', 'RY']), rng.choice(['-1', '1', '-0.5', '0.5']), k))
        n += k
    return ' '.join(steps)


def setup(snapshot):
    orig, mine = core.load_builds(snapshot, ORIG_DATA, ALIASES, d3d_state(), ORIG_STATICS)
    # The CRT's x87 dispatch tables (fmod, pow...) hold code addresses:
    # ours keep their own.
    img = core.xbelib.Xbe(core.OURS).flat_image()
    for n, a in mine.syms.items():
        if n.endswith('jmptab') and core.BASE <= a < core.BASE + len(img):
            size = mine.sizes.get(n, 0)
            if size:
                mine.uc.mem_write(a, bytes(img[a - core.BASE:a - core.BASE + size]))
    logs = ([], [])
    ticks = []
    for b, log in zip((orig, mine), logs):
        ticks.append(core.install_os_stubs(b, log))
        # The pad as XBInput sees it: the script's buttons, with "pressed"
        # against what the previous poll saw.
        b.pad = {'held': set(), 'sticks': {}, 'last': set()}

        def poll(bb, a):
            set_pad(bb, bb.pad['held'], bb.pad['last'], bb.pad['sticks'])
            bb.pad['last'] = bb.pad['held']
            return 0
        b.stub('CInput::Update', 0, poll)

        for n in mine.args:
            if n.startswith(SOUND_PREFIXES) and n in b.syms:
                b.stub_decorated(n, lambda bb, a, n=n, log=log: (log.append('%s(%s)' % (
                    n, ', '.join(bb.sym(v) for v in a[1:]))), 0)[1])
        for n, nargs in MUSIC.items():
            b.stub(n, nargs, lambda bb, a, n=n, log=log: (log.append(n), 0)[1])
    names = core.d3d_names(orig, mine)
    tracers = (core.Tracer(orig, names), core.Tracer(mine, names))
    return orig, mine, logs, ticks, tracers


def state(b):
    g = b.syms['g_Game']
    st, = b.read(g + STATE, 'i')
    page, = b.read(g + MENU + 0xE0, 'i')
    item, = b.read(g + MENU + 0x98 + 0xC * page + 8, 'i') if 0 <= page < 6 else (-1,)
    return 'state %d menu page %d item %d' % (st, page, item)


def run(snapshot, script, frames=None):
    orig, mine, logs, ticks, tracers = setup(snapshot)
    print('%d globals copied; %d D3D functions traced' % (len(mine.sizes), len(core.d3d_names(orig, mine))))
    steps = parse_script(script)
    if frames:
        steps = (steps + [(set(), {}, [])] * frames)[:frames]
    prev = None
    for f, (held, sticks, pokes) in enumerate(steps):
        for b, t in zip((orig, mine), ticks):
            for where, fmt, value in pokes:
                b.write(address(b, where), fmt, value)
            b.pad['held'], b.pad['sticks'] = held, sticks
            t['t'] += 16
            b.call('CGame::Update', ecx=b.syms['g_Game'])
            b.call('RenderFrame')
        tag = 'frame %d (%s)' % (f, ' '.join(sorted(held)) or '-')
        st = state(orig)
        if st != prev:
            print('%s: %s' % (tag, st))
            prev = st
        bad = False
        if core.compare_calls(*tracers, out=lambda s: print('  ' + s)):
            print('%s: D3D calls differ' % tag)
            bad = True
        if logs[0] != logs[1]:
            import difflib
            print('%s: calls differ' % tag)
            for line in list(difflib.unified_diff(logs[0], logs[1], 'orig', 'mine', lineterm='', n=2))[:20]:
                print('  ' + line)
            bad = True
        mem = core.compare_memory(orig, mine, skip=[n for n in mine.sizes if n.endswith('jmptab')])
        if mem:
            print('%s: memory differs' % tag)
            for what, k, a, m in mem[:int(os.environ.get("DIFFS", "24"))]:
                print('  %-40s orig %s mine %s' % (what, a.hex(), m.hex()))
            bad = True
        if bad:
            return False
        for tr in tracers:
            tr.clear()
        logs[0].clear()
        logs[1].clear()
    print('%d frames identical (%s)' % (len(steps), state(orig)))
    return core.compare_saves(orig, mine)


def main():
    a = sys.argv[1:]
    if len(a) >= 3 and a[0] == 'run':
        run(a[1], a[2], int(a[3]) if len(a) > 3 else None)
    elif len(a) >= 2 and a[0] == 'fuzz':
        frames = int(a[2]) if len(a) > 2 else 2000
        script = fuzz_script(frames, int(a[3]) if len(a) > 3 else 1)
        if len(a) > 4:
            script = a[4] + ' ' + script
        print('script:', script)
        run(a[1], script)
    else:
        raise SystemExit(__doc__)


if __name__ == '__main__':
    main()
