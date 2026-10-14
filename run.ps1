param([ValidateSet('Build','Test','Demo','Capture')][string]$Mode='Test')
$ErrorActionPreference = 'Stop'
$projectRoot = $PSScriptRoot
Set-Location -LiteralPath $projectRoot
$compilerPath = $env:FIXTURE_CXX
if (-not $compilerPath) {
    if (Test-Path -LiteralPath 'C:\msys64\ucrt64\bin\g++.exe') { $compilerPath = 'C:\msys64\ucrt64\bin\g++.exe' }
    else { $compilerPath = (Get-Command g++ -ErrorAction Stop).Source }
}
New-Item -ItemType Directory -Path 'build','artifacts' -Force | Out-Null
$commonArgs = @('-std=c++17','-O2','-Wall','-Wextra','-Wpedantic','-static','-static-libgcc','-static-libstdc++','-Iinclude','src/geometry.cpp','src/polygon.cpp','src/dxf.cpp')
& $compilerPath @commonArgs 'src/cli.cpp' '-o' 'build/fixture_cli.exe'
if ($LASTEXITCODE -ne 0) { throw 'CLI build failed' }
& $compilerPath @commonArgs 'tests/test_geometry.cpp' '-o' 'build/fixture_tests.exe'
if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
& $compilerPath @commonArgs 'tests/test_cad.cpp' '-o' 'build/fixture_cad_tests.exe'
if ($LASTEXITCODE -ne 0) { throw 'CAD test build failed' }
& $compilerPath @commonArgs 'src/viewer.cpp' 'src/window_win32.cpp' '-lopengl32' '-lgdi32' '-luser32' '-o' 'build/fixture_viewer.exe'
if ($LASTEXITCODE -ne 0) { throw 'Viewer build failed' }
if ($Mode -eq 'Test') {
    & '.\build\fixture_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Geometry test failed' }
    & '.\build\fixture_cad_tests.exe'
    if ($LASTEXITCODE -ne 0) { throw 'CAD test failed' }
    & '.\build\fixture_cli.exe' '--scene' 'examples/nominal.scene'
    if ($LASTEXITCODE -ne 0) { throw 'CLI demonstration failed' }
} elseif ($Mode -eq 'Demo') {
    & '.\build\fixture_viewer.exe'
} elseif ($Mode -eq 'Capture') {
    & '.\build\fixture_viewer.exe' '--example' '2' '--capture' 'artifacts/opengl.ppm'
    if ($LASTEXITCODE -ne 0) { throw 'OpenGL capture failed' }
}
