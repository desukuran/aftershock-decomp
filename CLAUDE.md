This repository decompiles the original Xbox homebrew games of AfterShock (SHiZNO and friends, 2005) into readable code, one game per folder under games/. Done: AfterShock Pool SX (games/pool), Madness Interactive SX (games/madness), Star Wars: A New Hope for Homebrew (games/starwars) and Baku Baku X2 (games/bakubaku2), the last one; the rest of AfterShock's games have their source released. We COULD try to match the bytes, but a recreation comes first; if you feel matching matters and would speed things up, say so.

* A new game gets games/<name>/ like the others (game.ini, Makefile, src/, symbols/, original/), reusing tools/ and tools/xbox.mk; put game-specific tools in games/<name>/tools/. Games share engine code (input, font, music, sound, credits): tools/crossmatch.py finds it, but each game keeps its own copy, as built.
* There is XDK reference material in ~/decomp/unleashx/; xdk, reference, tools/venv and tools/ghidra_* are softlinks to it, not copies.
* If you need anything else to help take the guessing game out, please explicitly say so.
* Ghidra and the XBE tools are installed thanks to the unleashx decomp.
* Keep any quirks or bugs, vanilla as possible. Workarounds (e.g. for xemu) go behind a build switch (XEMU=1), never into the default build.
* This is for a learning experiment, but also so that mods and additions can be made.
* Keep progress and drop % from time to time (tools/progress.py, from the game's folder).
* Go until you run into something that requires reading material or something.
