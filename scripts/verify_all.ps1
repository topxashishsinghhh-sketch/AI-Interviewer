<#
    verify_all.ps1 - convenience wrapper used before claiming work is done.

    Runs, in order:
      1. scripts\prepare_data.ps1   (regenerate the merged question bank)
      2. the console build + scripts\smoke_test.ps1
      3. the GUI build + QtTest suite (scripts\run_gui_tests.ps1)

    Exits non-zero if any step fails. Console warnings are treated as failures
    because the project target is a warning-free build.
#>
$root = Split-Path -Parent $PSScriptRoot
$failures = New-Object System.Collections.ArrayList
$currentStep = ""

function Write-Step([string]$name) {
    $script:currentStep = $name
    Write-Host ""
    Write-Host "=== $name ===" -ForegroundColor Cyan
}

function Step-Pass() {
    Write-Host "OK: $script:currentStep" -ForegroundColor Green
}

function Step-Fail([string]$detail) {
    if ($detail) { Write-Host $detail -ForegroundColor Red }
    Write-Host "FAILED: $script:currentStep" -ForegroundColor Red
    [void]$script:failures.Add($script:currentStep)
}

Write-Step "prepare data"
& powershell -ExecutionPolicy Bypass -File "$root\scripts\prepare_data.ps1" | Out-Null
if ($LASTEXITCODE -ne 0) { Step-Fail } else { Step-Pass }

Write-Step "console build (GCC 6.3)"
$env:PATH = "C:\MinGW\bin;" + $env:PATH
& cmake -G "MinGW Makefiles" -S $root -B "$root\build" | Out-Null
if ($LASTEXITCODE -ne 0) {
    Step-Fail "cmake configure failed"
} else {
    $buildOutput = & cmake --build "$root/build" --config Release 2>&1
    if ($LASTEXITCODE -ne 0) {
        Step-Fail ($buildOutput | Select-Object -Last 10 | Out-String)
    } else {
        $warnings = $buildOutput | Select-String -Pattern "warning:"
        if ($warnings) {
            Step-Fail "console build emitted warnings:`n$($warnings | Out-String)"
        } else {
            Step-Pass
        }
    }
}

Write-Step "console smoke test"
& powershell -ExecutionPolicy Bypass -File "$root\scripts\smoke_test.ps1"
if ($LASTEXITCODE -ne 0) { Step-Fail } else { Step-Pass }

Write-Step "gui build + QtTest (Qt 6 / GCC 13)"
& powershell -ExecutionPolicy Bypass -File "$root\scripts\run_gui_tests.ps1" | Out-Null
$guiExit = $LASTEXITCODE
if ($guiExit -ne 0) { Step-Fail "GUI build or QtTest failed (exit $guiExit)" } else { Step-Pass }

Write-Host ""
if ($failures.Count -gt 0) {
    Write-Host "VERIFY FAILED: $($failures -join ', ')" -ForegroundColor Red
    exit 1
}
Write-Host "VERIFY OK: all steps passed" -ForegroundColor Green