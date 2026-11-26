param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateSet('Editor','Game','Content','Landscape','LandscapeCheck','Coast','CoastCheck','Buildings','BuildingsCheck','Package')][string]$Target = 'Editor',
    [string]$PackageDirectory='',
    [switch]$EditorSandbox,
    [ValidatePattern('^[A-Za-z0-9_]{0,24}$')][string]$BuildSlot='',
    [switch]$OnlyEnvironment,
    [switch]$OnlyAudio,
    [switch]$OnlyShore,
    [switch]$OnlyImagery,
    [switch]$OnlyOptics,
    [switch]$OnlyCaustics,
    [switch]$OnlyTorch,
    [switch]$OnlyParticles,
    [switch]$OnlySediment,
    [switch]$OnlyAvatar,
    [switch]$OnlyBed,
    [switch]$OnlyWater,
    [switch]$OnlyBuildings,
    [switch]$OnlyVegetation,
    [switch]$OnlyGround,
    [switch]$OnlyFish
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$project = Join-Path $repo 'divingsim/divingsim.uproject'
if($EditorSandbox) { . (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1'); $project=Get-DiveEditorSandbox }
if($BuildSlot -and $Target -notin @('Game','Package')) { throw 'BuildSlot applies to Game or Package only.' }
$slotOptions=@()
$ubtOptions='-NoUBA -MaxParallelActions=2 -NoHotReloadFromIDE'
if($BuildSlot) { $slotOptions+="-DiveBuildSlot=$BuildSlot"; $ubtOptions+=" -DiveBuildSlot=$BuildSlot" }
if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
# UAT changes its working directory to the engine. Resolve caller-supplied
# relative output paths against the repository before passing them to UAT.
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
# Preserve the installed engine's WITH_LIVE_CODING ABI. -NoLiveCoding changes
# registration structure layouts in a monolithic Development build and is unsafe
# with this engine's precompiled libraries. -NoHotReloadFromIDE only selects a
# normal build; it does not change those structures.
if ($Target -eq 'Content') {
    $script = Join-Path $PSScriptRoot 'build_unreal_content.py'
    if($OnlyAudio) { $script=Join-Path $PSScriptRoot 'build_audio_content.py' }
    if($OnlyShore) { $script=Join-Path $PSScriptRoot 'build_shore_content.py' }
    if($OnlyImagery) { $script=Join-Path $PSScriptRoot 'build_imagery_content.py' }
    if($OnlyBuildings) { $script=Join-Path $PSScriptRoot 'build_buildings_content.py' }
    if($OnlyVegetation) { $script=Join-Path $PSScriptRoot 'build_vegetation_content.py' }
    if($OnlyGround) { $script=Join-Path $PSScriptRoot 'build_ground_content.py' }
    if($OnlyFish) { $script=Join-Path $PSScriptRoot 'build_fish_content.py' }
    if($OnlyOptics) { $script=Join-Path $PSScriptRoot 'build_optics_content.py' }
    if($OnlyCaustics) { $script=Join-Path $PSScriptRoot 'build_caustics_content.py' }
    if($OnlyTorch) { $script=Join-Path $PSScriptRoot 'build_torch_content.py' }
    if($OnlyParticles) { $script=Join-Path $PSScriptRoot 'build_particle_content.py' }
    if($OnlySediment) { $script=Join-Path $PSScriptRoot 'build_sediment_content.py' }
    if($OnlyAvatar) { $script=Join-Path $PSScriptRoot 'build_avatar_content.py' }
    if($OnlyBed) { $script=Join-Path $PSScriptRoot 'build_bed_content.py' }
    $contentOptions=@()
    if($OnlyEnvironment) { $contentOptions+='-DiveEnvironmentOnly' }
    if($OnlyWater) { $contentOptions+='-DiveWaterOnly' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=pythonscript' "-script=$script" -unattended -nullrhi -nosplash '-log=ContentBuild.log' @contentOptions
} elseif ($Target -in @('Buildings','BuildingsCheck')) {
    $buildingOptions=@()
    if($Target -eq 'BuildingsCheck') { $buildingOptions+='-VerifyOnly' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveBuildingsBake' @buildingOptions -unattended -nullrhi -nosplash '-log=BuildingsBuild.log'
} elseif ($Target -in @('Coast','CoastCheck')) {
    $coastOptions=@()
    if($Target -eq 'CoastCheck') { $coastOptions+='-VerifyOnly' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveCoastBake' @coastOptions -unattended -nullrhi -nosplash '-log=CoastBuild.log'
} elseif ($Target -in @('Landscape','LandscapeCheck')) {
    $landOptions=@()
    if($Target -eq 'LandscapeCheck') { $landOptions+='-VerifyOnly' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveLandscapeBake' -unattended -nullrhi -nosplash '-log=LandscapeBuild.log' @landOptions
} elseif ($Target -eq 'Package') {
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveCoastBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=CoastCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Coast assets are missing or stale; run -Target Editor and -Target Coast.' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveBuildingsBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=BuildingsCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Building assets are missing or stale; run -Target Editor, Content -OnlyBuildings and -Target Buildings.' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveLandscapeBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=LandscapeCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Landscape assets are missing or stale; run -Target Editor and -Target Landscape.' }
    & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -nocompileeditor -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Diving/Maps/L_DiveTraining' -stage -pak -archive "-archivedirectory=$PackageDirectory" "-ubtargs=$ubtOptions" -unattended
} else {
    $name = if ($Target -eq 'Editor') { 'divingsimEditor' } else { 'divingsim' }
    & "$EngineRoot/Engine/Build/BatchFiles/Build.bat" $name Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=2 @slotOptions
}
if ($LASTEXITCODE -ne 0) { throw "Unreal $Target failed with exit code $LASTEXITCODE" }
if($Target -eq 'Package') {
    # Keep scenario-data attribution readable without extracting the game pak.
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/TEMPERATURE_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'TEMPERATURE_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/BUILDINGS_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'BUILDINGS_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/COAST_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'COAST_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/VEGETATION_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'VEGETATION_NOTICE.txt')
}
