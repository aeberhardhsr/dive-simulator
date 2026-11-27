function Get-DiveEditorSandbox {
    param([string]$RepositoryRoot=(Split-Path $PSScriptRoot -Parent))
    # Separate binaries/intermediates keep a user-owned editor's loaded DLL intact.
    # Content remains shared so generated assets belong to the real project.
    $repo=[IO.Path]::GetFullPath($RepositoryRoot)
    $original=Join-Path $repo 'divingsim'
    $sandbox=[IO.Path]::GetFullPath((Join-Path $repo 'Builds/EditorSandbox/divingsim'))
    foreach($relative in @('Builds','Builds/EditorSandbox','Builds/EditorSandbox/divingsim')) {
        $path=Join-Path $repo $relative
        if((Test-Path -LiteralPath $path) -and ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Sandbox parent must be a real directory: $path" }
    }
    New-Item -ItemType Directory -Path $sandbox -Force | Out-Null
    foreach($name in @('Source','Config','Shaders')) {
        $source=Join-Path $original $name
        $destination=Join-Path $sandbox $name
        if(Test-Path -LiteralPath $destination) {
            if((Get-Item -LiteralPath $destination).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Sandbox $name must be a real directory." }
            if(Get-ChildItem -LiteralPath $destination -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }) { throw "Sandbox $name contains a link; refusing to synchronize." }
            foreach($file in Get-ChildItem -LiteralPath $destination -Recurse -File) {
                $relative=$file.FullName.Substring($destination.Length+1)
                if(-not (Test-Path -LiteralPath (Join-Path $source $relative))) {
                    if(-not $file.FullName.StartsWith($sandbox+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Unsafe sandbox file path.' }
                    Remove-Item -LiteralPath $file.FullName
                }
            }
        }
        New-Item -ItemType Directory -Path $destination -Force | Out-Null
        foreach($file in Get-ChildItem -LiteralPath $source -Recurse -File) {
            $relative=$file.FullName.Substring($source.Length+1)
            $target=Join-Path $destination $relative
            $changed=-not (Test-Path -LiteralPath $target)
            if(-not $changed) { $changed=(Get-FileHash -LiteralPath $file.FullName).Hash -ne (Get-FileHash -LiteralPath $target).Hash }
            if($changed) {
                New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
                Copy-Item -LiteralPath $file.FullName -Destination $target -Force
                # Source edits may happen during a build of the prior snapshot.
                # Preserving their older timestamp could leave objects compiled
                # later against the OLD header falsely considered up to date.
                (Get-Item -LiteralPath $target).LastWriteTimeUtc=[DateTime]::UtcNow
            }
        }
    }
    $content=Join-Path $sandbox 'Content'
    $expected=[IO.Path]::GetFullPath((Join-Path $original 'Content'))
    if(Test-Path -LiteralPath $content) {
        $link=Get-Item -LiteralPath $content
        if($link.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath($link.Target) -ne $expected) { throw 'Unexpected sandbox Content target.' }
    } else { New-Item -ItemType Junction -Path $content -Target $expected | Out-Null }
    Copy-Item -LiteralPath (Join-Path $original 'divingsim.uproject') -Destination $sandbox -Force
    return (Join-Path $sandbox 'divingsim.uproject')
}
