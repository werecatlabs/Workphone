param(
    [Parameter(Mandatory = $true)][string]$RootKeyFile,
    [ValidateSet('Debug', 'RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [string]$VisualStudioPath,
    [switch]$PassThru
)

$ErrorActionPreference = 'Stop'
$networkRoot = Split-Path -Parent $PSScriptRoot
$networkDirectory = Join-Path $networkRoot '.codex/network'
$networkTransport = Join-Path $networkRoot 'Tests/NetworkTransport'
$networkLock = Get-Content -LiteralPath (Join-Path $networkTransport 'dependencies.lock.json') -Raw | ConvertFrom-Json
$networkGNS = $networkLock.packages | Where-Object name -eq 'gamenetworkingsockets'
$networkPatchName = 'TrustedGNS/offline-certificate.patch'
$networkPatch = Join-Path $networkTransport $networkPatchName
$networkPatchHash = (Get-FileHash -LiteralPath $networkPatch -Algorithm SHA256).Hash
$networkLockedHash = $networkLock.localPatches | ForEach-Object { $_.files.PSObject.Properties } |
    Where-Object Name -eq $networkPatchName | Select-Object -ExpandProperty Value
if ($networkPatchHash -ne $networkLockedHash) { throw 'The offline certificate patch failed its pinned SHA256 check.' }
$networkArchive = Join-Path $networkDirectory "vcpkg/downloads/ValveSoftware-GameNetworkingSockets-v$($networkGNS.version).tar.gz"
$networkDependencyPrefix = Join-Path $networkDirectory "installed/$($networkLock.targetTriplet)"
$networkProtoc = Join-Path $networkDirectory "installed/$($networkLock.hostTriplet)/tools/protobuf/protoc.exe"
if (-not (Test-Path -LiteralPath $networkArchive -PathType Leaf) -or
    -not (Test-Path -LiteralPath $networkProtoc -PathType Leaf)) {
    throw 'Run Validate-WPNetworkTransport.ps1 to install the pinned sources and dependencies first.'
}
if ((Get-FileHash -LiteralPath $networkArchive -Algorithm SHA512).Hash -ne $networkGNS.sourceSHA512) {
    throw 'The cached GameNetworkingSockets archive failed its pinned SHA512 check.'
}

$networkRootFile = Get-Item -LiteralPath $RootKeyFile
if ($networkRootFile.PSIsContainer -or $networkRootFile.Length -gt 160) { throw 'Invalid public root file.' }
$networkPublicRoot = [IO.File]::ReadAllText($networkRootFile.FullName).Trim()
if ($networkPublicRoot -cnotmatch '^ssh-ed25519 ([A-Za-z0-9+/]{68}) ID(0|[1-9][0-9]{0,19})$') {
    throw 'Use the OpenSSH public root exported by WPNetworkGNSIssuer.'
}
$networkBlob = [Convert]::FromBase64String($Matches[1])
$networkExpectedPrefix = [byte[]]@(0, 0, 0, 11, 115, 115, 104, 45, 101, 100, 50, 53, 53, 49, 57, 0, 0, 0, 32)
if ($networkBlob.Length -ne 51 -or [Convert]::ToBase64String([byte[]]$networkBlob[0..18]) -cne
    [Convert]::ToBase64String($networkExpectedPrefix)) { throw 'Invalid OpenSSH Ed25519 key blob.' }
$networkHasher = [Security.Cryptography.SHA256]::Create()
try {
    $networkKeyHash = $networkHasher.ComputeHash([byte[]]$networkBlob[19..50])
} finally { $networkHasher.Dispose() }
if ($Matches[2] -cne ([BitConverter]::ToUInt64($networkKeyHash, 0)).ToString([Globalization.CultureInfo]::InvariantCulture)) {
    throw 'OpenSSH root key ID does not match its Ed25519 public key.'
}

$networkFingerprint = [BitConverter]::ToString($networkKeyHash).Replace('-', '').ToLowerInvariant()
$networkTrustBuild = Join-Path $networkDirectory ('trust-packages/' + $networkFingerprint.Substring(0, 16))
$networkLibraryConfiguration = if ($Configuration -eq 'Debug') { 'Debug' } else { 'Release' }
$networkPackage = Join-Path $networkTrustBuild "$networkLibraryConfiguration/package"
$networkSourceName = "GameNetworkingSockets-$($networkGNS.version)"
$networkSource = Join-Path $networkTrustBuild $networkSourceName
New-Item -ItemType Directory -Path $networkTrustBuild -Force | Out-Null
$networkCanonicalRoot = Join-Path $networkTrustBuild 'root.pub'
if ((Test-Path -LiteralPath $networkCanonicalRoot) -and
    [IO.File]::ReadAllText($networkCanonicalRoot).Trim() -cne $networkPublicRoot) {
    throw 'Another root is already present at this package path.'
}
[IO.File]::WriteAllText($networkCanonicalRoot, $networkPublicRoot + "`n", [Text.Encoding]::ASCII)

function Invoke-TrustedGNSCommand {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Trusted GNS build command failed: $File ($LASTEXITCODE)" }
}

# Refresh the pinned sources from the checked archive on each invocation.
Invoke-TrustedGNSCommand tar @('-xzf', $networkArchive, '-C', $networkTrustBuild,
    '--exclude', "$networkSourceName/.github", '--exclude', "$networkSourceName/vcpkg_ports")
$networkRelativeSource = $networkSource.Substring($networkRoot.Length + 1).Replace('\', '/')
Invoke-TrustedGNSCommand git @('-C', $networkRoot, 'apply', '--no-index', "--directory=$networkRelativeSource", $networkPatch)
if (-not $VisualStudioPath) {
    $networkVSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $VisualStudioPath = (& $networkVSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
    if (-not $VisualStudioPath) { throw 'A stable Visual Studio C++ installation is required.' }
    $VisualStudioPath = $VisualStudioPath.Trim()
}
$networkBuildTree = Join-Path $networkTrustBuild "$networkLibraryConfiguration/build"
Invoke-TrustedGNSCommand cmake @('-S', (Join-Path $networkTransport 'TrustedGNS'), '-B', $networkBuildTree,
    '-G', 'Visual Studio 18 2026', '-A', 'x64', "-DCMAKE_GENERATOR_INSTANCE=$VisualStudioPath",
    "-DCMAKE_PREFIX_PATH=$networkDependencyPrefix", "-DOPENSSL_ROOT_DIR=$networkDependencyPrefix",
    "-DProtobuf_PROTOC_EXECUTABLE=$networkProtoc", "-DWP_GNS_SOURCE=$networkSource",
    "-DWP_GNS_PUBLIC_ROOT=$networkCanonicalRoot", "-DCMAKE_INSTALL_PREFIX=$networkPackage")
Invoke-TrustedGNSCommand cmake @('--build', $networkBuildTree, '--config', $networkLibraryConfiguration, '--parallel', '4')
Invoke-TrustedGNSCommand cmake @('--install', $networkBuildTree, '--config', $networkLibraryConfiguration)
$networkLibrary = Join-Path $networkPackage 'lib/GameNetworkingSockets_s.lib'
if (-not (Test-Path -LiteralPath $networkLibrary -PathType Leaf)) {
    throw 'The trusted static GNS library was not installed.'
}
if ($PassThru) {
    [pscustomobject]@{
        Path = $networkPackage
        RootFingerprint = $networkFingerprint
        LibrarySHA256 = (Get-FileHash -LiteralPath $networkLibrary -Algorithm SHA256).Hash
        SourceSHA512 = $networkGNS.sourceSHA512
        PatchSHA256 = $networkPatchHash
    }
} else {
    Write-Output "Package: $networkPackage"
}
