# AfterShock Pool SX decompilation

A readable C++ reconstruction of **AfterShock Pool SX** (SHiZNO / AfterShock,
April 2005), an Xbox homebrew pool game: 8 Ball, 9 Ball, Cut Throat, Red vs
Blue, Mini-Golf-Pool and an in-game table editor.

The goal is a source tree that behaves like the original, quirks and bugs
included (they're kept and commented, not fixed), so it can be studied,
modded and extended. It is not a byte-matching decomp.

## Layout

* `aftershock_pool_sx/` — the original release (not committed). `default.xbe`
  is the target (sha1 `5ed22bc296c98e1dc02cabdfc6247a47e72a2df9`).
* `src/` — the reconstructed source. Every function carries its original
  address (`// 0x00012400`), every class member its offset (`// +0x1C`).
* `symbols/` — the naming database for Ghidra: `names.txt` (by hand),
  `xdklib.txt` (XDK 5558 library code matched byte for byte by
  `tools/libmatch.py`), `xdk.txt` (XbSymbolDatabase), `consts.txt`
  (float/double constants), `types.h` (XDK types).
* `tools/` — XBE parser, headless Ghidra pipeline (`analyze.sh`, `apply.sh`),
  `show.py` (decompiler output with constants and D3D render states
  resolved), `disasm.py`, `xrefs.py`, `callgraph.py`, `progress.py`,
  `gen_src_structs.py` (Ghidra structs and compile-time layout checks
  generated from `src/*.h`).

## Building

`make -f Makefile.xbox` builds `build/xbox/default.xbe` with the XDK 5558
compiler under Wine (the XDK isn't included; `./xdk` points at it). The build
also compiles `build/layout_check.cpp`, which asserts that every class
layout in `src/` matches the original binary's offsets.

## How the game is put together

Nearly all state lives in one global aggregate, `g_Game` (`CGame`, at
0xBBC20 in the original), used by free functions; its member order is the
original declaration order. `main()` loads everything and then loops
`CInput::Update`, `Update`, `Render`, with `g_Game.state` choosing between
the loading splash, the intro, the menus, the game and the editor.

Borrowed code: the XDK samples' gamepad handling (`XBGAMEPAD`) and
BackgroundMusic `CMusicManager`, and RSA's MD5, which checks that the intro's
credits text hasn't been edited (despite the readme's "no CRC/MD5 checks").
