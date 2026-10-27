# CMake fetches the pinned GoogleTest dependency on the first configure.
# Requires CMake, Ninja, and a C++17 compiler on PATH.
$ErrorActionPreference = 'Stop'
Push-Location (Split-Path -Parent $PSScriptRoot)
try {
    & cmake -S . -B build/cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
    & cmake --build build/cmake
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
    & ctest --test-dir build/cmake --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
    $actual = & .\build\cmake\three_sat_example.exe
    if ($LASTEXITCODE -ne 0) { throw 'Example execution failed' }
    $expected = @('Assignment satisfies formula: true', 'Satisfying assignments: 6', 'x1 = false', 'x2 = false', 'x3 = true')
    if (($actual -join "`n") -ne ($expected -join "`n")) { throw 'Unexpected example output' }
    Write-Output 'Example: output check passed.'
} finally {
    Pop-Location
}
