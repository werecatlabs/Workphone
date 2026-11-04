param(
    [ValidateSet('Debug','RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [string]$VisualStudioPath
)
$ErrorActionPreference = 'Stop'
$networkRoot = Split-Path -Parent $PSScriptRoot
$networkBaseline = '520c3b781ac2abf391887d286d569f74ae6bf752'
$networkDirectory = Join-Path $networkRoot '.codex/network'
$networkVcpkg = Join-Path $networkDirectory 'vcpkg'
function Invoke-TransportCommand {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Transport command failed: $File ($LASTEXITCODE)" }
}
if (-not (Test-Path -LiteralPath $networkVcpkg)) {
    New-Item -ItemType Directory -Path $networkDirectory -Force | Out-Null
    Invoke-TransportCommand git @('clone', '--no-checkout', '--filter=blob:none', 'https://github.com/microsoft/vcpkg.git', $networkVcpkg)
    Invoke-TransportCommand git @('-C', $networkVcpkg, 'checkout', '--detach', $networkBaseline)
}
$networkActualBaseline = (& git -C $networkVcpkg rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $networkActualBaseline -ne $networkBaseline) {
    throw 'Existing vcpkg checkout does not match the reviewed manifest baseline. Use a separate matching checkout.'
}
Invoke-TransportCommand git @('-C', $networkVcpkg, 'diff', '--quiet', 'HEAD', '--')
$networkLock = Get-Content -LiteralPath (Join-Path $networkRoot 'Tests/NetworkTransport/dependencies.lock.json') -Raw | ConvertFrom-Json
if ($networkLock.vcpkgBaseline -ne $networkBaseline) { throw 'Dependency lock baseline mismatch.' }
foreach ($networkPackage in $networkLock.packages) {
    $networkPort = Join-Path $networkVcpkg ('ports/' + $networkPackage.name)
    $networkMetadata = Get-Content -LiteralPath (Join-Path $networkPort 'vcpkg.json') -Raw | ConvertFrom-Json
    $networkPortText = Get-Content -LiteralPath (Join-Path $networkPort 'portfile.cmake') -Raw
    if ($networkMetadata.version -ne $networkPackage.version -or
        [int]($networkMetadata.'port-version') -ne [int]($networkPackage.portVersion) -or
        $networkPortText -notmatch ('SHA512\s+' + [regex]::Escape($networkPackage.sourceSHA512))) {
        throw "Locked dependency does not match registry: $($networkPackage.name)"
    }
}
foreach ($networkPatch in $networkLock.localPatches) {
    foreach ($networkFile in $networkPatch.files.PSObject.Properties) {
        $networkFilePath = Join-Path (Join-Path $networkRoot 'Tests/NetworkTransport') $networkFile.Name
        if ((Get-FileHash -LiteralPath $networkFilePath -Algorithm SHA256).Hash -ne $networkFile.Value) {
            throw "Overlay differs from dependency lock: $($networkFile.Name)"
        }
    }
}
Invoke-TransportCommand (Join-Path $networkVcpkg 'bootstrap-vcpkg.bat') @('-disableMetrics')
$networkInstall = Join-Path $networkDirectory 'installed'
$networkSource = Join-Path $networkRoot 'Tests/NetworkTransport'
$networkBuild = Join-Path $networkDirectory 'build'
$networkPreviousConcurrency = $env:VCPKG_MAX_CONCURRENCY
$networkPreviousVS = $env:VCPKG_VISUAL_STUDIO_PATH
if (-not $VisualStudioPath) {
    $networkVSWhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $VisualStudioPath = & $networkVSWhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $VisualStudioPath) { throw 'A stable Visual Studio C++ installation is required; pass -VisualStudioPath for another installation.' }
    $VisualStudioPath = $VisualStudioPath.Trim()
}
try {
    $env:VCPKG_MAX_CONCURRENCY = '4'
    $env:VCPKG_VISUAL_STUDIO_PATH = $VisualStudioPath
    Invoke-TransportCommand (Join-Path $networkVcpkg 'vcpkg.exe') @('install', "--x-manifest-root=$networkSource", "--x-install-root=$networkInstall", '--triplet=x64-windows-static-md', '--host-triplet=x64-windows', '--disable-metrics')
    Invoke-TransportCommand cmake @('-S', $networkSource, '-B', $networkBuild, '-G', 'Visual Studio 18 2026', '-A', 'x64', "-DCMAKE_GENERATOR_INSTANCE=$VisualStudioPath", "-DCMAKE_TOOLCHAIN_FILE=$networkVcpkg/scripts/buildsystems/vcpkg.cmake", '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md', '-DVCPKG_HOST_TRIPLET=x64-windows', "-DVCPKG_INSTALLED_DIR=$networkInstall")
    Invoke-TransportCommand cmake @('--build', $networkBuild, '--config', $Configuration, '--parallel', '4')
    Invoke-TransportCommand ctest @('--test-dir', $networkBuild, '-C', $Configuration, '--no-tests=error', '--output-on-failure', '--output-junit', (Join-Path $networkBuild "results-$Configuration.xml"))
    [ordered]@{
        vcpkgBaseline = $networkActualBaseline
        manifestSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkSource 'vcpkg.json') -Algorithm SHA256).Hash
        installedStatusSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkInstall 'vcpkg/status') -Algorithm SHA256).Hash
        configuration = $Configuration
        visualStudioPath = $VisualStudioPath
        executableSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkBuild "$Configuration/WPNetworkGNSTests.exe") -Algorithm SHA256).Hash
        cmakeCacheSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkBuild 'CMakeCache.txt') -Algorithm SHA256).Hash
        dependencyLockSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkSource 'dependencies.lock.json') -Algorithm SHA256).Hash
        conformanceSourceSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkSource 'GNSConformance.cpp') -Algorithm SHA256).Hash
        engineIntegrated = $false
        peerIdentityCertified = $false
        completedUTC = [DateTime]::UtcNow.ToString('o')
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $networkBuild "conformance-result-$Configuration.json") -Encoding UTF8
} finally {
    $env:VCPKG_MAX_CONCURRENCY = $networkPreviousConcurrency
    $env:VCPKG_VISUAL_STUDIO_PATH = $networkPreviousVS
}
