"""Builds onlinegdb/GradeTracker_SingleFile.cpp by pasting the headers into main.cpp.

OnlineGDB is easiest to use with ONE file, so this script replaces every
    #include "Something.h"
with the contents of that header (only the first time it appears) and
drops the "#pragma once" lines.

Usage:  python tools/make_single_file.py
"""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
OUT = ROOT / "onlinegdb" / "GradeTracker_SingleFile.cpp"

INCLUDE = re.compile(r'^\s*#include\s+"([^"]+)"')
# Matches "#pragma once" even with a comment after it on the same line.
PRAGMA_ONCE = re.compile(r"^\s*#pragma\s+once\b")
already_included = set()


def inline(path: pathlib.Path) -> list[str]:
    lines = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if PRAGMA_ONCE.match(line):
            continue
        match = INCLUDE.match(line)
        if match:
            name = match.group(1)
            if name not in already_included:
                already_included.add(name)
                lines.append(f"// ----- begin {name} -----")
                lines.extend(inline(SRC / name))
                lines.append(f"// ----- end {name} -----")
            continue
        lines.append(line)
    return lines


banner = [
    "// =====================================================================",
    "//  GradeTracker_SingleFile.cpp  (GENERATED - edit the files in src/ instead)",
    "//  Student.h + GradeBook.h + CommandShell.h + main.cpp in one file, so it",
    "//  can be pasted straight into OnlineGDB (Language: C++17) and run.",
    "// =====================================================================",
]
OUT.parent.mkdir(exist_ok=True)
OUT.write_text("\n".join(banner + inline(SRC / "main.cpp")) + "\n", encoding="utf-8", newline="\n")
print(f"Wrote {OUT.relative_to(ROOT)}")
