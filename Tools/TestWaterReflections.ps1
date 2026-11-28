param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$sandbox=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/water-reflections-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring($repo.Length+1).Replace('\','/')
    $hash=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    $copy=Join-Path $sandbox $relative.Substring('divingsim/'.Length)
    if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash.ToLowerInvariant() -ne $hash) {throw "Sandbox source differs: $relative"}
    @{file=$relative;sha256=$hash}
})
$inputs=@('Tools/SmokeTest.ps1','Tools/TestWaterReflections.ps1','Tools/build_water_interface_content.py','Tools/build_unreal_content.py',
    'Tools/Shaders/WaterInterfaceFresnel.ush','Tools/Shaders/WaterOptics.ush','Tools/Shaders/UnderwaterView.ush','Tools/Shaders/WaterVolume.ush',
    'divingsim/Config/DefaultEngine.ini','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll')
$inputs+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -Filter '*.uasset' -File | ForEach-Object {$_.FullName.Substring($repo.Length+1).Replace('\','/')})
foreach($relative in $inputs) {$files+=@{file=$relative;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $relative)).Hash.ToLowerInvariant()}}
@{native_editor=$true;packaged=$false;started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);files=$files} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='surface-software';surface=$true},
    @{name='surface-hardware-cache';surface=$true;hardware=$true},
    @{name='surface-hit';surface=$true;hardware=$true;hit=$true},
    @{name='surface-hit-raster';surface=$true;hardware=$true;hit=$true;raster=$true},
    @{name='storm-hit';storm=$true;hardware=$true;hit=$true},
    @{name='dark-hit-on';torch=$true;hardware=$true;hit=$true},
    @{name='dark-hit-off';torch=$true;hardware=$true;hit=$true;off=$true}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $mode=if($case.hit){'hit'}else{'front'}
    $options=@{EditorSandbox=$true;EngineRoot=$EngineRoot;Entry=0;Hour=12;Date='2026-06-21';MaxFPS=30;WaterInterface=$true;BoundedWater=$true;
        HardwareLumen=[bool]$case.hardware;WaterReflection=$mode;DisableNanite=[bool]$case.raster;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.surface) {$options.Scenario='rocks';$options.WaterView=$true;$options.WaterStart=1;$options.WaterEnd=1}
    elseif($case.storm) {$options.Scenario='surface-storm';$options.Entry=1}
    else {$options.Scenario='torch';$options.Hour=1.5;$options.LampOff=[bool]$case.off}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering check failed: $($case.name)"}
    $expected=if($case.torch){2}else{3}
    $images=@(Get-ChildItem (Join-Path $sandbox 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $expected) {throw "Missing fresh captures: $($case.name) $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;hardware=[bool]$case.hardware;hit=[bool]$case.hit;raster=[bool]$case.raster;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "WATER_REFLECTIONS=$root"
