param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
$logPath=Join-Path $repo 'artifacts/ControlsTests.log'
$reportPath=Join-Path $repo 'artifacts/ControlsTests'
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$arguments='"{0}" -unattended -nullrhi -nosound -nosplash -ExecCmds="Automation RunTest DiveSim.Controls;Quit" -ReportExportPath="{1}" -abslog="{2}"' -f $project,$reportPath,$logPath
$startedAt=[DateTime]::UtcNow
$process=Start-Process -FilePath "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)) { throw 'Control tests exceeded three minutes; inspect the running process and log.' }
$reportFile=Get-Item (Join-Path $reportPath 'index.json')
if($reportFile.LastWriteTimeUtc -lt $startedAt) { throw 'No fresh control test report was produced.' }
$report=Get-Content $reportFile.FullName -Raw | ConvertFrom-Json
if($process.ExitCode -ne 0 -or $report.succeeded -ne 2 -or $report.failed -ne 0) { throw "Control tests failed. See $logPath." }
$report.tests | Select-Object fullTestPath,state
