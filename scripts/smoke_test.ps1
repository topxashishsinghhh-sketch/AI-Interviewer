[void][System.Reflection.Assembly]::LoadWithPartialName("System.Windows.Forms")

$root = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $root "build\src\ai_interviewer_console.exe"

if (-not (Test-Path $exe)) {
    Write-Host "[smoke] executable not found: $exe" -ForegroundColor Red
    Write-Host "[smoke] run: cmake --build build --config Release"
    exit 1
}

Write-Host "[smoke] preparing fresh data files..."
"[]" | Set-Content (Join-Path $root "data\users.json")
"[]" | Set-Content (Join-Path $root "data\attempts.json")
"{}" | Set-Content (Join-Path $root "data\scoreboard.json")
"{}" | Set-Content (Join-Path $root "data\daily_challenges.json")

$email = "smoke_" + [guid]::NewGuid().ToString("N").Substring(0,8) + "@test.local"

# register, login, dashboard, interview (10 answers), profile, leaderboard, logout
$before = @(
    "1"                # register
    $email             # email
    "smokepass123"     # password
    "Smoke Tester"     # name
    "1"                # role: student
    "Smoke College"    # college
    "CSE"              # branch
    "2"                # login
    $email
    "smokepass123"
    "1"                # dashboard: start interview
    "1"                # course: cse
    "3"                # subject: oop
    "1"                # mode: practice
)
$answers = @("A", "A", "A", "A", "A", "A", "B", "B", "B", "skip")
$after = @(
    ""                 # press enter after report
    "2"                # dashboard: daily challenge
    "A"                # challenge answer
    ""                 # press enter after bonus
    "3"                # dashboard: profile
    ""                 # press enter
    "4"                # dashboard: leaderboard
    "2"                # scope: my college
    "2"                # window: week
    "3"                # ranking: streak
    ""                 # press enter
    "5"                # logout
    # ---- admin flow ----
    "1"                # register admin
    "smoke_admin@test.local"
    "adminpass123"
    "Smoke Admin"
    "2"                # role: admin
    "Smoke College"
    "IT"
    "2"                # login as admin
    "smoke_admin@test.local"
    "adminpass123"
    "1"                # admin: platform statistics
    ""                 # press enter
    "3"                # admin: question bank stats
    ""                 # press enter
    "5"                # admin logout
)

$scriptInput = $before + $answers + $after

# PowerShell 5.1 piping into a native exe from a script file is unreliable,
# so write the scripted answers to a file and feed stdin via cmd redirection.
$inputFile = Join-Path $env:TEMP ("ai_interviewer_smoke_" + [guid]::NewGuid().ToString("N") + ".txt")
($scriptInput -join "`r`n") | Set-Content -LiteralPath $inputFile -Encoding ASCII
try {
    $output = cmd /c "`"$exe`" < `"$inputFile`" 2>&1" | Out-String
} finally {
    Remove-Item -LiteralPath $inputFile -Force -ErrorAction SilentlyContinue
}

if ($env:SMOKE_DEBUG -eq "1") {
    Write-Host "----- RAW OUTPUT START -----"
    Write-Host $output
    Write-Host "----- RAW OUTPUT END -----"
    Write-Host "----- INPUT LINES: $($scriptInput.Count) -----"
}

$checks = @(
    @{ Name = "registration";      Pattern = "Registration successful" },
    @{ Name = "login";             Pattern = "Login successful" },
    @{ Name = "course menu";       Pattern = "Select Course" },
    @{ Name = "subject menu";      Pattern = "Select Subject" },
    @{ Name = "mode menu";         Pattern = "Select Mode" },
    @{ Name = "interview started"; Pattern = "Interview Started" },
    @{ Name = "question 1";        Pattern = "Question 1/10" },
    @{ Name = "question 10";       Pattern = "Question 10/10" },
    @{ Name = "report";            Pattern = "Interview Complete" },
    @{ Name = "streak updated";    Pattern = "Streak:" },
    @{ Name = "scoreboard upsert"; Pattern = "ScoreboardRepository.upsert" },
    @{ Name = "daily challenge";   Pattern = "=== Daily Challenge ===" },
    @{ Name = "challenge bonus";   Pattern = "Bonus Points: 10" },
    @{ Name = "challenge score";   Pattern = "With Bonus:" },
    @{ Name = "admin register";    Pattern = "Registration successful" },
    @{ Name = "admin login";       Pattern = "Admin Dashboard" },
    @{ Name = "admin stats";       Pattern = "=== Platform Statistics ===" },
    @{ Name = "admin stats total"; Pattern = "Total Users:" },
    @{ Name = "admin questions";   Pattern = "=== Question Bank Stats ===" },
    @{ Name = "admin totals";      Pattern = "Subject\s+Easy\s+Medium\s+Hard\s+Total" },
    @{ Name = "profile";           Pattern = "=== Profile ===" },
    @{ Name = "streak calendar";   Pattern = "Streak Calendar" },
    @{ Name = "leaderboard";       Pattern = "=== Leaderboard ===" },
    @{ Name = "leaderboard filters"; Pattern = "Scope: college \| Window: week \| Ranking: streak" }
)

$failed = 0
foreach ($c in $checks) {
    if ($output -match $c.Pattern) {
        Write-Host ("[smoke] PASS  {0}" -f $c.Name)
    } else {
        Write-Host ("[smoke] FAIL  {0}" -f $c.Name) -ForegroundColor Red
        $failed++
    }
}

# verify persistence
$usersRaw = Get-Content (Join-Path $root "data\users.json") -Raw
$users = $usersRaw | ConvertFrom-Json
$me = $users | Where-Object { $_.email -eq $email } | Select-Object -First 1
if ($me -and $me.streak -ge 1) {
    Write-Host "[smoke] PASS  user streak persisted"
} else {
    Write-Host "[smoke] FAIL  user streak not persisted" -ForegroundColor Red
    $failed++
}

$attempts = (Get-Content (Join-Path $root "data\attempts.json") -Raw | ConvertFrom-Json)
if ($attempts.Count -ge 10) {
    Write-Host ("[smoke] PASS  attempts persisted ({0})" -f $attempts.Count)
} else {
    Write-Host ("[smoke] FAIL  attempts persisted: {0}" -f $attempts.Count) -ForegroundColor Red
    $failed++
}

$board = Get-Content (Join-Path $root "data\scoreboard.json") -Raw | ConvertFrom-Json
if ($board.'global_alltime_score' -and $board.'global_alltime_score'.Count -ge 1) {
    Write-Host "[smoke] PASS  leaderboard entry persisted"
} else {
    Write-Host "[smoke] FAIL  leaderboard entry missing" -ForegroundColor Red
    $failed++
}

$challenges = Get-Content (Join-Path $root "data\daily_challenges.json") -Raw | ConvertFrom-Json
$challengeProps = $challenges.PSObject.Properties.Name
if ($challengeProps.Count -ge 1) {
    Write-Host "[smoke] PASS  daily challenge persisted ($($challengeProps.Count) entry)"
} else {
    Write-Host "[smoke] FAIL  daily challenge not persisted" -ForegroundColor Red
    $failed++
}

if ($failed -gt 0) {
    Write-Host ("[smoke] {0} check(s) failed" -f $failed) -ForegroundColor Red
    exit 1
}

Write-Host "[smoke] all checks passed"
