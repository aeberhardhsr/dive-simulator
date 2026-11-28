param([string]$PackageDirectory='Builds/WindowsVegetation')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/vegetation-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$saved=Join-Path $PackageDirectory 'divingsim/Saved/Screenshots'
$containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
    @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;packaged=$true;package_containers=$containers} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{name='walenstadt-crossing';scenario='coast';entry=2;viewport='1600x900'},
    @{name='weesen-land';scenario='shore-preview';entry=0;viewport='1600x900';land=$true;stream=$true},
    @{name='murg-land';scenario='shore-preview';entry=1;viewport='1600x900';land=$true},
    @{name='mols-land';scenario='shore-preview';entry=2;viewport='1600x900';land=$true},
    @{name='murg-fallback';scenario='shore-preview';entry=1;viewport='1280x1024';land=$true;fallback=$true},
    @{name='weesen-fallback';scenario='shore-preview';entry=0;viewport='1600x900';land=$true;fallback=$true},
    @{name='mols-water-4k';scenario='shore-preview';entry=2;viewport='3840x2160'},
    @{name='weesen-shore';scenario='shore';entry=0;viewport='1600x900'},
    @{name='murg-shore';scenario='shore';entry=1;viewport='1600x900'},
    @{name='mols-shore';scenario='shore';entry=2;viewport='1600x900'},
    @{name='weesen-crossing';scenario='coast';entry=0;viewport='1600x900'},
    @{name='murg-crossing';scenario='coast';entry=1;viewport='1600x900'},
    @{name='discoveries';scenario='discoveries';entry=2;viewport='1600x900'},
    @{name='controls';scenario='controls';entry=0;viewport='1600x900'}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;Viewport=$case.viewport;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.fallback) { $options.DisableNanite=$true }
    if($case.land) { $options.ShoreLookBack=$true }
    if($case.stream) { $options.VegetationChecks=$true }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_VEGETATION_READY candidates=810598 cells=4042 models=5' -or $log -notmatch 'DIVE_VEGETATION_PASS' -or $log -match 'DIVE_\w+_FAIL|Failed to add hitbox') { throw "Runtime vegetation validation failed: $folder" }
    if($case.stream -and $log -notmatch 'DIVE_VEGETATION_STREAM_PASS retained=1 retired=1 returned=1') { throw 'Vegetation streaming check missing.' }
    $captures=@(Get-ChildItem -LiteralPath $saved -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($captures.Count -lt 1) { throw 'Missing fresh rendered captures.' }
    $captures | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;viewport=$case.viewport;fallback=[bool]$case.fallback;status='PASS';screenshots=$captures.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "VEGETATION_VALIDATION=$result"
