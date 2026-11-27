param(
 [ValidateCount(4,4)][double[]]$Pose=@(329185.873,172781.736,180,0),
 [ValidateSet('standard','volume','light','shadow','geometry','sky','composition','depth','camera','correction')][string]$Study='standard',
 [string]$PackageDirectory=''
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if($PackageDirectory) {
 if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
 $runtime=Join-Path $PackageDirectory 'divingsim'
 . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
 $exe=Resolve-DivePackageExecutable $PackageDirectory
}
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/waterline-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse)
if(-not $PackageDirectory) {foreach($file in $files) {
 $copy=Join-Path $runtime $file.FullName.Substring((Join-Path $repo 'divingsim').Length+1)
 if((Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $copy).Hash) {throw "Stale sandbox: $copy"}
}}
foreach($name in @('Tools/SmokeTest.ps1','Tools/TestWaterline.ps1','divingsim/Config/DefaultEngine.ini',
 'Tools/build_optics_content.py','Tools/Shaders/UnderwaterView.ush')) {$files+=Get-Item (Join-Path $repo $name)}
if($PackageDirectory) {
 $files+=Get-Item $exe
 $files+=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File)
} else {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$files+=@(Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -File -Filter '*.uasset')
$records=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
@{source_head=(& git -C $repo rev-parse HEAD);native_editor=(-not $PackageDirectory);package=$PackageDirectory;study=$Study;pose=$Pose;files=$records;started_utc=[DateTime]::UtcNow.ToString('o')} |
 ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=if($Study -eq 'correction') {@(@{name='normal'},@{name='no-post-process-materials';post=$true},@{name='fog-fallback';volume=$true})}
 elseif($Study -eq 'camera') {@(@{name='no-post-process-materials';post=$true})}
 elseif($Study -eq 'depth') {@(@{name='no-fog-depth-culling';depth=$true},@{name='no-fog-history';history=$true})}
 elseif($Study -eq 'composition') {@(@{name='separate-fog';composition=$true},@{name='no-fog-cutoff';cutoff=$true},@{name='no-sky-layers';hide='deep,proxy,stars,planets,moon'},@{name='no-light-shafts';shafts=$true})}
 elseif($Study -eq 'sky') {@(@{name='no-deep-sky';hide='deep'},@{name='no-velocity-proxy';hide='proxy'})}
 elseif($Study -eq 'geometry') {@(@{name='geometry-depth';surface=$true;buffer='SceneDepth'},@{name='geometry-normals';surface=$true;buffer='WorldNormal'})}
 elseif($Study -eq 'shadow') {@(@{name='normal'},@{name='no-water-density';density=$true},@{name='no-volume-shadow';volumeShadow=$true})}
 elseif($Study -eq 'light') {@(@{name='no-light-function';function=$true},@{name='no-water-density';density=$true})}
 elseif($Study -eq 'volume') {@(@{name='normal'},@{name='no-shadows';shadows=$true},@{name='near-volume';distance=5600})}
 else {@(@{name='normal'},@{name='no-screen-traces';screens=$true},@{name='no-volume';volume=$true},@{name='no-surface';surface=$true})}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{EditorSandbox=$true;Scenario='rocks';Entry=0;WaterView=$true;WaterStart=0;WaterEnd=0;WaterlinePose=$Pose;
  WaterPreview=$true;Hour=12;Date='2026-06-21';MaxFPS=30;NoReflectionScreenTraces=[bool]$case.screens;
  DisableVolumetrics=[bool]$case.volume;WaterlineNoSurface=[bool]$case.surface;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 if($PackageDirectory) {$opts.EditorSandbox=$false;$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory}
 $opts.DisableShadows=[bool]$case.shadows;$opts.WaterlineFogDistance=[double]$case.distance
 if($case.function) {$opts.LightIsolation='function'}
 if($case.density) {$opts.WaterVolumeGain=0}
 if($case.volumeShadow) {$opts.WaterlineNoVolumeShadow=$true}
 if($case.buffer) {$opts.BufferView=$case.buffer}
 if($case.hide) {$opts.HideSky=$case.hide}
 if($case.composition) {$opts.FogComposition=1}
 if($case.cutoff) {$opts.WaterlineNoFogCutoff=$true}
 if($case.shafts) {$opts.DisableLightShafts=$true}
 if($case.depth) {$opts.DisableFogDepthCulling=$true}
 if($case.history) {$opts.DisableFogHistory=$true}
 if($case.post) {$opts.DisablePostProcessMaterials=$true}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Rendering failure: $($case.name)"}
 $captures=[regex]::Matches($log,'DIVE_WATERLINE_VIEW view=(\d) surface_cm=([-\d.]+) eye_height_cm=([-\d.]+) clock=([-\d.]+)')
 if($captures.Count -ne 3) {throw 'Missing waterline poses.'}
 foreach($match in $captures) {
  $view=[int]$match.Groups[1].Value
  $height=[double]::Parse($match.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
  if([math]::Abs($height-@(-20,-2,20)[$view]) -gt .001) {throw 'Camera is not at the requested wave-relative height.'}
 }
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 if($images.Count -ne 3) {throw "Missing fresh images: $($case.name)"}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;status='PASS';options=$opts;captures=@($captures | ForEach-Object {$_.Value});images=@($images.Name)}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($item in $records) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed: $($item.file)"}}
Write-Output "WATERLINE=$root"
