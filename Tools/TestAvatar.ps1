param([string]$PackageDirectory='Builds/WindowsAvatar',[switch]$EditorSandbox)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$result=Join-Path $repo ('artifacts/avatar-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$exe=if($EditorSandbox) { 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' } else { Resolve-DivePackageExecutable $PackageDirectory }
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe;executable_sha256=(Get-FileHash -LiteralPath $exe).Hash;
    editor_sandbox=[bool]$EditorSandbox;source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$completed=@()
foreach($case in 0..5) {
    $folder=Join-Path $result ([string]$case)
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Scenario='avatar';LoadoutCase=$case;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($EditorSandbox) { $options.EditorSandbox=$true } else { $options.Packaged=$true;$options.PackageDirectory=$PackageDirectory }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $saved=if($EditorSandbox) { Join-Path $repo 'Builds/EditorSandbox/divingsim/Saved/Screenshots' } else { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' }
    Get-ChildItem -LiteralPath $saved -File -Filter "Avatar_${case}_*.png" |
        Where-Object { $_.LastWriteTimeUtc -ge $started } | Copy-Item -Destination $folder
    $completed+=@{name=$case;started_utc=$started.ToString('o');completed_utc=[DateTime]::UtcNow.ToString('o');status='PASS'}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $case $folder"
}
Write-Output "AVATAR_VALIDATION=$result"
