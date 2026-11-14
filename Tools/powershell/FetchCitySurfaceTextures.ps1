#Requires -Version 5.1
<#
.SYNOPSIS
Restores or verifies the pinned CC0 Poly Haven procedural-city texture pack.
.DESCRIPTION
Uses city-surfaces.json beside the shipped textures. Files are verified against
their SHA-256, upstream MD5, encoded byte count and 1024x1024 dimensions. A
download is staged and validated before replacing an existing file. No images
are resized, repacked, recolored or otherwise modified.
.PARAMETER VerifyOnly
Checks local files without network access or filesystem changes.
.PARAMETER Force
Allows replacing an invalid existing texture after its replacement verifies.
.PARAMETER DestinationDirectory
Optional restore location; the default is bin/Media/Procedural/City/Textures.
.EXAMPLE
.\Tools\powershell\FetchCitySurfaceTextures.ps1 -VerifyOnly
#>
[CmdletBinding()]
param(
    [string]$DestinationDirectory,
    [string]$ManifestPath,
    [switch]$VerifyOnly,
    [switch]$Force
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$defaultDirectory = Join-Path $repositoryRoot 'bin\Media\Procedural\City\Textures'
if (-not $DestinationDirectory) { $DestinationDirectory = $defaultDirectory }
if (-not $ManifestPath) { $ManifestPath = Join-Path $defaultDirectory 'city-surfaces.json' }
$DestinationDirectory = [IO.Path]::GetFullPath($DestinationDirectory)
$manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or $manifest.assets.Count -ne 6) {
    throw 'Expected the six-surface city manifest, schema version 1.'
}

$maps = @($manifest.assets | ForEach-Object {
    if ($_.maps.Count -ne 4) { throw ('Expected four maps for ' + $_.id) }
    $_.maps
})
$names = @{}
$totalBytes = 0L
foreach ($map in $maps) {
    if ($map.file -notmatch '^[a-z0-9_]+_1k\.(jpg|png)$' -or $names.ContainsKey($map.file)) {
        throw ('Invalid or duplicate manifest filename: ' + $map.file)
    }
    $names[$map.file] = $true
    $uri = [Uri]$map.url
    if ($uri.Scheme -ne 'https' -or $uri.Host -ne 'dl.polyhaven.org' -or
        $uri.AbsolutePath -notmatch '^/file/ph-assets/Textures/(jpg|png)/1k/[a-z0-9_]+/[a-z0-9_]+_1k\.(jpg|png)$') {
        throw ('Expected an official 1K Poly Haven download URL: ' + $map.file)
    }
    if ($map.sha256 -notmatch '^[a-f0-9]{64}$' -or $map.upstreamMd5 -notmatch '^[a-f0-9]{32}$' -or
        $map.bytes -le 0 -or $map.bytes -gt 6500000 -or
        $map.dimensions.Count -ne 2 -or $map.dimensions[0] -ne 1024 -or $map.dimensions[1] -ne 1024) {
        throw ('Invalid bounded texture metadata: ' + $map.file)
    }
    $totalBytes += [long]$map.bytes
}
if ($totalBytes -ge 40000000 -or $totalBytes -ne $manifest.totalEncodedBytes) {
    throw 'The texture pack exceeds its 40 MB budget or has an inconsistent manifest.'
}
Add-Type -AssemblyName System.Drawing

function Assert-TextureFile {
    param([string]$Path, [object]$Map)
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw ('Missing texture: ' + $Map.file) }
    if ((Get-Item -LiteralPath $Path).Length -ne $Map.bytes) { throw ('Byte count mismatch: ' + $Map.file) }
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Map.sha256 -or
        (Get-FileHash -LiteralPath $Path -Algorithm MD5).Hash.ToLowerInvariant() -ne $Map.upstreamMd5) {
        throw ('Texture checksum mismatch: ' + $Map.file)
    }
    # Decode only after the pinned bytes/hash checks; dispose immediately.
    $image = [Drawing.Image]::FromFile($Path)
    try {
        if ($image.Width -ne $Map.dimensions[0] -or $image.Height -ne $Map.dimensions[1]) {
            throw ('Texture dimension mismatch: ' + $Map.file)
        }
    } finally { $image.Dispose() }
}

$client = $null
$downloaded = 0
try {
    if (-not $VerifyOnly) {
        [IO.Directory]::CreateDirectory($DestinationDirectory) | Out-Null
        Add-Type -AssemblyName System.Net.Http
        $client = [Net.Http.HttpClient]::new()
        $client.Timeout = [TimeSpan]::FromSeconds(60)
        $client.DefaultRequestHeaders.UserAgent.ParseAdd('WorkphoneCitySurfaceFetcher/1.0')
    }
    Write-Output 'City surface assets: Poly Haven, CC0-1.0 (https://polyhaven.com/license)'
    foreach ($map in $maps) {
        $target = Join-Path $DestinationDirectory $map.file
        if (Test-Path -LiteralPath $target -PathType Leaf) {
            try {
                Assert-TextureFile -Path $target -Map $map
                Write-Output ('Verified ' + $map.file)
                continue
            } catch {
                if ($VerifyOnly -or -not $Force) { throw }
            }
        } elseif ($VerifyOnly) { throw ('Missing texture: ' + $map.file) }

        $temporary = Join-Path $DestinationDirectory ($map.file + '.download-' + [Guid]::NewGuid().ToString('N'))
        $response = $null
        $inputStream = $null
        $outputStream = $null
        try {
            $response = $client.GetAsync([Uri]$map.url, [Net.Http.HttpCompletionOption]::ResponseHeadersRead).GetAwaiter().GetResult()
            $response.EnsureSuccessStatusCode() | Out-Null
            $length = $response.Content.Headers.ContentLength
            if ($null -ne $length -and $length -ne $map.bytes) { throw ('Unexpected download length: ' + $map.file) }
            $inputStream = $response.Content.ReadAsStreamAsync().GetAwaiter().GetResult()
            $outputStream = [IO.FileStream]::new($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
            $buffer = New-Object byte[] 65536
            $received = 0L
            while (($read = $inputStream.Read($buffer, 0, $buffer.Length)) -gt 0) {
                $received += $read
                if ($received -gt $map.bytes) { throw ('Download exceeds its pinned byte budget: ' + $map.file) }
                $outputStream.Write($buffer, 0, $read)
            }
            $outputStream.Dispose()
            $outputStream = $null
            Assert-TextureFile -Path $temporary -Map $map
            Move-Item -LiteralPath $temporary -Destination $target -Force
            ++$downloaded
            Write-Output ('Downloaded and verified ' + $map.file)
        } finally {
            if ($null -ne $outputStream) { $outputStream.Dispose() }
            if ($null -ne $inputStream) { $inputStream.Dispose() }
            if ($null -ne $response) { $response.Dispose() }
            if (Test-Path -LiteralPath $temporary -PathType Leaf) { Remove-Item -LiteralPath $temporary }
        }
    }
} finally { if ($null -ne $client) { $client.Dispose() } }
Write-Output ('PASS: {0} verified maps, {1} downloaded, {2:N2} MB encoded.' -f $maps.Count, $downloaded, ($totalBytes / 1000000))
