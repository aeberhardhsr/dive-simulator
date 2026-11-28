param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$sandbox=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/bounded-water-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring($repo.Length+1).Replace('\','/')
    $hash=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    $copy=Join-Path $sandbox $relative.Substring('divingsim/'.Length)
    if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash.ToLowerInvariant() -ne $hash) {throw "Sandbox source differs: $relative"}
    @{file=$relative;sha256=$hash}
})
foreach($relative in @('Tools/SmokeTest.ps1','Tools/TestBoundedWater.ps1','Tools/build_water_volume_content.py','Tools/Shaders/WaterVolume.ush','Tools/Shaders/UnderwaterView.ush','Tools/build_optics_content.py','divingsim/Content/Diving/Materials/M_WaterVolume.uasset','divingsim/Content/Diving/Materials/M_Underwater.uasset','divingsim/Content/Diving/Materials/M_SolarWater.uasset','Tools/build_water_interface_content.py','Tools/build_unreal_content.py','Tools/Shaders/WaterInterfaceFresnel.ush','Tools/Shaders/WaterOptics.ush','divingsim/Config/DefaultEngine.ini','divingsim/Content/Diving/Materials/M_Water.uasset','divingsim/Content/Diving/Materials/M_WaterTop.uasset','divingsim/Content/Diving/Materials/M_WaterUnder.uasset','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll')) {
    $files+=@{file=$relative;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $relative)).Hash.ToLowerInvariant()}
}
@{native_editor=$true;packaged=$false;started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);files=$files} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='surface-before';surface=$true},
    @{name='surface-bounded';surface=$true;bounded=$true},
    @{name='surface-zero-density';surface=$true;bounded=$true;zero=$true},
    @{name='bed-before';bed=$true},@{name='bed-bounded';bed=$true;bounded=$true},
    @{name='sun-bounded';bed=$true;sun=$true;bounded=$true},
    @{name='storm-bounded';storm=$true;bounded=$true},
    @{name='dark-bounded-on';torch=$true;bounded=$true},
    @{name='dark-bounded-off';torch=$true;bounded=$true;off=$true},
    @{name='night-sky';sky=$true;bounded=$true},
    @{name='dark-moonlit';torch=$true;bounded=$true;off=$true;moon=$true},
    @{name='fallback';bed=$true;bounded=$true;fallback=$true}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{EditorSandbox=$true;EngineRoot=$EngineRoot;Entry=0;Hour=12;Date='2026-06-21';MaxFPS=30;WaterInterface=$true;BoundedWater=[bool]$case.bounded;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.zero) {$options.WaterVolumeGain=0}
    if($case.fallback) {$options.DisableVolumetrics=$true}
    if($case.surface) {$options.Scenario='rocks';$options.WaterView=$true;$options.WaterStart=1;$options.WaterEnd=$options.WaterStart}
    elseif($case.bed) {$options.Scenario='rocks';$options.HabitatView=$true;$options.HabitatSunView=[bool]$case.sun;$options.Entry=2}
    elseif($case.storm) {$options.Scenario='surface-storm';$options.Entry=1}
    elseif($case.sky) {$options.Scenario='surface-calm';$options.SkyView=$true;$options.Hour=1.5;$options.SkyPitch=65}
    else {$options.Scenario='torch';$options.Hour=1.5;$options.LampOff=[bool]$case.off}
    if($case.moon) {$options.Date='2026-12-21';$options.Hour=20}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering check failed: $($case.name)"}
    $expected=if($case.torch){2}else{3}
    $images=@(Get-ChildItem (Join-Path $sandbox 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $expected) {throw "Missing fresh captures: $($case.name) $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;bounded=[bool]$case.bounded;zero=[bool]$case.zero;fallback=[bool]$case.fallback;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "BOUNDED_WATER=$root"
