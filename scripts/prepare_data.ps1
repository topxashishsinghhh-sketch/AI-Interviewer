<#
    prepare_data.ps1 - (re)generate the JSON data files the console app expects.

    - merges data/questions/*.json into data/questions.json
    - creates empty users/attempts/scoreboard/daily_challenges files if missing
#>
$root = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $root "data"
$questionsDir = Join-Path $dataDir "questions"

if (-not (Test-Path $dataDir)) {
    New-Item -ItemType Directory -Path $dataDir | Out-Null
    Write-Host "[prepare] created $dataDir"
}

if (-not (Test-Path $questionsDir)) {
    New-Item -ItemType Directory -Path $questionsDir | Out-Null
    Write-Host "[prepare] created $questionsDir"
}

$merged = @()
$files = Get-ChildItem -Path $questionsDir -Filter *.json -File | Sort-Object Name
foreach ($f in $files) {
    try {
        $items = Get-Content -LiteralPath $f.FullName -Raw | ConvertFrom-Json
    } catch {
        Write-Host "[prepare] WARN: could not parse $($f.Name): $_" -ForegroundColor Yellow
        continue
    }
    if ($null -eq $items) { continue }
    if ($items -is [System.Array]) { $merged += $items } else { $merged += ,$items }
    Write-Host ("[prepare] {0}: {1} question(s)" -f $f.Name, @($items).Count)
}

$mergedPath = Join-Path $dataDir "questions.json"
$merged | ConvertTo-Json -Depth 20 | Set-Content -LiteralPath $mergedPath -Encoding UTF8
Write-Host ("[prepare] wrote {0} questions to questions.json" -f $merged.Count)

$emptyFiles = @{
    "users.json"           = "[]"
    "attempts.json"        = "[]"
    "scoreboard.json"      = "{}"
    "daily_challenges.json" = "{}"
}
foreach ($name in $emptyFiles.Keys) {
    $path = Join-Path $dataDir $name
    if (-not (Test-Path $path)) {
        $emptyFiles[$name] | Set-Content -LiteralPath $path -Encoding UTF8
        Write-Host "[prepare] created $name"
    } else {
        Write-Host "[prepare] kept existing $name"
    }
}

# Sanity: difficulty/subject coverage report
$subjects = $merged | Group-Object subject | Sort-Object Name
Write-Host "[prepare] subjects:"
foreach ($s in $subjects) {
    $easy   = @($s.Group | Where-Object { $_.difficulty -eq "Easy" }).Count
    $medium = @($s.Group | Where-Object { $_.difficulty -eq "Medium" }).Count
    $hard   = @($s.Group | Where-Object { $_.difficulty -eq "Hard" }).Count
    Write-Host ("[prepare]   {0,-12} total={1,-3} easy={2} medium={3} hard={4}" -f $s.Name, $s.Count, $easy, $medium, $hard)
}
