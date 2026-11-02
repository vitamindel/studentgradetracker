# =====================================================================
#  build.ps1 - Windows version of build.sh
#  Compiles the C++ engine to WebAssembly (web/gradebook.wasm),
#  regenerates the OnlineGDB single file, and builds the console version.
#
#  Needs:  Python 3 + ziglang   ->   python -m pip install ziglang
#  Usage:  powershell -ExecutionPolicy Bypass -File build.ps1
# =====================================================================
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

function Invoke-Step([string]$exe, [string[]]$arguments) {
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { throw "$exe failed with exit code $LASTEXITCODE" }
}

Write-Host "==> 1/4  C++ -> WebAssembly (web/gradebook.wasm)"
# -target wasm32-wasi       build for WebAssembly + the WASI system interface
# -Os -s                    optimize for small size, strip debug info
# -fno-exceptions           no try/throw (keeps the file small; we never throw)
# -mexec-model=reactor      a library with no main(); JS calls _initialize() then run()
Invoke-Step python @("-m", "ziglang", "c++", "-target", "wasm32-wasi", "-Os", "-s", "-fno-exceptions",
    "-mexec-model=reactor", "-std=c++17", "-o", "web/gradebook.wasm", "src/web_main.cpp")
Get-Item web/gradebook.wasm | Format-Table Name, Length

Write-Host "==> 2/4  OnlineGDB single file"
Invoke-Step python @("tools/make_single_file.py")

Write-Host "==> 3/4  Console version (warnings on)"
New-Item -ItemType Directory -Force build | Out-Null
if (Get-Command g++ -ErrorAction SilentlyContinue) {
    Invoke-Step g++ @("-std=c++17", "-Wall", "-Wextra", "-o", "build/tracker.exe", "src/main.cpp")
    Write-Host "    built build/tracker.exe with g++"
} else {
    Invoke-Step python @("-m", "ziglang", "c++", "-std=c++17", "-Wall", "-Wextra", "-o", "build/tracker.exe", "src/main.cpp")
    Write-Host "    g++ not found; built build/tracker.exe with zig's clang instead"
}

Write-Host "==> 4/4  Engine tests (runs the real .wasm in Node)"
if (Get-Command node -ErrorAction SilentlyContinue) {
    node tests/engine_test.mjs | Select-Object -Last 1
} else {
    Write-Host "    node not found; skipped"
}

Write-Host "Done. Serve the site with:  python -m http.server 8000 -d web"
