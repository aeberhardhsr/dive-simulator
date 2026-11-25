$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'PrepareEditorSandbox.ps1')
$repo=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $repo ('artifacts/sandbox-sync-'+[Guid]::NewGuid().ToString('N'))
foreach($name in @('Source','Config','Content')) { New-Item -ItemType Directory -Path (Join-Path $fixture "divingsim/$name") -Force | Out-Null }
Set-Content -LiteralPath (Join-Path $fixture 'divingsim/divingsim.uproject') -Value '{}' -Encoding utf8
$header=Join-Path $fixture 'divingsim/Source/Layout.h'
$target=Join-Path $fixture 'Builds/EditorSandbox/divingsim/Source/Layout.h'
Set-Content -LiteralPath $header -Value 'struct Layout { int Old; };' -Encoding utf8
Get-DiveEditorSandbox -RepositoryRoot $fixture | Out-Null
$first=(Get-Item -LiteralPath $target).LastWriteTimeUtc
Get-DiveEditorSandbox -RepositoryRoot $fixture | Out-Null
if((Get-Item -LiteralPath $target).LastWriteTimeUtc -ne $first) { throw 'Unchanged source should preserve the snapshot timestamp.' }
# Reproduce a source edit whose timestamp predates an object compiled against
# the PREVIOUS snapshot. Copying that old timestamp caused mixed class layouts.
Set-Content -LiteralPath $header -Value 'struct Layout { double New; int Old; };' -Encoding utf8
(Get-Item -LiteralPath $header).LastWriteTimeUtc=[DateTime]::SpecifyKind([DateTime]'2020-01-01',[DateTimeKind]::Utc)
$before=[DateTime]::UtcNow
Get-DiveEditorSandbox -RepositoryRoot $fixture | Out-Null
if((Get-Item -LiteralPath $target).LastWriteTimeUtc -lt $before) { throw 'Changed snapshot header retained a stale timestamp.' }
if((Get-FileHash -LiteralPath $header).Hash -ne (Get-FileHash -LiteralPath $target).Hash) { throw 'Snapshot content differs.' }
Write-Output "EDITOR_SANDBOX_SYNC_PASS old_timestamp_edit=detected unchanged_timestamp=preserved fixture=$fixture"
