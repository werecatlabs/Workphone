param(
    [switch]$IncludeEngine,
    [switch]$IncludeEditor,
    [switch]$IncludeGNS,
    [ValidateSet('Debug', 'RelWithDebInfo')]
    [string]$Configuration = 'RelWithDebInfo'
)

$ErrorActionPreference = 'Stop'
$networkRoot = Split-Path -Parent $PSScriptRoot
function Invoke-NetworkCheck {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "WPNetwork check failed ($LASTEXITCODE): $Executable $($Arguments -join ' ')"
    }
}

Push-Location -LiteralPath $networkRoot
try {
    Invoke-NetworkCheck cmake @('--preset', 'network-contracts')
    foreach ($networkPreset in @('network-contracts-debug', 'network-contracts')) {
        Invoke-NetworkCheck cmake @('--build', '--preset', $networkPreset)
        Invoke-NetworkCheck ctest @('--preset', $networkPreset)
    }
    if ($IncludeEngine -or $IncludeEditor -or $IncludeGNS) {
        $networkConfigure = @('-S', '.', '-B', 'project_x64')
        $networkTargets = @('WPNetworkIntegrationTests', 'WPNetworkProcessTests')
        $networkPattern = '^WPNetwork\.(integration|local_multiprocess)$'
        if ($IncludeGNS) {
            & (Join-Path $PSScriptRoot 'Validate-WPNetworkTransport.ps1') -Configuration $Configuration
            $networkPrefix = Join-Path $networkRoot '.codex/network/installed'
            $networkConfigure += @('-DWP_NETWORK_GNS=ON', "-DGameNetworkingSockets_DIR=$networkPrefix/x64-windows-static-md/share/gamenetworkingsockets", "-DProtobuf_DIR=$networkPrefix/x64-windows-static-md/share/protobuf", "-DCMAKE_PREFIX_PATH=$networkPrefix/x64-windows-static-md", "-DProtobuf_PROTOC_EXECUTABLE=$networkPrefix/x64-windows/tools/protobuf/protoc.exe")
            $networkTargets += 'WPNetworkGNSIntegrationTests'
            $networkPattern = '^WPNetwork\.(integration|local_multiprocess|gns_engine|gns_multiprocess)$'
        }
        Invoke-NetworkCheck cmake $networkConfigure
        if ($IncludeEditor) { $networkTargets += @('WPLuaBind', 'Editor') }
        Invoke-NetworkCheck cmake (@('--build', 'project_x64', '--config', $Configuration, '--target') + $networkTargets + @('--parallel', '4'))
        Invoke-NetworkCheck ctest @('--test-dir', 'project_x64', '-C', $Configuration, '-R', $networkPattern, '--no-tests=error', '--output-on-failure')
    }
    Invoke-NetworkCheck git @('diff', '--check')
} finally {
    Pop-Location
}
