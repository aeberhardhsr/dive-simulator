param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8',[switch]$EditorSandbox,
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='EditorSandbox',
    [string]$ResultDirectory='',[ValidateRange(30,7200)][int]$TimeoutSeconds=180)
$ErrorActionPreference='Stop'
if($SandboxName -ne 'EditorSandbox' -and -not $EditorSandbox) {throw 'SandboxName requires EditorSandbox.'}
$repo=Split-Path $PSScriptRoot -Parent
$project=Join-Path $repo 'divingsim/divingsim.uproject'
if($EditorSandbox) { $project=Join-Path $repo "Builds/$SandboxName/divingsim/divingsim.uproject" }
$logPath=Join-Path $repo 'artifacts/DecompressionAutomation.log'
$reportPath=Join-Path $repo 'artifacts/DecompressionAutomation'
if($ResultDirectory) {
    if(-not [IO.Path]::IsPathRooted($ResultDirectory)) {$ResultDirectory=Join-Path $repo $ResultDirectory}
    $logPath=Join-Path $ResultDirectory 'Runtime.log'
    $reportPath=Join-Path $ResultDirectory 'Report'
}
New-Item -ItemType Directory -Path (Split-Path $logPath) -Force | Out-Null
$arguments='"{0}" -unattended -nullrhi -nosound -nosplash -ExecCmds="Automation RunTest DiveSim;Quit" -ReportExportPath="{1}" -abslog="{2}"' -f $project,$reportPath,$logPath
$startedAt=[DateTime]::UtcNow
$process=Start-Process -FilePath "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit($TimeoutSeconds*1000)) { throw "Native tests exceeded $TimeoutSeconds seconds. Process $($process.Id) was left running; inspect $logPath before starting another test." }
$reportFile=Get-Item (Join-Path $reportPath 'index.json')
if($reportFile.LastWriteTimeUtc -lt $startedAt) { throw 'No fresh native test report was produced.' }
$report=Get-Content $reportFile.FullName -Raw | ConvertFrom-Json
if($process.ExitCode -ne 0 -or $report.succeeded -ne 35 -or $report.failed -ne 0) { throw "Native tests failed. See $logPath." }
$report.tests | Select-Object fullTestPath,state
