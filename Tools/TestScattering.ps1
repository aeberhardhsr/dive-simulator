param([string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$sandbox=Join-Path $repo 'Builds/EditorSandbox/divingsim'
$result=Join-Path $repo ('artifacts/scattering-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'Close the existing Unreal/game instance before running the comparison.'}
New-Item -ItemType Directory -Force $result | Out-Null
$sources=@(Get-ChildItem (Join-Path $repo 'divingsim/Source/DiveSim') -Recurse -File | Sort-Object FullName | ForEach-Object {
    $relative=$_.FullName.Substring((Join-Path $repo 'divingsim').Length+1)
    $copy=Join-Path $sandbox $relative
    $hash=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()
    if(-not (Test-Path -LiteralPath $copy) -or (Get-FileHash -LiteralPath $copy).Hash.ToLowerInvariant() -ne $hash) {throw "Sandbox source differs: $relative"}
    @{path=$relative.Replace('\','/');sha256=$hash}
})
$dll=Get-Item -LiteralPath (Join-Path $sandbox 'Binaries/Win64/UnrealEditor-DiveSim.dll')
@{started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);
    native_editor=$true;packaged=$false;engine_root=$EngineRoot;sources=$sources;
    module=@{path=$dll.FullName;sha256=(Get-FileHash -LiteralPath $dll.FullName).Hash.ToLowerInvariant();modified_utc=$dll.LastWriteTimeUtc.ToString('o')};
    entry=2;hour=12;viewport='1600x900';max_fps=30;lamp_on=$false} |
    ConvertTo-Json -Depth 8 | Set-Content (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{name='baseline';fog=1;view=1;defaults=$true},
    @{name='no-fog-addition';fog=0;view=1},
    @{name='no-view-addition';fog=1;view=0},
    @{name='no-additions';fog=0;view=0}
)
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{EditorSandbox=$true;EngineRoot=$EngineRoot;Scenario='rocks';HabitatView=$true;Entry=2;Hour=12;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if(-not $case.defaults) {$options.FogScatterGain=$case.fog;$options.ViewScatterGain=$case.view}
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:') {throw "Scattering run failed: $($case.name)"}
    foreach($view in 0..2) {
        $capture=Get-Item -LiteralPath (Join-Path $sandbox "Saved/Screenshots/Habitat_0_$view.png")
        if($capture.LastWriteTimeUtc -lt $started -or $capture.Length -lt 10000) {throw "Missing fresh capture: $($case.name) $view"}
        Copy-Item -LiteralPath $capture.FullName -Destination $folder
    }
    Write-Output "PASS $($case.name) $folder"
}
$cases | ConvertTo-Json | Set-Content (Join-Path $result 'Cases.json') -Encoding utf8
Write-Output "SCATTERING_VALIDATION=$result"
