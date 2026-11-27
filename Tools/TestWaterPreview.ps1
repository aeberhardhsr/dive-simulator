param([string]$PackageDirectory='Builds/WindowsWaterPreview')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$runtime=Join-Path $PackageDirectory 'divingsim'
$root=Join-Path $repo ('artifacts/water-preview-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -Recurse -File)
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -Filter '*.uasset' -File
foreach($name in @('Tools/TestWaterPreview.ps1','Tools/SmokeTest.ps1','Tools/WaterPreviewLauncher.cmd','divingsim/Config/DefaultEngine.ini')) {$files+=Get-Item (Join-Path $repo $name)}
$launcher=Join-Path $PackageDirectory 'Start-WaterPreview.cmd'
if((Get-FileHash $launcher).Hash -ne (Get-FileHash (Join-Path $PSScriptRoot 'WaterPreviewLauncher.cmd')).Hash) {throw 'Preview launcher differs from its source.'}
$inputs=@($files | Sort-Object FullName -Unique | ForEach-Object {@{file=$_.FullName.Substring($repo.Length+1).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
@{packaged=$true;water_preview=$true;source_head=(& git -C $repo rev-parse HEAD);started_utc=[DateTime]::UtcNow.ToString('o');
    executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant();files=$inputs;
    package_containers=@(Get-ChildItem (Join-Path $runtime 'Content/Paks') -File | Sort-Object Name | ForEach-Object {@{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}})
} | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='surface';surface=$true;count=3},
    @{name='weesen';entry=0;count=3},
    @{name='murg';entry=1;count=3},
    @{name='ledi';entry=2;count=3},
    @{name='storm';entry=1;scenario='surface-storm';count=3},
    @{name='dark-on';scenario='torch';night=$true;count=2},
    @{name='dark-off';scenario='torch';night=$true;off=$true;count=2},
    @{name='fog-fallback';entry=2;noFog=$true;count=3},
    @{name='raster';surface=$true;raster=$true;count=3},
    @{name='software';surface=$true;noRT=$true;count=3},
    @{name='stream';entry=1;scenario='stream';count=2},
    @{name='shore';scenario='shore';count=9},
    @{name='sediment';scenario='sediment';count=7}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;WaterPreview=$true;Entry=[int]$case.entry;Scenario='rocks';
        Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log');
        DisableVolumetrics=[bool]$case.noFog;DisableNanite=[bool]$case.raster;DisableRayTracing=[bool]$case.noRT}
    if($case.surface) {$options.WaterView=$true;$options.WaterStart=1;$options.WaterEnd=1}
    elseif($case.scenario) {$options.Scenario=$case.scenario;$options.PopulationChecks=$case.scenario -eq 'stream'}
    else {$options.HabitatView=$true;$options.PopulationChecks=$true}
    if($case.night) {$options.Hour=1.5;$options.LampOff=[bool]$case.off}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Failed: $($case.name)"}
    $hardware=if($case.noRT){0}else{1}
    $native=if($case.noFog){0}else{1}
    if($log -notmatch "DIVE_WATER_PREVIEW_READY smoke=1 hardware=$hardware bounded=1 interface=1 lighting_mode=2") {throw "Unexpected preview path: $($case.name)"}
    if($log -notmatch "DIVE_BOUNDED_WATER_PASS native=$native" -or $log -notmatch 'DIVE_WATER_INTERFACE_PASS[^\r\n]*ray_surface=0') {throw "Missing water validation: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $runtime 'Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $case.count) {throw "Fresh capture count: $($case.name) $($images.Count)/$($case.count)"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;status='PASS';hardware=$hardware;native_scattering=$native;screenshots=@($images | ForEach-Object {@{file=$_.Name;sha256=(Get-FileHash $_.FullName).Hash.ToLowerInvariant()}});log_sha256=(Get-FileHash $options.RuntimeLog).Hash.ToLowerInvariant()}
    $completed | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
foreach($input in $inputs) {if((Get-FileHash -LiteralPath (Join-Path $repo $input.file)).Hash.ToLowerInvariant() -ne $input.sha256) {throw "Input changed during tests: $($input.file)"}}
Write-Output "WATER_PREVIEW=$root"
