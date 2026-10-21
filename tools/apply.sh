#!/bin/sh
# Re-applies symbols to the existing Ghidra project (no re-analysis) and
# refreshes build/export. Run tools/analyze.sh once first.
set -e
TOOLS=$(cd "$(dirname "$0")" && pwd)
# the game: $GAME or the games/<game>/ folder this is run from
ROOT=$(python3 -I "$TOOLS/gameconf.py")
PROJECT=$(python3 -I "$TOOLS/gameconf.py" ghidra_project)
GHIDRA=${GHIDRA:-$TOOLS/ghidra_12.1.4_PUBLIC}
BUILD=$ROOT/build

python3 -I "$TOOLS/gen_src_structs.py" > /dev/null
cat "$ROOT/symbols/types.h" "$BUILD/src_structs.h" > "$BUILD/types_all.h"

"$GHIDRA/support/analyzeHeadless" "$BUILD/ghidra" "$PROJECT" \
	-process default.flat -noanalysis \
	-scriptPath "$TOOLS/ghidra_scripts" \
	-postScript ApplyTypes.java "$BUILD/types_all.h" \
	-postScript ApplySymbols.java "$ROOT/symbols/xdklib.txt" "$ROOT/symbols/xdk.txt" "$ROOT/symbols/consts.txt" "$ROOT/symbols/names.txt" \
	-postScript ExportAll.java "$BUILD/export"
