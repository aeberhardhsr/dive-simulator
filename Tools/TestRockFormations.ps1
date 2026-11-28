param([switch]$Packaged,[switch]$Preview,[string]$PackageDirectory='Builds/WindowsFormations')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if($Packaged -and $Preview) {throw 'Packaged validation requires all cases.'}
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/rock-formations-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
$hostRecord=@{packaged=[bool]$Packaged;preview=[bool]$Preview;source_head=(& git -C $repo rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o')}
if($Packaged) {
    if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
    . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
    $exe=Resolve-DivePackageExecutable $PackageDirectory;$runtime=Join-Path $PackageDirectory 'divingsim'
    $hostRecord.executable=$exe;$hostRecord.sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    $hostRecord.package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse)
foreach($folder in @('Materials','Rocks','Plants','Gravel')) {$files+=Get-ChildItem (Join-Path $repo "divingsim/Content/Diving/$folder") -Filter '*.uasset'}
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Textures') -Filter 'T_RockScan*.uasset'
foreach($f in @('Tools/TestRockFormations.ps1','Tools/SmokeTest.ps1','Tools/create_rock_assets.py','Tools/build_formation_content.py','Tools/build_rock_content.py','Tools/build_rock_surface_content.py','Documentation/ROCK_FORMATION_ASSETS.json','blender/exports/FormationRocks/Manifest.json','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $f)}
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$hostRecord.files=@($files | Sort-Object FullName -Unique | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='before';entry=0;low=$true;legacy=$true},@{name='after';entry=0;low=$true},@{name='models';models=$true})
if(-not $Preview) {$cases+=@(@{name='weesen';entry=0},@{name='murg';entry=1},@{name='walenstadt';entry=2},@{name='raster';entry=0;low=$true;raster=$true},@{name='stream';stream=$true},@{name='shore';shore=$true},@{name='sediment';sediment=$true})}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
    $opts=@{Scenario='rocks';Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log');LegacyFormations=[bool]$case.legacy;WaterPreview=$true}
    if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
    if($case.models) {$opts.FormationView=$true;$opts.PopulationChecks=$true}
    elseif($case.sediment) {$opts.Scenario='sediment';$opts.Entry=0}
    elseif($case.shore) {$opts.Scenario='shore';$opts.Entry=0}
    elseif($case.stream) {$opts.Scenario='stream';$opts.Entry=1;$opts.PopulationChecks=$true}
    else {$opts.HabitatView=$true;$opts.Entry=$case.entry;$opts.PopulationChecks=$true;$opts.HabitatLowView=[bool]$case.low;$opts.DisableNanite=[bool]$case.raster}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $opts.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering failed: $($case.name)"}
    if($opts.PopulationChecks -and $log -notmatch 'DIVE_FORMATIONS_PASS') {throw 'Missing formation validation.'}
    if($case.models -and $log -notmatch 'DIVE_FORMATION_INSPECTION_PASS variants=2 views=2 captures=4') {throw 'Formation inspection incomplete.'}
    $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($case.models){4}elseif($case.sediment){7}elseif($case.shore){9}elseif($case.stream){2}else{3}
    if($images.Count -ne $expected) {throw "Missing captures $($case.name): $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;options=$opts;status='PASS';images=@($images.Name)}
    $completed | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name)"
}
foreach($input in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $input.file)).Hash.ToLowerInvariant() -ne $input.sha256) {throw "Input changed: $($input.file)"}}
Write-Output "ROCK_FORMATIONS=$root"
