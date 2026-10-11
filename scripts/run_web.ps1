<#
    run_web.ps1 - build (if needed) and start the AI Interviewer web server.

    Serves the browser front-end from ./web and exposes the JSON API on a local
    port, using the same JSON or MongoDB storage backends as the console/GUI.

    Usage:
      powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1
      powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1 -Port 3000
      powershell -ExecutionPolicy Bypass -File scripts\run_web.ps1 -Storage mongo

    Then open http://127.0.0.1:<port> in a browser.
#>
param(
    [string]$BindHost = "127.0.0.1",
    [int]$Port = 8080,
    [string]$Storage = "json"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

# The server shares ai_interviewer_core with the GUI and needs 64-bit
# libraries, so it is built in build-gui with the Qt MinGW toolchain.
$qtTools = "C:\Qt\Tools\mingw1310_64"
$qtPrefix = "C:\Qt\6.11.2\mingw_64"
$mongoPrefix = "C:/mongo-install"
$exe = Join-Path $root "build-gui\src\ai_interviewer_web.exe"

if (-not (Test-Path $exe)) {
    Write-Host "[web] not built yet - configuring and building"
    $env:PATH = "$qtTools\bin;$qtPrefix\bin;" + $env:PATH

    if ($Storage -eq "mongo") {
        & cmake --preset gui -DBUILD_WEB=ON -DBUILD_GUI=ON -DUSE_MONGO=ON `
            -DMONGO_DRIVER_PREFIX="$mongoPrefix" 2>&1 | Out-Null
    } else {
        & cmake --preset gui -DBUILD_WEB=ON -DBUILD_GUI=ON 2>&1 | Out-Null
    }
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[web] cmake configure failed" -ForegroundColor Red
        exit 1
    }

    & cmake --build --preset build-gui --config Release 2>&1 | Out-Null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[web] build failed" -ForegroundColor Red
        exit 1
    }
}

if (-not (Test-Path $exe)) {
    Write-Host "[web] executable still missing at $exe" -ForegroundColor Red
    exit 1
}

$env:PATH = "$qtPrefix\bin;$mongoPrefix\bin;" + $env:PATH

# cpp-httplib resolves the mount directory by appending its own separator to
# whatever is passed, so an absolute Windows path turns into a mixed-separator
# path that FileStat cannot resolve -- the server then 404s every static file
# and the site looks blank. Run from the project root with relative paths
# instead, which is what the mount code handles correctly.
$staticDir = "web"
$dataDir = "data"

Write-Host "[web] starting on http://$BindHost`:$Port (storage: $Storage)"
Write-Host "[web] serving $root\$staticDir"
Write-Host "[web] open http://127.0.0.1:$Port in your browser; Ctrl+C to stop"
Write-Host ""

Push-Location $root
try {
    & $exe --host $BindHost --port "$Port" --storage $Storage `
        --static-dir $staticDir --data-dir $dataDir
} finally {
    Pop-Location
}
exit $LASTEXITCODE
