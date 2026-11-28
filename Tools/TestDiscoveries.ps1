param([string]$PackageDirectory='',[switch]$EditorSandbox)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$result=Join-Path $repo ('artifacts/discoveries-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
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
$cases=@(@{name='walenstadt-letterbox';entry=2;viewport='1280x1024'},@{name='weesen';entry=0;viewport='1600x900'},@{name='murg-4k';entry=1;viewport='3840x2160'})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Scenario='discoveries';Entry=$case.entry;Viewport=$case.viewport;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($PackageDirectory) { $options.Packaged=$true;$options.PackageDirectory=$PackageDirectory }
    if($EditorSandbox) { $options.EditorSandbox=$true }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -notmatch 'DIVE_DISCOVERY_PASS phases=14' -or $log -match 'DIVE_\w+_FAIL|Failed to add hitbox') { throw "Discovery phases failed: $folder" }
    if(([regex]::Matches($log,'DIVE_DISCOVERY_VISIBILITY_PASS')).Count -ne 2) { throw 'Missing fish and plant visibility checks.' }
    if($log -notmatch 'DIVE_DISCOVERY_LOG ([^\r\n]+)') { throw 'Missing dive log path.' }
    $file=$Matches[1].Trim();Copy-Item -LiteralPath $file -Destination (Join-Path $folder 'Dive.json')
    $dive=Get-Content -LiteralPath $file -Raw | ConvertFrom-Json
    if($dive.schema_version -ne 7 -or $dive.photographs.Count -ne 2) { throw 'Photo association missing from schema 7.' }
    foreach($photo in $dive.photographs) {
        if($photo.id -notmatch '^[a-fA-F0-9]{32}$') { throw 'Unsafe photograph identity.' }
        $image=Get-Item -LiteralPath (Join-Path $saved ('PhotographsTest/'+$photo.id+'.png'))
        if($image.LastWriteTimeUtc -lt $started -or $image.Length -lt 10000) { throw 'Missing fresh photograph.' }
        Copy-Item -LiteralPath $image.FullName -Destination $folder
    }
    $images=@(Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -Filter 'Discoveries_*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -lt 5) { throw 'Missing gallery screenshots.' }
    $images | Copy-Item -Destination $folder
    if($case.entry -eq 0) {
        $options.RuntimeLog=Join-Path $folder 'Restart.log';$options.DiscoveryVerify=$file;$restarted=[DateTime]::UtcNow
        & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'RestartSummary.log')
        if((Get-Content -LiteralPath $options.RuntimeLog -Raw) -notmatch 'DIVE_DISCOVERY_RESTART_PASS photos=2') { throw 'Restart failed to load photographs.' }
        Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -Filter '*Restart.png' -File |
            Where-Object {$_.LastWriteTimeUtc -ge $restarted} | Copy-Item -Destination $folder
    }
    $completed+=@{name=$case.name;entry=$case.entry;viewport=$case.viewport;status='PASS';screenshots=@(Get-ChildItem -LiteralPath $folder -Filter '*.png').Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "DISCOVERY_VALIDATION=$result"
