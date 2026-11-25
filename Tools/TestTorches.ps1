param([string]$PackageDirectory='Builds/WindowsTorch')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'PackageExecutable.ps1')
if(-not [IO.Path]::IsPathRooted($PackageDirectory)) { $PackageDirectory=Join-Path $repo $PackageDirectory }
$result=Join-Path $repo ('artifacts/torch-validation-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $result -Force | Out-Null
$exe=Resolve-DivePackageExecutable $PackageDirectory
@{
    started_utc=[DateTime]::UtcNow.ToString('o');executable=$exe
    executable_sha256=(Get-FileHash -LiteralPath $exe).Hash
    source_head=(& git -C $repo rev-parse HEAD);source_dirty=[bool](& git -C $repo status --porcelain)
    background_processes=@(Get-Process UnrealEditor,divingsim* -ErrorAction SilentlyContinue | Select-Object Id,ProcessName,StartTime)
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Host.json') -Encoding utf8
$cases=@(
    @{Name='allround';Torch=0}, @{Name='spot';Torch=1}, @{Name='wide';Torch=2},
    @{Name='off';LampOff=$true}, @{Name='empty';Battery=0}, @{Name='low';Battery=5},
    @{Name='turbid';Visibility=3}, @{Name='clear';Visibility=30}, @{Name='day';Hour=12},
    @{Name='day-spot';Torch=1;Hour=12}, @{Name='day-wide';Torch=2;Hour=12},
    @{Name='config';Scenario='config'}, @{Name='profiles';Scenario='profiles'},
    @{Name='surface';Scenario='surface-calm';Hour=12},
    @{Name='storm';Scenario='surface-storm';Hour=12}
)
$completed=@()
foreach($case in $cases) {
    $name=$case.Name
    $options=@{Packaged=$true;PackageDirectory=$PackageDirectory;Scenario='torch';Entry=1;MaxFPS=30}
    foreach($key in $case.Keys) { if($key -ne 'Name') { $options[$key]=$case[$key] } }
    $folder=Join-Path $result $name
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $started=[DateTime]::UtcNow
    $runtime=Join-Path $repo 'artifacts/DiveSmoke.log'
    try { & (Join-Path $PSScriptRoot 'SmokeTest.ps1') @options *> (Join-Path $folder 'Summary.log') }
    finally { if(Test-Path -LiteralPath $runtime) { Copy-Item -LiteralPath $runtime -Destination (Join-Path $folder 'Runtime.log') } }
    $saved=Join-Path $PackageDirectory 'divingsim/Saved'
    Get-ChildItem -LiteralPath (Join-Path $saved 'Screenshots') -File -Filter '*.png' |
        Where-Object { $_.LastWriteTimeUtc -ge $started } | Copy-Item -Destination $folder
    # Copy the exact final recording referenced by the smoke result, not a
    # directory's newest file (another game may have its own active recording).
    $log=Get-Content -LiteralPath $runtime -Raw
    $match=[regex]::Match($log,'DIVE_SMOKE_PASS[^\r\n]* log=([^\r\n]+)')
    if($match.Success) {
        # Packaged Unreal logs paths relative to its executable directory.
        $recording=Join-Path $saved ('DiveLogs/'+[IO.Path]::GetFileName($match.Groups[1].Value.Trim()))
        Copy-Item -LiteralPath $recording -Destination (Join-Path $folder 'Dive.json')
    }
    $completed+=@{name=$name;options=$options;started_utc=$started.ToString('o');completed_utc=[DateTime]::UtcNow.ToString('o')}
    $completed | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $result 'Cases.json') -Encoding utf8
    Write-Output "PASS $name $folder"
}
Write-Output "TORCH_VALIDATION=$result"
