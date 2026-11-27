param([ValidateRange(0,2)][int]$Entry=0,[switch]$SunView,[ValidateRange(0,23.99)][double]$Hour=12)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/spectral-view-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source'),(Join-Path $repo 'divingsim/Shaders') -Recurse -File)
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -File | Where-Object {$_.Name -match 'Water|Underwater'}
foreach($name in @('divingsim/divingsim.uproject','divingsim/Config/DefaultEngine.ini','Tools/SmokeTest.ps1','Tools/TestSpectralView.ps1','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll')) {$files+=Get-Item (Join-Path $repo $name)}
$hostRecord=@{entry=$Entry;sun_view=[bool]$SunView;hour=$Hour;source_head=(& git -C $repo rev-parse HEAD);files=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})}
$hostRecord | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
foreach($item in $hostRecord.files) {
 $snapshot=Join-Path (Join-Path $root 'inputs') $item.file
 New-Item -ItemType Directory -Force (Split-Path $snapshot -Parent) | Out-Null
 Copy-Item -LiteralPath (Join-Path $repo $item.file) -Destination $snapshot
}
$cases=@(@{name='before';enabled=$false;gain=1},@{name='zero';enabled=$true;gain=0},@{name='after';enabled=$true;gain=1})
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{EditorSandbox=$true;Scenario='rocks';Entry=$Entry;Hour=$Hour;Date='2026-06-21';MaxFPS=30;WaterPreview=$true;HabitatView=$true;HabitatLowView=$true;HabitatSunView=[bool]$SunView;SpectralView=[bool]$case.enabled;AbsorptionGain=$case.gain;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used') {throw "Runtime failure: $($case.name)"}
 if($case.enabled -and $log -notmatch ('DIVE_SPECTRAL_VIEW_READY gain='+$case.gain.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)+' samples=16 fog_resource=1 stage=BeforeDOF')) {throw 'Missing spectral rendering pass.'}
 $images=@(Get-ChildItem (Join-Path $repo 'Builds/EditorSandbox/divingsim/Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 if($images.Count -ne 3) {throw "Wrong capture count: $($case.name)"}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;options=$opts;images=@($images.Name);status='PASS'}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($item in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed during test: $($item.file)"}}
Write-Output "SPECTRAL_VIEW=$root"
