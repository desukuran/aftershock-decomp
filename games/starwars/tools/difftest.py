#!/usr/bin/env python3
"""Differential test: run frames of the original default.xbe and of our
rebuild side by side in Unicorn, from the same snapshot of the original
running in xemu, with the same pad input, and compare everything after every
frame: the globals, every heap page either touched, the D3D calls (with
their matrices, viewports, lights and vertex data), the text drawn and the
sound calls.

  difftest.py run <snapshot.ram> "<pad script>" [frames]
  difftest.py fuzz <snapshot.ram> [frames] [seed] ["<script to start with>"]

The snapshot is a whole-RAM dump from xemu_drive.py ("ram:name" saves
name.ram and name.cr3). The script is frames of input: N idle frames,
BUTTON (one frame down, one up), hold:BUTTON:N, LX/LY/RX/RY:value:N (+LY is
up as the game reads it), and set:SYMBOL+OFFSET:FORMAT:VALUE to write the
same value into both builds before the next frame (to skip ahead: say
set:g_Game+0x2178:i:11 for the kills that end the battle;
[SYMBOL+OFFSET]+OFFSET writes through a pointer: [g_Game+0x1310]+0x90 is the
player's health). Each frame is 16 ms: FrameMove, then RenderFrame,
as main() does (the music player isn't run).
"""
import os
import random
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '../../../tools'))
import difftest_core as core  # noqa: E402

ORIG_DATA = (0x000BA540, 0x000C55E0)     # the original's .data
D3D_SECTION = (0x0003CFA0, 0x0004F77C)  # the original's D3D section: its data is the device state


def d3d_state():
    """The D3D library's globals (render states, the device, the index data
    the inline DrawIndexedPrimitive reads), copied to ours as they are."""
    out = []
    for line in open(os.path.join(core.ROOT, 'symbols', 'xdklib.txt')):
        p = line.split()
        if len(p) == 3 and p[2] == 'data' and D3D_SECTION[0] <= int(p[0], 16) < D3D_SECTION[1]:
            out.append(p[1])
    return out
GAME_SIZE = 0x23A8
PAD0 = 0x1D20 + 0x60                     # g_Game.input.m_Gamepads[0]
ANALOG = ['A', 'B', 'X', 'Y', 'BLACK', 'WHITE', 'LT', 'RT']
DIGITAL = {'UP': 1, 'DOWN': 2, 'LEFT': 4, 'RIGHT': 8, 'START': 0x10, 'BACK': 0x20, 'L3': 0x40, 'R3': 0x80}
# Statics inside functions: the original's addresses (from the comments in
# src/), and where ours are (our PDB names the variable but not its guard).
ORIG_STATICS = {'RenderFrame::s_dwTitleStart': 0x000C060C, 'RenderFrame::s_dwTitleStart$guard': 0x000C0610}
ALIASES = {'RenderFrame::s_dwTitleStart$guard': 'RenderFrame::s_dwTitleStart+4'}
# Sound and music: what the game calls, logged instead of run.
SOUND_PREFIXES = ('IXACT', 'XACT', 'IDirectSound', 'DirectSound', 'XAudio', 'XInputSetState')
MUSIC = {'CMusicManager::Process': 0, 'CMusicManager::Play': 1, 'CMusicManager::Stop': 1,
         'CMusicManager::NextSong': 0, 'CMusicManager::PrevSong': 0, 'CMusicManager::NextSoundtrack': 0}


# The heap arrays g_Game points to: (member, its offset, element class,
# element size, offset of the count or a fixed count).
HEAP_ARRAYS = [('pXWings', 0x1310, 'CXWing', 0x17C, 0x1758), ('pTies', 0x1314, 'CTie', 0x128, 0x1754),
               ('pInterceptors', 0x1748, 'CInterceptor', 0x140, 0x1760),
               ('pStarDestroyers', 0x174C, 'CStarDestroyer', 0x134, 0x1764),
               ('pPlayerShips', 0x1750, 'CPlayerShip', 0x174, 8)]


