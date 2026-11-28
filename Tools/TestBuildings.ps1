param([string]$PackageDirectory='Builds/WindowsBuildings',[string]$ResumeDirectory='',[switch]$ShoreSeams)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$executable=Resolve-DivePackageExecutable $PackageDirectory
$prefix=if($ShoreSeams) { 'shore-seam' } else { 'buildings' }
$result=if($ResumeDirectory) { [IO.Path]::GetFullPath($ResumeDirectory) } else { Join-Path $repo ('artifacts/'+$prefix+'-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss')) }
New-Item -ItemType Directory -Path $result -Force | Out-Null
if($ResumeDirectory) {
    $hostData=Get-Content -LiteralPath (Join-Path $result 'Host.json') -Raw | ConvertFrom-Json
    if($hostData.executable_sha256 -ne (Get-FileHash -LiteralPath $executable).Hash) { throw 'Cannot resume checks for a different executable.' }
    if([bool]$hostData.shore_seams -ne [bool]$ShoreSeams) { throw 'Cannot change validation scope while resuming.' }
} else {
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$executable;shore_seams=[bool]$ShoreSeams;
  executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
  source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
}
$cases=@(@{name='buildings0';scenario='shore-preview';entry=0;BuildingsView=$true},
    @{name='buildings1';scenario='shore-preview';entry=1;BuildingsView=$true},
    @{name='buildings2';scenario='shore-preview';entry=2;BuildingsView=$true},
    @{name='fallback1';scenario='shore-preview';entry=1;BuildingsView=$true;DisableNanite=$true},
    @{name='shore0';scenario='shore';entry=0},@{name='shore1';scenario='shore';entry=1},@{name='shore2';scenario='shore';entry=2},
    @{name='sediment0';scenario='sediment';entry=0;NoBubbles=$true},
    @{name='avatar0';scenario='avatar';entry=0;LoadoutCase=0},
    @{name='breathing';scenario='breathing';entry=0},@{name='wrist';scenario='wrist';entry=1},
    @{name='stream1';scenario='stream';entry=1;PopulationChecks=$true})
$completed=@()
if($ResumeDirectory) { $completed=@(Get-Content -LiteralPath (Join-Path $result 'Cases.json') -Raw | ConvertFrom-Json) }
foreach($case in $cases) {
    if($completed | Where-Object { $_.name -eq $case.name -and $_.status -eq 'PASS' }) { continue }
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in @('NoBubbles','PopulationChecks','LoadoutCase','BuildingsView','DisableNanite')) { if($case.ContainsKey($key)) { $options[$key]=$case[$key] } }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    if((Get-Content -LiteralPath (Join-Path $folder 'Runtime.log') -Raw) -notmatch 'DIVE_BUILDINGS_PASS buildings=16249 cells=715') { throw 'The target package did not validate the complete building dataset.' }
    if($ShoreSeams -and $case.scenario -in @('shore-preview','shore')) {
        $log=Get-Content -LiteralPath (Join-Path $folder 'Runtime.log') -Raw
        foreach($index in 0..2) {
            if($log -notmatch ('DIVE_SHORE_SEAM_PASS entry='+$index+' ')) { throw "Missing source-boundary validation for shore $index." }
        }
    }
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object { $_.LastWriteTimeUtc -ge $started })
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;status='PASS';started_utc=$started.ToString('o');screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "BUILDINGS_VALIDATION=$result"
