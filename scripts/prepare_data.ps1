<#
    prepare_data.ps1 - (re)generate the JSON data files the console app expects.

    - merges data/questions/*.json into data/questions.json
    - creates empty users/attempts/scoreboard/daily_challenges files if missing
#>
$root = Split-Path -Parent $PSScriptRoot
$dataDir = Join-Path $root "data"
$questionsDir = Join-Path $dataDir "questions"
$theoryDir = Join-Path $dataDir "theory"

if (-not (Test-Path $dataDir)) {
    New-Item -ItemType Directory -Path $dataDir | Out-Null
    Write-Host "[prepare] created $dataDir"
}

if (-not (Test-Path $questionsDir)) {
    New-Item -ItemType Directory -Path $questionsDir | Out-Null
    Write-Host "[prepare] created $questionsDir"
}

if (-not (Test-Path $theoryDir)) {
    New-Item -ItemType Directory -Path $theoryDir | Out-Null
    Write-Host "[prepare] created $theoryDir"
}

function Merge-JsonArray($dir, $label) {
    $merged = @()
    $files = Get-ChildItem -Path $dir -Filter *.json -File | Sort-Object Name
    foreach ($f in $files) {
        try {
            $items = Get-Content -LiteralPath $f.FullName -Raw -Encoding UTF8 | ConvertFrom-Json
        } catch {
            Write-Host "[prepare] WARN: could not parse $($f.Name): $_" -ForegroundColor Yellow
            continue
        }
        if ($null -eq $items) { continue }
        if ($items -is [System.Array]) { $merged += $items } else { $merged += ,$items }
        Write-Host ("[prepare] {0}: {1} {2}" -f $f.Name, @($items).Count, $label)
    }
    return ,$merged
}

function Write-JsonArrayUtf8($path, $dir, $label) {
    # Built by concatenating the raw JSON text of each source file.
    #
    # This is deliberate: ConvertTo-Json round-trips non-ASCII through the
    # Windows-1252 console code page, which turns an em dash into "a-hat euro"
    # mojibake, and Set-Content -Encoding UTF8 adds a BOM. Reading the files as
    # UTF-8 text and writing them back as UTF-8 text preserves the characters
    # exactly and keeps the browser, nlohmann and the C++ parser in agreement.
    $bodies = @()
    foreach ($f in (Get-ChildItem -Path $dir -Filter *.json -File | Sort-Object Name)) {
        $raw = [System.IO.File]::ReadAllText($f.FullName, [System.Text.Encoding]::UTF8).Trim()
        if ($raw.StartsWith('[')) { $raw = $raw.Substring(1) }
        if ($raw.EndsWith(']'))   { $raw = $raw.Substring(0, $raw.Length - 1) }
        $raw = $raw.Trim().TrimEnd(',')
        if ($raw.Length -eq 0) { continue }
        $bodies += $raw
    }
    $text = "[`n  " + ($bodies -join ",`n  ") + "`n]`n"
    $encoding = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($path, $text, $encoding)
    return $bodies.Count
}

$merged = Merge-JsonArray $questionsDir "question(s)"

$mergedPath = Join-Path $dataDir "questions.json"
$questionFiles = Write-JsonArrayUtf8 $mergedPath $questionsDir "question"
Write-Host ("[prepare] wrote questions.json from {0} file(s), {1} question(s)" -f $questionFiles, $merged.Count)

# Theory articles (DSA sheet, algorithms, core CS, soft skills, ...).
# Mirrored into web/data/ so the offline / static mode can serve them too.
$theory = Merge-JsonArray $theoryDir "theory article(s)"
$theoryPath = Join-Path $dataDir "theory.json"
$theoryFiles = Write-JsonArrayUtf8 $theoryPath $theoryDir "theory article"
Write-Host ("[prepare] wrote theory.json from {0} file(s), {1} article(s)" -f $theoryFiles, $theory.Count)

# Theory is served to the browser too, so it has to live under web/data.
# (Join-Path takes two arguments on Windows PowerShell 5.1; nest the calls.)
$webData = Join-Path (Join-Path $root "web") "data"
if (-not (Test-Path $webData)) {
    New-Item -ItemType Directory -Path $webData -Force | Out-Null
    Write-Host "[prepare] created web/data"
}
Copy-Item -LiteralPath $mergedPath -Destination (Join-Path $webData "questions.json") -Force
Copy-Item -LiteralPath $theoryPath -Destination (Join-Path $webData "theory.json") -Force
Write-Host "[prepare] mirrored questions.json and theory.json into web/data"

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
