param([string]$PackageDirectory='Builds/WindowsPlantColor')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$root=Join-Path $repo ('artifacts/plant-color-package-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source') -Recurse -File)
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Plants') -Filter '*.uasset'
foreach($file in @('Tools/TestPlantColor.ps1','Tools/SmokeTest.ps1','Tools/Shaders/AquaticPlant.ush','divingsim/Content/Diving/Materials/M_AquaticPlant.uasset','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $file)}
$hostRecord=@{packaged=$true;source_head=(& git -C $repo rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()}
$hostRecord.files=@($files | Sort-Object FullName -Unique | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord.package_containers=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='weesen';entry=0;low=$true;images=3},@{name='murg';entry=1;images=3},
    @{name='forms';entry=0;forms=$true;images=12},@{name='raster';entry=0;low=$true;raster=$true;images=3},
    @{name='torch';entry=0;scenario='torch';images=2},@{name='shore';entry=0;scenario='shore';images=9},
    @{name='stream';entry=1;scenario='stream';images=2})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
    $opts=@{Packaged=$true;PackageDirectory=$PackageDirectory;Entry=$case.entry;Scenario='rocks';WaterPreview=$true;Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.scenario) {$opts.Scenario=$case.scenario;$opts.PopulationChecks=$case.scenario -eq 'stream'}
    else {$opts.HabitatView=-not $case.forms;$opts.PlantView=[bool]$case.forms;$opts.HabitatLowView=[bool]$case.low;$opts.PopulationChecks=$true;$opts.DisableNanite=[bool]$case.raster}
    if($case.name -eq 'torch') {$opts.Hour=0}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $opts.RuntimeLog -Raw
    if($log -notmatch 'DIVE_SMOKE_PASS' -or $log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Plant color runtime failed: $($case.name)"}
    if($opts.PopulationChecks -and $log -notmatch 'DIVE_PLANT_BEDS_PASS') {throw 'Missing plant population validation.'}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $case.images) {throw "Unexpected captures $($case.name): $($images.Count)/$($case.images)"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
foreach($input in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $input.file)).Hash.ToLowerInvariant() -ne $input.sha256) {throw "Input changed during validation: $($input.file)"}}
foreach($input in $hostRecord.package_containers) {if((Get-FileHash -LiteralPath (Join-Path $PackageDirectory ('divingsim/Content/Paks/'+$input.file))).Hash.ToLowerInvariant() -ne $input.sha256) {throw 'Package changed during validation.'}}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $root -WaterPreview
Write-Output "PLANT_COLOR_PACKAGE=$root"
