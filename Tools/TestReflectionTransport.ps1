param([ValidateRange(180,1800)][int]$TimeoutSeconds=1200,
 [ValidateSet('waterline','response','consistency','seam','sweep','shadows')][string]$Suite='waterline',[switch]$Unified,
 [ValidateSet('full','unshadowed','cached','vsm','vsm-fallback','vsm-coverage','vsm-path')][string]$Profile='full',
 [string]$CaseName='')
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
$prepareOptions=@('--profile',$Profile);if($Unified) {$prepareOptions+='--unified'}
& (Join-Path $repo 'Tools/.geodata-env/Scripts/python.exe') (Join-Path $repo 'Tools/PrepareReflectionTransport.py') --engine $engine --project $runtime @prepareOptions
if($LASTEXITCODE -ne 0) {throw 'Shader preparation failed.'}
$root=Join-Path $repo ('artifacts/reflection-transport-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
Copy-Item -LiteralPath (Join-Path $runtime 'Intermediate/DiveReflectionTransportStudy/Manifest.json') -Destination (Join-Path $root 'ShaderManifest.json')
$files+=@(Get-ChildItem (Join-Path $runtime 'Intermediate/DiveReflectionTransportStudy') -File -Recurse)
$files+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -File -Filter '*.uasset')
foreach($name in @('Tools/SmokeTest.ps1','Tools/TestReflectionTransport.ps1','Tools/PrepareReflectionTransport.py',
 'Tools/build_water_interface_content.py',
 'Builds/SourceEditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveSim.dll',
 'Builds/UE584/Engine/Binaries/Win64/UnrealEditor-Renderer.dll',
 'Builds/SourceEditorSandbox/divingsim/Binaries/Win64/UnrealEditor-DiveRendering.dll')) {$files+=Get-Item (Join-Path $repo $name)}
