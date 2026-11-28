param([Parameter(Mandatory=$true)][string]$PackageDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory,[switch]$WaterPreview,[switch]$LandscapePilot)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$OutputDirectory=[IO.Path]::GetFullPath($OutputDirectory)
$child=[IO.Path]::GetFullPath((Resolve-DivePackageExecutable $PackageDirectory))
if(Get-CimInstance Win32_Process | Where-Object {$_.ExecutablePath -eq $child}) {throw 'The package is already running.'}
New-Item -ItemType Directory -Force $OutputDirectory | Out-Null
$log=Join-Path $OutputDirectory 'Launcher.log'
$started=[DateTime]::UtcNow
$arguments='-DiveSmokeTest -DiveScenario=dive -DiveEntry=1 -DiveAutomaticEnvironment -DiveDate=2026-08-15 -saveddirsuffix=launchercheck -windowed -ResX=1600 -ResY=900 -nosplash -unattended -ExecCmds="t.MaxFPS 30" -abslog="'+$log+'"'
if($WaterPreview) {$arguments+=' -DiveWaterPreview'}
if($LandscapePilot) {$arguments+=' -DiveLandscapePilot'}
$launcher=Start-Process (Join-Path $PackageDirectory 'divingsim.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
$found=$null
for($i=0;$i -lt 30 -and -not $found;++$i) {
    $found=Get-CimInstance Win32_Process | Where-Object {$_.ExecutablePath -eq $child -and $_.ParentProcessId -eq $launcher.Id}
    if(-not $found) {Start-Sleep -Milliseconds 250}
}
if(-not $found -or @($found).Count -ne 1) {throw 'Launcher did not start the expected package executable.'}
$game=[Diagnostics.Process]::GetProcessById($found.ProcessId)
$null=$game.Handle
if(-not $game.WaitForExit(120000)) {throw 'Launcher test exceeded its time limit.'}
$text=Get-Content -LiteralPath $log -Raw
if($game.ExitCode -ne 0 -or $text -notmatch 'DIVE_SMOKE_PASS' -or $text -match 'DIVE_\w*FAIL|Fatal error:') {throw 'Root launcher gameplay failed.'}
if($WaterPreview -and $text -notmatch 'DIVE_WATER_PREVIEW_READY') {throw 'Root launcher did not activate the water preview.'}
if($LandscapePilot -and $text -notmatch 'DIVE_LAND_PILOT_READY tiles=2 spacing=2m root_pages=1330 budget_pages=2048 cooked=1') {throw 'Root launcher did not activate the cooked terrain pilot.'}
@{status='PASS';landscape_pilot=[bool]$LandscapePilot;child=$child;child_id=$found.ProcessId;exit_code=$game.ExitCode;started_utc=$started.ToString('o');finished_utc=[DateTime]::UtcNow.ToString('o');
    log_sha256=(Get-FileHash -LiteralPath $log).Hash;executable_sha256=(Get-FileHash -LiteralPath $child).Hash} |
    ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'Launcher.json') -Encoding utf8
Write-Output "LAUNCHER_PASS=$OutputDirectory"
