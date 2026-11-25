param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8', [switch]$Packaged, [switch]$EditorSandbox,
    [ValidateRange(0,2)][int]$Entry=0, [ValidateSet('dive','surface','surface-calm','surface-rain','surface-storm','daycycle','stream','wrist','wrist-night','torch','logbook','audio','controls','tutorial','profiles','environment','config','shore-preview','shore','breathing','reserve-loss','modes','loadout','avatar','sediment','navigation')][string]$Scenario='dive',
    [ValidateRange(0,5)][int]$LoadoutCase=0,
    [ValidateSet('exploration','manual','simulation','hardcore','warm','hypoxia')][string]$ModeCase='simulation',
    [string]$PackageDirectory='', [switch]$DisableNanite, [switch]$LampOff, [switch]$CelestialReferences, [switch]$SkyView,
    [ValidatePattern('^[a-z,]*$')][string]$HideSky='',
    [ValidateRange(-180,360)][double]$SkyYaw=180, [ValidateRange(-85,85)][double]$SkyPitch=45,
    [ValidateRange(0,240)][int]$MaxFPS=0,
    [ValidateSet('1600x900','1920x1080','2560x1440','3840x2160','1280x1024')][string]$Viewport='1600x900',
    [ValidateRange(0,2)][int]$Torch=0,
    [ValidateRange(2,40)][double]$Visibility=14,
    [ValidateRange(0,100)][double]$Battery=100,
    [ValidateRange(0,30)][double]$ParticleScale=1,
    [switch]$NavShore, [switch]$NoBubbles, [switch]$AutomaticEnvironment, [string]$RuntimeLog='',
    [ValidateRange(0,23.99)][double]$Hour,
    [ValidatePattern('^[0-9]{4}-[0-9]{2}-[0-9]{2}$')][string]$Date='')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
