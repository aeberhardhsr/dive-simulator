param([string]$PackageDirectory='Builds/WindowsFish')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/fish-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$saved=Join-Path $PackageDirectory 'divingsim/Saved/Screenshots'
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;packaged=$true;package_containers=$containers} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{name='fish';scenario='fish';entry=0},
    @{name='weesen';scenario='dive';entry=0},
    @{name='murg';scenario='dive';entry=1},
    @{name='mols';scenario='dive';entry=2},
    @{name='stream';scenario='stream';entry=1;population=$true},
    @{name='discoveries';scenario='discoveries';entry=0},
    @{name='decompression';scenario='decompression';entry=0}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;Viewport='1600x900';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.population) { $options.PopulationChecks=$true }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used') { throw "Runtime validation failed: $folder" }
    if($case.scenario -eq 'decompression' -and $log -notmatch 'DIVE_DECOMPRESSION_PASS stages=6 pause=1 hold=1 input_cycle=4') { throw 'Incomplete decompression fixture' }
    if($case.scenario -eq 'fish' -and $log -notmatch 'DIVE_FISH_INSPECTION_PASS species=5 views=20 captures=40 lods=4') { throw 'Incomplete fish inspection' }
    $captures=@(Get-ChildItem -LiteralPath $saved -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($captures.Count -lt 1) { throw 'Missing fresh rendered captures.' }
    $captures | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;viewport='1600x900';mode=$case.mode;status='PASS';screenshots=$captures.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "FISH_VALIDATION=$result"
