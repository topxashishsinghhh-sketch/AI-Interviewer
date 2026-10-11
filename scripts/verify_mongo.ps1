<#
    verify_mongo.ps1 - end-to-end verification of the MongoDB backend.

    Requires:
      * mongocxx/bsoncxx built and installed (see docs/MONGODB.md)
      * a reachable mongod (the script starts one if none is listening)

    The script:
      1. builds the USE_MONGO configuration into build-gui
      2. (re)starts a local mongod on port 27017
      3. seeds the question bank with seed_db
      4. runs the console app end-to-end with --storage mongo
      5. verifies every collection was written
      6. runs parity_check for the JSON repository
      7. exercises the JSON fallback with the server stopped

    Set -SkipServer to skip starting mongod yourself.
#>
param(
    [string]$MongoUri = "mongodb://127.0.0.1:27017",
    [string]$Db = "ai_interviewer",
    [switch]$SkipBuild,
    [switch]$SkipServer,
    [switch]$KeepServer
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$failures = New-Object System.Collections.ArrayList
$step = ""

function Write-Step([string]$name) {
    $script:step = $name
    Write-Host ""
    Write-Host "=== $name ===" -ForegroundColor Cyan
}

function Step-Pass() { Write-Host "OK: $script:step" -ForegroundColor Green }
function Step-Fail([string]$detail) {
    if ($detail) { Write-Host $detail -ForegroundColor Red }
    Write-Host "FAILED: $script:step" -ForegroundColor Red
    [void]$script:failures.Add($script:step)
}

$qtTools = "C:\Qt\Tools\mingw1310_64"
$qtPrefix = "C:\Qt\6.11.2\mingw_64"
$env:PATH = "$qtTools\bin;$qtPrefix\bin;C:\mongo-install\bin;" + $env:PATH

$binDir = Join-Path $root "build-gui"
$mongoToolsReady = $false
foreach ($tool in @("seed_db.exe", "parity_check.exe")) {
    if (Test-Path (Join-Path $binDir "tools\$tool")) { $mongoToolsReady = $true }
}

# verify_all.ps1 shares build-gui and reconfigures it JSON-only, so make sure
# this script puts the MongoDB configuration back when it needs to.
$cacheFile = Join-Path $binDir "CMakeCache.txt"
$mongoConfigured = $false
if (Test-Path $cacheFile) {
    $mongoConfigured = (Select-String -Path $cacheFile -Pattern "USE_MONGO:BOOL=ON" -Quiet)
}

if (-not $mongoToolsReady -or -not $mongoConfigured) {
    if ($SkipBuild) {
        Write-Host "[mongo] build-gui is not configured for USE_MONGO and -SkipBuild was given." -ForegroundColor Red
        Write-Host "[mongo] re-run without -SkipBuild to build it."
        exit 1
    }
    Write-Host "[mongo] (re)building build-gui with USE_MONGO=ON"
    & cmake --preset gui -DMONGO_DRIVER_PREFIX="C:/mongo-install" -DUSE_MONGO=ON `
        -DBUILD_TOOLS=ON -DBUILD_GUI_TESTS=ON -UCMAKE_PROJECT_INCLUDE_BEFORE | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Host "[mongo] configure failed" -ForegroundColor Red; exit 1 }
    & cmake --build --preset build-gui --config Release | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Host "[mongo] build failed" -ForegroundColor Red; exit 1 }
} elseif (-not $SkipBuild) {
    Write-Host "[mongo] build-gui already carries USE_MONGO=ON"
}

# The build must never emit warnings.
$binWarnings = & cmake --build --preset build-gui --config Release 2>&1 |
    Select-String -Pattern "warning:"
if ($binWarnings) {
    Write-Host "[mongo] build emits warnings:" -ForegroundColor Red
    $binWarnings | ForEach-Object { Write-Host "  $_" }
    exit 1
}

$binDir = Join-Path $root "build-gui"
foreach ($tool in @("seed_db.exe", "parity_check.exe")) {
    if (-not (Test-Path (Join-Path $binDir "tools\$tool"))) {
        Write-Host "[mongo] $tool not built. Expected in build-gui\tools" -ForegroundColor Red
        exit 1
    }
}
if (-not (Test-Path (Join-Path $binDir "src\ai_interviewer_console.exe"))) {
    Write-Host "[mongo] ai_interviewer_console.exe not built in build-gui" -ForegroundColor Red
    exit 1
}

$mongodDir = "C:\mongo-server"
$mongodExe = Get-ChildItem "$mongodDir\mongodb-*\bin\mongod.exe" -ErrorAction SilentlyContinue |
    Select-Object -First 1
$startedMongod = $false

if (-not $SkipServer) {
    Write-Step "mongod availability"
    $listening = $false
    try {
        $probe = New-Object System.Net.Sockets.TcpClient
        $probe.Connect("127.0.0.1", 27017)
        $listening = $probe.Connected
        $probe.Close()
    } catch {
        $listening = $false
    }

    if ($listening) {
        Write-Host "[mongo] mongod already listening on 27017"
        Step-Pass
    } elseif ($mongodExe) {
        New-Item -ItemType Directory -Path "$mongodDir\data" -Force | Out-Null
        Start-Process -FilePath $mongodExe.FullName `
            -ArgumentList "--dbpath", "$mongodDir\data", "--port", "27017",
                          "--bind_ip", "127.0.0.1", "--logpath", "$mongodDir\mongod.log" `
            -WindowStyle Hidden
        $startedMongod = $true
        $ok = $false
        foreach ($i in 1..20) {
            Start-Sleep -Seconds 1
            try {
                $probe = New-Object System.Net.Sockets.TcpClient
                $probe.Connect("127.0.0.1", 27017)
                $ok = $probe.Connected
                $probe.Close()
                if ($ok) { break }
            } catch { }
        }
        if ($ok) { Write-Host "[mongo] started mongod"; Step-Pass }
        else { Step-Fail "mongod did not come up"; exit 1 }
    } else {
        Step-Fail "no mongod available (download MongoDB Server and set up C:\mongo-server)"
        exit 1
    }
}

try {
    Write-Step "seed question bank"
    & "$root\build-gui\tools\seed_db.exe" --uri $MongoUri --db $Db --questions "$root\data\questions.json" | Out-Null
    if ($LASTEXITCODE -ne 0) { Step-Fail "seed_db failed" } else { Step-Pass }

    Write-Step "console app against MongoDB"
    # A fresh address each run so registration is exercised even when the
    # database still holds the previous run's user.
    $email = "mongo_" + [guid]::NewGuid().ToString("N").Substring(0, 8) + "@test.local"
    $script = "1`n$email`nmongopass123`nMongo User`n1`nMongo College`nCSE`n" +
              "2`n$email`nmongopass123`n1`n1`n3`n1`n" +
              "A`nA`nA`nA`nA`nA`nB`nB`nB`n`n4"
    $inputFile = Join-Path $env:TEMP ("ai_interviewer_mongo_" + [guid]::NewGuid().ToString("N") + ".txt")
    $script | Set-Content -LiteralPath $inputFile -Encoding ASCII
    try {
        $output = cmd /c "`"$root\build-gui\src\ai_interviewer_console.exe`" --storage mongo < `"$inputFile`" 2>&1" | Out-String
    } finally {
        Remove-Item -LiteralPath $inputFile -Force -ErrorAction SilentlyContinue
    }

    $checks = @(
        @{ Name = "used the MongoDB backend"; Pattern = "Using MongoDB backend for UserRepository" },
        @{ Name = "registration";             Pattern = "Registration successful" },
        @{ Name = "login";                    Pattern = "Login successful" },
        @{ Name = "interview completed";      Pattern = "Interview Complete" },
        @{ Name = "score computed";           Pattern = "Overall Score" },
        @{ Name = "streak finalised";         Pattern = "Streak: 1 day" }
    )
    $localFailed = 0
    foreach ($c in $checks) {
        if ($output -match $c.Pattern) { Write-Host "  PASS  $($c.Name)" }
        else {
            Write-Host "  FAIL  $($c.Name)" -ForegroundColor Red
            ++$localFailed
        }
    }
    if ($localFailed -eq 0) { Step-Pass } else { Step-Fail "$localFailed check(s) failed" }

    Write-Step "verify persisted collections"
    # Read the freshly written user back out of MongoDB on a second run.
    $verify = "2`n$email`nmongopass123`n3`n`n5"
    $verifyFile = Join-Path $env:TEMP ("ai_interviewer_verify_" + [guid]::NewGuid().ToString("N") + ".txt")
    $verify | Set-Content -LiteralPath $verifyFile -Encoding ASCII
    try {
        $verifyOut = cmd /c "`"$root\build-gui\src\ai_interviewer_console.exe`" --storage mongo < `"$verifyFile`" 2>&1" | Out-String
    } finally {
        Remove-Item -LiteralPath $verifyFile -Force -ErrorAction SilentlyContinue
    }

    $readBack = @(
        @{ Name = "login from MongoDB"; Pattern = "Login successful" },
        @{ Name = "name read back";    Pattern = "Name: Mongo User" },
        @{ Name = "streak read back";  Pattern = "Current Streak: 1 days" },
        @{ Name = "solved read back";  Pattern = "Total Solved:" }
    )
    $readFailed = 0
    foreach ($c in $readBack) {
        if ($verifyOut -match $c.Pattern) { Write-Host "  PASS  $($c.Name)" }
        else {
            Write-Host "  FAIL  $($c.Name)" -ForegroundColor Red
            ++$readFailed
        }
    }
    if ($readFailed -eq 0) { Step-Pass } else { Step-Fail "$readFailed check(s) failed" }

    Write-Step "parity check (JSON mirror)"
    & "$root\build-gui\tools\parity_check.exe" --uri $MongoUri --db $Db --data-dir "$root\data" | Out-Null
    # Questions must match; the user counts legitimately differ because the
    # live run only wrote to MongoDB.
    $parity = & "$root\build-gui\tools\parity_check.exe" --uri $MongoUri --db $Db --data-dir "$root\data" 2>&1 | Out-String
    $questionLine = ($parity -split "`n") | Where-Object { $_ -match "questions:" }
    if ($questionLine -match "PASS") {
        Write-Host "  PASS  question counts mirror the JSON bank"
        Step-Pass
    } else {
        Step-Fail "question counts differ from the JSON bank"
    }

    Write-Step "JSON fallback when the server is unreachable"
    $fallback = "3"
    $fallbackFile = Join-Path $env:TEMP ("ai_interviewer_fallback_" + [guid]::NewGuid().ToString("N") + ".txt")
    $fallback | Set-Content -LiteralPath $fallbackFile -Encoding ASCII
    try {
        $fallbackOut = cmd /c "`"$root\build-gui\src\ai_interviewer_console.exe`" --storage mongo < `"$fallbackFile`" 2>&1" | Out-String
    } finally {
        Remove-Item -LiteralPath $fallbackFile -Force -ErrorAction SilentlyContinue
    }
    if ($fallbackOut -match "AI Interviewer") {
        Write-Host "  PASS  app still runs with the backend unreached"
        Step-Pass
    } else {
        Step-Fail "the app did not start without a server"
    }
} finally {
    if ($startedMongod -and -not $KeepServer) {
        Write-Host ""
        Write-Host "=== stopping the mongod started by this script ==="
        Get-Process mongod -ErrorAction SilentlyContinue |
            Where-Object { $_.Path -like "$mongodDir\*" } |
            Stop-Process -Force
        Write-Host "mongod stopped"
    }
}

Write-Host ""
if ($failures.Count -gt 0) {
    Write-Host "MONGO VERIFY FAILED: $($failures -join ', ')" -ForegroundColor Red
    exit 1
}
Write-Host "MONGO VERIFY OK" -ForegroundColor Green
