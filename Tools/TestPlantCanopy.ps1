param([string]$PackageDirectory='Builds/WindowsPlantCanopy')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/plant-canopy-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;package_containers=$containers;
    source_head=(& git -C $repo rev-parse HEAD);packaged=$true;started_utc=[DateTime]::UtcNow.ToString('o')} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='forms';entry=0;forms=$true},@{name='entry0';entry=0},
    @{name='entry1';entry=1},@{name='entry2';entry=2},@{name='stream';entry=0;stream=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.stream) {$options.Scenario='stream';$options.PopulationChecks=$true}
    else {
        $options.Scenario='rocks'
        if($case.forms) {$options.PlantView=$true}
        else {$options.HabitatView=$true;$options.PopulationChecks=$true;$options.Hour=12}
    }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_SMOKE_PASS' -or $log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Plant check failed: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    $expected=if($case.forms) {12} elseif($case.stream) {0} else {3}
    if($images.Count -lt 1 -or ($expected -and $images.Count -ne $expected)) {throw 'Missing fresh plant captures'}
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;entry=$case.entry;forms=[bool]$case.forms;stream=[bool]$case.stream;status='PASS';screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $result
Write-Output "PLANT_CANOPY_VALIDATION=$result"
