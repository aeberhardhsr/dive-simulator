param([string]$PackageDirectory='Builds/WindowsSediment')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$executable=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/sediment-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$executable;
  executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
  source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(@{name='sediment0';scenario='sediment';entry=0;NoBubbles=$true},
    @{name='sediment1';scenario='sediment';entry=1;NoBubbles=$true},
    @{name='sediment2';scenario='sediment';entry=2;NoBubbles=$true},
    @{name='sediment-night';scenario='sediment';entry=0;NoBubbles=$true;Hour=23},
    @{name='shore0';scenario='shore';entry=0},@{name='shore1';scenario='shore';entry=1},@{name='shore2';scenario='shore';entry=2},
    @{name='breathing';scenario='breathing';entry=0},@{name='stream';scenario='stream';entry=0})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in @('NoBubbles','Hour')) { if($case.ContainsKey($key)) { $options[$key]=$case[$key] } }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object { $_.LastWriteTimeUtc -ge $started })
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;status='PASS';started_utc=$started.ToString('o');screenshots=$images.Count}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "SEDIMENT_VALIDATION=$result"
