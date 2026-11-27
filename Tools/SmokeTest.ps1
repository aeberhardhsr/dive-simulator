param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8', [switch]$Packaged, [switch]$EditorSandbox,
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='EditorSandbox',
    [ValidateRange(0,2)][int]$Entry=0, [ValidateSet('ui','dive','surface','surface-calm','surface-rain','surface-storm','daycycle','stream','wrist','wrist-night','torch','logbook','audio','controls','tutorial','profiles','environment','config','shore-preview','shore','breathing','reserve-loss','modes','loadout','avatar','sediment','navigation','waypoints','discoveries','coast','decompression','fish','rocks')][string]$Scenario='dive',
    [ValidateRange(0,5)][int]$LoadoutCase=0,
    [ValidateSet('exploration','manual','simulation','hardcore','warm','hypoxia')][string]$ModeCase='simulation',
    [string]$PackageDirectory='', [string]$DiscoveryVerify='', [string]$WaypointVerify='', [switch]$DisableNanite, [switch]$LampOff, [switch]$CelestialReferences, [switch]$SkyView, [switch]$ShoreLookBack, [switch]$BuildingsView,
    [ValidatePattern('^[a-z,]*$')][string]$HideSky='',
    [switch]$GroundView, [switch]$GroundOff, [switch]$GravelView, [switch]$PlantView, [switch]$WaterView, [switch]$HabitatView,
    [ValidateCount(5,5)][double[]]$HabitatPose,
    [ValidateCount(4,4)][double[]]$WaterlinePose,
    [switch]$WaterlineNoSurface, [switch]$WaterlineNoVolumeShadow, [switch]$NoReflectionScreenTraces,
    [ValidateRange(0,60000)][double]$WaterlineFogDistance=0,
    [ValidateSet('none','BaseColor','WorldNormal','SceneDepth')][string]$BufferView='none',
    [ValidateRange(-1,1)][int]$FogComposition=-1,
    [switch]$WaterlineNoFogCutoff, [switch]$DisableLightShafts,
    [switch]$DisableFogDepthCulling, [switch]$DisableFogHistory,
    [switch]$DisablePostProcessMaterials,
    [switch]$HabitatClose, [switch]$NoRockAprons, [switch]$NoRockRubble, [switch]$SparseGravel, [switch]$LegacyRockSurface, [switch]$LegacyPlantPlacement, [switch]$HabitatLowView, [switch]$LegacyRockPose,
    [switch]$LegacyTalus, [switch]$NoPlantUnderstory, [switch]$UnlimitedPlants, [switch]$LegacyFormations, [switch]$FormationView, [switch]$FootDeposits, [switch]$NoFootDeposits, [switch]$ImmediateGravelBuild, [switch]$ImmediatePopulationBuild,
    [ValidateRange(0,1)][double]$FogScatterGain=1, [ValidateRange(0,1)][double]$ViewScatterGain=1,
    [switch]$NativeScattering, [switch]$HabitatSunView, [switch]$DisableVolumetrics,
    [switch]$WaterInterface, [switch]$HardwareLumen, [switch]$BoundedWater, [switch]$WaterPreview, [switch]$DisableRayTracing,
    [switch]$SpectralView, [switch]$SpectralDepthDebug, [ValidateRange(0,2)][double]$AbsorptionGain=1, [switch]$SolarShaderStudy, [switch]$SolarVolumeStudy, [switch]$WaveTransportStudy,
    [switch]$LakeFootprint, [switch]$LakeSurfaceOff,
    [ValidateSet('default','cache','front','hit')][string]$WaterReflection='default',
    [ValidateRange(0,1)][double]$WaterVolumeGain=1,
    [switch]$DisableShadows, [switch]$DisableLighting, [switch]$LegacyExposureStart, [switch]$ExposureProbe,
    [ValidateSet('none','direct','sky','indirect','function')][string]$LightIsolation='none',
    [ValidateRange(0,9)][int]$WaterStart=0,
    [ValidateRange(0,9)][int]$WaterEnd=9,
    [ValidateRange(-180,360)][double]$SkyYaw=180, [ValidateRange(-85,85)][double]$SkyPitch=45,
    [ValidateRange(0,240)][int]$MaxFPS=0,
    [ValidateRange(30,7200)][int]$TimeoutSeconds=180,
    [ValidateSet('1600x900','1920x1080','2560x1440','3840x2160','1280x1024')][string]$Viewport='1600x900',
    [ValidateRange(0,2)][int]$Torch=0,
    [ValidateRange(2,40)][double]$Visibility=14,
    [ValidateRange(0,100)][double]$Battery=100,
    [ValidateRange(0,30)][double]$ParticleScale=1,
    [switch]$PopulationChecks, [switch]$VegetationChecks, [switch]$PlantColorProbe, [switch]$NavShore, [switch]$NoBubbles, [switch]$AutomaticEnvironment, [switch]$FinInspection, [string]$RuntimeLog='',
    [ValidateRange(0,23.99)][double]$Hour,
    [ValidatePattern('^[0-9]{4}-[0-9]{2}-[0-9]{2}$')][string]$Date='')
