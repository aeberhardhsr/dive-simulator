param([string]$PackageDirectory='', [switch]$EditorSandbox, [switch]$WaterPreview, [switch]$SolarShaderStudy,
    [ValidateSet('day','calm','night','air','deep')][string]$Case='day',
    [string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
if($SolarShaderStudy -and ($PackageDirectory -or -not $EditorSandbox -or -not $WaterPreview)) {throw 'SolarShaderStudy requires the native editor sandbox and WaterPreview.'}
$repo=Split-Path $PSScriptRoot -Parent
$result=Join-Path $repo ('artifacts/caustics-'+$Case+'-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$log=Join-Path $result 'Runtime.log'
$arguments='-DiveSmokeTest -DiveScenario=caustics -DiveEntry=0 -DiveNoBubbles -windowed -ResX=1600 -ResY=900 -nosplash -unattended -ExecCmds="t.MaxFPS 30" -abslog="'+$log+'"'
if($PackageDirectory) {
    . (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
    if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
    $executable=Resolve-DivePackageExecutable $PackageDirectory
    $saved=Join-Path $PackageDirectory 'divingsim/Saved'
} else {
    $project=Join-Path $repo 'divingsim/divingsim.uproject'
    if($EditorSandbox) { . (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1');$project=Get-DiveEditorSandbox }
    $executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $arguments='"'+$project+'" /Game/Diving/Maps/L_DiveTraining -game '+$arguments
    $saved=Join-Path (Split-Path $project -Parent) 'Saved'
}
$arguments+=' -DiveCausticsCase='+$Case
if($WaterPreview) {$arguments+=' -DiveWaterPreview'}
if($SolarShaderStudy) {$arguments+=' -DiveSolarShaderStudy'}
$started=[DateTime]::UtcNow
$archives=@()
if($PackageDirectory) {
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File) {
        $archives+=@{path=$file.FullName;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
    }
}
@{started_utc=$started.ToString('o');executable=$executable;executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
    case=$Case;packaged=[bool]$PackageDirectory;water_preview=[bool]$WaterPreview;solar_shader_study=[bool]$SolarShaderStudy;package_archives=$archives;head=(& git -C $repo rev-parse HEAD)} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(180000)) { throw "Caustics capture exceeded its time limit: process $($process.Id). It was not restarted or terminated." }
$text=Get-Content -LiteralPath $log -Raw
if($process.ExitCode -ne 0 -or $text -notmatch 'DIVE_CAUSTICS_PROBE_PASS captures=4' -or $text -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used') { throw "Caustics probe failed: $log" }
if($SolarShaderStudy -and $text -notmatch 'DIVE_SOLAR_SHADER_STUDY_READY') {throw 'Missing project-local solar shader mapping.'}
if($WaterPreview -and $text -notmatch 'DIVE_WATER_PREVIEW_READY') {throw 'Water preview was not enabled.'}
foreach($i in 0..3) {
    if($text -notmatch ('DIVE_CAUSTICS_CAPTURE phase='+$i+' ')) { throw "Missing phase $i" }
    $file=Get-Item -LiteralPath (Join-Path $saved ('Screenshots/Caustics_{0:00}.png' -f $i))
    if($file.LastWriteTimeUtc -lt $started) { throw "Stale capture: $file" }
    Copy-Item -LiteralPath $file.FullName -Destination $result
}
Write-Output "DIVE_CAUSTICS_VALIDATION=$result"