if($EditorSandbox) { $project=Join-Path $repo 'Builds/EditorSandbox/divingsim/divingsim.uproject' }
$logPath=if($RuntimeLog) { [IO.Path]::GetFullPath($RuntimeLog) } else { Join-Path $repo 'artifacts/DiveSmoke.log' }
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$size=$Viewport.Split('x')
$arguments='-DiveSmokeTest -windowed -ForceRes -ResX={1} -ResY={2} -nosplash -unattended -abslog="{0}"' -f $logPath,$size[0],$size[1]
$arguments+=' -DiveEntry={0} -DiveScenario={1}' -f $Entry,$Scenario
if($Scenario -eq 'modes') { $arguments+=' -DiveModeCase='+$ModeCase }
if($Scenario -in @('loadout','avatar')) { $arguments+=' -DiveLoadoutCase='+$LoadoutCase }
$consoleCommands=@()
if($DisableNanite) { $consoleCommands+='r.Nanite 0' }
if($MaxFPS) { $consoleCommands+="t.MaxFPS $MaxFPS" }
if($consoleCommands.Count) { $arguments+=' -ExecCmds="'+($consoleCommands -join ',')+'"' }
if($LampOff) { $arguments+=' -DiveLampOff' }
if($SkyView) { $arguments+=' -DiveSkyView -DiveSkyYaw='+$SkyYaw.ToString([Globalization.CultureInfo]::InvariantCulture)+' -DiveSkyPitch='+$SkyPitch.ToString([Globalization.CultureInfo]::InvariantCulture) }
if($HideSky) { $arguments+=' -DiveHideSky='+$HideSky }
if($CelestialReferences) { $arguments+=' -DiveCelestialReferences' }
if($PSBoundParameters.ContainsKey('Hour')) { $arguments+=' -DiveHour='+$Hour.ToString([Globalization.CultureInfo]::InvariantCulture) }
if($Date) { $arguments+=" -DiveDate=$Date" }
$arguments+=' -DiveTorch='+$Torch+' -DiveTorchBattery='+$Battery.ToString([Globalization.CultureInfo]::InvariantCulture)
$arguments+=' -DiveParticleScale='+$ParticleScale.ToString([Globalization.CultureInfo]::InvariantCulture)
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
if(-not $process.WaitForExit(180000)) { throw 'Smoke test exceeded three minutes; inspect the running process and DiveSmoke.log.' }
$log=Get-Content $logPath -Raw
if($process.ExitCode -ne 0 -or $log -notmatch 'DIVE_SMOKE_PASS') { throw "Gameplay smoke test failed. See $logPath." }
if($Scenario -eq 'navigation' -and ($log -notmatch 'DIVE_NAV_PASS' -or $log -match 'DIVE_NAV_FAIL|DIVE_NAV_PHASE \d+ FAIL')) { throw 'Navigation selection test failed.' }
if($Scenario -eq 'sediment' -and ($log -notmatch 'DIVE_SEDIMENT_PASS' -or $log -notmatch 'DIVE_SEDIMENT_PAUSE_PASS' -or $log -match 'DIVE_SEDIMENT_FAIL')) { throw 'Sediment lifecycle or pause failed.' }
if($log -match 'DIVE_CONTACT_FAIL') { throw "Body collision integrity failed. See $logPath." }
if($log -notmatch 'DIVE_IMAGERY_PASS') { throw "Imagery registration/content test failed. See $logPath." }
if($log -notmatch 'DIVE_LANDSCAPE_PASS') { throw "Landscape asset test failed. See $logPath." }
if($log -notmatch 'DIVE_MEDIUM_PASS') { throw "Camera medium test failed. See $logPath." }
if($log -notmatch 'DIVE_OPTICS_PASS' -or $log -notmatch 'DIVE_LIGHT_PASS') { throw "Lighting/material stage test failed. See $logPath." }
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
    $images=if($Scenario -eq 'torch') { @('DiveUnderwater','DiveSmoke') | ForEach-Object { "$($_)_${Entry}_torch" } } elseif($Scenario -eq 'daycycle') { $startHour=if($PSBoundParameters.ContainsKey('Hour')) { [int][Math]::Floor($Hour) } else { 17 }; @('Before','After') | ForEach-Object { 'DayCycle_{0}_{1:00}' -f $_,$startHour } } elseif($Scenario -eq 'surface-storm') { @('DiveUnderwater','SurfaceAir','DiveSmoke') | ForEach-Object { "$($_)_${Entry}_surface-storm" } } elseif($Scenario -eq 'environment') { @('EnvironmentWinter_DE','EnvironmentSummer_EN','EnvironmentManual_EN','EnvironmentRain_EN','EnvironmentDive_EN','EnvironmentLocked_EN') } elseif($Scenario -eq 'config') { @('ConfigPressure_DE','ConfigRange_DE','ConfigTime_DE','ConfigValues_DE','ConfigAir_EN','ConfigValues_EN','ConfigDate_EN','ConfigDateValues_EN','ConfigSpringGap_EN','ConfigTorch_EN') } elseif($Scenario -eq 'reserve-loss') { @('Reserve_Short','Reserve_Low','Reserve_Summary','Logbook_0','Logbook_1','Logbook_2','Logbook_3') } elseif($Scenario -eq 'breathing') { @('Breathing_NoSupply','Breathing_Air') } elseif($Scenario -eq 'controls') { @('Controls_DE','Controls_EN') } elseif($Scenario -eq 'profiles') { @('ProfileName','Profiles_DE','ProfileDuplicate','ProfilesArchive','ProfilesActive','Profiles_EN') } elseif($Scenario -eq 'shore-preview') { @("Shore_$Entry") } elseif($Scenario -eq 'shore') { @('ShoreStart','ShoreWater','ShoreDive','ShoreReturn','ShoreComplete','ShoreMenu') | ForEach-Object { "$($_)_$Entry" } } else { (0..7 | ForEach-Object { "Tutorial_$_" })+@('TutorialMap','TutorialComplete') }
    foreach($name in $images) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "$name.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh screenshot: $name" }
    }
}
if($log -match 'Failed to compile Material|Default Material will be used|WALENSEE_DATA_FAILED|DIVE_ECOLOGY_FAIL|DIVE_EQUIPMENT_FAIL|DIVE_WATER_FAIL|DIVE_MEDIUM_FAIL|DIVE_LIGHT_FAIL|DIVE_OPTICS_FAIL|DIVE_AUDIO_FAIL|DIVE_ACCESS_FAIL|DIVE_BREATHING_FAIL|DIVE_RESERVE_FAIL|DIVE_ENVIRONMENT_FAIL|DIVE_CONFIG_FAIL|DIVE_WEATHER_FAIL|DIVE_DAYCYCLE_FAIL|DIVE_CELESTIAL_FAIL|DIVE_PARTICLES_FAIL') {
    throw "Runtime content validation failed. See $logPath."
}
($log -split "`n") | Where-Object { $_ -match 'DIVE_SMOKE_|DIVE_EQUIPMENT_CHECK|DIVE_LOGBOOK_|DIVE_AUDIO_|DIVE_INPUT_|DIVE_TUTORIAL_|DIVE_PROFILE|DIVE_SHORE_|DIVE_ACCESS_|DIVE_BREATHING_|DIVE_RESERVE_|DIVE_ENVIRONMENT_|DIVE_CONFIG_|DIVE_WEATHER_|DIVE_DAYCYCLE_|DIVE_CELESTIAL_' }
