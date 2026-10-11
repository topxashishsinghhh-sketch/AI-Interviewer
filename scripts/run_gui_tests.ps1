<#
    run_gui_tests.ps1 - configure, build and run the Qt 6 GUI + its QtTest suite.

    Qt 6.11.2 ships prebuilt with MinGW-w64 GCC 13.1, so the GUI is compiled in a
    separate build directory (build-gui) with that toolchain. The console build
    keeps using C:/MinGW GCC 6.3 in ./build.
#>
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$qtPrefix = "C:/Qt/6.11.2/mingw_64"
$qtTools = "C:/Qt/Tools/mingw1310_64"

if (-not (Test-Path $qtPrefix)) {
    Write-Host "[gui] Qt not found at $qtPrefix - skipping GUI build" -ForegroundColor Yellow
    exit 0
}

$buildDir = "$root\build-gui"
$cache = Join-Path $buildDir "CMakeCache.txt"
$needsClean = $false

if (Test-Path $cache) {
    $cached = Select-String -Path $cache -Pattern "CMAKE_(CXX_COMPILER|GENERATOR):" |
        ForEach-Object { $_.Line.Trim() }
    $usesQtCompiler = $cached | Where-Object { $_ -like "*mingw1310_64*" }
    if (-not $usesQtCompiler) {
        Write-Host "[gui] existing build cache uses a different toolchain - reconfiguring"
        $needsClean = $true
    }
} else {
    $needsClean = $true
}

if ($needsClean) {
    if (Test-Path $buildDir) {
        Remove-Item $buildDir -Recurse -Force
    }
}

Write-Host "[gui] configuring with Qt toolchain..."
# -DBUILD_WEB=ON is deliberate: this script shares build-gui with run_web.ps1.
# Omitting it reconfigures the tree without the web target, which deletes
# ai_interviewer_web.exe and breaks a running site plus the next run_web call.
# -UCMAKE_PROJECT_INCLUDE_BEFORE clears a debug hook left in the cache by an
# earlier session.
& cmake -G "MinGW Makefiles" `
    -DCMAKE_TOOLCHAIN_FILE="$root\cmake\qt-mingw.cmake" `
    -DCMAKE_PREFIX_PATH="$qtPrefix" `
    -DBUILD_GUI=ON `
    -DBUILD_GUI_TESTS=ON `
    -DBUILD_WEB=ON `
    -UCMAKE_PROJECT_INCLUDE_BEFORE `
    -DUSE_MONGO=OFF `
    -S $root -B $buildDir
if ($LASTEXITCODE -ne 0) { Write-Host "[gui] configure failed" -ForegroundColor Red; exit 1 }

Write-Host "[gui] building..."
$guiBuildOutput = & cmake --build $buildDir --config Release 2>&1
$guiWarnings = $guiBuildOutput | Select-String -Pattern "warning:"
if ($LASTEXITCODE -ne 0) {
    Write-Host "[gui] build failed" -ForegroundColor Red
    $guiBuildOutput | Select-Object -Last 20
    exit 1
}
if ($guiWarnings) {
    Write-Host "[gui] build emitted warnings:" -ForegroundColor Red
    $guiWarnings | ForEach-Object { Write-Host "  $_" }
    exit 1
}

$env:PATH = "$qtTools\bin;$qtPrefix\bin;" + $env:PATH
$env:QT_QPA_PLATFORM = "offscreen"

$testExe = "$buildDir\tests\gui\gui_smoke_test.exe"
if (Test-Path $testExe) {
    Write-Host "[gui] running QtTest suite..."
    & $testExe
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[gui] QtTest suite FAILED (exit $LASTEXITCODE)" -ForegroundColor Red
        exit $LASTEXITCODE
    }
    Write-Host "[gui] QtTest suite passed"
} else {
    Write-Host "[gui] test executable not built" -ForegroundColor Yellow
}

$guiExe = "$buildDir\src\ai_interviewer_gui.exe"
if (Test-Path $guiExe) {
    Write-Host "[gui] GUI executable: $guiExe"
} else {
    Write-Host "[gui] GUI executable not built" -ForegroundColor Red
    exit 1
}
