param([string]$PackageDirectory='',[switch]$EditorSandbox)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$result=Join-Path $repo ('artifacts/waypoints-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
if($PackageDirectory) {
    if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
    . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
    $exe=Resolve-DivePackageExecutable $PackageDirectory
    $saved=Join-Path $PackageDirectory 'divingsim/Saved'
} else {
    $exe='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
    $saved=Join-Path $repo $(if($EditorSandbox) {'Builds/EditorSandbox/divingsim/Saved'} else {'divingsim/Saved'})
}
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;sha256=(Get-FileHash -LiteralPath $exe).Hash;packaged=[bool]$PackageDirectory} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='weesen';entry=0;viewport='1600x900'},@{name='murg-4k';entry=1;viewport='3840x2160'},@{name='walenstadt-letterbox';entry=2;viewport='1280x1024'})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Scenario='waypoints';Entry=$case.entry;Viewport=$case.viewport;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($PackageDirectory) { $options.Packaged=$true;$options.PackageDirectory=$PackageDirectory }
    if($EditorSandbox) { $options.EditorSandbox=$true }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_WAYPOINT_PASS phases=21' -or $log -match 'DIVE_\w+_FAIL|Failed to add hitbox') { throw "Waypoint phases failed: $folder" }
    if($log -notmatch 'DIVE_WAYPOINT_FILE ([^\r\n]+)') { throw 'Missing persisted waypoint file.' }
    $file=$Matches[1].Trim()
    Copy-Item -LiteralPath $file -Destination (Join-Path $folder 'Waypoints.json')
    $images=@(Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -lt 8) { throw 'Missing waypoint screenshots.' }
    $images | Copy-Item -Destination $folder
    $restart=Join-Path $folder 'Restart.log';$options.RuntimeLog=$restart;$options.WaypointVerify=$file
    $restarted=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'RestartSummary.log')
    if((Get-Content -LiteralPath $restart -Raw) -notmatch 'DIVE_WAYPOINT_PASS restart=1') { throw 'Restart did not restore waypoint target.' }
    Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -Filter '*Restart.png' -File |
        Where-Object {$_.LastWriteTimeUtc -ge $restarted} | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;entry=$case.entry;viewport=$case.viewport;status='PASS';restart='PASS';screenshots=@(Get-ChildItem -LiteralPath $folder -Filter '*.png').Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "WAYPOINT_VALIDATION=$result"
