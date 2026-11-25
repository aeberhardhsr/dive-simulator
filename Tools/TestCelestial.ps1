param([string]$PackageDirectory='Builds/WindowsCelestial')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$exe=Resolve-DivePackageExecutable $PackageDirectory
$folder=Join-Path $repo ('artifacts/celestial-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $folder -Force | Out-Null
$cases=@(
    @{Name='summer-noon';Scenario='surface-calm';Entry=1;Date='2026-06-21';Hour=13;LampOff=$true;CelestialReferences=$true},
    @{Name='winter-noon';Scenario='surface-calm';Entry=1;Date='2026-12-21';Hour=13;LampOff=$true},
    @{Name='summer-dusk';Scenario='surface-calm';Entry=1;Date='2026-06-21';Hour=(21+20/60);LampOff=$true},
    @{Name='winter-night';Scenario='surface-calm';Entry=1;Date='2026-12-23';Hour=23;LampOff=$true;SkyView=$true},
    @{Name='summer-night';Scenario='surface-calm';Entry=1;Date='2026-06-21';Hour=23.5;LampOff=$true;SkyView=$true;SkyYaw=245;SkyPitch=20},
    @{Name='config';Scenario='config'},
    @{Name='profiles';Scenario='profiles'},
    @{Name='spring-clock';Scenario='daycycle';Date='2026-03-29';Hour=(1+59/60);LampOff=$true},
    @{Name='autumn-clock';Scenario='daycycle';Date='2026-10-25';Hour=(2+59/60);LampOff=$true},
    @{Name='utc-midnight';Scenario='daycycle';Date='2026-06-21';Hour=(1+59/60);LampOff=$true},
    @{Name='local-midnight';Scenario='daycycle';Date='2026-12-21';Hour=(23+59/60);LampOff=$true},
    @{Name='night-torch';Scenario='wrist-night';Entry=1;Date='2026-12-23'},
    @{Name='storm';Scenario='surface-storm';Entry=1;Date='2026-06-21';Hour=13}
)
$summary=[ordered]@{
    started_utc=[DateTime]::UtcNow.ToString('o'); completed=$false
    executable=$exe; executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    source_head=(& git -C $repo rev-parse HEAD); source_dirty=[bool](& git -C $repo status --porcelain)
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime)
    cases=@()
}
$summaryPath=Join-Path $folder 'Summary.json'
try {
    foreach($case in $cases) {
        $name=$case.Name; $options=$case.Clone(); $options.Remove('Name')
        $caseFolder=Join-Path $folder $name; New-Item -ItemType Directory -Path $caseFolder | Out-Null
        $started=[DateTime]::UtcNow
        Write-Output "START $name"
        & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options -Packaged -PackageDirectory $PackageDirectory -MaxFPS 30 *> (Join-Path $caseFolder 'Result.log')
        Copy-Item -LiteralPath (Join-Path $repo 'artifacts/DiveSmoke.log') -Destination (Join-Path $caseFolder 'Runtime.log')
        foreach($file in Get-ChildItem -LiteralPath (Join-Path $PackageDirectory 'divingsim/Saved/Screenshots') -Filter '*.png') {
            if($file.LastWriteTimeUtc -ge $started) { Copy-Item -LiteralPath $file.FullName -Destination $caseFolder }
        }
        $logs=Join-Path $PackageDirectory 'divingsim/Saved/DiveLogs'
        if(Test-Path -LiteralPath $logs) {
            foreach($file in Get-ChildItem -LiteralPath $logs -Filter '*.json') {
                if($file.LastWriteTimeUtc -ge $started) { Copy-Item -LiteralPath $file.FullName -Destination $caseFolder }
            }
        }
        $summary.cases+=@{name=$name;options=$options;started_utc=$started.ToString('o');finished_utc=[DateTime]::UtcNow.ToString('o');passed=$true}
        $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding utf8
        Write-Output "PASS $name"
    }
    if((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash -ne $summary.executable_sha256) { throw 'Executable changed during validation.' }
    $summary.completed=$true
} catch {
    $summary.error=$_.Exception.Message
    if($caseFolder -and (Test-Path -LiteralPath (Join-Path $repo 'artifacts/DiveSmoke.log'))) {
        Copy-Item -LiteralPath (Join-Path $repo 'artifacts/DiveSmoke.log') -Destination (Join-Path $caseFolder 'Runtime.log')
    }
    throw
} finally {
    $summary.finished_utc=[DateTime]::UtcNow.ToString('o')
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding utf8
    Write-Output "Artifacts: $folder"
}
