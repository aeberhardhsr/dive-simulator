param([ValidateRange(180,1800)][int]$TimeoutSeconds=1200,
 [ValidateSet('waterline','response')][string]$Suite='waterline')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$engine=Join-Path $repo 'Builds/UE584'
$runtime=Join-Path $repo 'Builds/SourceEditorSandbox/divingsim'
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source'),(Join-Path $repo 'divingsim/Shaders'),(Join-Path $repo 'divingsim/Config') -File -Recurse)
foreach($file in $files) {
 $copy=Join-Path $runtime $file.FullName.Substring((Join-Path $repo 'divingsim').Length+1)
 if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $copy).Hash) {throw "Stale source sandbox: $copy"}
}
& (Join-Path $repo 'Tools/.geodata-env/Scripts/python.exe') (Join-Path $repo 'Tools/PrepareReflectionTransport.py') --engine $engine --project $runtime
if($LASTEXITCODE -ne 0) {throw 'Shader preparation failed.'}
$root=Join-Path $repo ('artifacts/reflection-transport-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
Copy-Item -LiteralPath (Join-Path $runtime 'Intermediate/DiveReflectionTransportStudy/Manifest.json') -Destination (Join-Path $root 'ShaderManifest.json')
$files+=@(Get-ChildItem (Join-Path $runtime 'Intermediate/DiveReflectionTransportStudy') -File -Recurse)
foreach($name in @('Tools/SmokeTest.ps1','Tools/TestReflectionTransport.ps1','Tools/PrepareReflectionTransport.py',
 'Builds/SourceEditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll',
 'Builds/SourceEditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll')) {$files+=Get-Item (Join-Path $repo $name)}
$records=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
@{source_head=(& git -C $repo rev-parse HEAD);files=$records;started_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=if($Suite -eq 'waterline') {@(@{name='baseline';control=$true},@{name='transport-no-screens';screens=$true},@{name='transport-hybrid'})}
else {@(@{name='baseline-no-screens';control=$true;screens=$true},
 @{name='clear';visibility=28;screens=$true},@{name='turbid';visibility=4;screens=$true},
 @{name='near-baseline';control=$true;near=$true},@{name='near-transport';near=$true},
 @{name='night-off';night=$true;screens=$true},@{name='night-lamp-baseline';night=$true;lamp=$true;control=$true;screens=$true},
 @{name='night-lamp-transport';night=$true;lamp=$true;screens=$true})}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{EditorSandbox=$true;SandboxName='SourceEditorSandbox';EngineRoot=$engine;Scenario='rocks';Entry=0;
  WaterView=$true;WaterStart=0;WaterEnd=0;WaterlinePose=@(329185.873,172781.736,180,0);WaterPreview=$true;
  Hour=12;Date='2026-06-21';MaxFPS=30;TimeoutSeconds=$TimeoutSeconds;NoReflectionScreenTraces=[bool]$case.screens;
  ReflectionTransportStudy=$true;ReflectionTransportControl=[bool]$case.control;LakeFootprint=$true;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 if($case.near) {$opts.Remove('WaterlinePose')}
 if($case.visibility) {$opts.Visibility=$case.visibility}
 if($case.night) {$opts.Hour=1.5}
 if($case.lamp) {$opts.WaterInspectionLamp=$true}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Rendering failure: $($case.name)"}
 $views=[regex]::Matches($log,'DIVE_WATERLINE_VIEW view=(\d) surface_cm=([-\d.]+) eye_height_cm=([-\d.]+) clock=([-\d.]+)')
 if(-not $case.near -and $views.Count -ne 3) {throw 'Missing waterline views.'}
 foreach($view in $views) {
  $index=[int]$view.Groups[1].Value
  $height=[double]::Parse($view.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
  if([math]::Abs($height-@(-20,-2,20)[$index]) -gt .001) {throw 'Unexpected camera immersion.'}
 }
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 if($images.Count -ne 3) {throw 'Missing fresh captures.'}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;status='PASS';options=$opts;captures=@($views | ForEach-Object {$_.Value});images=@($images.Name)}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($record in $records) {if((Get-FileHash -LiteralPath (Join-Path $repo $record.file)).Hash.ToLowerInvariant() -ne $record.sha256) {throw "Input changed: $($record.file)"}}
Write-Output "REFLECTION_TRANSPORT=$root"