$records=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
@{source_head=(& git -C $repo rev-parse HEAD);files=$records;started_utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=if($Suite -eq 'waterline') {@(@{name='baseline';control=$true},@{name='transport-no-screens';screens=$true},@{name='transport-hybrid'})}
elseif($Suite -eq 'consistency') {@(@{name='near-hybrid';near=$true},@{name='near-no-screens';near=$true;screens=$true})}
elseif($Suite -eq 'seam') {@(@{name='normal'},@{name='no-surface';surface=$true},@{name='no-post-material';post=$true},@{name='no-direct';direct=$true})}
elseif($Suite -eq 'shadows') {@(
 @{name='sun-shadow';probe='sun';near=$true;screens=$true},
 @{name='moon-shadow';probe='moon';near=$true;screens=$true},
 @{name='missing-maps';probe='missing';near=$true;screens=$true},
 @{name='ambiguous-light';probe='ambiguous';near=$true;screens=$true})}
elseif($Suite -eq 'sweep') {@(
 @{name='weesen-strandbad';sweep=$true;pose=@(329004.7802942805,172781.7364295479,180,0)},
 @{name='murg-west';sweep=$true;pose=@(53031.55998047441,830331.2463976908,0,0)},
 @{name='mols-lediwracks';sweep=$true;pose=@(95502.19710376114,1469706.6948947497,0,0)})}
else {@(@{name='baseline-no-screens';control=$true;screens=$true},
 @{name='clear';visibility=28;screens=$true},@{name='turbid';visibility=4;screens=$true},
 @{name='near-baseline';control=$true;near=$true},@{name='near-transport';near=$true},
 @{name='night-off';night=$true;screens=$true},@{name='night-lamp-baseline';night=$true;lamp=$true;control=$true;screens=$true},
 @{name='night-lamp-transport';night=$true;lamp=$true;screens=$true})}
if($CaseName) {
 $cases=@($cases | Where-Object {$_.name -eq $CaseName})
 if($cases.Count -ne 1) {throw 'The selected case is not part of this suite.'}
}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{EditorSandbox=$true;SandboxName='SourceEditorSandbox';EngineRoot=$engine;Scenario='rocks';Entry=0;
  WaterView=$true;WaterStart=0;WaterEnd=0;WaterlinePose=@(329185.873,172781.736,180,0);WaterPreview=$true;
  Hour=12;Date='2026-06-21';MaxFPS=30;TimeoutSeconds=$TimeoutSeconds;NoReflectionScreenTraces=[bool]$case.screens;
  ReflectionTransportStudy=$true;ReflectionTransportControl=[bool]$case.control;LakeFootprint=$true;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 $opts.ReflectionUnified=[bool]$Unified
 $opts.ReflectionProfile=$Profile
 if($case.surface) {$opts.WaterlineNoSurface=$true}
 if($case.post) {$opts.DisablePostProcessMaterials=$true}
 if($case.direct) {$opts.LightIsolation='direct'}
 if($case.sweep) {$opts.WaterlineSweep=$true;$opts.WaterlinePose=$case.pose}
 if($case.near) {$opts.Remove('WaterlinePose')}
 if($case.probe) {
  $opts.ReflectionShadowProbe=$case.probe
  if($case.probe -eq 'moon') {$opts.Date='2026-03-29';$opts.Hour=1.98333333333333}
 }
 if($case.visibility) {$opts.Visibility=$case.visibility}
 if($case.night) {$opts.Hour=1.5}
 if($case.lamp) {$opts.WaterInspectionLamp=$true}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Rendering failure: $($case.name)"}
 $views=[regex]::Matches($log,'DIVE_WATERLINE_VIEW view=(\d) surface_cm=([-\d.]+) eye_height_cm=([-\d.]+) clock=([-\d.]+)')
 if(-not $case.near -and -not $case.sweep -and $views.Count -ne 3) {throw 'Missing waterline views.'}
 foreach($view in $views) {
  $index=[int]$view.Groups[1].Value
  $height=[double]::Parse($view.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
  if([math]::Abs($height-@(-20,-2,20)[$index]) -gt .001) {throw 'Unexpected camera immersion.'}
 }
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 if($case.sweep) {
  $views=[regex]::Matches($log,'DIVE_WATERLINE_SWEEP_FRAME index=(\d+) seconds=([-\d.]+) eye_height_cm=([-\d.]+) wave_seconds=([-\d.]+) eye=([^\r\n]+)')
  if($views.Count -ne 201 -or $images.Count -ne 201) {throw 'Missing sweep frames.'}
  $previous=-1.;$highest=-100.;$lowest=100.
  for($i=0;$i -lt $views.Count;$i++) {
   $v=$views[$i];$t=[double]::Parse($v.Groups[2].Value,[Globalization.CultureInfo]::InvariantCulture)
   $h=[double]::Parse($v.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
   if([int]$v.Groups[1].Value -ne $i -or $t -le $previous -or ($i -gt 0 -and $t-$previous -gt .35) -or [math]::Abs($h) -gt 20.01) {throw 'Invalid or discontinuous sweep.'}
   $previous=$t;$highest=[math]::Max($highest,$h);$lowest=[math]::Min($lowest,$h)
  }
  if($highest -lt 19.9 -or $lowest -gt -19.9 -or $previous -lt 20) {throw 'Incomplete up/down crossing.'}
 } elseif($images.Count -ne 3) {throw 'Missing fresh captures.'}
 $images | Copy-Item -Destination $folder
 if($case.probe) {
  $views=[regex]::Matches($log,'DIVE_REFLECTION_SHADOW_SETUP kind=(\w+) view=(\d) blocked=(\d) source_lux=([\d.]+) source_up=([\d.]+) map_enabled=(-?\d+) duplicate=(\d) eye=([^\r\n]+)')
  if($views.Count -ne 3) {throw 'Missing physical shadow fixture states.'}
  for($i=0;$i -lt 3;$i++) {
   $v=$views[$i]
   if($v.Groups[1].Value -ne $case.probe -or [int]$v.Groups[2].Value -ne $i -or [int]$v.Groups[3].Value -ne @(0,1,0)[$i] -or
      [int]$v.Groups[6].Value -ne $(if($case.probe -eq 'missing'){0}else{1}) -or
      [int]$v.Groups[7].Value -ne $(if($case.probe -eq 'ambiguous'){1}else{0})) {throw 'Unexpected shadow fixture state.'}
  }
  if($views[0].Groups[8].Value -ne $views[1].Groups[8].Value -or $views[0].Groups[8].Value -ne $views[2].Groups[8].Value) {throw 'Shadow camera or caster position moved.'}
 }
 $done+=@{name=$case.name;status='PASS';options=$opts;captures=@($views | ForEach-Object {$_.Value});images=@($images.Name)}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($record in $records) {if((Get-FileHash -LiteralPath (Join-Path $repo $record.file)).Hash.ToLowerInvariant() -ne $record.sha256) {throw "Input changed: $($record.file)"}}
Write-Output "REFLECTION_TRANSPORT=$root"
