param([Parameter(Mandatory=$true)][string]$PackageDirectory,
    [ValidateRange(180,1800)][int]$TimeoutSeconds=1200,[string]$CaseName='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
$exe=Resolve-DivePackageExecutable $PackageDirectory
if(Get-Process | Where-Object {$_.ProcessName -like 'UnrealEditor*' -or $_.ProcessName -like 'divingsim*'}) {throw 'An Unreal/game process is already running.'}
$root=Join-Path $repo ('artifacts/production-water-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force $root | Out-Null
$files=@($exe,(Join-Path $PackageDirectory 'divingsim.exe'),(Join-Path $PackageDirectory 'Start-DiveSimulator.cmd'))
$files+=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Select-Object -ExpandProperty FullName)
$inputs=@($files | Sort-Object -Unique | ForEach-Object {@{file=$_;sha256=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant()}})
if((Get-FileHash (Join-Path $PackageDirectory 'Start-DiveSimulator.cmd')).Hash -ne (Get-FileHash (Join-Path $PSScriptRoot 'StartDiveSimulator.cmd')).Hash) {throw 'Normal launcher differs from source.'}
@{started_utc=[DateTime]::UtcNow.ToString('o');source_head=(& git -C $repo rev-parse HEAD);inputs=$inputs;
    source_dirty=[bool](& git -C $repo status --porcelain);note='Automated packaged scenarios. Normal interactive launch is a separate check.'} |
    ConvertTo-Json -Depth 5 | Set-Content (Join-Path $root 'Host.json') -Encoding utf8
$cases=@(
    @{name='shore-weesen';scenario='shore';entry=0;count=9},
    @{name='shore-murg';scenario='shore';entry=1;count=9},
    @{name='shore-mols';scenario='shore';entry=2;count=9},
    @{name='water-weesen';entry=0;habitat=$true;count=3},
    @{name='water-murg';entry=1;habitat=$true;count=3},
    @{name='water-walenstadt';entry=2;habitat=$true;count=3},
    @{name='line-weesen';pose=@(329004.7802942805,172781.7364295479,180,0);count=3},
    @{name='line-murg';pose=@(53031.55998047441,830331.2463976908,0,0);count=3},
    @{name='line-mols';pose=@(95502.19710376114,1469706.6948947497,0,0);count=3},
    @{name='clear';visibility=28;count=3},
    @{name='turbid';visibility=4;count=3},
    @{name='night-off';night=$true;count=3},
    @{name='night-lamp';night=$true;lamp=$true;count=3},
    @{name='sun-shadow';probe='sun';count=3},
    @{name='moon-shadow';probe='moon';count=3},
    @{name='software';noRT=$true;count=3},
    @{name='ui';scenario='ui';count=14},
    @{name='wrist';scenario='wrist';count=4;
        imageNames=@('DiveSmoke_0_wrist.png','DiveUnderwater_0_wrist.png','WristTexture_wrist.png','WristVitals_wrist.png')}
)
if($CaseName) {
    $cases=@($cases | Where-Object {$_.name -eq $CaseName})
    if($cases.Count -ne 1) {throw 'Unknown production case.'}
}
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $root $case.name
    New-Item -ItemType Directory -Force $folder | Out-Null
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;ProductionWater=$true;Scenario='rocks';Entry=[int]$case.entry;
        Hour=12;Date='2026-06-21';MaxFPS=30;TimeoutSeconds=$TimeoutSeconds;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    if($case.scenario) {$options.Scenario=$case.scenario}
    elseif($case.habitat) {$options.HabitatView=$true;$options.PopulationChecks=$true}
    else {
        $options.WaterView=$true;$options.WaterStart=0;$options.WaterEnd=0
        $options.WaterlinePose=@(329185.873,172781.736,180,0)
    }
    if($case.pose) {$options.WaterlinePose=$case.pose}
    if($case.visibility) {$options.Visibility=$case.visibility}
    if($case.night) {$options.Hour=1.5}
    if($case.lamp) {$options.WaterInspectionLamp=$true}
    if($case.noRT) {$options.DisableRayTracing=$true}
    if($case.probe) {
        $options.Remove('WaterlinePose')
        $options.ReflectionShadowProbe=$case.probe
        $options.NoReflectionScreenTraces=$true
        if($case.probe -eq 'moon') {$options.Date='2026-03-29';$options.Hour=1.98333333333333}
    }
    $started=[DateTime]::UtcNow
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $log=Get-Content $options.RuntimeLog -Raw
    if($log -match 'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used') {throw "Rendering failure: $($case.name)"}
    $images=@(Get-ChildItem (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne $case.count) {throw "Fresh capture count: $($case.name) $($images.Count)/$($case.count)"}
    if($case.imageNames -and @(Compare-Object @($case.imageNames | Sort-Object) @($images.Name | Sort-Object)).Count) {
        throw "Unexpected capture names: $($case.name)"
    }
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;status='CAPTURE_PASS';options=$options;
        images=@($images | ForEach-Object {@{file=$_.Name;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLowerInvariant()}});
        log_sha256=(Get-FileHash -LiteralPath $options.RuntimeLog).Hash.ToLowerInvariant()}
    $completed | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $root 'Cases.json') -Encoding utf8
    Write-Output "CAPTURE_PASS $($case.name)"
}
foreach($input in $inputs) {
    if((Get-FileHash -LiteralPath $input.file).Hash.ToLowerInvariant() -ne $input.sha256) {throw "Package changed: $($input.file)"}
}
Write-Output "PRODUCTION_WATER=$root"
