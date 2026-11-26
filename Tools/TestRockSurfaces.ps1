param([switch]$Packaged,[string]$PackageDirectory='Builds/WindowsRockSurfaces')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/rock-surfaces-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
$hostRecord=@{packaged=[bool]$Packaged;source_head=(& git -C $repo rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o')}
if($Packaged) {
    if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
    . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
    $exe=Resolve-DivePackageExecutable $PackageDirectory
    $runtime=Join-Path $PackageDirectory 'divingsim'
    $hostRecord.executable=$exe;$hostRecord.sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    $hostRecord.package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {
        @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
    })
}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse)
$files+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -Filter 'M_Rock*.uasset')
$files+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Textures') -Filter 'T_RockScan*.uasset')
$files+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Rocks') -Filter '*.uasset')
foreach($f in @('Tools/TestRockSurfaces.ps1','Tools/SmokeTest.ps1','Tools/build_rock_surface_content.py','Tools/Shaders/Lakebed.ush','Documentation/ROCK_SURFACES.json','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $f)}
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$hostRecord.files=@($files | Sort-Object FullName -Unique | ForEach-Object {
    @{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='forms-legacy';legacy=$true},@{name='forms-scan'},@{name='forms-raster';raster=$true},
    @{name='habitat-legacy';habitat=$true;legacy=$true;bounded=$true},@{name='habitat-scan';habitat=$true;bounded=$true},
    @{name='habitat-default';habitat=$true},@{name='stream';stream=$true},@{name='torch';torch=$true})
if(-not $Packaged) {$cases+=@{name='unlit-scan';unlit=$true}}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
    $opts=@{Scenario='rocks';Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log');LegacyRockSurface=[bool]$case.legacy;DisableNanite=[bool]$case.raster}
    if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
    if($case.habitat) {$opts.HabitatView=$true;$opts.Entry=2;$opts.PopulationChecks=$true}
    if($case.bounded) {$opts.BoundedWater=$true;$opts.WaterInterface=$true}
    if($case.unlit) {$opts.DisableLighting=$true}
    if($case.stream) {$opts.Scenario='stream';$opts.Entry=1;$opts.PopulationChecks=$true}
    if($case.torch) {$opts.Scenario='torch';$opts.Hour=1.5}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $opts.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering failed: $($case.name)"}
    for($i=1;$i -le 3;$i++) {
        $material=if($case.legacy){'M_RockSurface'}else{"M_RockScan$i"}
        if($log -notmatch "DIVE_ROCK_SURFACE_READY variant=$i material=/Game/Diving/Materials/$material\.") {throw "Wrong rock material: $($case.name) $i"}
    }
    $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($case.habitat){3}elseif($case.torch -or $case.stream){2}else{8}
    if($images.Count -ne $expected) {throw "Missing captures $($case.name): $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;legacy=[bool]$case.legacy;bounded=[bool]$case.bounded;raster=[bool]$case.raster;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "ROCK_SURFACES=$root"
