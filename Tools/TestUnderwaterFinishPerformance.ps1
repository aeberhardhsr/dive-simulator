param([Parameter(Mandatory=$true)][string]$PackageDirectory,
    [ValidateSet('baseline','confirmation')][string]$Pass='baseline')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
$root=Join-Path $repo ('artifacts/underwater-finish-performance-'+$Pass+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$packageFiles=@($exe)+@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Select-Object -ExpandProperty FullName)
$identity=@($packageFiles | Sort-Object -Unique | ForEach-Object {@{file=$_;sha256=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant()}})
function Assert-DiveMeasurementIdle {
    $busy=@(Get-CimInstance Win32_Process | Where-Object {
        $_.Name -like 'UnrealEditor*' -or $_.Name -like 'divingsim*' -or
        ($_.Name -eq 'dotnet.exe' -and $_.CommandLine -like '*UnrealBuildTool.dll*')
    })
    if($busy.Count) {throw 'A game, editor or Unreal build is still running; defer this measurement.'}
}
$cases=@(
    @{name='weesen-water-underside';entry=0;scenario='underwater';seconds=60;pitch=35;location='Weesen / West'},
    @{name='walenstadt-bed';entry=2;scenario='underwater';seconds=60;pitch=-12;location='Walenstadt / Ost'},
    @{name='murg-streaming';entry=1;scenario='stream';seconds=90;location='Murg / Süd'}
)
$results=@()
foreach($case in $cases) {
    Assert-DiveMeasurementIdle
    $options=@{PackageDirectory=$PackageDirectory;ProductionWater=$true;Scenario=$case.scenario;Entry=$case.entry;
        Seconds=$case.seconds;Resolution='1080p';Quality=2;RenderScale=100}
    if($case.ContainsKey('pitch')) {$options.LookPitch=$case.pitch}
    $log=Join-Path $root ($case.name+'.log')
    & (Join-Path $PSScriptRoot 'Benchmark.ps1') @options *> $log
    $match=[regex]::Match((Get-Content $log -Raw),'DIVE_BENCHMARK_CAPTURE_READY ([^\r\n]+)')
    if(-not $match.Success) {throw "No completed capture: $($case.name)"}
    $capture=$match.Groups[1].Value.Trim()
    $summary=Get-Content (Join-Path $capture 'Summary.json') -Raw | ConvertFrom-Json
    $metadata=$summary.metadata
    if($metadata.scenario -ne ('perf-'+$case.scenario) -or $metadata.requested_capture_seconds -ne $case.seconds -or
        $metadata.warmup_seconds -ne 15 -or $metadata.width -ne 1920 -or $metadata.height -ne 1080 -or $metadata.quality -ne 2 -or
        $metadata.gpu -notmatch 'RTX\s*3090' -or $metadata.cpu -notmatch '9950X3D') {throw 'Capture differs from the agreed route, duration or host.'}
    $runtime=Get-Content (Join-Path $capture 'Runtime.log') -Raw
    if($runtime -notmatch ('LogInit: Command Line:[^\r\n]*-DiveEntry='+$case.entry+'(?:\s|$)')) {throw 'Runtime did not use the requested entry.'}
    $failures=@()
    if($summary.average_fps -lt 60) {$failures+='Mean FPS below 60'}
    if($summary.frame.p95_ms -gt 20) {$failures+='Frame p95 above 20 ms'}
    if($summary.frame.max_ms -gt 100) {$failures+='Interior frame above 100 ms'}
    $results+=@{name=$case.name;location=$case.location;start_mode='direct-water';entry=$case.entry;options=$options;
        status=$(if($failures.Count){'FAIL'}else{'NUMERICAL_PASS'});failures=$failures;directory=$capture;summary=$summary;
        evidence=@('Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png','Summary.json' | ForEach-Object {
            $file=Join-Path $capture $_
            @{file=$file;sha256=(Get-FileHash -LiteralPath $file).Hash.ToLowerInvariant()}
        })}
    @{pass=$Pass;status='IN_PROGRESS';package=$identity;routes=$results} | ConvertTo-Json -Depth 20 | Set-Content (Join-Path $root 'Results.json') -Encoding utf8
    Write-Output "$($results[-1].status) $($case.name)"
}
foreach($item in $identity) {
    if((Get-FileHash -LiteralPath $item.file).Hash.ToLowerInvariant() -ne $item.sha256) {throw 'Package changed during measurements.'}
}
$failed=@($results | Where-Object {$_.status -eq 'FAIL'}).Count -gt 0
@{pass=$Pass;status=$(if($failed){'FAIL'}else{'NUMERICAL_PASS_REQUIRES_IMAGE_REVIEW'});package=$identity;routes=$results;
    criteria=@{mean_fps_min=60;p95_ms_max=20;maximum_interior_frame_ms=100;warmup_seconds=15;render_scale_percent=100};
    scope='Three fixed packaged routes on the specified host. Review final images and streaming before acceptance; this is not a global performance guarantee.'} |
    ConvertTo-Json -Depth 20 | Set-Content (Join-Path $root 'Results.json') -Encoding utf8
Write-Output "UNDERWATER_FINISH_PERFORMANCE=$root"
if($failed) {throw 'At least one agreed performance criterion failed. Full measurements remain in Results.json.'}