$ErrorActionPreference='Stop'
if($SandboxName -ne 'EditorSandbox' -and -not $EditorSandbox) {throw 'SandboxName requires EditorSandbox.'}
if($SpectralView -and -not $WaterPreview) {throw 'SpectralView requires WaterPreview.'}
if($SolarShaderStudy -and ($Packaged -or -not $EditorSandbox -or -not $WaterPreview)) {throw 'SolarShaderStudy currently requires EditorSandbox and WaterPreview; it is not cooked into a package.'}
if($SolarVolumeStudy -and -not $SolarShaderStudy) {throw 'SolarVolumeStudy requires SolarShaderStudy.'}
if($WaveTransportStudy -and -not $SolarVolumeStudy) {throw 'WaveTransportStudy requires SolarVolumeStudy.'}
if($LakeFootprint -and -not $WaveTransportStudy) {throw 'LakeFootprint requires WaveTransportStudy.'}
if($LakeSurfaceOff -and -not $LakeFootprint) {throw 'LakeSurfaceOff requires LakeFootprint.'}
if($SpectralDepthDebug -and -not $SpectralView) {throw 'SpectralDepthDebug requires SpectralView.'}
if($PlantColorProbe -and $Packaged) {throw 'PlantColorProbe requires the editor runtime with CPU vertex data.'}
if($PlantColorProbe -and $Scenario -ne 'dive') {throw 'PlantColorProbe requires the dive scenario, which validates the actual mesh buffers.'}
if($PSBoundParameters.ContainsKey('WaterVolumeGain') -and -not ($BoundedWater -or $WaterPreview)) {throw 'WaterVolumeGain requires BoundedWater or WaterPreview.'}
if($DisableLighting -and $Packaged) { throw 'Unlit diagnostic requires the native editor runtime.' }
if($WaterReflection -ne 'default' -and -not $WaterInterface) { throw 'WaterReflection requires WaterInterface.' }
if($WaterReflection -eq 'hit' -and -not $HardwareLumen) { throw 'Hit lighting requires HardwareLumen.' }
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
if($EditorSandbox) { $project=Join-Path $repo "Builds/$SandboxName/divingsim/divingsim.uproject" }
$logPath=if($RuntimeLog) { [IO.Path]::GetFullPath($RuntimeLog) } else { Join-Path $repo 'artifacts/DiveSmoke.log' }
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$size=$Viewport.Split('x')
$arguments='-DiveSmokeTest -windowed -ForceRes -ResX={1} -ResY={2} -nosplash -unattended -abslog="{0}"' -f $logPath,$size[0],$size[1]
$arguments+=' -DiveEntry={0} -DiveScenario={1}' -f $Entry,$Scenario
if($SpectralView) {$arguments+=' -DiveSpectralView -DiveAbsorptionGain='+$AbsorptionGain.ToString([Globalization.CultureInfo]::InvariantCulture)}
if($SpectralDepthDebug) {$arguments+=' -DiveSpectralDepthDebug'}
if($SolarShaderStudy) {$arguments+=' -DiveSolarShaderStudy'}
if($SolarVolumeStudy) {$arguments+=' -DiveSolarVolumeStudy'}
if($WaveTransportStudy) {$arguments+=' -DiveWaveTransportStudy'}
if($LakeFootprint) {$arguments+=' -DiveLakeFootprint'}
if($LakeSurfaceOff) {$arguments+=' -DiveLakeSurfaceOff'}
if($DiscoveryVerify) {
    if($Scenario -ne 'discoveries' -or $DiscoveryVerify.Contains('"')) { throw 'DiscoveryVerify requires the discoveries scenario and an unquoted path.' }
    $arguments+=' -DiveDiscoveryVerify="'+[IO.Path]::GetFullPath($DiscoveryVerify)+'"'
}
if($WaypointVerify) {
    if($Scenario -ne 'waypoints' -or $WaypointVerify.Contains('"')) { throw 'WaypointVerify requires the waypoints scenario and an unquoted file path.' }
    $arguments+=' -DiveWaypointVerify="'+[IO.Path]::GetFullPath($WaypointVerify)+'"'
}
if($Scenario -eq 'modes') { $arguments+=' -DiveModeCase='+$ModeCase }
if($Scenario -in @('loadout','avatar')) { $arguments+=' -DiveLoadoutCase='+$LoadoutCase }
$consoleCommands=@()
if($DisableNanite) { $consoleCommands+='r.Nanite 0' }
if($DisableShadows) { $consoleCommands+='ShowFlag.DynamicShadows 0' }
if($LightIsolation -eq 'direct') {$consoleCommands+='ShowFlag.DirectLighting 0'}
if($LightIsolation -eq 'sky') {$consoleCommands+='ShowFlag.SkyLighting 0'}
if($LightIsolation -eq 'indirect') {$consoleCommands+='ShowFlag.GlobalIllumination 0'}
if($LightIsolation -eq 'function') {$consoleCommands+='ShowFlag.LightFunctions 0'}
if($DisableVolumetrics) { $consoleCommands+='r.VolumetricFog 0' }
if($NoReflectionScreenTraces) { $consoleCommands+='r.Lumen.Reflections.ScreenTraces 0' }
if($FogComposition -ne -1) { $consoleCommands+='r.Fog.SeparateComposition '+$FogComposition }
if($DisableLightShafts) { $consoleCommands+='ShowFlag.LightShafts 0' }
if($DisableFogDepthCulling) { $consoleCommands+='r.VolumetricFog.ConservativeDepth 0' }
if($DisableFogHistory) { $consoleCommands+='r.VolumetricFog.TemporalReprojection 0' }
if($DisablePostProcessMaterials) { $consoleCommands+='ShowFlag.PostProcessMaterial 0' }
if($HardwareLumen) { $consoleCommands+='r.Lumen.HardwareRayTracing 1' }
if($WaterReflection -eq 'cache') { $consoleCommands+='r.Lumen.TranslucencyReflections.FrontLayer.Allow 0' }
if($WaterReflection -in @('front','hit')) { $consoleCommands+='r.Lumen.TranslucencyReflections.FrontLayer.Allow 1'; $consoleCommands+='r.Lumen.TranslucencyReflections.FrontLayer.Enable 1' }
if($WaterReflection -eq 'hit') { $consoleCommands+='r.Lumen.HardwareRayTracing.HitLighting.Allowed 1'; $consoleCommands+='r.Lumen.HardwareRayTracing.LightingMode 2' }
if($DisableLighting) { $consoleCommands+='viewmode unlit'; $consoleCommands+='ShowFlag.PostProcessing 0' }
if($BufferView -ne 'none') { $consoleCommands+='viewmode VisualizeBuffer'; $consoleCommands+='r.BufferVisualizationTarget '+$BufferView }
if($MaxFPS) { $consoleCommands+="t.MaxFPS $MaxFPS" }
if($consoleCommands.Count) { $arguments+=' -ExecCmds="'+($consoleCommands -join ',')+'"' }
if($LampOff) { $arguments+=' -DiveLampOff' }
if($LegacyExposureStart) { $arguments+=' -DiveLegacyExposureStart' }
if($ExposureProbe) { $arguments+=' -DiveExposureProbe' }
if($VegetationChecks) { $arguments+=' -DiveVegetationChecks' }
if($PlantColorProbe) { $arguments+=' -DivePlantColorProbe' }
if($GroundOff) { $arguments+=' -DiveGroundOff' }
if($LegacyRockPose) { $arguments+=' -DiveLegacyRockPose' }
if($LegacyTalus) { $arguments+=' -DiveLegacyTalus' }
if($NoPlantUnderstory) { $arguments+=' -DiveNoPlantUnderstory' }
if($UnlimitedPlants) { $arguments+=' -DiveUnlimitedPlants' }
if($LegacyFormations) { $arguments+=' -DiveLegacyFormations' }
if($FootDeposits -and $NoFootDeposits) {throw 'FootDeposits and NoFootDeposits are mutually exclusive.'}
if($FootDeposits) { $arguments+=' -DiveFootDeposits' }
if($NoFootDeposits) { $arguments+=' -DiveNoFootDeposits' }
if($ImmediateGravelBuild) { $arguments+=' -DiveImmediateGravelBuild' }
if($ImmediatePopulationBuild) { $arguments+=' -DiveImmediatePopulationBuild' }
if($FormationView) {
    if($Scenario -ne 'rocks' -or $HabitatView -or $PlantView -or $WaterView -or $GravelView) {throw 'FormationView requires rocks without other comparison modes.'}
    $arguments+=' -DiveFormationView'
}
if($HabitatView) {
    if($Scenario -ne 'rocks' -or $GravelView -or $PlantView -or $WaterView) { throw 'HabitatView requires rocks without other comparison modes.' }
    $arguments+=' -DiveHabitatView'
}
if($PSBoundParameters.ContainsKey('HabitatPose')) {
    if(-not $HabitatView -or $HabitatClose -or $HabitatSunView) {throw 'HabitatPose requires HabitatView without Close or SunView.'}
    foreach($value in $HabitatPose) {if([double]::IsNaN($value) -or [double]::IsInfinity($value)) {throw 'HabitatPose must be finite.'}}
    $arguments+=' -DiveHabitatPose='+ (($HabitatPose | ForEach-Object {$_.ToString('R',[Globalization.CultureInfo]::InvariantCulture)}) -join ',')
}
if($HabitatClose) {
    if(-not $HabitatView) {throw 'HabitatClose requires HabitatView.'}
    $arguments+=' -DiveHabitatClose'
}
if($NoRockAprons) {$arguments+=' -DiveNoRockAprons'}
if($NoRockRubble) {$arguments+=' -DiveNoRockRubble'}
if($SparseGravel) {$arguments+=' -DiveSparseGravel'}
if($LegacyRockSurface) {$arguments+=' -DiveLegacyRockSurface'}
if($NativeScattering) {$arguments+=' -DiveNativeScattering'}
if($WaterInterface) {$arguments+=' -DiveWaterInterface'}
if($BoundedWater) {$arguments+=' -DiveBoundedWater'}
if($WaterPreview) {$arguments+=' -DiveWaterPreview'}
if($DisableRayTracing) {$arguments+=' -noraytracing'}
if($PSBoundParameters.ContainsKey('WaterVolumeGain')) {$arguments+=' -DiveWaterVolumeGain='+$WaterVolumeGain.ToString([Globalization.CultureInfo]::InvariantCulture)}
if($HabitatSunView) {
    if(-not $HabitatView -or $HabitatClose) {throw 'HabitatSunView requires HabitatView without HabitatClose.'}
    $arguments+=' -DiveHabitatSunView'
}
foreach($parameter in @('FogScatterGain','ViewScatterGain')) {
    if($PSBoundParameters.ContainsKey($parameter)) {
        $value=$PSBoundParameters[$parameter].ToString([Globalization.CultureInfo]::InvariantCulture)
        $arguments+=" -Dive$parameter=$value"
    }
}
if($GravelView) {
    if($Scenario -ne 'rocks') { throw 'GravelView requires the rocks comparison scenario.' }
    $arguments+=' -DiveGravelView'
}
if($PlantView) {
    if($Scenario -ne 'rocks' -or $GravelView) { throw 'PlantView requires the rocks scenario without GravelView.' }
    $arguments+=' -DivePlantView'
}
if($WaterView) {
    if($Scenario -ne 'rocks' -or $GravelView -or $PlantView) { throw 'WaterView requires the rocks scenario without other comparisons.' }
    if($WaterStart -gt $WaterEnd) { throw 'WaterStart must not exceed WaterEnd.' }
    $arguments+=' -DiveWaterView -DiveWaterStart='+$WaterStart+' -DiveWaterEnd='+$WaterEnd
}
if(($WaterStart -or $WaterEnd -ne 9) -and -not $WaterView) { throw 'WaterStart/WaterEnd require WaterView.' }
if($WaterlinePose) {
    if(-not $WaterView -or $WaterStart -ne 0 -or $WaterEnd -ne 0) {throw 'WaterlinePose requires WaterView state 0 only.'}
    if(@($WaterlinePose | Where-Object {[double]::IsNaN($_) -or [double]::IsInfinity($_)}).Count) {throw 'WaterlinePose must be finite.'}
    $arguments+=' -DiveWaterline -DiveWaterlinePose='+ (($WaterlinePose | ForEach-Object {$_.ToString('R',[Globalization.CultureInfo]::InvariantCulture)}) -join ',')
}
if($WaterlineNoSurface) {
    if(-not $WaterlinePose) {throw 'WaterlineNoSurface requires WaterlinePose.'}
    $arguments+=' -DiveWaterlineNoSurface'
}
if($WaterlineFogDistance) {
    if(-not $WaterlinePose -or $WaterlineFogDistance -lt 2000) {throw 'WaterlineFogDistance requires WaterlinePose and at least 2000 cm.'}
    $arguments+=' -DiveWaterlineFogDistance='+$WaterlineFogDistance.ToString([Globalization.CultureInfo]::InvariantCulture)
}
if($WaterlineNoVolumeShadow) {
    if(-not $WaterlinePose) {throw 'WaterlineNoVolumeShadow requires WaterlinePose.'}
    $arguments+=' -DiveWaterlineNoVolumeShadow'
}
if($WaterlineNoFogCutoff) {
    if(-not $WaterlinePose) {throw 'WaterlineNoFogCutoff requires WaterlinePose.'}
    $arguments+=' -DiveWaterlineNoFogCutoff'
}
if($GroundView) {
    if($Scenario -ne 'shore-preview') { throw 'GroundView applies to shore-preview only.' }
    $arguments+=' -DiveGroundView'
}
if($ShoreLookBack) {
    if($Scenario -ne 'shore-preview') { throw 'ShoreLookBack applies to shore-preview only.' }
    $arguments+=' -DiveShoreLookBack'
}
if($BuildingsView) {
    if($Scenario -ne 'shore-preview' -or $ShoreLookBack) { throw 'BuildingsView requires shore-preview without ShoreLookBack.' }
    $arguments+=' -DiveBuildingsView'
}
if($SkyView) { $arguments+=' -DiveSkyView -DiveSkyYaw='+$SkyYaw.ToString([Globalization.CultureInfo]::InvariantCulture)+' -DiveSkyPitch='+$SkyPitch.ToString([Globalization.CultureInfo]::InvariantCulture) }
if($HideSky) { $arguments+=' -DiveHideSky='+$HideSky }
if($CelestialReferences) { $arguments+=' -DiveCelestialReferences' }
if($PSBoundParameters.ContainsKey('Hour')) { $arguments+=' -DiveHour='+$Hour.ToString([Globalization.CultureInfo]::InvariantCulture) }
if($Date) { $arguments+=" -DiveDate=$Date" }
$arguments+=' -DiveTorch='+$Torch+' -DiveTorchBattery='+$Battery.ToString([Globalization.CultureInfo]::InvariantCulture)
$arguments+=' -DiveParticleScale='+$ParticleScale.ToString([Globalization.CultureInfo]::InvariantCulture)
if($PopulationChecks) { $arguments+=' -DivePopulationChecks' }
if($LegacyPlantPlacement) { $arguments+=' -DiveLegacyPlantPlacement' }
if($HabitatLowView) { $arguments+=' -DiveHabitatLowView' }
if($FinInspection) { $arguments+=' -DiveFinInspection' }
if($NavShore) { $arguments+=' -DiveNavShore' }
if($NoBubbles) { $arguments+=' -DiveNoBubbles' }
if($AutomaticEnvironment) { $arguments+=' -DiveAutomaticEnvironment' }
if($PSBoundParameters.ContainsKey('Visibility')) { $arguments+=' -DiveVisibility='+$Visibility.ToString([Globalization.CultureInfo]::InvariantCulture) }
$executable="$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe"
if($Packaged) {
    if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
    $executable=Resolve-DivePackageExecutable $PackageDirectory
} else {
    $arguments=('"{0}" /Game/Diving/Maps/L_DiveTraining -game ' -f $project)+$arguments
}
$startedAt=[DateTime]::UtcNow
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit($TimeoutSeconds*1000)) { throw "Smoke test exceeded $TimeoutSeconds seconds; process $($process.Id) was left running. Inspect it and $logPath before starting another test." }
$log=Get-Content $logPath -Raw
if($process.ExitCode -ne 0 -or $log -notmatch 'DIVE_SMOKE_PASS') { throw "Gameplay smoke test failed. See $logPath." }
if($BufferView -ne 'none' -and ($log -match 'view mode not recognized|Debug viewmodes not allowed' -or $log -notmatch ('r.BufferVisualizationTarget = "'+$BufferView+'"'))) {throw 'Requested buffer visualization was not accepted.'}
if($WaterPreview -and ($log -notmatch 'DIVE_WATER_PREVIEW_READY' -or $log -notmatch 'DIVE_WATER_INTERFACE_PASS' -or $log -notmatch 'DIVE_BOUNDED_WATER_PASS')) {throw 'Water preview was not active.'}
if($SolarShaderStudy -and $log -notmatch ('DIVE_SOLAR_SHADER_STUDY_READY mapping=project-local directional_only=1 mean_plane='+[int](-not $WaveTransportStudy))) {throw 'Project-local solar shader mapping was not enabled.'}
if($SolarVolumeStudy -and $log -notmatch 'DIVE_SOLAR_VOLUME_STUDY_READY receiver=per-froxel local_lights=unchanged') {throw 'Project-local volume shader mapping was not enabled.'}
if($WaveTransportStudy -and $log -notmatch 'DIVE_WATER_SCENE_READY waves=4') {throw 'Shared water scene parameters were not uploaded.'}
if($LakeFootprint -and $log -notmatch 'DIVE_LAKE_FOOTPRINT_READY') {throw 'Lake footprint was not loaded.'}
if($LakeFootprint -and $log -notmatch ('DIVE_LAKE_SURFACE_READY enabled='+[int](-not $LakeSurfaceOff)+' materials=2')) {throw 'Lake surface coverage was not bound to both water materials.'}
if($HabitatView) {
    if($log -notmatch 'DIVE_HABITAT_INSPECTION_PASS views=3') { throw 'Habitat comparison did not finish.' }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($view in 0..2) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Habitat_0_${view}.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh habitat comparison: $view" }
    }
}
if($WaterView) {
    if($log -notmatch "DIVE_WATER_INSPECTION_PASS final_state=$WaterEnd views=3 diagnostic=1") { throw 'Water comparison did not finish.' }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($state in $WaterStart..$WaterEnd) { foreach($view in 0..2) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Water_${state}_${view}.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh water comparison: $state/$view" }
    } }
}
if($PopulationChecks -and ($log -notmatch 'DIVE_POPULATION_PASS' -or $log -match 'DIVE_POPULATION_FAIL')) { throw 'Population streaming checks failed.' }
if($Scenario -eq 'waypoints' -and ($log -notmatch 'DIVE_WAYPOINT_PASS' -or $log -match 'DIVE_WAYPOINT_FAIL')) { throw 'Waypoint test failed.' }
if($Scenario -eq 'navigation' -and ($log -notmatch 'DIVE_NAV_PASS' -or $log -match 'DIVE_NAV_FAIL|DIVE_NAV_PHASE \d+ FAIL')) { throw 'Navigation selection test failed.' }
if($Scenario -eq 'sediment' -and ($log -notmatch 'DIVE_SEDIMENT_PASS' -or $log -notmatch 'DIVE_SEDIMENT_PAUSE_PASS' -or $log -match 'DIVE_SEDIMENT_FAIL')) { throw 'Sediment lifecycle or pause failed.' }
if($log -match 'DIVE_CONTACT_FAIL') { throw "Body collision integrity failed. See $logPath." }
if($log -notmatch 'DIVE_IMAGERY_PASS') { throw "Imagery registration/content test failed. See $logPath." }
if($GroundOff -and $log -notmatch 'DIVE_GROUND_PASS materials=5 textures=7 enabled=0') { throw 'Ground comparison override missing.' }
if($log -notmatch 'DIVE_LANDSCAPE_PASS') { throw "Landscape asset test failed. See $logPath." }
if($log -match 'DIVE_COAST_READY|DIVE_COAST_FAIL') {
    if($log -notmatch 'DIVE_COAST_COLLISION_PASS' -or $log -match 'DIVE_COAST_\w*FAIL') { throw "Coast collision test failed. See $logPath." }
}
if($Scenario -eq 'coast') {
    if($log -notmatch 'DIVE_COAST_ROUTE_PASS' -or $log -notmatch 'DIVE_COAST_ROUTE_CROSSED') { throw 'Coast traversal was not completed.' }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($stage in @('Start','Crossed','Returned')) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Coast_${Entry}_$stage.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh coast screenshot: $stage" }
    }
}
# Retained older packages predate buildings. New worlds advertise their data;
# TestBuildings additionally requires this marker for its target package.
if($log -match 'DIVE_BUILDINGS_READY|DIVE_BUILDINGS_FAIL') {
    if($log -notmatch 'DIVE_BUILDINGS_PASS' -or $log -match 'DIVE_BUILDINGS_FAIL') { throw "Building registration/collision test failed. See $logPath." }
}
if($log -notmatch 'DIVE_MEDIUM_PASS') { throw "Camera medium test failed. See $logPath." }
if($WaterInterface -and $log -notmatch 'DIVE_WATER_INTERFACE_PASS layers=2') {throw 'Requested water interface was not validated.'}
if($BoundedWater) {
    $native=if($DisableVolumetrics){0}else{1}
    $gain=$WaterVolumeGain.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)
    if($log -notmatch "DIVE_BOUNDED_WATER_PASS native=$native[^\r\n]*gain=$gain") {throw 'Requested bounded water volume or fallback was not validated.'}
}
if($HardwareLumen -and $log -notmatch 'r.Lumen.HardwareRayTracing = "1"') {throw 'Hardware Lumen override was not applied.'}
if($NativeScattering) {
    $expected=if($DisableVolumetrics){'native=0 requested=1'}else{'native=1 requested=1'}
    if($log -notmatch ('DIVE_SCATTER_MODEL[^\r\n]*'+$expected)) {throw 'Requested native scattering mode or its fallback was not applied.'}
}
if($PSBoundParameters.ContainsKey('FogScatterGain') -or $PSBoundParameters.ContainsKey('ViewScatterGain')) {
    $expected='DIVE_SCATTER_MODEL fog_gain={0} view_gain={1}' -f $FogScatterGain.ToString('F3',[Globalization.CultureInfo]::InvariantCulture),$ViewScatterGain.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)
    if(-not $log.Contains($expected)) {throw 'Scattering diagnostic gains were not applied.'}
}
if($log -notmatch 'DIVE_OPTICS_PASS' -or $log -notmatch 'DIVE_LIGHT_PASS') { throw "Lighting/material stage test failed. See $logPath." }
if($PlantColorProbe) {
    foreach($mesh in @('SM_Pondweed','SM_Stonewort','SM_Milfoil')) {
        if($log -notmatch ('DIVE_PLANT_COLOR_BUFFER mesh='+$mesh+' ')) {throw 'Missing plant color-buffer inspection; use the dive scenario.'}
    }
}
if($log -notmatch 'Set CVar \[\[r.EyeAdaptation.CachedLightingPreExposure:8\]\]') { throw 'Lighting cache exposure range was not applied.' }
if($LampOff -and $Scenario -eq 'dive' -and $log -notmatch 'DIVE_TORCH_PASS on=0') { throw 'Night comparison did not switch the torch off.' }
if($Scenario -like 'surface*' -or $Scenario -eq 'shore') {
    foreach($phase in @('air','water','transition')) {
        if($log -notmatch "DIVE_MEDIUM_PASS phase=$phase") { throw "Missing optical phase: $phase" }
    }
}
if($Scenario -eq 'shore' -and $log -notmatch 'DIVE_SHORE_SURFACE') { throw 'Shore return did not reach the offshore surface waypoint.' }
if($DisableNanite -and $log -notmatch 'r.Nanite = "0"') { throw 'Nanite fallback override was not applied.' }
if($MaxFPS -and $log -notmatch ('t.MaxFPS = "'+$MaxFPS+'"')) { throw 'Requested frame limit was not applied.' }
if($Scenario -in @('logbook','reserve-loss') -and $log -notmatch 'DIVE_LOGBOOK_PASS') { throw "Logbook smoke test failed. See $logPath." }
if($Scenario -eq 'audio' -and ($log -notmatch 'DIVE_AUDIO_PASS' -or $log -notmatch 'DIVE_AUDIO_SETTINGS_PASS')) { throw "Audio smoke test failed. See $logPath." }
if($Scenario -eq 'controls' -and $log -notmatch 'DIVE_INPUT_PASS') { throw "Input smoke test failed. See $logPath." }
if($Scenario -eq 'ui') {
    if($log -notmatch 'DIVE_SMOKE_PASS ui=1 phases=14' -or $log -match 'DIVE_UI_TEXT_OVERFLOW') { throw 'UI input/layout test failed.' }
    foreach($phase in 0..27) {
        if($log -notmatch ('DIVE_UI_PHASE '+$phase+' PASS')) { throw "Missing UI phase $phase." }
    }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($stage in @('Plan_DE','Environment_DE','Loadout_DE','Loadout_EN','Plan_EN','Config_DE','Controls_DE','Standard_DE','WarningsMapTutorial_DE','Minimal_EN','Compact_EN','PauseWarnings_EN','Resumed_EN','Final_EN')) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "UI_$stage.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh UI screenshot: $stage" }
    }
}
if($Scenario -eq 'reserve-loss' -and $log -notmatch 'DIVE_RESERVE_PASS') { throw "Breath reserve depletion test failed. See $logPath." }
if($Scenario -eq 'breathing' -and $log -notmatch 'DIVE_BREATHING_PASS') { throw "Breathing-source smoke test failed. See $logPath." }
if($Scenario -eq 'tutorial' -and $log -notmatch 'DIVE_TUTORIAL_PASS') { throw "Tutorial smoke test failed. See $logPath." }
if($Scenario -eq 'surface-storm' -and $log -notmatch 'DIVE_WEATHER_PASS kind=3 wind=22.0') { throw "Storm surface test failed. See $logPath." }
if($Scenario -eq 'daycycle' -and ($log -notmatch 'DIVE_DAYCYCLE_PASS' -or $log -notmatch 'DIVE_DAYCYCLE_PAUSE_PASS')) { throw 'Day/night cycle or real pause test failed.' }
if($CelestialReferences -and $log -notmatch 'DIVE_CELESTIAL_PASS references=6') { throw 'Astronomical reference comparison failed.' }
if($Scenario -eq 'environment' -and $log -notmatch 'DIVE_ENVIRONMENT_PASS') { throw 'Automatic environment smoke test failed.' }
if($Scenario -eq 'loadout') {
    if($log -notmatch 'DIVE_LOADOUT_PASS' -or $log -notmatch 'DIVE_LOADOUT_IMPULSE_PASS' -or $log -match 'DIVE_LOADOUT_\w*FAIL') { throw 'Loadout runtime or inertia test failed.' }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($stage in @('DE','EN','Paused')) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Loadout_${LoadoutCase}_$stage.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh loadout screenshot: $stage" }
    }
}
if($Scenario -eq 'modes') {
    if($log -notmatch 'DIVE_MODE_PASS' -or $log -match 'DIVE_MODE_FAIL') { throw 'Mode runtime test failed.' }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($stage in @('Baseline','Effects','Menu')) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Mode_${ModeCase}_$stage.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh mode screenshot: $stage" }
    }
}
if($Scenario -eq 'config' -and $log -notmatch 'DIVE_CONFIG_PASS') { throw "Configuration smoke test failed. See $logPath." }
if($Scenario -eq 'avatar') {
    foreach($phase in 0..24) {
        if($log -notmatch ('DIVE_AVATAR_PHASE '+$phase+' PASS')) { throw "Missing avatar phase $phase. See $logPath." }
    }
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    foreach($stage in @('Front','Back','Swimming','Detail','FirstPerson','Computer','Paused','LookDown','LookUp','RegulatorOut','RegulatorIn')) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "Avatar_${LoadoutCase}_$stage.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh avatar screenshot: $stage" }
    }
}
if($Scenario -eq 'profiles' -and $log -notmatch 'DIVE_PROFILES_PASS') { throw "Profile smoke test failed. See $logPath." }
if($Scenario -in @('shore-preview','shore') -and $log -notmatch 'DIVE_SHORE_PASS') { throw "Shore test failed. See $logPath." }
if($Scenario -in @('controls','tutorial','profiles','environment','config','torch','surface-storm','daycycle','shore-preview','shore','breathing','reserve-loss')) {
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path (Split-Path $project -Parent) 'Saved/Screenshots' }
    $images=if($Scenario -eq 'torch') { @('DiveUnderwater','DiveSmoke') | ForEach-Object { "$($_)_${Entry}_torch" } } elseif($Scenario -eq 'daycycle') { $startHour=if($PSBoundParameters.ContainsKey('Hour')) { [int][Math]::Floor($Hour) } else { 17 }; @('Before','After') | ForEach-Object { 'DayCycle_{0}_{1:00}' -f $_,$startHour } } elseif($Scenario -eq 'surface-storm') { @('DiveUnderwater','SurfaceAir','DiveSmoke') | ForEach-Object { "$($_)_${Entry}_surface-storm" } } elseif($Scenario -eq 'environment') { @('EnvironmentWinter_DE','EnvironmentSummer_EN','EnvironmentManual_EN','EnvironmentRain_EN','EnvironmentDive_EN','EnvironmentLocked_EN') } elseif($Scenario -eq 'config') { @('ConfigPressure_DE','ConfigRange_DE','ConfigTime_DE','ConfigValues_DE','ConfigAir_EN','ConfigValues_EN','ConfigDate_EN','ConfigDateValues_EN','ConfigSpringGap_EN','ConfigTorch_EN') } elseif($Scenario -eq 'reserve-loss') { @('Reserve_Short','Reserve_Low','Reserve_Summary','Logbook_0','Logbook_1','Logbook_2','Logbook_3') } elseif($Scenario -eq 'breathing') { @('Breathing_NoSupply','Breathing_Air') } elseif($Scenario -eq 'controls') { @('Controls_DE','Controls_EN') } elseif($Scenario -eq 'profiles') { @('ProfileName','Profiles_DE','ProfileDuplicate','ProfilesArchive','ProfilesActive','Profiles_EN') } elseif($Scenario -eq 'shore-preview') { @(if($BuildingsView) { "Shore_${Entry}_Buildings" } elseif($GroundView) { "Shore_${Entry}_Ground" } elseif($ShoreLookBack) { "Shore_${Entry}_Land" } else { "Shore_$Entry" }) } elseif($Scenario -eq 'shore') { @('ShoreStart','ShoreWater','ShoreDive','ShoreReturn','ShoreComplete','ShoreMenu') | ForEach-Object { "$($_)_$Entry" } } else { (0..7 | ForEach-Object { "Tutorial_$_" })+@('TutorialMap','TutorialComplete') }
    foreach($name in $images) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "$name.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh screenshot: $name" }
    }
}
if($log -match 'unbound parameters|DIVE_WAVE_RENDERER_UNAVAILABLE|Failed to compile Material|Default Material will be used|WALENSEE_DATA_FAILED|DIVE_ECOLOGY_FAIL|DIVE_EQUIPMENT_FAIL|DIVE_WATER_FAIL|DIVE_MEDIUM_FAIL|DIVE_LIGHT_FAIL|DIVE_OPTICS_FAIL|DIVE_AUDIO_FAIL|DIVE_ACCESS_FAIL|DIVE_BREATHING_FAIL|DIVE_RESERVE_FAIL|DIVE_ENVIRONMENT_FAIL|DIVE_CONFIG_FAIL|DIVE_WEATHER_FAIL|DIVE_DAYCYCLE_FAIL|DIVE_CELESTIAL_FAIL|DIVE_PARTICLES_FAIL') {
    throw "Runtime content validation failed. See $logPath."
}
($log -split "`n") | Where-Object { $_ -match 'DIVE_SMOKE_|DIVE_EQUIPMENT_CHECK|DIVE_LOGBOOK_|DIVE_AUDIO_|DIVE_INPUT_|DIVE_TUTORIAL_|DIVE_PROFILE|DIVE_SHORE_|DIVE_ACCESS_|DIVE_BREATHING_|DIVE_RESERVE_|DIVE_ENVIRONMENT_|DIVE_CONFIG_|DIVE_WEATHER_|DIVE_DAYCYCLE_|DIVE_CELESTIAL_' }
