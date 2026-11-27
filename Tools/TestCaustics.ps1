param([string]$PackageDirectory='', [switch]$EditorSandbox, [switch]$WaterPreview, [switch]$SolarShaderStudy, [switch]$SolarVolumeStudy, [switch]$WaveTransportStudy,
    [switch]$LakeFootprint, [switch]$LakeSurfaceOff, [switch]$LakeSurfaceDepthDebug, [string]$LakeFootprintProbeFile='',
    [ValidateSet('day','calm','night','air','deep','shore','shore-top','shore-under')][string]$Case='day',
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='EditorSandbox',
    [ValidateRange(180,7200)][int]$TimeoutSeconds=180,
    [string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
if($SandboxName -ne 'EditorSandbox' -and -not $EditorSandbox) {throw 'SandboxName requires EditorSandbox.'}
if($SolarShaderStudy -and ($PackageDirectory -or -not $EditorSandbox -or -not $WaterPreview)) {throw 'SolarShaderStudy requires the native editor sandbox and WaterPreview.'}
if($SolarVolumeStudy -and -not $SolarShaderStudy) {throw 'SolarVolumeStudy requires SolarShaderStudy.'}
if($WaveTransportStudy -and -not $SolarVolumeStudy) {throw 'WaveTransportStudy requires SolarVolumeStudy.'}
if($LakeFootprint -and -not $WaveTransportStudy) {throw 'LakeFootprint requires WaveTransportStudy.'}
if($LakeSurfaceOff -and -not $LakeFootprint) {throw 'LakeSurfaceOff requires LakeFootprint.'}
if($LakeSurfaceDepthDebug -and (-not $LakeFootprint -or $Case -ne 'shore-under')) {throw 'LakeSurfaceDepthDebug requires LakeFootprint and shore-under.'}
if($LakeFootprintProbeFile) {
    if(-not $LakeFootprint) {throw 'LakeFootprintProbeFile requires LakeFootprint.'}
    $LakeFootprintProbeFile=(Resolve-Path -LiteralPath $LakeFootprintProbeFile).Path
}
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
    if($EditorSandbox) { . (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1');$project=Get-DiveEditorSandbox -SandboxName $SandboxName }
    $executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $arguments='"'+$project+'" /Game/Diving/Maps/L_DiveTraining -game '+$arguments
    $saved=Join-Path (Split-Path $project -Parent) 'Saved'
}
$arguments+=' -DiveCausticsCase='+$Case
if($WaterPreview) {$arguments+=' -DiveWaterPreview'}
if($SolarShaderStudy) {$arguments+=' -DiveSolarShaderStudy'}
if($SolarVolumeStudy) {$arguments+=' -DiveSolarVolumeStudy'}
if($WaveTransportStudy) {$arguments+=' -DiveWaveTransportStudy'}
if($LakeFootprint) {$arguments+=' -DiveLakeFootprint'}
if($LakeSurfaceOff) {$arguments+=' -DiveLakeSurfaceOff'}
if($LakeSurfaceDepthDebug) {$arguments+=' -DiveSpectralView -DiveSpectralDepthDebug'}
if($LakeFootprintProbeFile) {$arguments+=' -DiveLakeFootprintProbeFile="'+$LakeFootprintProbeFile+'"'}
$started=[DateTime]::UtcNow
$archives=@()
$rendererIdentity=$null
if(-not $PackageDirectory) {
    $rendererModule=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Renderer.dll'
    $rendererIdentity=@{path=[IO.Path]::GetFullPath($rendererModule);sha256=(Get-FileHash -LiteralPath $rendererModule).Hash}
}
if($PackageDirectory) {
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File) {
        $archives+=@{path=$file.FullName;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
    }
}
@{started_utc=$started.ToString('o');executable=$executable;executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
    renderer_module=$rendererIdentity;sandbox_name=$SandboxName;timeout_seconds=$TimeoutSeconds;
    lake_footprint=[bool]$LakeFootprint;lake_surface_off=[bool]$LakeSurfaceOff;lake_surface_depth_debug=[bool]$LakeSurfaceDepthDebug;lake_footprint_probe_file=$LakeFootprintProbeFile;
    case=$Case;packaged=[bool]$PackageDirectory;water_preview=[bool]$WaterPreview;solar_shader_study=[bool]$SolarShaderStudy;solar_volume_study=[bool]$SolarVolumeStudy;wave_transport_study=[bool]$WaveTransportStudy;package_archives=$archives;head=(& git -C $repo rev-parse HEAD)} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit($TimeoutSeconds*1000)) { throw "Caustics capture exceeded its $TimeoutSeconds second time limit: process $($process.Id). It was not restarted or terminated." }
$text=Get-Content -LiteralPath $log -Raw
if($process.ExitCode -ne 0 -or $text -notmatch 'DIVE_CAUSTICS_PROBE_PASS captures=4' -or $text -match 'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used|unbound parameters|DIVE_WAVE_RENDERER_UNAVAILABLE') { throw "Caustics probe failed: $log" }
if($SolarShaderStudy -and $text -notmatch 'DIVE_SOLAR_SHADER_STUDY_READY') {throw 'Missing project-local solar shader mapping.'}
if($SolarVolumeStudy -and $text -notmatch 'DIVE_SOLAR_VOLUME_STUDY_READY') {throw 'Missing project-local volume shader mapping.'}
if($WaveTransportStudy -and $text -notmatch 'DIVE_WATER_SCENE_READY waves=4') {throw 'Shared water scene parameters were not uploaded.'}
if($WaterPreview -and $text -notmatch 'DIVE_WATER_PREVIEW_READY') {throw 'Water preview was not enabled.'}
if($LakeFootprint -and $text -notmatch 'DIVE_LAKE_FOOTPRINT_READY') {throw 'Lake footprint was not loaded.'}
if($LakeFootprint -and $text -notmatch ('DIVE_LAKE_SURFACE_READY enabled='+[int](-not $LakeSurfaceOff)+' materials=2')) {throw 'Lake surface coverage was not bound to both water materials.'}
if($LakeSurfaceDepthDebug -and $text -notmatch 'DIVE_SPECTRAL_VIEW_READY') {throw 'Water custom-depth diagnostic did not execute.'}
if($LakeFootprintProbeFile) {
    if($text -notmatch 'DIVE_LAKE_GPU_PASS') {throw 'GPU footprint audit did not complete.'}
    $probe=Get-Item -LiteralPath (Join-Path $saved 'LakeFootprintGPU.csv')
    if($probe.LastWriteTimeUtc -lt $started) {throw 'Stale GPU footprint readback.'}
    Copy-Item -LiteralPath $probe.FullName -Destination $result
}
foreach($i in 0..3) {
    if($text -notmatch ('DIVE_CAUSTICS_CAPTURE phase='+$i+' ')) { throw "Missing phase $i" }
    $file=Get-Item -LiteralPath (Join-Path $saved ('Screenshots/Caustics_{0:00}.png' -f $i))
    if($file.LastWriteTimeUtc -lt $started) { throw "Stale capture: $file" }
    Copy-Item -LiteralPath $file.FullName -Destination $result
}
Write-Output "DIVE_CAUSTICS_VALIDATION=$result"
