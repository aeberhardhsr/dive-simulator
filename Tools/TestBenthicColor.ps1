param([switch]$Packaged,[string]$PackageDirectory='Builds/WindowsBenthicColor')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/benthic-color-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
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
foreach($pattern in @('T_Bed_*.uasset','T_RockScan*.uasset')) {$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Textures') -Filter $pattern}
foreach($f in @('Tools/texture_color.py','Tools/build_benthic_color.py','Tools/build_bed_content.py','Tools/build_rock_surface_content.py','Tools/TestBenthicColor.ps1','Tools/SmokeTest.ps1','Tools/Shaders/Lakebed.ush','Documentation/BED_MATERIALS.json','Documentation/ROCK_SURFACES.json','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $f)}
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$hostRecord.files=@($files | Sort-Object FullName -Unique | ForEach-Object {
    @{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='weesen';entry=0},@{name='murg';entry=1},@{name='ledi-close';entry=2;close=$true},
    @{name='ledi-bounded';entry=2;bounded=$true},@{name='torch';torch=$true},@{name='shore';shore=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
    $opts=@{Scenario='rocks';Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
    if($case.torch) {$opts.Scenario='torch';$opts.Hour=1.5}
    elseif($case.shore) {$opts.Scenario='shore-preview';$opts.Entry=0;$opts.GroundView=$true}
    else {$opts.HabitatView=$true;$opts.Entry=$case.entry;$opts.PopulationChecks=$true;$opts.HabitatClose=[bool]$case.close}
    if($case.bounded) {$opts.BoundedWater=$true;$opts.WaterInterface=$true}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $opts.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering failed: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($case.shore){1}elseif($case.torch){2}else{3}
    if($images.Count -ne $expected) {throw "Missing captures $($case.name): $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "BENTHIC_COLOR=$root"
