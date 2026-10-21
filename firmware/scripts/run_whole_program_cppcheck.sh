#!/bin/bash
# run_whole_program_cppcheck.sh — Build firmware and run whole-program cppcheck.
#
# Intended to run inside the espressif/idf Docker container (via the
# esp-idf-ci-action's `command` parameter) so that both the ESP-IDF headers
# and the project sources are available at the paths recorded in
# compile_commands.json. Running cppcheck on the host runner would fail
# because the ESP-IDF include paths (/opt/esp/idf/...) only exist inside
# the container.
#
# Usage (from the firmware/ directory):
#   bash scripts/run_whole_program_cppcheck.sh
#
# Exit code: 0 if cppcheck finds nothing, 1 otherwise (--error-exitcode=1).

set -e

echo "=== Building firmware (idf.py build → build/compile_commands.json) ==="
idf.py build

echo "=== Installing cppcheck ==="
apt-get update -qq
apt-get install -y -qq cppcheck

echo "=== Filtering compile_commands.json to project sources ==="
mkdir -p analysis
python3 scripts/filter_compile_commands.py \
    build/compile_commands.json \
    build/compile_commands_project.json

echo "=== Whole-program cppcheck (--enable=all --check-level=exhaustive) ==="
echo "Uses real idf.py compile_commands.json with cross-TU linkage."
echo "Suppressions: analysis/cppcheck-suppressions.txt (unusedFunction only)."
cppcheck \
    --enable=all \
    --check-level=exhaustive \
    --suppress=missingIncludeSystem \
    --suppress=missingInclude \
    --suppressions-list=analysis/cppcheck-suppressions.txt \
    --project=build/compile_commands_project.json \
    --inline-suppr \
    --error-exitcode=1 \
    --xml \
    --xml-version=2 \
    2> analysis/cppcheck-whole-program.xml

echo "=== Whole-program cppcheck passed (0 findings) ==="
