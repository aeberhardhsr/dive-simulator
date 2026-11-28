param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateSet('Editor','Game','Content','Landscape','LandscapeCheck','LandscapeDetail','LandscapeDetailCheck','LandscapeFallbackAudit','LandscapeFallback','LandscapeFallbackCheck','Coast','CoastCheck','Buildings','BuildingsCheck','Package')][string]$Target = 'Editor',
    [string]$PackageDirectory='',
    [ValidateRange(1,64)][int]$MaxParallelActions=2,
    [switch]$EditorSandbox,
    [switch]$RefreshSources,
    [switch]$BoundaryStudy,
    [switch]$ResidentStudy,
    [switch]$LandscapePilot,
    [switch]$NaturalGround,
    [switch]$ShoreStones,
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='EditorSandbox',
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
    [switch]$OnlyAvatarMaterials,
    [switch]$OnlyBed,
    [switch]$OnlyBenthic,
    [switch]$OnlyBenthicColor,
    [switch]$OnlyWater,
    [switch]$OnlyWaterVolume,
    [switch]$OnlyBuildings,
    [switch]$OnlyVegetation,
    [switch]$OnlyGround,
    [switch]$OnlyFarCliffs,
    [switch]$OnlyFish,
    [switch]$OnlyRocks,
    [switch]$OnlyFormations,
    [switch]$OnlyRockSurfaces,
    [switch]$OnlyGravel,
    [switch]$OnlyPlants,
    [switch]$OnlyPlantMaterial
)
$ErrorActionPreference = 'Stop'
if($ShoreStones -and -not $NaturalGround) {throw 'ShoreStones packaging requires NaturalGround.'}
if($NaturalGround -and ($Target -ne 'Package' -or -not $LandscapePilot)) {throw 'NaturalGround packaging requires Target Package and LandscapePilot.'}
if($RefreshSources -and $Target -notin @('Editor','Game')) {throw 'RefreshSources applies to Editor or Game compilation.'}
if($ResidentStudy -and $Target -notin @('LandscapeFallback','LandscapeFallbackCheck')) {throw 'ResidentStudy requires LandscapeFallback or LandscapeFallbackCheck.'}
if($LandscapePilot -and ($ResidentStudy -or $BoundaryStudy -or $Target -notin @('LandscapeFallback','LandscapeFallbackCheck','Package'))) {throw 'LandscapePilot requires LandscapeFallback, LandscapeFallbackCheck or Package without other study flags.'}
$repo = Split-Path $PSScriptRoot -Parent
$project = Join-Path $repo 'divingsim/divingsim.uproject'
if($SandboxName -ne 'EditorSandbox' -and -not $EditorSandbox) {throw 'SandboxName requires EditorSandbox.'}
if($EditorSandbox) { . (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1'); $project=Get-DiveEditorSandbox -SandboxName $SandboxName }
if($BuildSlot -and $Target -notin @('Game','Package')) { throw 'BuildSlot applies to Game or Package only.' }
$slotOptions=@()
if($RefreshSources) { $slotOptions+='-gather' }
$ubtOptions="-NoUBA -MaxParallelActions=$MaxParallelActions -NoHotReloadFromIDE"
if($BuildSlot) { $slotOptions+="-DiveBuildSlot=$BuildSlot"; $ubtOptions+=" -DiveBuildSlot=$BuildSlot" }
if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
# UAT changes its working directory to the engine. Resolve caller-supplied
# relative output paths against the repository before passing them to UAT.
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
# The patched source engine needs the selected water shaders before normal
# editor/cook startup. Preparation stays local; Epic shader sources are not
# copied into the repository or required by a cooked executable.
$reflectionBinding=Join-Path $EngineRoot 'Engine/Source/Runtime/Renderer/Private/Lumen/LumenReflectionHardwareRayTracing.cpp'
if((Test-Path -LiteralPath $reflectionBinding) -and
   (Select-String -LiteralPath $reflectionBinding -Pattern 'r.DiveWater.ReflectionShadowBindingVersion' -Quiet)) {
    & (Join-Path $PSScriptRoot '.geodata-env/Scripts/python.exe') (Join-Path $PSScriptRoot 'PrepareReflectionTransport.py') `
        --engine $EngineRoot --project (Split-Path $project -Parent) --production --unified --profile vsm
    if($LASTEXITCODE -ne 0) {throw 'Production water shader preparation failed.'}
}
# Preserve the installed engine's WITH_LIVE_CODING ABI. -NoLiveCoding changes
# registration structure layouts in a monolithic Development build and is unsafe
# with this engine's precompiled libraries. -NoHotReloadFromIDE only selects a
# normal build; it does not change those structures.
if ($Target -eq 'Content') {
    $script = Join-Path $PSScriptRoot 'build_unreal_content.py'
    if($OnlyWaterVolume) { $script=Join-Path $PSScriptRoot 'build_water_volume_content.py' }
    if($OnlyAudio) { $script=Join-Path $PSScriptRoot 'build_audio_content.py' }
    if($OnlyShore) { $script=Join-Path $PSScriptRoot 'build_shore_content.py' }
    if($OnlyImagery) { $script=Join-Path $PSScriptRoot 'build_imagery_content.py' }
    if($OnlyBuildings) { $script=Join-Path $PSScriptRoot 'build_buildings_content.py' }
    if($OnlyVegetation) { $script=Join-Path $PSScriptRoot 'build_vegetation_content.py' }
    if($OnlyGround) { $script=Join-Path $PSScriptRoot 'build_ground_content.py' }
    if($OnlyFarCliffs) { $script=Join-Path $PSScriptRoot 'build_landscape_cliff_study.py' }
    if($OnlyFish) { $script=Join-Path $PSScriptRoot 'build_fish_content.py' }
    if($OnlyRocks) { $script=Join-Path $PSScriptRoot 'build_rock_content.py' }
    if($OnlyFormations) { $script=Join-Path $PSScriptRoot 'build_formation_content.py' }
    if($OnlyRockSurfaces) { $script=Join-Path $PSScriptRoot 'build_rock_surface_content.py' }
    if($OnlyGravel) { $script=Join-Path $PSScriptRoot 'build_gravel_content.py' }
    if($OnlyPlants -or $OnlyPlantMaterial) { $script=Join-Path $PSScriptRoot 'build_plant_content.py' }
    if($OnlyOptics) { $script=Join-Path $PSScriptRoot 'build_optics_content.py' }
    if($OnlyCaustics) { $script=Join-Path $PSScriptRoot 'build_caustics_content.py' }
    if($OnlyTorch) { $script=Join-Path $PSScriptRoot 'build_torch_content.py' }
    if($OnlyParticles) { $script=Join-Path $PSScriptRoot 'build_particle_content.py' }
    if($OnlySediment) { $script=Join-Path $PSScriptRoot 'build_sediment_content.py' }
    if($OnlyAvatar -or $OnlyAvatarMaterials) { $script=Join-Path $PSScriptRoot 'build_avatar_content.py' }
    if($OnlyBed) { $script=Join-Path $PSScriptRoot 'build_bed_content.py' }
    if($OnlyBenthic) { $script=Join-Path $PSScriptRoot 'build_benthic_content.py' }
    if($OnlyBenthicColor) { $script=Join-Path $PSScriptRoot 'build_benthic_color.py' }
    $contentOptions=@()
    if($OnlyAvatarMaterials) { $contentOptions+='-DiveAvatarMaterialsOnly' }
    if($OnlyPlantMaterial) { $contentOptions+='-DivePlantMaterialOnly' }
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
} elseif ($Target -in @('Landscape','LandscapeCheck','LandscapeDetail','LandscapeDetailCheck','LandscapeFallbackAudit','LandscapeFallback','LandscapeFallbackCheck')) {
    $landOptions=@()
    if($BoundaryStudy) { $landOptions+='-BoundaryStudy' }
    if($ResidentStudy) { $landOptions+='-ResidentStudy' }
    if($LandscapePilot) { $landOptions+='-LandscapePilot' }
    if($Target -in @('LandscapeFallback','LandscapeFallbackCheck')) { $landOptions+='-FallbackStudy' }
    if($Target -eq 'LandscapeFallbackCheck') { $landOptions+='-VerifyOnly' }
    if($Target -eq 'LandscapeFallbackAudit') { $landOptions+='-FallbackAudit' }
    if($Target -in @('LandscapeCheck','LandscapeDetailCheck')) { $landOptions+='-VerifyOnly' }
    if($Target -in @('LandscapeDetail','LandscapeDetailCheck')) { $landOptions+='-DetailStudy' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveLandscapeBake' -unattended -nullrhi -nosplash '-log=LandscapeBuild.log' @landOptions
} elseif ($Target -eq 'Package') {
    if($ShoreStones) {
        & (Join-Path $repo 'Tools/.geodata-env/Scripts/python.exe') (Join-Path $repo 'Tools/validate_shore_stones.py') --verify-content
        if($LASTEXITCODE -ne 0) {throw 'Shore bank source/content verification failed.'}
    }
    if($NaturalGround) {
        $groundScript=Join-Path $repo 'Tools/build_natural_ground.py'
        & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=pythonscript' "-script=$groundScript" '-NaturalGroundVerify' -unattended -nullrhi -nosplash '-log=NaturalGroundCheck.log'
        if($LASTEXITCODE -ne 0) {throw 'Natural ground material source verification failed.'}
    }
    if($LandscapePilot) {
        & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveLandscapeBake' '-LandscapePilot' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=LandscapePilotCheck.log'
        if($LASTEXITCODE -ne 0) {throw 'Landscape pilot assets/data failed verification.'}
    }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveCoastBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=CoastCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Coast assets are missing or stale; run -Target Editor and -Target Coast.' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveBuildingsBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=BuildingsCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Building assets are missing or stale; run -Target Editor, Content -OnlyBuildings and -Target Buildings.' }
    & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project '-run=DiveLandscapeBake' '-VerifyOnly' -unattended -nullrhi -nosplash '-log=LandscapeCheck.log'
    if ($LASTEXITCODE -ne 0) { throw 'Landscape assets are missing or stale; run -Target Editor and -Target Landscape.' }
    & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -nocompileeditor -platform=Win64 -clientconfig=Development -build -cook '-map=/Game/Diving/Maps/L_DiveTraining' -stage -pak -archive "-archivedirectory=$PackageDirectory" "-ubtargs=$ubtOptions" -unattended
} else {
    $name = if ($Target -eq 'Editor') { 'divingsimEditor' } else { 'divingsim' }
    & "$EngineRoot/Engine/Build/BatchFiles/Build.bat" $name Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA "-MaxParallelActions=$MaxParallelActions" @slotOptions
}
if ($LASTEXITCODE -ne 0) { throw "Unreal $Target failed with exit code $LASTEXITCODE" }
if($Target -eq 'Package') {
    Copy-Item -LiteralPath (Join-Path $repo 'Tools/StartDiveSimulator.cmd') -Destination (Join-Path $PackageDirectory 'Start-DiveSimulator.cmd')
    if($LandscapePilot) {
        Copy-Item -LiteralPath (Join-Path $repo 'Tools/StartLandscapePilot.cmd') -Destination (Join-Path $PackageDirectory 'Start-DiveSimulator.cmd')
    }
    if($NaturalGround) {
        Copy-Item -LiteralPath (Join-Path $repo 'Tools/StartNaturalLandscape.cmd') -Destination (Join-Path $PackageDirectory 'Start-DiveSimulator.cmd')
    }
    if($ShoreStones) {
        Copy-Item -LiteralPath (Join-Path $repo 'Tools/StartShoreLandscape.cmd') -Destination (Join-Path $PackageDirectory 'Start-DiveSimulator.cmd')
    }
    Copy-Item -LiteralPath (Join-Path $repo 'Tools/WaterPreviewLauncher.cmd') -Destination (Join-Path $PackageDirectory 'Start-WaterPreview.cmd')
    # Keep scenario-data attribution readable without extracting the game pak.
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/TEMPERATURE_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'TEMPERATURE_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/BUILDINGS_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'BUILDINGS_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/COAST_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'COAST_NOTICE.txt')
    Copy-Item -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/VEGETATION_NOTICE.txt') -Destination (Join-Path $PackageDirectory 'VEGETATION_NOTICE.txt')
}
