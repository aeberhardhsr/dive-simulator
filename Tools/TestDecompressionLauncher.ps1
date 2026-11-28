$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$child=[IO.Path]::GetFullPath((Join-Path $repo 'Builds/WindowsDecompression/divingsim/Binaries/Win64/divingsim-Decompression.exe'))
$log=Join-Path $repo 'artifacts/decompression-bootstrap.log'
if(Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -eq $child }) { throw 'The package is already running.' }
$started=[DateTime]::UtcNow
$arguments='-DiveSmokeTest -DiveScenario=dive -DiveEntry=1 -DiveAutomaticEnvironment -DiveDate=2026-08-15 -saveddirsuffix=decompressionbootstrap -windowed -ResX=1600 -ResY=900 -nosplash -unattended -ExecCmds="t.MaxFPS 30" -abslog="'+$log+'"'
$launcher=Start-Process -FilePath (Join-Path $repo 'Builds/WindowsDecompression/divingsim.exe') -ArgumentList $arguments -WindowStyle Hidden -PassThru
$found=$null
for($i=0;$i -lt 30 -and -not $found;++$i) {
    $found=Get-CimInstance Win32_Process | Where-Object { $_.ExecutablePath -eq $child -and $_.ParentProcessId -eq $launcher.Id }
    if(-not $found) { Start-Sleep -Milliseconds 250 }
}
if(-not $found -or @($found).Count -ne 1) { throw 'Root launcher did not start the expected named executable.' }
$game=[Diagnostics.Process]::GetProcessById($found.ProcessId)
# Retain a process handle before exit; otherwise a discovered child can lose
# its exit-code query when Windows removes it from the process table.
$null=$game.Handle
if(-not $game.WaitForExit(120000)) { throw 'Root launcher test exceeded its time limit.' }
$text=Get-Content -LiteralPath $log -Raw
if($game.ExitCode -ne 0 -or $text -notmatch 'DIVE_SMOKE_PASS' -or $text -match 'DIVE_\w+_FAIL') { throw ('Root launcher gameplay failed: exit='+$game.ExitCode) }
@{status='PASS';child=$child;child_id=$found.ProcessId;exit_code=$game.ExitCode;started_utc=$started.ToString('o');finished_utc=[DateTime]::UtcNow.ToString('o');log_sha256=(Get-FileHash -LiteralPath $log).Hash;executable_sha256=(Get-FileHash -LiteralPath $child).Hash} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $repo 'artifacts/decompression-bootstrap.json') -Encoding utf8
Get-Content -LiteralPath (Join-Path $repo 'artifacts/decompression-bootstrap.json')
