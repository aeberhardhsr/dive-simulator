param([string]$PackageDirectory='Builds/WindowsCurrent', [switch]$WaterPreview,
    [ValidateSet('1600x900','1920x1080','1280x1024')][string[]]$Viewports=@('1600x900','1920x1080','1280x1024'),
    [ValidateRange(30,7200)][int]$TimeoutSeconds=300)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) {$PackageDirectory=Join-Path $repo $PackageDirectory}
$exe=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/playable-ui-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$inputs=@($exe)+(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Content/Paks') -File | Select-Object -ExpandProperty FullName)
$identities=@($inputs | ForEach-Object {@{path=$_;sha256=(Get-FileHash -LiteralPath $_).Hash.ToLowerInvariant()}})
@{started_utc=[DateTime]::UtcNow.ToString('o');water_preview=[bool]$WaterPreview;inputs=$identities;
    source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$completed=@()
foreach($viewport in $Viewports) {
    $folder=Join-Path $result $viewport
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario='ui';Viewport=$viewport;
        MaxFPS=30;TimeoutSeconds=$TimeoutSeconds;RuntimeLog=(Join-Path $folder 'Runtime.log');WaterPreview=[bool]$WaterPreview}
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -File -Filter 'UI_*.png' |
        Where-Object {$_.LastWriteTimeUtc -ge $started})
    if($images.Count -ne 14) {throw "Expected 14 fresh screenshots for $viewport, found $($images.Count)."}
    $images | Copy-Item -Destination $folder
    $completed+=@{viewport=$viewport;status='PASS';started_utc=$started.ToString('o');completed_utc=[DateTime]::UtcNow.ToString('o');
        images=@($images | ForEach-Object {@{name=$_.Name;sha256=(Get-FileHash -LiteralPath (Join-Path $folder $_.Name)).Hash.ToLowerInvariant()}})}
    $completed | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $viewport $folder"
}
foreach($item in $identities) {
    if((Get-FileHash -LiteralPath $item.path).Hash.ToLowerInvariant() -ne $item.sha256) {throw "Package changed during test: $($item.path)"}
}
Write-Output "PLAYABLE_UI_VALIDATION=$result"
