param([switch]$Preview,[switch]$Packaged,[string]$PackageDirectory='Builds/WindowsExposure')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/exposure-start-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
$hostRecord=@{packaged=[bool]$Packaged;preview=[bool]$Preview;source_head=(& git -C $repo rev-parse HEAD)}
if($Packaged) {
 if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
 . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
 $exe=Resolve-DivePackageExecutable $PackageDirectory;$runtime=Join-Path $PackageDirectory 'divingsim'
 $hostRecord.executable=$exe;$hostRecord.sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
 $hostRecord.package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse)
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
foreach($name in @('Tools/SmokeTest.ps1','Tools/TestExposureStart.ps1','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $name)}
$hostRecord.files=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='day-before';scenario='dive';hour=12;legacy=$true;count=3},@{name='day-after';scenario='dive';hour=12;count=3})
if(-not $Preview) {$cases+=@(
 @{name='night-before';scenario='torch';hour=1.5;legacy=$true;count=3},
 @{name='night-after';scenario='torch';hour=1.5;count=3},
 @{name='night-dark';scenario='torch';hour=1.5;off=$true;count=3},
 @{name='shore';scenario='shore';hour=12;count=9},
 @{name='sediment-before';scenario='sediment';hour=12;legacy=$true;count=7},
 @{name='sediment-after';scenario='sediment';hour=12;count=7})}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{Scenario=$case.scenario;Entry=0;Hour=$case.hour;Date='2026-06-21';MaxFPS=30;WaterPreview=$true;
  LegacyExposureStart=[bool]$case.legacy;ExposureProbe=($case.scenario -notin @('shore','sediment'));LampOff=[bool]$case.off;
  RuntimeLog=(Join-Path $folder 'Runtime.log')}
 if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Runtime failure: $($case.name)"}
 $exposureMatches=[regex]::Matches($log,'DIVE_EXPOSURE_START legacy=(\d) active=1 previous=([-\d.]+) target=([-\d.]+) actual=([-\d.]+) depth=([-\d.]+) eye=([^\r\n]+)')
 if(-not $exposureMatches.Count) {throw 'Missing initial exposure evidence.'}
 foreach($match in $exposureMatches) {
  $target=[double]::Parse($match.Groups[3].Value,[Globalization.CultureInfo]::InvariantCulture)
  $actual=[double]::Parse($match.Groups[4].Value,[Globalization.CultureInfo]::InvariantCulture)
  if($case.legacy) {if([math]::Abs($actual-$target) -lt .1) {throw 'Legacy control did not reproduce initial adaptation.'}}
  elseif([math]::Abs($actual-$target) -gt .00001) {throw 'Initial camera exposure does not match the new scene.'}
 }
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 if($images.Count -ne $case.count) {throw "Fresh capture count mismatch: $($case.name) $($images.Count)/$($case.count)"}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;options=$opts;images=@($images.Name);exposure=@($exposureMatches | ForEach-Object {$_.Value});status='PASS'}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
foreach($item in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed during test: $($item.file)"}}
Write-Output "EXPOSURE_START=$root"
