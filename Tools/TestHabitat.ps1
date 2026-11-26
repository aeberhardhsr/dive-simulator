param([string]$PackageDirectory='Builds/WindowsHabitat')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/habitat-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;
    package_containers=$containers;source_head=(& git -C $repo rev-parse HEAD);packaged=$true} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@()
foreach($entry in 0..2) {
    $cases+=@{name="look$entry";scenario='rocks';entry=$entry;HabitatView=$true}
    $cases+=@{name="stream$entry";scenario='stream';entry=$entry;PopulationChecks=$true}
    $cases+=@{name="shore$entry";scenario='shore';entry=$entry}
}
$cases+=@{name='look-raster';scenario='rocks';entry=0;HabitatView=$true;DisableNanite=$true}
$cases+=@{name='sediment';scenario='sediment';entry=0;NoBubbles=$true}
$cases+=@{name='navigation';scenario='navigation';entry=2;NavShore=$true}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;
        Hour=12;Date='2026-06-21';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in @('HabitatView','DisableNanite','PopulationChecks','NoBubbles','NavShore')) {
        if($case.ContainsKey($key)) {$options[$key]=$case[$key]}
    }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Runtime failure: $folder"}
    if($case.PopulationChecks -and $log -notmatch 'DIVE_HABITAT_PASS') {throw 'Missing cluster ownership/clearance check'}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if(-not $images.Count) {throw "Missing fresh captures: $folder"}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $result
Write-Output "HABITAT_VALIDATION=$result"
