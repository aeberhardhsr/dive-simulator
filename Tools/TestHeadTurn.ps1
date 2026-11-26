param([string]$PackageDirectory='Builds/WindowsHeadTurn')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$executable=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/headturn-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$executable;
  executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
  source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='sediment0';scenario='sediment';entry=0;NoBubbles=$true},
    @{name='sediment1';scenario='sediment';entry=1;NoBubbles=$true},@{name='sediment2';scenario='sediment';entry=2;NoBubbles=$true},
    @{name='shore0';scenario='shore';entry=0},@{name='shore1';scenario='shore';entry=1},@{name='shore2';scenario='shore';entry=2},
    @{name='avatar0';scenario='avatar';entry=0;LoadoutCase=0},@{name='avatar5';scenario='avatar';entry=0;LoadoutCase=5},
    @{name='breathing';scenario='breathing';entry=0},@{name='wrist';scenario='wrist';entry=1},
    @{name='stream1';scenario='stream';entry=1;PopulationChecks=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in @('NoBubbles','NavShore','PopulationChecks','LoadoutCase')) { if($case.ContainsKey($key)) { $options[$key]=$case[$key] } }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object { $_.LastWriteTimeUtc -ge $started })
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;status='PASS';started_utc=$started.ToString('o');screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "HEADTURN_VALIDATION=$result"
