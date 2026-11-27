param([switch]$Preview,[switch]$Packaged,[string]$PackageDirectory='Builds/WindowsFootDeposits')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/foot-deposits-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$runtime=Join-Path $repo 'Builds/EditorSandbox/divingsim'
if($Packaged -and $Preview) {throw 'Packaged validation requires all cases.'}
$hostRecord=@{packaged=[bool]$Packaged;preview=[bool]$Preview;source_head=(& git -C $repo rev-parse HEAD)}
if($Packaged) {
 if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
 . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
 $exe=Resolve-DivePackageExecutable $PackageDirectory;$runtime=Join-Path $PackageDirectory 'divingsim'
 $hostRecord.executable=$exe;$hostRecord.sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
 $hostRecord.package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
}
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -File -Recurse)
if(-not $Packaged) {$files+=Get-Item (Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll')}
$files+=Get-Item (Join-Path $repo 'divingsim/Config/DefaultEngine.ini')
foreach($name in @('Tools/SmokeTest.ps1','Tools/TestFootDeposits.ps1')) {$files+=Get-Item (Join-Path $repo $name)}
$hostRecord.files=@($files | Sort-Object FullName | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
$hostRecord | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(@{name='before';entry=0;off=$true},@{name='after';entry=0})
if(-not $Preview) {$cases=@(@{name='shore';entry=0;scenario='shore'})+$cases+@(@{name='murg';entry=1},@{name='walenstadt';entry=2},@{name='stream';entry=1;scenario='stream'},@{name='sediment';entry=0;scenario='sediment'})}
$done=@()
foreach($case in $cases) {
 $folder=Join-Path $root $case.name;New-Item -ItemType Directory -Force $folder | Out-Null
 $opts=@{Scenario='rocks';Entry=$case.entry;Hour=12;Date='2026-06-21';MaxFPS=30;WaterPreview=$true;NoFootDeposits=[bool]$case.off;PopulationChecks=$true;RuntimeLog=(Join-Path $folder 'Runtime.log')}
 if($Packaged) {$opts.Packaged=$true;$opts.PackageDirectory=$PackageDirectory} else {$opts.EditorSandbox=$true}
 if($case.scenario) {$opts.Scenario=$case.scenario;$opts.PopulationChecks=$case.scenario -notin @('shore','sediment')} else {$opts.HabitatView=$true;$opts.HabitatLowView=$true}
 $started=[DateTime]::UtcNow
 & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @opts *> (Join-Path $folder 'Summary.log')
 $log=Get-Content $opts.RuntimeLog -Raw
 if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Runtime failure: $($case.name)"}
 if($opts.PopulationChecks -and -not $opts.NoFootDeposits -and $log -notmatch 'DIVE_FOOT_DEPOSIT_PASS') {throw 'Missing deposited-surface contact check.'}
 $enabled=if($case.off){0}else{1};if($log -notmatch "DIVE_FOOT_DEPOSITS_READY enabled=$enabled maximum_cm=70") {throw 'Wrong deposited surface state.'}
 $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' | Where-Object {$_.LastWriteTimeUtc -ge $started})
 $expected=if($case.scenario -eq 'shore'){9}elseif($case.scenario -eq 'stream'){2}elseif($case.scenario -eq 'sediment'){7}else{3}
 if($images.Count -ne $expected) {throw "Missing captures: $($case.name)"}
 $images | Copy-Item -Destination $folder
 $done+=@{name=$case.name;options=$opts;images=@($images.Name);status='PASS'}
 $done | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
 Write-Output "PASS $($case.name)"
}
$hostRecord=Get-Content (Join-Path $root 'Host.json') -Raw | ConvertFrom-Json
foreach($item in $hostRecord.files) {if((Get-FileHash -LiteralPath (Join-Path $repo $item.file)).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed during test: $($item.file)"}}
Write-Output "FOOT_DEPOSITS=$root"
