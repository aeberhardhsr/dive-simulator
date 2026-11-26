param([string]$PackageDirectory='Builds/WindowsPopulationCache',[switch]$RequireStaged)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$prefix=if($RequireStaged){'population-staged'}else{'population-cache'}
$result=Join-Path $repo ("artifacts/$prefix-validation-"+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $result | Out-Null
$containers=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;
    package_containers=$containers;source_head=(& git -C $repo rev-parse HEAD);packaged=$true} |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@()
foreach($entry in 0..2) {
    $folder=Join-Path $result "entry$entry"
    New-Item -ItemType Directory -Force $folder | Out-Null
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') -Packaged -PackageDirectory $PackageDirectory -Scenario rocks -HabitatView -PopulationChecks -Entry $entry -Hour 12 -MaxFPS 30 -RuntimeLog (Join-Path $folder 'Runtime.log') *> (Join-Path $folder 'Summary.log')
    $log=Get-Content (Join-Path $folder 'Runtime.log') -Raw
    if($log -notmatch 'DIVE_ROCK_LAYOUT_PASS layouts=25 max=25 fresh_equal=1' -or $log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Cache check failed: $entry"}
    if($RequireStaged -and ($log -notmatch 'DIVE_POPULATION_STAGED_PASS cells=7,8,9' -or $log -notmatch 'DIVE_POPULATION_NEAR_TELEPORT_PASS immediate=1')) {throw 'Missing staged/nearby-teleport checks'}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter 'Habitat_*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne 3) {throw 'Missing comparison captures'}
    $images | Copy-Item -Destination $folder
    $cases+=@{entry=$entry;status='PASS';screenshots=3}
    $cases | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS entry$entry $folder"
}
& (Join-Path $PSScriptRoot 'TestPackageLauncher.ps1') -PackageDirectory $PackageDirectory -OutputDirectory $result
Write-Output "POPULATION_CACHE_VALIDATION=$result"
