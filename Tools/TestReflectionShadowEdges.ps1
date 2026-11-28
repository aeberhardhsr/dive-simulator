param([ValidateRange(180,1800)][int]$TimeoutSeconds=1200,
 [ValidateSet('full','vsm','vsm-coverage')][string[]]$Profiles=@('full','vsm','vsm-coverage'),
 [string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$root=if($OutputDirectory){[IO.Path]::GetFullPath($OutputDirectory)}else{Join-Path $repo ('artifacts/reflection-shadow-edges-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))}
New-Item -ItemType Directory -Force $root | Out-Null
$results=@()
if(Test-Path (Join-Path $root 'Runs.json')) {$results=@(Get-Content (Join-Path $root 'Runs.json') -Raw | ConvertFrom-Json)}
# Four actual scene/resource controls, each in normal, reference and diagnostic
# rendering. Each control captures clear / blocked / clear at one camera pose.
foreach($profile in $Profiles) {
 if($results | Where-Object {$_.profile -eq $profile}) {throw "Profile already recorded: $profile"}
 $output=Join-Path $root ($profile+'.log')
 & (Join-Path $PSScriptRoot 'TestReflectionTransport.ps1') -Unified -Profile $profile -Suite shadows -TimeoutSeconds $TimeoutSeconds *> $output
 $text=Get-Content $output -Raw
 $match=[regex]::Match($text,'REFLECTION_TRANSPORT=([^\r\n]+)')
 if(-not $match.Success) {throw "Incomplete shadow edge suite: $profile"}
 $results+=@{profile=$profile;directory=$match.Groups[1].Value.Trim()}
 $results | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $root 'Runs.json') -Encoding utf8
 Write-Output "PASS $profile"
}
Write-Output "REFLECTION_SHADOW_EDGES=$root"
