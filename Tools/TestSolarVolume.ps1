param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
if(-not $OutputDirectory) {$OutputDirectory=Join-Path $repo ('artifacts/solar-volume-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))}
$root=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath (Join-Path $root 'Host.json')) {throw 'Use a fresh result directory.'}
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source'),(Join-Path $repo 'divingsim/Shaders'),(Join-Path $repo 'divingsim/Config') -Recurse -File)
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -File | Where-Object {$_.Name -match 'Water|Underwater|Caustic'}
foreach($name in @('divingsim/divingsim.uproject','Tools/SmokeTest.ps1','Tools/TestCaustics.ps1','Tools/TestSolarVolume.ps1','Tools/PrepareSolarShaderStudy.py','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll','Builds/EditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll')) {$files+=Get-Item (Join-Path $repo $name)}
$inputs=@($files | Sort-Object FullName -Unique | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
@{source_head=(& git -C $repo rev-parse HEAD);native_editor=$true;files=$inputs} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
foreach($mode in @('DiveSolarStudy','DiveSolarVolumeStudy')) {Copy-Item -LiteralPath (Join-Path $repo "Builds/EditorSandbox/divingsim/Intermediate/$mode/Manifest.json") -Destination (Join-Path $root ($mode+'.json'))}
$cases=@()
foreach($view in @('low','sun')) {
 foreach($mode in @('direct','volume','combined')) {$cases+=@{name="$view-$mode";kind='habitat';sun=($view -eq 'sun');volume=($mode -ne 'direct');spectral=($mode -eq 'combined');count=3}}
}
foreach($mode in @('direct','volume')) {
 $cases+=@{name="torch-$mode";kind='torch';volume=($mode -eq 'volume');spectral=$false;count=2}
 $cases+=@{name="air-$mode";kind='air';volume=($mode -eq 'volume');spectral=$false;count=4}
}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $started=[DateTime]::UtcNow
 if($case.kind -eq 'air') {
  $result=& (Join-Path $PSScriptRoot 'TestCaustics.ps1') -EditorSandbox -WaterPreview -SolarShaderStudy -SolarVolumeStudy:$case.volume -Case air
  $result | Set-Content (Join-Path $folder 'Summary.log')
  $line=@($result | Where-Object {$_ -like 'DIVE_CAUSTICS_VALIDATION=*'})
  if($line.Count -ne 1) {throw "Missing fixture result: $($case.name)"}
  Get-ChildItem -LiteralPath $line[0].Substring('DIVE_CAUSTICS_VALIDATION='.Length) -File | Copy-Item -Destination $folder
 } else {
  $opts=@{EditorSandbox=$true;Entry=0;Date='2026-06-21';WaterPreview=$true;SolarShaderStudy=$true;SolarVolumeStudy=[bool]$case.volume;SpectralView=[bool]$case.spectral;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
  if($case.kind -eq 'habitat') {$opts.Scenario='rocks';$opts.Hour=12;$opts.HabitatView=$true;$opts.HabitatLowView=$true;$opts.HabitatSunView=[bool]$case.sun;$opts.PopulationChecks=$true}
  else {$opts.Scenario='torch';$opts.Hour=0;$opts.NoBubbles=$true;$opts.ParticleScale=0}
  & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
  $images=@(Get-ChildItem (Join-Path $repo 'Builds/EditorSandbox/divingsim/Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
  if($images.Count -ne $case.count) {throw "Incorrect fresh image count: $($case.name)"}
  $images | Copy-Item -Destination $folder
 }
 $log=Get-Content -LiteralPath (Join-Path $folder 'Runtime.log') -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used') {throw "Render failure: $($case.name)"}
 if(($log -match 'DIVE_SOLAR_VOLUME_STUDY_READY') -ne [bool]$case.volume) {throw 'Incorrect volume mode.'}
 if(($log -match 'DIVE_SPECTRAL_VIEW_READY') -ne [bool]$case.spectral) {throw 'Incorrect camera-path mode.'}
 $case.status='PASS';$done+=$case
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "SOLAR_VOLUME_CASE_PASS $($case.name)"
}
foreach($item in $inputs) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed: $($item.file)"}}
Write-Output "SOLAR_VOLUME_RESULTS=$root"
