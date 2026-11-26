param([string]$PackageDirectory='Builds/WindowsRockDetail')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/rock-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;
  packaged=$true;package_containers=$containers;source_head=(& git -C $repo rev-parse HEAD);
  background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime)} |
    ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='forms';scenario='rocks';entry=0},@{name='forms-raster';scenario='rocks';entry=0;raster=$true},
    @{name='stream0';scenario='stream';entry=0;population=$true},@{name='stream1';scenario='stream';entry=1;population=$true},@{name='stream2';scenario='stream';entry=2;population=$true},
    @{name='shore0';scenario='shore';entry=0},@{name='shore1';scenario='shore';entry=1},@{name='shore2';scenario='shore';entry=2},
    @{name='discoveries';scenario='discoveries';entry=0})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.population) { $options.PopulationChecks=$true }
    if($case.raster) { $options.DisableNanite=$true }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used') { throw "Runtime validation failed: $folder" }
    if($case.scenario -eq 'rocks' -and $log -notmatch 'DIVE_ROCK_INSPECTION_PASS forms=4 views=2 captures=8') { throw 'Incomplete rock comparison' }
    if($case.raster -and $log -notmatch 'r.Nanite = "0"') { throw 'Raster comparison override missing' }
    if($case.population -and $log -notmatch 'DIVE_STEERING_WORLD_PASS') { throw 'Fish path/turn checks missing' }
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -lt 1 -or ($case.scenario -eq 'rocks' -and $images.Count -ne 8)) { throw 'Missing fresh rendered captures' }
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;status='PASS';screenshots=$images.Count;raster=[bool]$case.raster}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "ROCK_VALIDATION=$result"
