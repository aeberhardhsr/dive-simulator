param(
    [string]$EngineRoot='O:/games/dive game/Builds/UE584',
    [ValidatePattern('^[A-Za-z0-9_-]{1,40}$')][string]$SandboxName='SourceEditorSandbox',
    [ValidateSet('habitat0','habitat1','habitat2','shore0','shore1','shore2')][string[]]$Cases=@('habitat0','habitat1','habitat2','shore0','shore1','shore2'),
    [switch]$SurfaceOff,
    [switch]$SurveyCandidates,
    [ValidateRange(0,2)][int]$CandidateIndex=0,
    [ValidateRange(0,23.99)][double]$Hour=12,
    [ValidateRange(180,7200)][int]$TimeoutSeconds=600
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$EngineRoot=(Resolve-Path -LiteralPath $EngineRoot).Path
$projectRoot=Join-Path $repo "Builds/$SandboxName/divingsim"
if($SurveyCandidates -and ($Cases | Where-Object {-not $_.StartsWith('habitat')})) {throw 'SurveyCandidates applies to habitat cases only.'}
$candidates=Get-Content -LiteralPath (Join-Path $repo 'Documentation/BATHYMETRY_VIEW_CANDIDATES.json') -Raw | ConvertFrom-Json
if($SurveyCandidates -and (Get-FileHash -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Geodata/manifest.json')).Hash.ToLowerInvariant() -ne $candidates.manifest_sha256) {throw 'Bathymetry candidate manifest has changed.'}
$result=Join-Path $repo ('artifacts/lake-landscape-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$inputs=@()
foreach($group in @('Source','Shaders','Config')) {
    foreach($file in Get-ChildItem -LiteralPath (Join-Path $repo "divingsim/$group") -Recurse -File) {
        $relative=$file.FullName.Substring((Join-Path $repo 'divingsim').Length+1)
        $hash=(Get-FileHash -LiteralPath $file.FullName).Hash
        if($hash -ne (Get-FileHash -LiteralPath (Join-Path $projectRoot $relative)).Hash) {throw "Sandbox differs: $relative"}
        $inputs+=@{file="divingsim/$($relative.Replace('\','/'))";sha256=$hash}
    }
}
foreach($path in @('Binaries/Win64/UnrealEditor-DiveSim.dll','Binaries/Win64/UnrealEditor-DiveRendering.dll','Intermediate/DiveWaveStudy/Manifest.json')) {
    $inputs+=@{file="Builds/$SandboxName/divingsim/$path";sha256=(Get-FileHash -LiteralPath (Join-Path $projectRoot $path)).Hash}
}
foreach($path in @('Tools/TestLakeLandscape.ps1','Tools/SmokeTest.ps1','Documentation/BATHYMETRY_VIEW_CANDIDATES.json')) {
    $inputs+=@{file=$path;sha256=(Get-FileHash -LiteralPath (Join-Path $repo $path)).Hash}
}
foreach($file in Get-ChildItem -LiteralPath (Join-Path $repo 'divingsim/Content/Diving/Materials') -Filter '*.uasset' -File) {
    $inputs+=@{file="divingsim/Content/Diving/Materials/$($file.Name)";sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
}
@{started_utc=[DateTime]::UtcNow.ToString('o');head=(& git -C $repo rev-parse HEAD);surface_off=[bool]$SurfaceOff;
    engine=$EngineRoot;renderer_sha256=(Get-FileHash -LiteralPath (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Renderer.dll')).Hash;
    cases=$Cases;survey_candidates=[bool]$SurveyCandidates;candidate_index=$CandidateIndex;hour=$Hour;inputs=$inputs} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$completed=@()
foreach($case in $Cases) {
    $entry=[int]::Parse($case.Substring($case.Length-1))
    $folder=Join-Path $result $case
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $habitat=$case.StartsWith('habitat')
    $options=@{EngineRoot=$EngineRoot;EditorSandbox=$true;SandboxName=$SandboxName;WaterPreview=$true;
        SolarShaderStudy=$true;SolarVolumeStudy=$true;WaveTransportStudy=$true;LakeFootprint=$true;LakeSurfaceOff=[bool]$SurfaceOff;
        Scenario=$(if($habitat){'rocks'}else{'shore-preview'});HabitatView=$habitat;Entry=$entry;
        Hour=$Hour;Date='2026-06-21';MaxFPS=30;TimeoutSeconds=$TimeoutSeconds;NoBubbles=$true;
        RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($SurveyCandidates) {
        $candidate=$candidates.entries[$entry].candidates[$CandidateIndex]
        $options.HabitatPose=@($candidate.camera_world_cm)+@($candidate.camera_yaw_degrees,$candidate.camera_pitch_degrees)
    }
    $start=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content -LiteralPath $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:|unbound parameters') {throw "Native failure: $case"}
    if($SurveyCandidates -and [regex]::Matches($log,'DIVE_HABITAT_POSE view=').Count -ne 3) {throw "Missing camera-clearance probes: $case"}
    $names=if($habitat){@('Habitat_0_0.png','Habitat_0_1.png','Habitat_0_2.png')}else{@("Shore_$entry.png")}
    $images=@()
    foreach($name in $names) {
        $file=Get-Item -LiteralPath (Join-Path $projectRoot "Saved/Screenshots/$name")
        if($file.LastWriteTimeUtc -lt $start) {throw "Stale capture: $name"}
        Copy-Item -LiteralPath $file.FullName -Destination $folder
        $images+=@{file=$name;sha256=(Get-FileHash -LiteralPath $file.FullName).Hash}
    }
    $completed+=@{case=$case;options=$options;images=$images;log_sha256=(Get-FileHash -LiteralPath $options.RuntimeLog).Hash;status='CAPTURED_NOT_REVIEWED'}
    $completed | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "CAPTURED $case $folder"
}
foreach($input in $inputs) {if((Get-FileHash -LiteralPath (Join-Path $repo $input.file)).Hash -ne $input.sha256) {throw "Input changed: $($input.file)"}}
Write-Output "LAKE_LANDSCAPE=$result"
