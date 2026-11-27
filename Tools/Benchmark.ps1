param(
    [string]$PackageDirectory='Builds/WindowsPerformance',
    [ValidateSet('underwater','surface','shore','stream','sediment')][string]$Scenario='underwater',
    [ValidateSet('1080p','1440p','2160p')][string]$Resolution='1080p',
    [ValidateRange(0,3)][int]$Quality=2,
    [ValidateRange(50,100)][double]$RenderScale=100,
    [ValidateRange(10,7200)][int]$Seconds=60,
    [ValidateRange(0,2)][int]$Entry=0,
    [switch]$Editor,
    [switch]$EditorSandbox,
    [switch]$DisableNanite,
    [switch]$WaterPreview,
    [switch]$ImmediateGravelRetirement,
    [switch]$ImmediatePopulationRetirement,
    [switch]$LegacyRockPose,
    [switch]$LegacyTalus, [switch]$NoPlantUnderstory, [switch]$UnlimitedPlants, [switch]$LegacyFormations, [switch]$FootDeposits, [switch]$NoFootDeposits,
    [string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference='Stop'
if($EditorSandbox) {$Editor=$true}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$size=@{'1080p'=@(1920,1080);'1440p'=@(2560,1440);'2160p'=@(3840,2160)}[$Resolution]
$runName='perf-{0}-{1}-q{2}-{3}' -f $Scenario,$Resolution,$Quality,(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$resultDir=Join-Path $repo "artifacts/$runName"
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
$logPath=Join-Path $resultDir 'Runtime.log'
$context=[ordered]@{
    started_utc=[DateTime]::UtcNow.ToString('o'); editor=[bool]$Editor; disable_nanite=[bool]$DisableNanite; water_preview=[bool]$WaterPreview; gravel_immediate_retirement=[bool]$ImmediateGravelRetirement
    population_immediate_retirement=[bool]$ImmediatePopulationRetirement
    legacy_rock_pose=[bool]$LegacyRockPose
    legacy_talus=[bool]$LegacyTalus
    no_plant_understory=[bool]$NoPlantUnderstory
    unlimited_plants=[bool]$UnlimitedPlants
    legacy_formations=[bool]$LegacyFormations
    foot_deposits=(-not [bool]$NoFootDeposits)
    editor_sandbox=[bool]$EditorSandbox
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime,WorkingSet64)
}
if(Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    $context.gpu_before=@(& nvidia-smi --query-gpu=name,driver_version,memory.total,memory.used,utilization.gpu --format=csv)
}
$scaleText=$RenderScale.ToString([Globalization.CultureInfo]::InvariantCulture)
$arguments='-DiveSmokeTest -DiveScenario=perf-{0} -DiveEntry={1} -DiveQuality={2} -DiveRenderScale={3} -DivePerfSeconds={4} -windowed -ForceRes -ResX={5} -ResY={6} -WinX=0 -WinY=0 -NoVSync -csvGpuStats -nosplash -unattended -abslog="{7}"' -f $Scenario,$Entry,$Quality,$scaleText,$Seconds,$size[0],$size[1],$logPath
if($DisableNanite) { $arguments+=' -ExecCmds="r.Nanite 0"' }
if($WaterPreview) { $arguments+=' -DiveWaterPreview' }
if($ImmediateGravelRetirement) { $arguments+=' -DiveImmediateGravelRetirement' }
if($ImmediatePopulationRetirement) { $arguments+=' -DiveImmediatePopulationRetirement' }
if($LegacyRockPose) { $arguments+=' -DiveLegacyRockPose' }
if($LegacyTalus) { $arguments+=' -DiveLegacyTalus' }
if($NoPlantUnderstory) { $arguments+=' -DiveNoPlantUnderstory' }
if($UnlimitedPlants) { $arguments+=' -DiveUnlimitedPlants' }
if($LegacyFormations) { $arguments+=' -DiveLegacyFormations' }
if($FootDeposits -and $NoFootDeposits) {throw 'FootDeposits and NoFootDeposits are mutually exclusive.'}
if($FootDeposits) { $arguments+=' -DiveFootDeposits' }
if($NoFootDeposits) { $arguments+=' -DiveNoFootDeposits' }
$executable=if($Editor) { '' } else { Resolve-DivePackageExecutable $PackageDirectory }
if($Editor) {
    $executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $projectRoot=if($EditorSandbox) {'Builds/EditorSandbox/divingsim'} else {'divingsim'}
    $project=Join-Path $repo "$projectRoot/divingsim.uproject"
    $context.game_module_sha256=(Get-FileHash -LiteralPath (Join-Path $repo "$projectRoot/Binaries/Win64/UnrealEditor-DiveSim.dll")).Hash.ToLowerInvariant()
    $arguments=('"{0}" /Game/Diving/Maps/L_DiveTraining -game ' -f $project)+$arguments
}
$context.executable=$executable
$context.executable_sha256=(Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash.ToLowerInvariant()
if(-not $Editor) {
    $context.package_containers=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Sort-Object Name | ForEach-Object {
        @{file=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    })
}
$context | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $resultDir 'Host.json') -Encoding utf8
$started=[DateTime]::UtcNow
$process=Start-Process -FilePath $executable -ArgumentList $arguments -WindowStyle Hidden -PassThru
if(-not $process.WaitForExit(($Seconds+120)*1000)) { throw "Benchmark exceeded its time limit; inspect process $($process.Id) and $logPath. It was not restarted or terminated." }
$log=Get-Content -LiteralPath $logPath -Raw
if($DisableNanite -and $log -notmatch 'r.Nanite = "0"') { throw 'Nanite fallback override was not applied.' }
if($process.ExitCode -ne 0 -or $log -match 'DIVE_\w+_FAIL|WALENSEE_DATA_FAILED|Failed to compile Material|Default Material will be used') { throw "Benchmark capture failed: $logPath" }
foreach($marker in @('DIVE_IMAGERY_PASS','DIVE_LANDSCAPE_PASS','DIVE_OPTICS_PASS','DIVE_LIGHT_PASS','DIVE_PERF_CAPTURE_BEGIN')) {
    if($log -notmatch $marker) { throw "Missing $marker in $logPath" }
}
if($log -notmatch 'DIVE_PERF_CAPTURE_READY directory=([^\r\n]+)') { throw 'No completed capture was reported.' }
$captureDir=$Matches[1].Trim()
foreach($name in @('Capture.csv','Metadata.json','Frame.png')) {
    $source=Get-Item -LiteralPath (Join-Path $captureDir $name)
    if($source.LastWriteTimeUtc -lt $started -or $source.Length -lt 100) { throw "Missing fresh benchmark artifact: $name" }
    Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $resultDir $name)
}
$metadata=Get-Content -LiteralPath (Join-Path $resultDir 'Metadata.json') -Raw | ConvertFrom-Json
if($WaterPreview -and (-not $metadata.water_preview -or -not $metadata.water_interface -or -not $metadata.bounded_water -or -not $metadata.native_scattering -or
    $metadata.'r.Lumen.TranslucencyReflections.FrontLayer.Allow' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.HitLighting.Allowed' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.LightingMode' -ne 2)) {throw 'Water preview benchmark profile was not preserved.'}
if($metadata.foot_deposits -ne $context.foot_deposits) {throw 'Actual foot deposits differ from requested state.'}
if($metadata.width -ne $size[0] -or $metadata.height -ne $size[1]) { throw 'Actual viewport dimensions differ from the requested output resolution.' }
if([Math]::Abs($metadata.'r.ScreenPercentage'-$RenderScale) -gt .01 -or $metadata.'r.SecondaryScreenPercentage.GameViewport' -ne 100 -or $metadata.'r.DynamicRes.OperationMode' -ne 0) { throw 'Actual render scale differs from the requested fixed scale.' }
if($metadata.'r.VSync' -ne 0 -or $metadata.'t.MaxFPS' -ne 0) { throw 'Benchmark was unintentionally frame-limited.' }
foreach($group in @('ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage')) {
    if($metadata."sg.${group}Quality" -ne $Quality) { throw "Actual $group quality differs from the requested preset." }
}
Write-Output "DIVE_BENCHMARK_CAPTURE_READY $resultDir"
& python (Join-Path $PSScriptRoot 'analyze_performance.py') $resultDir
if($LASTEXITCODE -ne 0) { throw "Benchmark analysis failed: $resultDir" }
