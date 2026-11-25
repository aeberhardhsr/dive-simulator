param([string]$PackageDirectory='Builds/WindowsSoak',
    [ValidateRange(180,7200)][int]$Seconds=7200,
    [ValidateRange(0,2)][int]$Entry=0,
    [ValidateRange(0,3)][int]$Quality=2)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$executable=Resolve-DivePackageExecutable $PackageDirectory
$resultDir=Join-Path $repo ('artifacts/soak-{0}s-entry{1}-{2}' -f $Seconds,$Entry,(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
$logPath=Join-Path $resultDir 'Runtime.log'
$hostInfo=[ordered]@{started_utc=[DateTime]::UtcNow.ToString('o');requested_seconds=$Seconds;
    executable=$executable;executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
    source_commit=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain);
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime,WorkingSet64)}
if(Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    $hostInfo.gpu_before=@(& nvidia-smi --query-gpu=name,driver_version,memory.total,memory.used,utilization.gpu --format=csv)
}
$arguments='-DiveSmokeTest -DiveScenario=perf-soak -DiveEntry={0} -DiveQuality={1} -DiveRenderScale=100 -DivePerfSeconds={2} -windowed -ForceRes -ResX=1920 -ResY=1080 -WinX=0 -WinY=0 -NoVSync -nosplash -unattended -abslog="{3}"' -f $Entry,$Quality,$Seconds,$logPath
$started=[DateTime]::UtcNow
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
$hostInfo.process_id=$process.Id
$hostInfo | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $resultDir 'Host.json') -Encoding utf8
Write-Output "DIVE_SOAK_STARTED pid=$($process.Id) directory=$resultDir"
$lastProgress=''
while(-not $process.WaitForExit(30000)) {
    if(Test-Path -LiteralPath $logPath) {
        $progress=Get-Content -LiteralPath $logPath -Tail 80 | Where-Object { $_ -match 'DIVE_SOAK_PROGRESS|DIVE_SOAK_FAIL' } | Select-Object -Last 1
        if($progress -and $progress -ne $lastProgress) { Write-Output $progress; $lastProgress=$progress }
    }
    if(([DateTime]::UtcNow-$started).TotalSeconds -gt $Seconds+600) {
        throw "Soak observation deadline exceeded. Inspect live process $($process.Id) and $resultDir; it was not terminated or restarted."
    }
}
$log=Get-Content -LiteralPath $logPath -Raw
if($process.ExitCode -ne 0 -or $log -match 'DIVE_\w+_FAIL|WALENSEE_DATA_FAILED|Failed to compile Material|Default Material will be used') { throw "Soak failed: $logPath" }
foreach($marker in @('DIVE_IMAGERY_PASS','DIVE_LANDSCAPE_PASS','DIVE_OPTICS_PASS','DIVE_LIGHT_PASS','DIVE_SOAK_BEGIN')) {
    if($log -notmatch $marker) { throw "Missing $marker" }
}
if($log -notmatch 'DIVE_SOAK_PASS directory=([^\r\n]+)') { throw 'Soak has no successful final record.' }
$captureDir=$Matches[1].Trim()
foreach($name in @('Result.json','Intervals.jsonl','Start.png','Final.png')) {
    $file=Get-Item -LiteralPath (Join-Path $captureDir $name)
    if($file.LastWriteTimeUtc -lt $started -or $file.Length -lt 100) { throw "Missing fresh artifact: $name" }
    Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $resultDir $name)
}
Get-ChildItem -LiteralPath $captureDir -Filter 'Frame_*.png' -File | Copy-Item -Destination $resultDir
$result=Get-Content -LiteralPath (Join-Path $resultDir 'Result.json') -Raw | ConvertFrom-Json
if($result.requested_seconds -ne $Seconds -or $result.quality -ne $Quality) { throw 'Soak differs from requested duration or quality.' }
Copy-Item -LiteralPath $result.log_path -Destination (Join-Path $resultDir 'DiveLog.json')
if(Test-Path -LiteralPath ($result.log_path+'.bak')) { Copy-Item -LiteralPath ($result.log_path+'.bak') -Destination (Join-Path $resultDir 'PreviousDiveLog.json') }
& python (Join-Path $PSScriptRoot 'analyze_soak.py') $resultDir
if($LASTEXITCODE -ne 0) { throw "Soak validation failed: $resultDir" }
Write-Output "DIVE_SOAK_VERIFIED $resultDir"
