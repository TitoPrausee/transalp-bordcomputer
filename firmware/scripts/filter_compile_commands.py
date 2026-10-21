#!/usr/bin/env python3
"""
filter_compile_commands.py — Extract project sources from an ESP-IDF compile_commands.json.

The compile_commands.json produced by `idf.py build` contains an entry for every
translation unit in the build, including every ESP-IDF managed component (hundreds
of files under $IDF_PATH). Running cppcheck --enable=all over all of them is slow
and surfaces findings in third-party code we do not own.

This script filters the database down to the project's own sources — the files
under ``components/`` and ``main/`` — so whole-program cppcheck analysis covers
only our code while still getting the real cross-TU linkage (include paths, macro
definitions, and call-graph edges) that a full `idf.py build` provides.

Usage:
    python3 scripts/filter_compile_commands.py \
        build/compile_commands.json \
        build/compile_commands_project.json

Exit code is 0 when at least one project entry is written, 1 otherwise.
"""

import json
import os
import sys


def _resolve(file_path: str, directory: str) -> str:
    if not os.path.isabs(file_path):
        file_path = os.path.join(directory, file_path)
    return os.path.normpath(file_path)


def filter_db(db, project_root):
    project_root = os.path.realpath(project_root)
    kept = []
    for entry in db:
        directory = entry.get("directory", project_root)
        file_path = _resolve(entry.get("file", ""), directory)
        rel = os.path.relpath(file_path, project_root)
        # Keep only sources that live under components/ or main/.
        if rel.startswith("components" + os.sep) or rel.startswith("main" + os.sep):
            kept.append(entry)
    return kept


def main(argv):
    if len(argv) != 3:
        print(
            "usage: filter_compile_commands.py <input.json> <output.json>",
            file=sys.stderr,
        )
        return 2
    in_path, out_path = argv[1], argv[2]
    with open(in_path) as fh:
        db = json.load(fh)
    project_root = os.getcwd()
    kept = filter_db(db, project_root)
    with open(out_path, "w") as fh:
        json.dump(kept, fh, indent=2)
    print(
        f"Filtered {len(kept)}/{len(db)} entries to project sources "
        f"-> {out_path}"
    )
    return 0 if kept else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
