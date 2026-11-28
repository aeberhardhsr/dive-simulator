param([string]$PackageDirectory='Builds/WindowsNavigation')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$executable=Resolve-DivePackageExecutable $PackageDirectory
$result=Join-Path $repo ('artifacts/navigation-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
@{started_utc=[DateTime]::UtcNow.ToString('o');executable=$executable;executable_sha256=(Get-FileHash -LiteralPath $executable).Hash;
  source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)} |
  ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@()
foreach($shore in @($false,$true)) {
    foreach($entry in 0..2) {
        $kind=if($shore) {'shore'} else {'water'}
        $cases+=@{name="navigation-$kind-$entry";scenario='navigation';entry=$entry;NavShore=$shore;Viewport='1920x1080'}
    }
}
$cases+=@(@{name='navigation-1440p';scenario='navigation';entry=0;Viewport='2560x1440'},
          @{name='navigation-4k';scenario='navigation';entry=1;Viewport='3840x2160'},
          @{name='navigation-letterbox';scenario='navigation';entry=2;NavShore=$true;Viewport='1280x1024'},
          @{name='shore';scenario='shore';entry=2},@{name='controls';scenario='controls';entry=0},
          @{name='tutorial';scenario='tutorial';entry=0},@{name='daycycle';scenario='daycycle';entry=1;CelestialReferences=$true},@{name='sediment';scenario='sediment';entry=0;NoBubbles=$true})
$completed=@()
foreach($case in $cases) {
    $folder=Join-Path $result $case.name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario=$case.scenario;Entry=$case.entry;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in @('NavShore','Viewport','NoBubbles','CelestialReferences')) { if($case.ContainsKey($key)) { $options[$key]=$case[$key] } }
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $images=@(Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png' -File | Where-Object { $_.LastWriteTimeUtc -ge $started })
    $images | Copy-Item -Destination $folder
    $completed+=@{name=$case.name;scenario=$case.scenario;entry=$case.entry;shore=[bool]$case.NavShore;
                  viewport=$(if($case.Viewport) {$case.Viewport} else {'1600x900'});status='PASS';screenshots=$images.Count;started_utc=$started.ToString('o')}
    $completed | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $($case.name) $folder"
}
Write-Output "NAVIGATION_VALIDATION=$result"
