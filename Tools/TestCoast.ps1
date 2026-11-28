param([string]$PackageDirectory='Builds/WindowsCoast')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/coast-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$saved=Join-Path $PackageDirectory 'divingsim/Saved/Screenshots'
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;packaged=$true} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{name='weesen-crossing';scenario='coast';entry=0;viewport='1600x900'},
    @{name='murg-crossing';scenario='coast';entry=1;viewport='1920x1080'},
    @{name='walenstadt-4k';scenario='coast';entry=2;viewport='3840x2160'},
    @{name='walenstadt-fallback';scenario='coast';entry=2;viewport='1280x1024';fallback=$true},
    @{name='weesen-shore';scenario='shore';entry=0;viewport='1600x900'},
    @{name='murg-shore';scenario='shore';entry=1;viewport='1600x900'},
    @{name='mols-shore';scenario='shore';entry=2;viewport='1600x900'},
    @{name='discoveries';scenario='discoveries';entry=2;viewport='1600x900'},
    @{name='controls';scenario='controls';entry=0;viewport='1600x900'}
)
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;Viewport=$case.viewport;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.fallback) { $options.DisableNanite=$true }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_COAST_COLLISION_PASS probes=496' -or $log -match 'DIVE_\w+_FAIL|Failed to add hitbox') { throw "Runtime coast validation failed: $folder" }
    $captures=@(Get-ChildItem -LiteralPath $saved -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($captures.Count -lt 2) { throw 'Missing fresh rendered captures.' }
    $captures | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;viewport=$case.viewport;fallback=[bool]$case.fallback;status='PASS';screenshots=$captures.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "COAST_VALIDATION=$result"
