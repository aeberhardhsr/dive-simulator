param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$sandbox=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/water-interface-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring($repo.Length+1).Replace('\','/')
    $hash=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    $copy=Join-Path $sandbox $relative.Substring('divingsim/'.Length)
    if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash.ToLowerInvariant() -ne $hash) {throw "Sandbox source differs: $relative"}
    @{file=$relative;sha256=$hash}
})
foreach($relative in @('Tools/SmokeTest.ps1','Tools/TestWaterInterface.ps1','Tools/build_water_interface_content.py','Tools/build_unreal_content.py','Tools/Shaders/WaterInterfaceFresnel.ush','Tools/Shaders/WaterOptics.ush','divingsim/Config/DefaultEngine.ini','divingsim/Content/Diving/Materials/M_Water.uasset','divingsim/Content/Diving/Materials/M_WaterTop.uasset','divingsim/Content/Diving/Materials/M_WaterUnder.uasset','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll')) {
    $files+=@{file=$relative;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $relative)).Hash.ToLowerInvariant()}
}
@{native_editor=$true;packaged=$false;started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);files=$files} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='surface-legacy';surface=$true},
    @{name='surface-software';surface=$true;interface=$true},
    @{name='surface-hardware';surface=$true;interface=$true;hardware=$true},
    @{name='surface-air-fog-control';surface=$true;interface=$true;airfog=$true},
    @{name='bed-legacy';bed=$true},@{name='bed-interface';bed=$true;interface=$true},
    @{name='storm-interface';storm=$true;interface=$true},
    @{name='dark-interface-on';torch=$true;interface=$true},
    @{name='dark-interface-off';torch=$true;interface=$true;off=$true}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{EditorSandbox=$true;EngineRoot=$EngineRoot;Entry=0;Hour=12;Date='2026-06-21';MaxFPS=30;WaterInterface=[bool]$case.interface;HardwareLumen=[bool]$case.hardware;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.surface) {$options.Scenario='rocks';$options.WaterView=$true;$options.WaterStart=if($case.airfog){3}else{1};$options.WaterEnd=$options.WaterStart}
    elseif($case.bed) {$options.Scenario='rocks';$options.HabitatView=$true;$options.Entry=2}
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
    $completed+=@{name=$case.name;interface=[bool]$case.interface;hardware=[bool]$case.hardware;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "WATER_INTERFACE=$root"
