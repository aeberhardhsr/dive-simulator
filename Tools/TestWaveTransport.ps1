param([string]$EngineRoot='', [string]$OutputDirectory='',
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='SourceEditorSandbox',
    [ValidateRange(180,7200)][int]$TimeoutSeconds=1200)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not $EngineRoot) {$EngineRoot=Join-Path $repo 'Builds/UE584'}
$EngineRoot=[IO.Path]::GetFullPath($EngineRoot)
$active=Get-CimInstance Win32_Process | Where-Object {
    $_.Name -like 'UnrealEditor*' -or $_.Name -like 'divingsim*' -or $_.Name -eq 'ShaderCompileWorker.exe' -or
    ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -match 'UnrealBuildTool\.dll')
}
if($active) {throw 'An Unreal build, editor, game or shader worker is running. Observe that process; do not start another test.'}
. (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1')
$project=Get-DiveEditorSandbox -SandboxName $SandboxName
$sandbox=Split-Path $project -Parent
$files=@(Get-ChildItem (Join-Path $repo 'divingsim/Source'),(Join-Path $repo 'divingsim/Shaders'),(Join-Path $repo 'divingsim/Config') -Recurse -File)
$files+=Get-ChildItem (Join-Path $repo 'divingsim/Content/Diving/Materials') -File | Where-Object {$_.Name -match 'Water|Underwater|Caustic'}
foreach($path in @(
    (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'),
    (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Renderer.dll'),
    (Join-Path $sandbox 'Binaries/Win64/UnrealEditor-DiveSim.dll'),
    (Join-Path $sandbox 'Binaries/Win64/UnrealEditor-DiveRendering.dll'),
    (Join-Path $repo 'divingsim/divingsim.uproject'),
    (Join-Path $PSScriptRoot 'Shaders/WaterOptics.ush'),
    (Join-Path $sandbox 'Intermediate/DiveWaveStudy/Private/DiveWaterOptics.ush'),
    (Join-Path $PSScriptRoot 'PrepareSolarShaderStudy.py'),
    (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1'),
    (Join-Path $PSScriptRoot 'TestCaustics.ps1'),$PSCommandPath)) {$files+=Get-Item -LiteralPath $path}
$manifests=@{}
foreach($variant in @('DiveSolarVolumeStudy','DiveWaveStudy')) {
    $manifest=Join-Path $sandbox "Intermediate/$variant/Manifest.json"
    $manifests[$variant]=Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
    $files+=Get-Item -LiteralPath $manifest
    foreach($shader in @('DeferredLightPixelShaders.usf','VolumetricFog.usf')) {
        $prepared=Get-Item -LiteralPath (Join-Path $sandbox "Intermediate/$variant/Private/$shader")
        $expected=if($shader -eq 'VolumetricFog.usf') {$manifests[$variant].volume.patched_sha256} else {$manifests[$variant].patched_sha256}
        if((Get-FileHash -LiteralPath $prepared.FullName).Hash -ne $expected) {throw "Prepared shader differs from manifest: $variant/$shader"}
        $files+=$prepared
    }
}
if($manifests.DiveWaveStudy.waves_enabled -ne $true -or $manifests.DiveSolarVolumeStudy.volume_enabled -ne $true) {
    throw 'Prepare both the projection and wave shader variants for this exact sandbox first.'
}
$inputs=@($files | Sort-Object FullName -Unique | ForEach-Object {
    @{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}
})
if(-not $OutputDirectory) {$OutputDirectory=Join-Path $repo ('artifacts/wave-transport-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))}
$root=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $root) {throw 'Use a new, empty result directory.'}
New-Item -ItemType Directory -Path $root | Out-Null
@{source_head=(& git -C $repo rev-parse HEAD);engine_root=$EngineRoot;project=$project;
    files=$inputs;shader_manifests=$manifests;scope='Five paired receiver fixtures; not habitat, lamp, performance or visual acceptance.'} |
    ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $root 'Host.json') -Encoding utf8
$completed=@()
foreach($fixture in @('day','calm','deep','air','night')) {
    foreach($wave in @($false,$true)) {
        $mode=if($wave) {'wave'} else {'projection'}
        $name="$fixture-$mode"
        $result=& (Join-Path $PSScriptRoot 'TestCaustics.ps1') -EngineRoot $EngineRoot -EditorSandbox -SandboxName $SandboxName `
            -WaterPreview -SolarShaderStudy -SolarVolumeStudy -WaveTransportStudy:$wave -Case $fixture -TimeoutSeconds $TimeoutSeconds
        $line=@($result | Where-Object {$_ -like 'DIVE_CAUSTICS_VALIDATION=*'})
        if($line.Count -ne 1) {throw "Missing native case result: $name"}
        $caseRoot=$line[0].Substring('DIVE_CAUSTICS_VALIDATION='.Length)
        $caseHost=Get-Content -LiteralPath (Join-Path $caseRoot 'Host.json') -Raw | ConvertFrom-Json
        $caseLog=Get-Content -LiteralPath (Join-Path $caseRoot 'Runtime.log') -Raw
        if($caseHost.case -ne $fixture -or $caseHost.wave_transport_study -ne $wave -or
           ($caseLog -match 'DIVE_WATER_SCENE_READY waves=4') -ne $wave -or
           $caseLog -match 'Fatal error:|Failed to compile|unbound parameters|Default Material will be used') {
            throw "Invalid paired capture: $name"
        }
        $folder=Join-Path $root $name
        New-Item -ItemType Directory -Path $folder | Out-Null
        Get-ChildItem -LiteralPath $caseRoot -File | Copy-Item -Destination $folder
        $completed+=@{name=$name;fixture=$fixture;wave=$wave;runtime_status='PASS';visual_status='NOT_REVIEWED'}
        $completed | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root 'Cases.json') -Encoding utf8
        Write-Output "WAVE_TRANSPORT_CASE_PASS $name"
    }
}
foreach($item in $inputs) {
    if((Get-FileHash -LiteralPath $item.path).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Input changed: $($item.path)"}
}
Write-Output "WAVE_TRANSPORT_RECEIVERS_CAPTURED=$root"
