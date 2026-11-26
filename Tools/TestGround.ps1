param([string]$PackageDirectory='Builds/WindowsGround')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/ground-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$saved=Join-Path $PackageDirectory 'divingsim/Saved/Screenshots'
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;packaged=$true;package_containers=$containers} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@()
foreach($entry in 0..2) {
    $cases+=@{name="entry-$entry-land";scenario='shore-preview';entry=$entry;land=$true}
    $cases+=@{name="entry-$entry-ground";scenario='shore-preview';entry=$entry;land=$true;ground=$true}
    $cases+=@{name="entry-$entry-shore";scenario='shore';entry=$entry}
    $cases+=@{name="entry-$entry-coast";scenario='coast';entry=$entry}
}
$cases+=@{name='weesen-before-land';scenario='shore-preview';entry=0;land=$true;off=$true}
$cases+=@{name='weesen-before-ground';scenario='shore-preview';entry=0;land=$true;ground=$true;off=$true}
$cases+=@{name='murg-fallback-ground';scenario='shore-preview';entry=1;land=$true;ground=$true;fallback=$true}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;Viewport='1600x900';MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.fallback) { $options.DisableNanite=$true }
    if($case.land) { $options.ShoreLookBack=$true }
    if($case.ground) { $options.GroundView=$true }
    if($case.off) { $options.GroundOff=$true }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    $enabled=if($case.off) { 0 } else { 1 }
    if($log -notmatch "DIVE_GROUND_PASS materials=5 textures=7 enabled=$enabled resident_bytes=33751040" -or
       $log -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used') { throw "Ground validation failed: $folder" }
    $captures=@(Get-ChildItem -LiteralPath $saved -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($captures.Count -lt 1) { throw 'Missing fresh rendered captures.' }
    $captures | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;viewport='1600x900';fallback=[bool]$case.fallback;ground_off=[bool]$case.off;status='PASS';screenshots=$captures.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "GROUND_VALIDATION=$result"