def class_members():
    """class -> [(offset, member)] from the `// +0x..` comments in Game.h,
    with the base class's members."""
    import re
    classes, cur, base = {}, None, {}
    for line in open(os.path.join(core.ROOT, 'src', 'Game.h')):
        m = re.match(r'(?:class|struct) (\w+)(?:\s*:\s*public (\w+))?\s*$', line)
        if m:
            cur = m.group(1)
            classes[cur] = []
            base[cur] = m.group(2)
            continue
        m = re.match(r'\s+[\w:<>*& ]+?\b(\w+)(\[[^]]*\])?;\s*// \+0x([0-9A-Fa-f]+)', line)
        if m and cur:
            classes[cur].append((int(m.group(3), 16), m.group(1)))
    for c in classes:
        b = base.get(c)
        while b:
            classes[c] = classes.get(b, []) + classes[c]
            b = base.get(b)
        classes[c].sort()
    return classes


def heap_names(b):
    """[(lo, hi, name)] of the heap objects, by g_Game's arrays."""
    g = b.syms['g_Game']
    out = []
    for name, off, cls, size, count in HEAP_ARRAYS:
        p, = b.read(g + off, 'I')
        n = count if count < 0x100 else b.read(g + count, 'i')[0]
        if p:
            out.append((p, p + size * max(n, 0), name, cls, size))
    return out


def describe(where, objects, members):
    if not where.startswith('heap '):
        return where
    a = int(where.split()[1], 16)
    for lo, hi, name, cls, size in objects:
        if lo <= a < hi:
            i, off = divmod(a - lo, size)
            field = max(((o, m) for o, m in members.get(cls, []) if o <= off), default=(0, '?'))
            return '%s[%d].%s+%#x' % (name, i, field[1], off - field[0])
    return where


def set_pad(b, held, last, sticks):
    base = b.syms['g_Game'] + PAD0
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
    """[(held, sticks, pokes)]: pokes are (address, format, value) to write
    into both builds before that frame."""
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
    buttons = ANALOG + [k for k in DIGITAL if k != 'START']   # START pauses: test the pause menu on its own
    steps, n = [], 0
    while n < frames:
        r = rng.random()
        if r < 0.25:
            k = rng.randint(1, 30)
            steps.append(str(k))
        elif r < 0.5:
            steps.append(rng.choice(buttons))
            k = 2
        elif r < 0.7:
            k = rng.randint(5, 40)
            steps.append('hold:%s:%d' % (rng.choice(buttons), k))
        else:
            k = rng.randint(3, 40)
            steps.append('%s:%s:%d' % (rng.choice(['LX', 'LY', 'RX', 'RY']), rng.choice(['-1', '1', '-0.5', '0.5']), k))
        n += k
    return ' '.join(steps)


def setup(snapshot):
    orig, mine = core.load_builds(snapshot, ORIG_DATA, ALIASES, d3d_state(), ORIG_STATICS)
    logs = ([], [])
    ticks = []
    for b, log in zip((orig, mine), logs):
        ticks.append(core.install_os_stubs(b, log))
        # The pad as XBInput sees it: the script's buttons, with "pressed"
        # against what the previous poll saw (the game polls every 100 ms).
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

        def wsprintfw(bb, _):
            args = core._stack_args(bb)
            buf, fmt = args.pop(0), core.wstr(bb.uc, args.pop(0), 400)
            text = core.printf_format(bb, fmt, args, wide=True)
            bb.uc.mem_write(buf, text.encode('utf-16le') + b'\0\0')
            return len(text)
        b.stub('wsprintfW', 0, wsprintfw, False)
    names = core.d3d_names(orig, mine)
    tracers = (core.Tracer(orig, names), core.Tracer(mine, names))
    return orig, mine, logs, ticks, tracers


def state(b):
    mode, = b.read(b.syms['g_Game'] + 0x270, 'i')
    page, item = b.read(b.syms['g_Game'] + 0x1C70 + 4, 'ii')
    return 'mode %d menu page %d item %d' % (mode, page, item)


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
            b.write(b.syms['g_Game'] + 0x274, 'If', 16, 16 * 0.06)
            b.call('FrameMove')
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
        mem = core.compare_memory(orig, mine)
        if mem:
            print('%s: memory differs' % tag)
            objects, members = heap_names(orig), class_members()
            for what, k, a, m in mem[:24]:
                print('  %-40s orig %s mine %s' % (describe(what, objects, members), a.hex(), m.hex()))
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
