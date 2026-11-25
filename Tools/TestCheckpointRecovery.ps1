param([string]$PackageDirectory='Builds/WindowsRecovery')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$PackageDirectory=[IO.Path]::GetFullPath($PackageDirectory)
$executable=Join-Path $PackageDirectory 'divingsim/Binaries/Win64/divingsim.exe'
$saved=Join-Path $PackageDirectory 'divingsim/Saved'
$output=Join-Path $repo ('artifacts/checkpoint-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $output -Force | Out-Null
$captureLog=Join-Path $output 'Capture.log'
$arguments='-DiveSmokeTest -DiveScenario=stream -windowed -ResX=1600 -ResY=900 -nosplash -unattended -abslog="{0}"' -f $captureLog
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    $deadline=[DateTime]::UtcNow.AddSeconds(130)
    $checkpoint=$null
    while([DateTime]::UtcNow -lt $deadline) {
        $process.Refresh()
        if($process.HasExited) { throw 'Capture process exited before the second checkpoint.' }
        if(Test-Path -LiteralPath $captureLog) {
            $log=Get-Content -LiteralPath $captureLog -Raw
            $checkpoints=[regex]::Matches($log,'DIVE_LOG_SAVED status=in_progress seconds=([\d.]+) samples=(\d+) path=([^\r\n]+)')
            if($checkpoints.Count -ge 2) { $checkpoint=$checkpoints[$checkpoints.Count-1]; break }
        }
        Start-Sleep -Milliseconds 500
    }
    if(-not $checkpoint) { throw 'No second checkpoint within 130 seconds.' }
    # Terminate only the exact process created above, without EndPlay/final save.
    $process.Kill()
    if(-not $process.WaitForExit(10000)) { throw 'Capture process did not terminate.' }
} finally {
    $process.Refresh()
    if(-not $process.HasExited) { $process.Kill(); $process.WaitForExit(10000) | Out-Null }
}
$path=[IO.Path]::GetFullPath((Join-Path (Split-Path $executable) $checkpoint.Groups[3].Value.Trim()))
$logsRoot=[IO.Path]::GetFullPath((Join-Path $saved 'DiveLogs'))+[IO.Path]::DirectorySeparatorChar
if(-not $path.StartsWith($logsRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected checkpoint path.' }
$primary=Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
$backup=Get-Content -LiteralPath ($path+'.bak') -Raw | ConvertFrom-Json
if(-not $primary.is_test -or $primary.status -ne 'in_progress' -or $primary.samples.Count -ne 60 -or
    [Math]::Abs($primary.duration_seconds-60) -gt .1 -or $backup.samples.Count -ne 30 -or
    [Math]::Abs($backup.duration_seconds-30) -gt .1) { throw 'Unexpected checkpoint generations after forced termination.' }
Copy-Item -LiteralPath $path -Destination (Join-Path $output 'Primary.json')
Copy-Item -LiteralPath ($path+'.bak') -Destination (Join-Path $output 'Previous.json')
# Corrupt only a new isolated copy of this automated run, retaining its valid backup.
$damaged=Join-Path $logsRoot ('recoveryQA_'+[guid]::NewGuid().ToString('N')+'.json')
[IO.File]::WriteAllText($damaged,'{"interrupted_write":', [Text.UTF8Encoding]::new($false))
Copy-Item -LiteralPath ($path+'.bak') -Destination ($damaged+'.bak')
foreach($variant in @('Primary','Backup')) {
    $target=if($variant -eq 'Primary') { $path } else { $damaged }
    $viewLog=Join-Path $output ($variant+'View.log')
    $arguments='-DiveSmokeTest -DiveScenario=recovery -DiveRecoveryLog="{0}" -windowed -ResX=1600 -ResY=900 -nosplash -unattended -abslog="{1}"' -f $target,$viewLog
    if($variant -eq 'Backup') { $arguments+=' -DiveExpectBackup' }
    $started=[DateTime]::UtcNow
    $view=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
    try {
        if(-not $view.WaitForExit(60000)) { throw 'Recovery logbook view timed out.' }
        $log=Get-Content -LiteralPath $viewLog -Raw
        if($view.ExitCode -ne 0 -or $log -notmatch 'DIVE_RECOVERY_PASS' -or $log -notmatch 'DIVE_LOGBOOK_PASS' -or
            $log -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used') { throw 'Recovery logbook validation failed.' }
        foreach($page in 0..3) {
            $image=Get-Item -LiteralPath (Join-Path $saved "Screenshots/Logbook_$page.png")
            if($image.LastWriteTimeUtc -lt $started -or $image.Length -lt 10000) { throw 'No fresh recovery screenshot.' }
            Copy-Item -LiteralPath $image.FullName -Destination (Join-Path $output ($variant+"_$page.png"))
        }
    } finally {
        $view.Refresh()
        if(-not $view.HasExited) { $view.Kill(); $view.WaitForExit(10000) | Out-Null }
    }
}
if((Get-FileHash -LiteralPath $path).Hash -ne (Get-FileHash -LiteralPath (Join-Path $output 'Primary.json')).Hash) {
    throw 'Opening the logbook changed the saved checkpoint.'
}
[ordered]@{primary_seconds=$primary.duration_seconds;primary_samples=$primary.samples.Count;
    backup_seconds=$backup.duration_seconds;backup_samples=$backup.samples.Count;forced_termination=$true;
    original_log=$path;damaged_test_copy=$damaged;executable_sha256=(Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'Result.json') -Encoding utf8
Write-Output "DIVE_CHECKPOINT_RECOVERY_PASS $output"
