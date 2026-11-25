param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateSet('Editor','Game','Content','Package')][string]$Target = 'Editor',
    [string]$PackageDirectory=''
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$project = Join-Path $repo 'divingsim/divingsim.uproject'
if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
# Preserve the installed engine's WITH_LIVE_CODING ABI. -NoLiveCoding changes
# registration structure layouts in a monolithic Development build and is unsafe
# with this engine's precompiled libraries. -NoHotReloadFromIDE only selects a
# normal build; it does not change those structures.
if ($Target -eq 'Content') {
    $script = Join-Path $PSScriptRoot 'build_unreal_content.py'
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=pythonscript' "-script=$script" -unattended -nullrhi -nosplash '-log=ContentBuild.log'
} elseif ($Target -eq 'Package') {
    & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -nocompileeditor -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Diving/Maps/L_DiveTraining' -stage -pak -archive "-archivedirectory=$PackageDirectory" '-ubtargs=-NoUBA -MaxParallelActions=2 -NoHotReloadFromIDE' -unattended
} else {
    $name = if ($Target -eq 'Editor') { 'divingsimEditor' } else { 'divingsim' }
    & "$EngineRoot/Engine/Build/BatchFiles/Build.bat" $name Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=2
}
if ($LASTEXITCODE -ne 0) { throw "Unreal $Target failed with exit code $LASTEXITCODE" }
