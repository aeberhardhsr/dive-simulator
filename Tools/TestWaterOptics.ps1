param([string]$PackageDirectory='Builds/WindowsWaterOptics')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/water-optics-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;
    package_containers=$containers;source_head=(& git -C $repo rev-parse HEAD);packaged=$true} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{name='look-day';scenario='rocks';entry=0;hour=13;comparison=$true},
    @{name='look-night';scenario='rocks';entry=0;hour=0;comparison=$true},
    @{name='surface0';scenario='surface';entry=0;hour=13},
    @{name='surface1';scenario='surface';entry=1;hour=13},
    @{name='surface2';scenario='surface';entry=2;hour=13},
    @{name='storm';scenario='surface-storm';entry=0;hour=13},
    @{name='murky-torch';scenario='torch';entry=1;hour=0;visibility=4})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;
        Hour=$case.hour;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.comparison) {$options.WaterView=$true;$options.WaterEnd=1}
    if($case.visibility) {$options.Visibility=$case.visibility}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Runtime failure: $folder"}
    $markers=[regex]::Matches($log,'DIVE_ENTRY_MARKER entry=(\d+).*?clearance_cm=([\d.]+) bed_cm=([-\d.]+)')
    if($markers.Count -ne 3) {throw 'Missing entry marker checks'}
    foreach($marker in $markers) {
        if([double]::Parse($marker.Groups[2].Value,[Globalization.CultureInfo]::InvariantCulture) -lt 150) {throw 'Obstructed entry marker'}
    }
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($case.comparison){6}elseif($case.scenario -eq 'torch'){2}else{3}
    if($images.Count -ne $expected) {throw "Expected $expected fresh captures; got $($images.Count): $folder"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;hour=$case.hour;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "WATER_OPTICS_VALIDATION=$result"
