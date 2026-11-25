param([string]$PackageDirectory='Builds/WindowsEnvironment', [string]$ResumeDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$result=if($ResumeDirectory) { [IO.Path]::GetFullPath($ResumeDirectory) } else { Join-Path $repo ('artifacts/environment-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss')) }
New-Item -ItemType Directory -Path $result -Force | Out-Null
$exe=Resolve-DivePackageExecutable $PackageDirectory
if($ResumeDirectory) {
    $hostRecord=Get-Content -LiteralPath (Join-Path $result 'Host.json') -Raw | ConvertFrom-Json
    if($hostRecord.executable_sha256 -ne (Get-FileHash -LiteralPath $exe).Hash) { throw 'Cannot resume with a different executable.' }
} else {
@{
    started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe
    executable_sha256=(Get-FileHash -LiteralPath $exe).Hash
    source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
}
$cases=@(
    @{Name='environment';Scenario='environment'},
    @{Name='winter';Scenario='dive';AutomaticEnvironment=$true;Date='2026-02-15'},
    @{Name='summer';Scenario='dive';AutomaticEnvironment=$true;Date='2026-08-15'},
    @{Name='manual';Scenario='dive'},
    @{Name='config';Scenario='config'}, @{Name='profiles';Scenario='profiles'},
    @{Name='shore';Scenario='shore';Entry=0}, @{Name='torch';Scenario='torch';Entry=1}
)
$completed=@()
if($ResumeDirectory) { $completed=@(Get-Content -LiteralPath (Join-Path $result 'Cases.json') -Raw | ConvertFrom-Json) }
foreach($case in $cases) {
    $name=$case.Name
    $folder=Join-Path $result $name
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario='dive';Entry=0;MaxFPS=30;RuntimeLog=(Join-Path $folder 'Runtime.log')}
    foreach($key in $case.Keys) { if($key -ne 'Name') { $options[$key]=$case[$key] } }
    $previous=$completed | Where-Object name -eq $name
    if($previous) {
        foreach($key in $options.Keys) {
            if(-not $previous.options.PSObject.Properties[$key] -or [string]$previous.options.$key -ne [string]$options[$key]) { throw "Cannot resume changed options for $name." }
        }
        if(@($previous.options.PSObject.Properties).Count -ne $options.Count) { throw "Cannot resume changed options for $name." }
        continue
    }
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $runtime=$options.RuntimeLog
    & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log')
    $saved=Join-Path $PackageDirectory 'divingsim/Saved'
    Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -File -Filter '*.png' |
        Where-Object { $_.LastWriteTimeUtc -ge $started } | Copy-Item -Destination $folder
    # Copy the exact final recording referenced by the smoke result, not a
    # directory's newest file (another game may have its own active recording).
    $log=Get-Content -LiteralPath $runtime -Raw
    $match=[regex]::Match($log,'DIVE_SMOKE_PASS[^\r\n]* log=([^\r\n]+)')
    if($name -eq 'environment') { $match=[regex]::Match($log,'DIVE_LOG_SAVED status=smoke_environment_completed[^\r\n]* path=([^\r\n]+)') }
    if($match.Success) {
        # Packaged Unreal logs paths relative to its executable directory.
        $recording=Join-Path $saved ('DiveLogs/'+[IO.Path]::GetFileName($match.Groups[1].Value.Trim()))
        Copy-Item -LiteralPath $recording -Destination (Join-Path $folder 'Dive.json')
    }
    $completed+=@{name=$name;options=$options;started_utc=$started.ToString('o');completed_utc=[DateTime]::UtcNow.ToString('o')}
    $completed | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $name $folder"
}
Write-Output "ENVIRONMENT_VALIDATION=$result"
