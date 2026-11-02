#!/usr/bin/env bash
# =====================================================================
#  build.sh - compiles the C++ engine to WebAssembly (web/gradebook.wasm),
#  regenerates the OnlineGDB single file, and builds the console version.
#
#  Needs:  Python 3 + ziglang   ->   python -m pip install ziglang
#  Usage:  ./build.sh
# =====================================================================
set -euo pipefail
cd "$(dirname "$0")"

PY="${PYTHON:-python}"
command -v "$PY" >/dev/null 2>&1 || PY=python3

echo "==> 1/4  C++ -> WebAssembly (web/gradebook.wasm)"
# -target wasm32-wasi       build for WebAssembly + the WASI system interface
# -Os -s                    optimize for small size, strip debug info
# -fno-exceptions           no try/throw (keeps the file small; we never throw)
# -mexec-model=reactor      a library with no main(); JS calls _initialize() then run()
"$PY" -m ziglang c++ -target wasm32-wasi -Os -s -fno-exceptions -mexec-model=reactor \
    -std=c++17 -o web/gradebook.wasm src/web_main.cpp
ls -l web/gradebook.wasm

echo "==> 2/4  OnlineGDB single file"
"$PY" tools/make_single_file.py

echo "==> 3/4  Console version (warnings on)"
mkdir -p build
if command -v g++ >/dev/null 2>&1; then
    g++ -std=c++17 -Wall -Wextra -o build/tracker src/main.cpp
    g++ -std=c++17 -Wall -Wextra -o build/tracker_single onlinegdb/GradeTracker_SingleFile.cpp
    echo "    built build/tracker with g++"
else
    "$PY" -m ziglang c++ -std=c++17 -Wall -Wextra -o build/tracker src/main.cpp
    echo "    g++ not found; built build/tracker with zig's clang instead"
fi

echo "==> 4/4  Engine tests (runs the real .wasm in Node)"
if command -v node >/dev/null 2>&1; then
    node tests/engine_test.mjs | tail -1
else
    echo "    node not found; skipped"
fi

echo "Done. Serve the site with:  $PY -m http.server 8000 -d web"
