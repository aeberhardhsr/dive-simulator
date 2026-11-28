param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$sandbox=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/native-water-lighting-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring($repo.Length+1).Replace('\','/')
    $hash=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    $copy=Join-Path $sandbox $relative.Substring('divingsim/'.Length)
    if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash.ToLowerInvariant() -ne $hash) {throw "Sandbox source differs: $relative"}
    @{file=$relative;sha256=$hash}
})
foreach($relative in @('Tools/Shaders/UnderwaterView.ush','Tools/Shaders/SolarCaustics.ush','Tools/build_optics_content.py','Tools/build_caustics_content.py','divingsim/Content/Diving/Materials/M_Underwater.uasset','divingsim/Content/Diving/Materials/M_SolarWater.uasset','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll')) {
    $files+=@{file=$relative;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $relative)).Hash.ToLowerInvariant()}
}
@{native_editor=$true;packaged=$false;started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);files=$files} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='sun-legacy';sun=$true},@{name='sun-native';sun=$true;native=$true},
    @{name='bed-legacy'},@{name='bed-native';native=$true},
    @{name='surface-legacy';surface=$true},@{name='surface-native';surface=$true;native=$true},
    @{name='dark-native-on';torch=$true;native=$true},@{name='dark-native-off';torch=$true;native=$true;off=$true},
    @{name='fallback';native=$true;fallback=$true}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{EditorSandbox=$true;EngineRoot=$EngineRoot;Entry=2;Hour=12;Date='2026-06-21';MaxFPS=30;NativeScattering=[bool]$case.native;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.torch) {$options.Scenario='torch';$options.Entry=0;$options.Hour=1.5;$options.LampOff=[bool]$case.off}
    elseif($case.surface) {$options.Scenario='rocks';$options.WaterView=$true;$options.WaterStart=1;$options.WaterEnd=1;$options.Entry=0}
    else {$options.Scenario='rocks';$options.HabitatView=$true;$options.HabitatSunView=[bool]$case.sun}
    if($case.fallback) {$options.DisableVolumetrics=$true}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Rendering check failed: $($case.name)"}
    $expected=if($case.torch){2}else{3}
    $images=@(Get-ChildItem (Join-Path $sandbox 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $expected) {throw "Missing fresh captures: $($case.name) $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;native=[bool]$case.native;fallback=[bool]$case.fallback;status='PASS';screenshots=$expected}
    $completed | ConvertTo-Json | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
& (Join-Path $PSScriptRoot 'TestCaustics.ps1') -EditorSandbox -EngineRoot $EngineRoot -Case day |
    Set-Content (Join-Path $root 'CausticsRun.txt') -Encoding utf8
Write-Output "NATIVE_WATER_LIGHTING=$root"
