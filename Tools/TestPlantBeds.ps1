param([switch]$Packaged,[switch]$Preview,[string]$PackageDirectory='Builds/WindowsPlantBedsReady')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if($Packaged -and $Preview) {throw 'Packaged validation requires all cases.'}
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/plant-beds-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
$hostRecord=@{packaged=[bool]$Packaged;preview=[bool]$Preview;source_head=(& git -C $repo rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o')}
if($Packaged) {
    if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
    . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
    $exe=Resolve-DivePackageExecutable $PackageDirectory;$runtime=Join-Path $PackageDirectory 'divingsim'
    $hostRecord.executable=$exe;$hostRecord.sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    $hostRecord.package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {
        @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
    })
}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -Recurse -File)
foreach($folder in @('Plants','Rocks')) {$files+=Get-ChildItem (Join-Path $repo "divingsim/Content/Diving/$folder") -Filter '*.uasset'}
foreach($pattern in @('M_AquaticPlant.uasset','M_Rock*.uasset')) {$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -Filter $pattern}
foreach($f in @('Tools/TestPlantBeds.ps1','Tools/SmokeTest.ps1','Tools/Shaders/AquaticPlant.ush','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $f)}
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$hostRecord.files=@($files | Sort-Object FullName -Unique | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='before';entry=0;legacy=$true;low=$true},@{name='after';entry=0;low=$true})
if(-not $Preview) {$cases+=@(@{name='murg';entry=1},@{name='ledi';entry=2},@{name='raster';entry=0;low=$true;raster=$true},@{name='stream';entry=1;scenario='stream'},@{name='shore';entry=0;scenario='shore'},@{name='discoveries';entry=0;scenario='discoveries'})}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
    $opts=@{Entry=$case.entry;Scenario='rocks';Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log');LegacyPlantPlacement=[bool]$case.legacy}
    if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
    if($case.scenario) {$opts.Scenario=$case.scenario;$opts.PopulationChecks=$case.scenario -eq 'stream'}
    else {$opts.HabitatView=$true;$opts.HabitatLowView=[bool]$case.low;$opts.PopulationChecks=$true;$opts.DisableNanite=[bool]$case.raster}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $opts.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Failed: $($case.name)"}
    if($opts.PopulationChecks -and $log -notmatch 'DIVE_PLANT_BEDS_PASS') {throw 'Missing plant layout check.'}
    $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($case.name -eq 'discoveries') {
        $required=@('Discoveries_0_Gallery_DE.png','Discoveries_0_Species_DE.png','Discoveries_0_Species_EN.png','Discoveries_0_Logbook.png','Discoveries_0_Plant_EN.png')
        $allowed=$required+@('DiveSmoke_0_discoveries.png','DiveUnderwater_0_discoveries.png')
        if(@($required | Where-Object {$_ -notin $images.Name}).Count -or @($images.Name | Where-Object {$_ -notin $allowed}).Count) {throw 'Missing or unexpected discovery capture.'}
    } else {
        $expected=if($case.name -eq 'shore'){9}elseif($case.name -eq 'stream'){2}else{3}
        if($images.Count -ne $expected) {throw "Capture count $($case.name): $($images.Count)/$expected"}
    }
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;legacy=[bool]$case.legacy;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "PLANT_BEDS=$root"
