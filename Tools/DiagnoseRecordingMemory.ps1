param(
    [string]$EngineRoot='O:/games/dive game/Builds/UE584',
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='SourceEditorSandbox',
    [string]$ResultDirectory='',
    [ValidateRange(60,3600)][int]$TimeoutSeconds=900
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not $ResultDirectory) { $ResultDirectory=Join-Path $repo ('artifacts/recording-memory-'+(Get-Date -Format 'yyyyMMdd-HHmmss')) }
if(-not [IO.Path]::IsPathRooted($ResultDirectory)) { $ResultDirectory=Join-Path $repo $ResultDirectory }
$ResultDirectory=[IO.Path]::GetFullPath($ResultDirectory)
if(Test-Path -LiteralPath $ResultDirectory) { throw 'Use a fresh diagnostic output directory.' }
$runtime=Join-Path $repo "Builds/$SandboxName/divingsim"
$project=Join-Path $runtime 'divingsim.uproject'
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$dll=Join-Path $runtime 'Binaries/Win64/UnrealEditor-DiveSim.dll'
foreach($file in @($project,$editor,$dll)) { if(-not (Test-Path -LiteralPath $file)) { throw "Missing diagnostic prerequisite: $file" } }
if(Get-Process -Name divingsim,UnrealEditor,UnrealEditor-Cmd,UnrealBuildTool -ErrorAction SilentlyContinue) {
    throw 'Another game, editor or build is active. Finish it before this isolated diagnostic.'
}
New-Item -ItemType Directory -Path $ResultDirectory | Out-Null
$inputs=@()
foreach($name in @('DiveRecording.cpp','DiveRecording.h','DiveLogbook.cpp','DiveRecordingMemoryDiagnostic.cpp')) {
    $source=Join-Path $repo "divingsim/Source/DiveSim/$name"
    $copy=Join-Path $runtime "Source/DiveSim/$name"
    $hash=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash
    if($hash -ne (Get-FileHash -LiteralPath $copy -Algorithm SHA256).Hash) { throw "Sandbox source differs: $name" }
    $inputs+=@{file=$source;sha256=$hash}
}
$identity=@{kind='accelerated_nullrhi_recording_diagnostic';package_acceptance=$false;
    source_commit=(git -C $repo rev-parse HEAD);inputs=$inputs;
    editor=@{file=$editor;sha256=(Get-FileHash -LiteralPath $editor).Hash};
    module=@{file=$dll;sha256=(Get-FileHash -LiteralPath $dll).Hash};runs=@()}
foreach($mode in @('control','writer')) {
    $run=Join-Path $ResultDirectory $mode
    New-Item -ItemType Directory -Path $run | Out-Null
    $log=Join-Path $run 'Runtime.log'
    $report=Join-Path $run 'Report'
    $measurements=Join-Path $run 'Measurements'
    $arguments='"{0}" -unattended -nullrhi -nosound -nosplash -ExecCmds="Automation RunTest DiveDiagnostics.RecordingMemory;Quit" -DiveRecordingMemory={1} -DiveRecordingMemoryOutput="{2}" -ReportExportPath="{3}" -abslog="{4}"' -f $project,$mode,$measurements,$report,$log
    $started=[DateTime]::UtcNow
    $process=Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $entry=@{mode=$mode;process_id=$process.Id;started_utc=$started.ToString('o');log=$log;arguments=$arguments}
    $identity.runs+=,$entry
    $identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $ResultDirectory 'Host.json') -Encoding UTF8
    Write-Output "RECORDING_MEMORY_STARTED mode=$mode pid=$($process.Id) log=$log"
    if(-not $process.WaitForExit($TimeoutSeconds*1000)) {
        throw "Diagnostic exceeded observation limit; PID $($process.Id) remains running. Inspect $log before another run."
    }
    $entry.exit_code=$process.ExitCode
    $entry.finished_utc=[DateTime]::UtcNow.ToString('o')
    $identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $ResultDirectory 'Host.json') -Encoding UTF8
    $index=Get-Item -LiteralPath (Join-Path $report 'index.json')
    $result=Get-Content -LiteralPath $index.FullName -Raw | ConvertFrom-Json
    if($index.LastWriteTimeUtc -lt $started -or $process.ExitCode -ne 0 -or $result.succeeded -ne 1 -or $result.failed -ne 0) {
        throw "Recording diagnostic failed: $log"
    }
    $csv=Join-Path $measurements 'Memory.csv'
    $rows=@(Import-Csv -LiteralPath $csv)
    if($rows.Count -ne 362 -or @($rows | Where-Object phase -eq 'grow').Count -ne 240 -or
        @($rows | Where-Object phase -eq 'hold').Count -ne 60 -or @($rows | Where-Object phase -eq 'reset').Count -ne 60 -or
        $rows[240].samples -ne '7201' -or $rows[300].samples -ne '7201' -or $rows[360].samples -ne '31' -or
        $rows[361].samples -ne '0') { throw 'Incomplete recording memory measurements.' }
    $entry.measurements=@{file=$csv;sha256=(Get-FileHash -LiteralPath $csv).Hash}
    Write-Output "RECORDING_MEMORY_FINISHED mode=$mode rows=$($rows.Count)"
}
if($identity.module.sha256 -ne (Get-FileHash -LiteralPath $dll).Hash) { throw 'Diagnostic module changed during measurement.' }
$identity.status='DIAGNOSTIC_COMPLETE_REQUIRES_INTERPRETATION'
$identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $ResultDirectory 'Host.json') -Encoding UTF8
Write-Output "RECORDING_MEMORY_REVIEW $ResultDirectory"
