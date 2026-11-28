param([string]$PackageDirectory='Builds/WindowsModes')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/mode-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;executable_sha256=(Get-FileHash -LiteralPath $exe).Hash;
    source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$completed=@()
foreach($case in @('exploration','manual','simulation','hardcore','warm','hypoxia')) {
    $folder=Join-Path $result $case
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') -Packaged -PackageDirectory $PackageDirectory -Scenario modes -ModeCase $case -MaxFPS 30 -RuntimeLog (Join-Path $folder 'Runtime.log') *> (Join-Path $folder 'Summary.log')
    $saved=Join-Path $PackageDirectory 'divingsim/Saved'
    Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -File -Filter "Mode_${case}_*.png" |
        Where-Object { $_.LastWriteTimeUtc -ge $started } | Copy-Item -Destination $folder
    $log=Get-Content -LiteralPath (Join-Path $folder 'Runtime.log') -Raw
    $match=[regex]::Match($log,'DIVE_MODE_PASS[^\r\n]* log=([^\r\n]+)')
    if(-not $match.Success) { throw 'Missing validated recording path.' }
    Copy-Item -LiteralPath (Join-Path $saved ('DiveLogs/'+[IO.Path]::GetFileName($match.Groups[1].Value.Trim()))) -Destination (Join-Path $folder 'Dive.json')
    $completed+=@{name=$case;started_utc=$started.ToString('o');completed_utc=[DateTime]::UtcNow.ToString('o');status='PASS'}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $case $folder"
}
Write-Output "MODE_VALIDATION=$result"
