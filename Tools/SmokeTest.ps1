param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8', [switch]$Packaged,
    [ValidateRange(0,2)][int]$Entry=0, [ValidateSet('dive','surface','surface-calm','surface-rain','stream','wrist','wrist-night','logbook','audio','controls','tutorial','profiles','config','shore-preview','shore','breathing','reserve-loss')][string]$Scenario='dive',
    [string]$PackageDirectory='', [switch]$DisableNanite, [switch]$LampOff,
    [ValidateRange(0,240)][int]$MaxFPS=0,
    [ValidateRange(0,23.99)][double]$Hour)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
$logPath=Join-Path $repo 'artifacts/DiveSmoke.log'
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$arguments='-DiveSmokeTest -windowed -ResX=1600 -ResY=900 -nosplash -unattended -abslog="{0}"' -f $logPath
$arguments+=' -DiveEntry={0} -DiveScenario={1}' -f $Entry,$Scenario
$consoleCommands=@()
if($DisableNanite) { $consoleCommands+='r.Nanite 0' }
if($MaxFPS) { $consoleCommands+="t.MaxFPS $MaxFPS" }
if($consoleCommands.Count) { $arguments+=' -ExecCmds="'+($consoleCommands -join ',')+'"' }
if($LampOff) { $arguments+=' -DiveLampOff' }
if($PSBoundParameters.ContainsKey('Hour')) { $arguments+=' -DiveHour='+$Hour.ToString([Globalization.CultureInfo]::InvariantCulture) }
$executable="$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe"
if($Packaged) {
    if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
    $executable=Join-Path $PackageDirectory 'divingsim/Binaries/Win64/divingsim.exe'
} else {
    $arguments=('"{0}" /Game/Diving/Maps/L_DiveTraining -game ' -f $project)+$arguments
}
$startedAt=[DateTime]::UtcNow
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)) { throw 'Smoke test exceeded three minutes; inspect the running process and DiveSmoke.log.' }
$log=Get-Content $logPath -Raw
if($process.ExitCode -ne 0 -or $log -notmatch 'DIVE_SMOKE_PASS') { throw "Gameplay smoke test failed. See $logPath." }
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
if($DisableNanite -and $log -notmatch 'r.Nanite = "0"') { throw 'Nanite fallback override was not applied.' }
if($MaxFPS -and $log -notmatch ('t.MaxFPS = "'+$MaxFPS+'"')) { throw 'Requested frame limit was not applied.' }
if($Scenario -in @('logbook','reserve-loss') -and $log -notmatch 'DIVE_LOGBOOK_PASS') { throw "Logbook smoke test failed. See $logPath." }
if($Scenario -eq 'audio' -and ($log -notmatch 'DIVE_AUDIO_PASS' -or $log -notmatch 'DIVE_AUDIO_SETTINGS_PASS')) { throw "Audio smoke test failed. See $logPath." }
if($Scenario -eq 'controls' -and $log -notmatch 'DIVE_INPUT_PASS') { throw "Input smoke test failed. See $logPath." }
if($Scenario -eq 'reserve-loss' -and $log -notmatch 'DIVE_RESERVE_PASS') { throw "Breath reserve depletion test failed. See $logPath." }
if($Scenario -eq 'breathing' -and $log -notmatch 'DIVE_BREATHING_PASS') { throw "Breathing-source smoke test failed. See $logPath." }
if($Scenario -eq 'tutorial' -and $log -notmatch 'DIVE_TUTORIAL_PASS') { throw "Tutorial smoke test failed. See $logPath." }
if($Scenario -eq 'config' -and $log -notmatch 'DIVE_CONFIG_PASS') { throw "Configuration smoke test failed. See $logPath." }
if($Scenario -eq 'profiles' -and $log -notmatch 'DIVE_PROFILES_PASS') { throw "Profile smoke test failed. See $logPath." }
if($Scenario -in @('shore-preview','shore') -and $log -notmatch 'DIVE_SHORE_PASS') { throw "Shore test failed. See $logPath." }
if($Scenario -in @('controls','tutorial','profiles','config','shore-preview','shore','breathing','reserve-loss')) {
    $saved=if($Packaged) { Join-Path $PackageDirectory 'divingsim/Saved/Screenshots' } else { Join-Path $repo 'divingsim/Saved/Screenshots' }
    $images=if($Scenario -eq 'config') { @('ConfigPressure_DE','ConfigRange_DE','ConfigTime_DE','ConfigValues_DE','ConfigAir_EN','ConfigValues_EN') } elseif($Scenario -eq 'reserve-loss') { @('Reserve_Short','Reserve_Low','Reserve_Summary','Logbook_0','Logbook_1','Logbook_2','Logbook_3') } elseif($Scenario -eq 'breathing') { @('Breathing_NoSupply','Breathing_Air') } elseif($Scenario -eq 'controls') { @('Controls_DE','Controls_EN') } elseif($Scenario -eq 'profiles') { @('ProfileName','Profiles_DE','ProfileDuplicate','ProfilesArchive','ProfilesActive','Profiles_EN') } elseif($Scenario -eq 'shore-preview') { @("Shore_$Entry") } elseif($Scenario -eq 'shore') { @('ShoreStart','ShoreWater','ShoreDive','ShoreReturn','ShoreComplete','ShoreMenu') | ForEach-Object { "$($_)_$Entry" } } else { (0..7 | ForEach-Object { "Tutorial_$_" })+@('TutorialMap','TutorialComplete') }
    foreach($name in $images) {
        $capture=Get-Item -LiteralPath (Join-Path $saved "$name.png")
        if($capture.LastWriteTimeUtc -lt $startedAt -or $capture.Length -lt 10000) { throw "Missing fresh screenshot: $name" }
    }
}
if($log -match 'Failed to compile Material|Default Material will be used|WALENSEE_DATA_FAILED|DIVE_ECOLOGY_FAIL|DIVE_EQUIPMENT_FAIL|DIVE_WATER_FAIL|DIVE_MEDIUM_FAIL|DIVE_LIGHT_FAIL|DIVE_OPTICS_FAIL|DIVE_AUDIO_FAIL|DIVE_ACCESS_FAIL|DIVE_BREATHING_FAIL|DIVE_RESERVE_FAIL|DIVE_CONFIG_FAIL') {
    throw "Runtime content validation failed. See $logPath."
}
($log -split "`n") | Where-Object { $_ -match 'DIVE_SMOKE_|DIVE_EQUIPMENT_CHECK|DIVE_LOGBOOK_|DIVE_AUDIO_|DIVE_INPUT_|DIVE_TUTORIAL_|DIVE_PROFILE|DIVE_SHORE_|DIVE_ACCESS_|DIVE_BREATHING_|DIVE_RESERVE_|DIVE_CONFIG_'  }
