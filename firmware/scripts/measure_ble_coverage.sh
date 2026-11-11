#!/usr/bin/env bash
# measure_ble_coverage.sh — measure llvm source-based coverage of ble_callbacks.c
# Usage: measure_ble_coverage.sh <test_source.c> <label>
set -u
cd "$(dirname "$0")/.."
TEST_SRC="${1:-test/test_ble_callbacks.c}"
LABEL="${2:-ble}"
WORK="test_results/cov_${LABEL}"
rm -rf "$WORK" && mkdir -p "$WORK"
UNITY_DIR=/tmp/unity/src
INCLUDES="-Icomponents/gps/include -Icomponents/utils/include -Icomponents/diagnostics/include"
INCLUDES="$INCLUDES -Icomponents/sensors/include -Icomponents/power/include -Icomponents/storage/include"
INCLUDES="$INCLUDES -Icomponents/ble/include -Icomponents/ble -Icomponents/display/include"
INCLUDES="$INCLUDES -Icomponents/ota/include -Icomponents/hid_controller/include"
INCLUDES="$INCLUDES -Icomponents/carplay_bridge/include -Icomponents/carplay_bridge"
INCLUDES="$INCLUDES -Imain -Itest -Itest/mocks -I$UNITY_DIR"

BIN="$WORK/runner"
if ! gcc $INCLUDES -fprofile-instr-generate -fcoverage-mapping -O0 -g \
     -DUNITY_INCLUDE_DOUBLE -DTEST_HOST -std=c11 -Wall -Wextra \
     -Wno-unused-parameter -Wno-deprecated-declarations \
     "$TEST_SRC" $UNITY_DIR/unity.c -o "$BIN" -lm -lpthread 2>"$WORK/build.log"; then
  echo "BUILD FAILED ($LABEL):"; tail -8 "$WORK/build.log"; exit 2
fi
LLVM_PROFILE_FILE="$WORK/runner.profraw" "$BIN" >/dev/null 2>&1
RC=$?
xcrun llvm-profdata merge -sparse "$WORK/runner.profraw" -o "$WORK/runner.profdata" 2>/dev/null
# Export per-file summary; filter for ble_callbacks.c
xcrun llvm-cov export "$BIN" -instr-profile="$WORK/runner.profdata" --format=text 2>/dev/null \
  > "$WORK/cov.json"
python3 - "$WORK/cov.json" <<'PYEOF'
import sys, json
try:
    d = json.load(open(sys.argv[1]))
except Exception:
    print("no coverage data"); sys.exit(0)
target = "ble_callbacks.c"
found = False
for f in d["data"][0]["files"]:
    fn = f["filename"]
    if fn.endswith(target):
        s = f["summary"]
        print("FILE:", fn)
        print("  lines:     %d/%d (%.1f%%)" % (s["lines"]["covered"], s["lines"]["count"], s["lines"]["percent"]))
        print("  regions:   %d/%d (%.1f%%)" % (s["regions"]["covered"], s["regions"]["count"], s["regions"]["percent"]))
        print("  functions: %d/%d (%.1f%%)" % (s["functions"]["covered"], s["functions"]["count"], s["functions"]["percent"]))
        print("  branches:  %d/%d (%.1f%%)" % (s["branches"]["covered"], s["branches"]["count"], s["branches"]["percent"]))
        found = True
if not found:
    print("ble_callbacks.c not in coverage data")
    for f in d["data"][0]["files"]:
        print("  seen:", f["filename"])
PYEOF
echo "test exit code: $RC"
