# Resolve the executable recorded by UAT, including optional named build slots.
# Do not choose a stale default executable left in an existing archive directory.
function Resolve-DivePackageExecutable([string]$PackageDirectory) {
    $root=[IO.Path]::GetFullPath($PackageDirectory)
    $manifest=Join-Path $root 'Manifest_NonUFSFiles_Win64.txt'
    $entries=@(Get-Content -LiteralPath $manifest | ForEach-Object {
        $relative=($_ -split "`t",2)[0]
        if($relative -match '^divingsim/Binaries/Win64/divingsim(?:-[A-Za-z0-9_]{1,24})?\.exe$') { $relative }
    } | Select-Object -Unique)
    if($entries.Count -ne 1) { throw "Expected one game executable in $manifest; found $($entries.Count)." }
    $executable=Join-Path $root $entries[0]
    if(-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Packaged executable missing: $executable" }
    return $executable
}
