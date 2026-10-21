#!/bin/sh
# Imports Default.xbe into a fresh headless Ghidra project and exports
# functions/strings/decompiler output to build/export.
set -e
TOOLS=$(cd "$(dirname "$0")" && pwd)
# the game: $GAME or the games/<game>/ folder this is run from
ROOT=$(python3 -I "$TOOLS/gameconf.py")
PROJECT=$(python3 -I "$TOOLS/gameconf.py" ghidra_project)
XBE=${XBE:-$ROOT/original/default.xbe}
GHIDRA=${GHIDRA:-$TOOLS/ghidra_12.1.4_PUBLIC}
BUILD=$ROOT/build
SYMBOLS="$ROOT/symbols/xdklib.txt $ROOT/symbols/xdk.txt $ROOT/symbols/names.txt"

mkdir -p "$BUILD/ghidra" "$BUILD/export"
python3 -I "$TOOLS/xbe.py" flat "$XBE" "$BUILD/default.flat"
python3 -I "$TOOLS/xbe.py" layout "$XBE" "$BUILD/layout.txt"

python3 -I "$TOOLS/gen_src_structs.py" > /dev/null
cat "$ROOT/symbols/types.h" "$BUILD/src_structs.h" > "$BUILD/types_all.h"

"$GHIDRA/support/analyzeHeadless" "$BUILD/ghidra" "$PROJECT" \
	-overwrite \
	-import "$BUILD/default.flat" \
	-loader BinaryLoader -loader-baseAddr 0x10000 \
	-processor x86:LE:32:default -cspec windows \
	-scriptPath "$TOOLS/ghidra_scripts" \
	-preScript XbeSetup.java "$BUILD/layout.txt" "$ROOT/symbols/noreturn.txt" \
	-postScript ApplyTypes.java "$BUILD/types_all.h" \
	-postScript ApplySymbols.java $SYMBOLS \
	-postScript ExportAll.java "$BUILD/export"
