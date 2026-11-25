param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8', [switch]$Packaged,
    [ValidateRange(0,2)][int]$Entry=0, [ValidateSet('dive','surface','surface-calm','surface-rain','stream','wrist','wrist-night','logbook')][string]$Scenario='dive',
    [string]$PackageDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
$logPath=Join-Path $repo 'artifacts/DiveSmoke.log'
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$arguments='-DiveSmokeTest -windowed -ResX=1600 -ResY=900 -nosplash -unattended -abslog="{0}"' -f $logPath
$arguments+=' -DiveEntry={0} -DiveScenario={1}' -f $Entry,$Scenario
$executable="$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe"
if($Packaged) {
    if(-not $PackageDirectory) { $PackageDirectory=Join-Path $repo 'Builds/Windows' }
    $executable=Join-Path $PackageDirectory 'divingsim/Binaries/Win64/divingsim.exe'
} else {
    $arguments=('"{0}" /Game/Diving/Maps/L_DiveTraining -game ' -f $project)+$arguments
}
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)) { throw 'Smoke test exceeded three minutes; inspect the running process and DiveSmoke.log.' }
$log=Get-Content $logPath -Raw
if($process.ExitCode -ne 0 -or $log -notmatch 'DIVE_SMOKE_PASS') { throw "Gameplay smoke test failed. See $logPath." }
if($Scenario -eq 'logbook' -and $log -notmatch 'DIVE_LOGBOOK_PASS') { throw "Logbook smoke test failed. See $logPath." }
if($log -match 'Failed to compile Material|Default Material will be used|WALENSEE_DATA_FAILED|DIVE_ECOLOGY_FAIL|DIVE_EQUIPMENT_FAIL|DIVE_WATER_FAIL') {
    throw "Runtime content validation failed. See $logPath."
}
($log -split "`n") | Where-Object { $_ -match 'DIVE_SMOKE_|DIVE_EQUIPMENT_CHECK|DIVE_LOGBOOK_' }
