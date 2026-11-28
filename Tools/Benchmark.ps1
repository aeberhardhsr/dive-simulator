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
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='EditorSandbox',
    [switch]$DisableNanite,
    [switch]$WaterPreview,
    [switch]$ProductionWater,
    [switch]$ReflectionTransportStudy, [switch]$ReflectionTransportControl, [switch]$ReflectionUnified,
    [ValidateSet('full','routing','unshadowed','cached','vsm','vsm-fallback')][string]$ReflectionProfile='full',
    [ValidateRange(-70,70)][double]$LookPitch,
    [switch]$FPVProbe,
    [switch]$SpectralView,
    [ValidateRange(0,2)][double]$AbsorptionGain=1,
    [switch]$ImmediateGravelRetirement,
    [switch]$ImmediatePopulationRetirement,
    [switch]$LegacyRockPose,
    [switch]$LegacyTalus, [switch]$NoPlantUnderstory, [switch]$UnlimitedPlants, [switch]$LegacyFormations, [switch]$FootDeposits, [switch]$NoFootDeposits, [switch]$ImmediateGravelBuild, [switch]$ImmediatePopulationBuild,
    [string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8'
)
$ErrorActionPreference='Stop'
if($SandboxName -ne 'EditorSandbox' -and -not $EditorSandbox) {throw 'SandboxName requires EditorSandbox.'}
if($SpectralView -and -not $WaterPreview) {throw 'SpectralView requires WaterPreview.'}
if($EditorSandbox) {$Editor=$true}
if($ProductionWater -and ($ReflectionTransportStudy -or $SpectralView)) {throw 'ProductionWater checks the normal cooked profile, not shader experiments.'}
if($ReflectionTransportStudy -and (-not $EditorSandbox -or -not $WaterPreview)) {throw 'Reflection transport benchmarking requires an editor sandbox and WaterPreview.'}
if(($ReflectionTransportControl -or $ReflectionUnified) -and -not $ReflectionTransportStudy) {throw 'Reflection routing/control requires ReflectionTransportStudy.'}
if($ReflectionProfile -ne 'full' -and (-not $ReflectionTransportStudy -or $ReflectionTransportControl)) {throw 'Diagnostic reflection profile requires active ReflectionTransportStudy.'}
if($PSBoundParameters.ContainsKey('LookPitch') -and $Scenario -ne 'underwater') {throw 'LookPitch currently applies to the underwater benchmark.'}
if($FPVProbe -and (-not $Editor -or $Scenario -ne 'underwater')) {throw 'FPVProbe requires an editor underwater benchmark.'}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$size=@{'1080p'=@(1920,1080);'1440p'=@(2560,1440);'2160p'=@(3840,2160)}[$Resolution]
$runName='perf-{0}-{1}-q{2}-{3}' -f $Scenario,$Resolution,$Quality,(Get-Date -Format 'yyyyMMdd-HHmmss-fff')
$resultDir=Join-Path $repo "artifacts/$runName"
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null
$logPath=Join-Path $resultDir 'Runtime.log'
$context=[ordered]@{
    started_utc=[DateTime]::UtcNow.ToString('o'); editor=[bool]$Editor; disable_nanite=[bool]$DisableNanite
    water_preview=([bool]$WaterPreview -or [bool]$ProductionWater); water_preview_requested=[bool]$WaterPreview
    gravel_immediate_retirement=[bool]$ImmediateGravelRetirement
    population_immediate_retirement=[bool]$ImmediatePopulationRetirement
    legacy_rock_pose=[bool]$LegacyRockPose
    legacy_talus=[bool]$LegacyTalus
    no_plant_understory=[bool]$NoPlantUnderstory
    unlimited_plants=[bool]$UnlimitedPlants
    legacy_formations=[bool]$LegacyFormations
    foot_deposits=(-not [bool]$NoFootDeposits)
    gravel_staged_build=(-not [bool]$ImmediateGravelBuild)
    population_staged_build=(-not [bool]$ImmediatePopulationBuild)
    spectral_view=[bool]$SpectralView
    absorption_gain=$AbsorptionGain
    editor_sandbox=[bool]$EditorSandbox
    sandbox_name=$SandboxName
    fpv_probe=[bool]$FPVProbe
    production_water=[bool]$ProductionWater
    reflection_transport=([bool]$ReflectionTransportStudy -or [bool]$ProductionWater)
    reflection_control=[bool]$ReflectionTransportControl
    reflection_unified=([bool]$ReflectionUnified -or [bool]$ProductionWater)
    reflection_profile=$(if($ProductionWater){'vsm'}else{$ReflectionProfile})
    requested_pitch=$(if($PSBoundParameters.ContainsKey('LookPitch')) {$LookPitch} else {$null})
    source_head=(& git -C $repo rev-parse HEAD)
    source_dirty=[bool](& git -C $repo status --porcelain)
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime,WorkingSet64)
}
# Aggregate host pressure only; unrelated process names/IDs do not belong in reports.
function Get-DiveHostCpuLoad {
    try {
        $sample=@(Get-CimInstance Win32_PerfFormattedData_PerfProc_Process -ErrorAction Stop | Where-Object {$_.Name -notin @('_Total','Idle')})
        if(-not $sample.Count) {return @{available=$false}}
        return @{available=$true;logical_processors=[Environment]::ProcessorCount;
            total_core_percent=($sample | Measure-Object PercentProcessorTime -Sum).Sum;
            peak_core_percent=($sample | Measure-Object PercentProcessorTime -Maximum).Maximum;
            processes_over_tenth_core=@($sample | Where-Object {$_.PercentProcessorTime -gt 10}).Count}
    } catch {return @{available=$false}}
}
$context.host_cpu_before=Get-DiveHostCpuLoad
if(Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    $context.gpu_before=@(& nvidia-smi --query-gpu=name,driver_version,memory.total,memory.used,utilization.gpu --format=csv)
}
$scaleText=$RenderScale.ToString([Globalization.CultureInfo]::InvariantCulture)
$arguments='-DiveSmokeTest -DiveScenario=perf-{0} -DiveEntry={1} -DiveQuality={2} -DiveRenderScale={3} -DivePerfSeconds={4} -windowed -ForceRes -ResX={5} -ResY={6} -WinX=0 -WinY=0 -NoVSync -csvGpuStats -nosplash -unattended -abslog="{7}"' -f $Scenario,$Entry,$Quality,$scaleText,$Seconds,$size[0],$size[1],$logPath
$consoleCommands=@()
if($DisableNanite) { $consoleCommands+='r.Nanite 0' }
if($ReflectionTransportStudy) {
 $arguments+=' -DiveReflectionTransportStudy -DiveLakeFootprint'
 $arguments+=' -DiveReflectionProfile='+$ReflectionProfile
 $consoleCommands+='r.Lumen.HardwareRayTracing.Inline 0'
 if($ReflectionTransportControl) {$arguments+=' -DiveReflectionTransportControl'}
 if($ReflectionUnified) {$arguments+=' -DiveReflectionUnified'}
}
if($consoleCommands.Count) {$arguments+=' -ExecCmds="'+($consoleCommands -join ',')+'"'}
if($PSBoundParameters.ContainsKey('LookPitch')) {$arguments+=' -DivePerfPitch='+$LookPitch.ToString([Globalization.CultureInfo]::InvariantCulture)}
if($WaterPreview) { $arguments+=' -DiveWaterPreview' }
if($FPVProbe) {$arguments+=' -DiveFPVProbe'}
if($SpectralView) {$arguments+=' -DiveSpectralView -DiveAbsorptionGain='+$AbsorptionGain.ToString([Globalization.CultureInfo]::InvariantCulture)}
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
if($ImmediateGravelBuild) { $arguments+=' -DiveImmediateGravelBuild' }
if($ImmediatePopulationBuild) { $arguments+=' -DiveImmediatePopulationBuild' }
$executable=if($Editor) { '' } else { Resolve-DivePackageExecutable $PackageDirectory }
if($Editor) {
    $executable=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
    $projectRoot=if($EditorSandbox) {"Builds/$SandboxName/divingsim"} else {'divingsim'}
    $project=Join-Path $repo "$projectRoot/divingsim.uproject"
    $context.game_module_sha256=(Get-FileHash -LiteralPath (Join-Path $repo "$projectRoot/Binaries/Win64/UnrealEditor-DiveSim.dll")).Hash.ToLowerInvariant()
    $context.renderer_module_sha256=(Get-FileHash -LiteralPath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Renderer.dll')).Hash.ToLowerInvariant()
    if($SpectralView -or $ReflectionTransportStudy) {
        $context.render_module_sha256=(Get-FileHash -LiteralPath (Join-Path $repo "$projectRoot/Binaries/Win64/UnrealEditor-DiveRendering.dll")).Hash.ToLowerInvariant()
        $context.spectral_shader_sha256=(Get-FileHash -LiteralPath (Join-Path $repo "$projectRoot/Shaders/SpectralWater.usf")).Hash.ToLowerInvariant()
    }
    if($ReflectionTransportStudy) {
        $context.reflection_shaders=@('ReflectionWater.ush','ReflectionWaterMask.ush','ReflectionWaterShadow.ush','LakeFootprint.ush') | ForEach-Object {
            @{name=$_;sha256=(Get-FileHash -LiteralPath (Join-Path $repo "$projectRoot/Shaders/$_")).Hash.ToLowerInvariant()}
        }
        Copy-Item -LiteralPath (Join-Path $repo "$projectRoot/Intermediate/DiveReflectionTransportStudy/Manifest.json") -Destination (Join-Path $resultDir 'ShaderManifest.json')
    }
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
if($SpectralView -and $log -notmatch ('DIVE_SPECTRAL_VIEW_READY gain='+$AbsorptionGain.ToString('F3',[Globalization.CultureInfo]::InvariantCulture)+' samples=16 fog_resource=1 stage=BeforeDOF')) {throw 'Missing spectral rendering pass.'}
$captureFiles=@('Capture.csv','Metadata.json','Frame.png')
if($FPVProbe) {
    if($log -notmatch 'DIVE_FPV_PROBE_PASS') {throw 'FPV probe did not complete.'}
    $captureFiles+=@('FPVGeometry.json','Frame_NoSuit.png','Frame_Restored.png')
}
foreach($name in $captureFiles) {
    $source=Get-Item -LiteralPath (Join-Path $captureDir $name)
    if($source.LastWriteTimeUtc -lt $started -or $source.Length -lt 100) { throw "Missing fresh benchmark artifact: $name" }
    Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $resultDir $name)
}
$metadata=Get-Content -LiteralPath (Join-Path $resultDir 'Metadata.json') -Raw | ConvertFrom-Json
if($ProductionWater) {
    $cooked=if($Editor){0}else{1}
    foreach($marker in @("DIVE_PRODUCTION_WATER_READY cooked=$cooked",'DIVE_REFLECTION_PROFILE_READY profile=vsm diagnostic=0',
        'DIVE_WATER_SCENE_READY','DIVE_LAKE_FOOTPRINT_READY','DIVE_REFLECTION_ROUTING_READY unified=1')) {
        if(-not $log.Contains($marker)) {throw "Production benchmark is missing $marker"}
    }
    if(-not $metadata.production_water -or -not $metadata.reflection_transport -or -not $metadata.reflection_unified -or $metadata.reflection_control -or
       -not $metadata.water_interface -or -not $metadata.bounded_water -or -not $metadata.native_scattering -or
       $metadata.'r.Lumen.HardwareRayTracing' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.Inline' -ne 0 -or $metadata.'r.CustomDepth' -ne 3 -or
       $metadata.'r.Lumen.TranslucencyReflections.FrontLayer.Allow' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.HitLighting.Allowed' -ne 1 -or
       $metadata.'r.Lumen.HardwareRayTracing.LightingMode' -ne 2) {throw 'Normal water benchmark profile was not preserved.'}
}
if($ReflectionTransportStudy -and $log -notmatch ('DIVE_REFLECTION_PROFILE_READY profile='+$ReflectionProfile+' ')) {throw 'Reflection diagnostic profile was not verified.'}
if($ReflectionTransportStudy -and (-not $metadata.reflection_transport -or $metadata.reflection_control -ne [bool]$ReflectionTransportControl -or $metadata.reflection_unified -ne [bool]$ReflectionUnified -or $metadata.'r.Lumen.HardwareRayTracing.Inline' -ne 0 -or $metadata.'r.CustomDepth' -ne 3 -or $log -notmatch 'DIVE_WATER_SCENE_READY' -or $log -notmatch 'DIVE_LAKE_FOOTPRINT_READY' -or $log -notmatch ('DIVE_REFLECTION_TRANSPORT_READY enabled='+[int](-not $ReflectionTransportControl)) -or $log -notmatch ('DIVE_REFLECTION_ROUTING_READY unified='+[int][bool]$ReflectionUnified))) {throw 'Reflection benchmark mode was not preserved.'}
if($WaterPreview -and (-not $metadata.water_preview -or -not $metadata.water_interface -or -not $metadata.bounded_water -or -not $metadata.native_scattering -or
    $metadata.'r.Lumen.TranslucencyReflections.FrontLayer.Allow' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.HitLighting.Allowed' -ne 1 -or $metadata.'r.Lumen.HardwareRayTracing.LightingMode' -ne 2)) {throw 'Water preview benchmark profile was not preserved.'}
if($metadata.foot_deposits -ne $context.foot_deposits) {throw 'Actual foot deposits differ from requested state.'}
if($metadata.width -ne $size[0] -or $metadata.height -ne $size[1]) { throw 'Actual viewport dimensions differ from the requested output resolution.' }
if([Math]::Abs($metadata.'r.ScreenPercentage'-$RenderScale) -gt .01 -or $metadata.'r.SecondaryScreenPercentage.GameViewport' -ne 100 -or $metadata.'r.DynamicRes.OperationMode' -ne 0) { throw 'Actual render scale differs from the requested fixed scale.' }
if($metadata.'r.VSync' -ne 0 -or $metadata.'t.MaxFPS' -ne 0) { throw 'Benchmark was unintentionally frame-limited.' }
foreach($group in @('ViewDistance','AntiAliasing','Shadow','GlobalIllumination','Reflection','PostProcess','Texture','Effects','Foliage')) {
    if($metadata."sg.${group}Quality" -ne $Quality) { throw "Actual $group quality differs from the requested preset." }
}
$context.host_cpu_after=Get-DiveHostCpuLoad
$context | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $resultDir 'Host.json') -Encoding utf8
Write-Output "DIVE_BENCHMARK_CAPTURE_READY $resultDir"
& python (Join-Path $PSScriptRoot 'analyze_performance.py') $resultDir
if($LASTEXITCODE -ne 0) { throw "Benchmark analysis failed: $resultDir" }
