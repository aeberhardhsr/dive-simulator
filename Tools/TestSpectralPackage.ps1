param([string]$PackageDirectory='Builds/WindowsSpectralStudy')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$runtime=Join-Path $PackageDirectory 'divingsim'
$root=Join-Path $repo ('artifacts/spectral-package-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source'),(Join-Path $repo 'divingsim/Shaders'),(Join-Path $repo 'divingsim/Config') -Recurse -File)
$files+=Get-Item (Join-Path $repo 'divingsim/divingsim.uproject'),(Join-Path $repo 'divingsim/Content/Diving/Materials/M_WaterUnder.uasset'),(Join-Path $PSScriptRoot 'SmokeTest.ps1'),$PSCommandPath
$hostRecord=@{source_head=(& git -C $repo rev-parse HEAD);executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();
 package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}});
 files=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})}
$hostRecord | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
foreach($item in $hostRecord.files) {
 $snapshot=Join-Path (Join-Path $root 'inputs') $item.file
 New-Item -ItemType Directory -Force (Split-Path $snapshot -Parent) | Out-Null
 Copy-Item -LiteralPath (Join-Path $repo $item.file) -Destination $snapshot
}
$cases=@(@{name='before';off=$true},@{name='zero';zero=$true},@{name='after'},@{name='night';night=$true},@{name='fallback-before';off=$true;fallback=$true},@{name='fallback-after';fallback=$true},@{name='shore';shore=$true},@{name='torch-before';off=$true;torch=$true},@{name='torch-after';torch=$true})
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario='rocks';Entry=0;Hour=12;Date='2026-06-21';MaxFPS=30;WaterPreview=$true;HabitatView=$true;HabitatLowView=$true;SpectralView=(-not [bool]$case.off);AbsorptionGain=1;DisableVolumetrics=[bool]$case.fallback;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 if($case.zero){$opts.AbsorptionGain=0}
 if($case.night){$opts.Hour=0}
 if($case.shore){$opts.Scenario='shore';$opts.HabitatView=$false;$opts.HabitatLowView=$false}
 if($case.torch){$opts.Scenario='torch';$opts.Entry=1;$opts.Hour=0;$opts.HabitatView=$false;$opts.HabitatLowView=$false}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile|Default Material will be used') {throw "Runtime failure: $($case.name)"}
 if($case.fallback) {
  # ExecCmds and render-thread startup can overlap. An early READY marker is
  # not evidence of an active pass at capture time; ReportSpectralView also
  # requires paired fallback images to remain within the null-control bound.
  $capture=$log.IndexOf('DIVE_HABITAT_VIEW')
  if($capture -lt 0 -or $log.LastIndexOf('DIVE_SPECTRAL_VIEW_READY') -ge $capture -or
     $log -notmatch 'DIVE_SCATTER_MODEL[^\r\n]*native=0 requested=1'){throw 'Fallback state was not established before inspection.'}
 } elseif(($log -match 'DIVE_SPECTRAL_VIEW_READY') -ne (-not [bool]$case.off)){throw "Wrong spectral pass state: $($case.name)"}
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 $count=if($case.shore){9}elseif($case.torch){2}else{3}
 if($images.Count -ne $count){throw "Wrong capture count: $($case.name)"}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;options=$opts;images=@($images.Name);status='PASS'}
 $done | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($item in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256){throw "Input changed during test: $($item.file)"}}
if((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne $hostRecord.sha256){throw 'Executable changed during test.'}
Write-Output "SPECTRAL_PACKAGE=$root"
