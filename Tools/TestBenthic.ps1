param([string]$PackageDirectory='Builds/WindowsBenthic')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/benthic-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;package_containers=$containers;
    source_head=(& git -C $repo rev-parse HEAD);packaged=$true;started_utc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='entry0';entry=0},@{name='entry1';entry=1},@{name='entry2';entry=2},
    @{name='raster';entry=2;raster=$true},@{name='night';entry=0;night=$true},
    @{name='shore0';entry=0;shore=$true},@{name='shore1';entry=1;shore=$true},@{name='shore2';entry=2;shore=$true},
    @{name='torch';entry=0;torch=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Entry=$case.entry;MaxFPS=30;Hour=12;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.torch) {$options.Scenario='torch';$options.Hour=0}
    elseif($case.shore) {$options.Scenario='shore-preview';$options.ShoreLookBack=$true;$options.GroundView=$true}
    else {$options.Scenario='rocks';$options.HabitatView=$true;$options.PopulationChecks=$true}
    if($case.raster) {$options.DisableNanite=$true}
    if($case.night) {$options.Hour=0}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_SMOKE_PASS' -or $log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Benthic check failed: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -lt 1 -or (-not $case.shore -and -not $case.torch -and $images.Count -ne 3)) {throw 'Missing fresh captures'}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;entry=$case.entry;raster=[bool]$case.raster;night=[bool]$case.night;shore=[bool]$case.shore;torch=[bool]$case.torch;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $result
Write-Output "BENTHIC_VALIDATION=$result"
