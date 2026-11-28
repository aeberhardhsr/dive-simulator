param([string]$PackageDirectory='Builds/WindowsViewPath')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/view-path-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;package_containers=$containers;
    source_head=(& git -C $repo rev-parse HEAD);packaged=$true;started_utc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='habitat0';entry=0;habitat=$true},@{name='habitat1';entry=1;habitat=$true},@{name='habitat2';entry=2;habitat=$true},
    @{name='murky';entry=2;habitat=$true;visibility=4},@{name='clear';entry=2;habitat=$true;visibility=40},
    @{name='look-day';entry=0;water=$true},@{name='look-night';entry=0;water=$true;night=$true},
    @{name='surface0';entry=0;surface=$true},@{name='surface1';entry=1;surface=$true},@{name='surface2';entry=2;surface=$true},
    @{name='storm';entry=0;storm=$true},@{name='night-on';entry=0;night=$true},@{name='night-off';entry=0;night=$true;off=$true},
    @{name='dark-on';entry=0;night=$true;hour=1.5},@{name='dark-off';entry=0;night=$true;hour=1.5;off=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Entry=$case.entry;MaxFPS=30;
        Hour=12;Date='2026-06-21';RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.habitat) {$options.Scenario='rocks';$options.HabitatView=$true}
    elseif($case.water) {$options.Scenario='rocks';$options.WaterView=$true;$options.WaterStart=1;$options.WaterEnd=1}
    elseif($case.surface) {$options.Scenario='surface'}
    elseif($case.storm) {$options.Scenario='surface-storm'}
    else {$options.Scenario='torch'}
    if($case.night) {$options.Hour=0}
    if($case.hour) {$options.Hour=$case.hour}
    if($case.off) {$options.LampOff=$true}
    if($case.visibility) {$options.Visibility=$case.visibility}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_SMOKE_PASS' -or $log -notmatch 'DIVE_VIEW_PATH_PASS' -or $log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "View path check failed: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($options.Scenario -eq 'torch') {2} else {3}
    if($images.Count -ne $expected) {throw "Missing fresh captures for $($case.name): $($images.Count)/$expected"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;entry=$case.entry;scenario=$options.Scenario;hour=$options.Hour;
        visibility=$(if($case.visibility){$case.visibility}else{14});status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $result
& (Join-Path $PSScriptRoot 'TestCaustics.ps1') -PackageDirectory $PackageDirectory -Case day |
    Set-Content (Join-Path $result 'CausticsRun.txt') -Encoding utf8
Write-Output "VIEW_PATH_VALIDATION=$result"
