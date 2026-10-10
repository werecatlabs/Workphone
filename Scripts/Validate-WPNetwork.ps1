param(
    [switch]$IncludeEngine,
    [switch]$IncludeEditor,
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
    if ($IncludeEngine -or $IncludeEditor) {
        Invoke-NetworkCheck cmake @('-S', '.', '-B', 'project_x64')
        $networkTargets = @('WPNetworkIntegrationTests')
        if ($IncludeEditor) { $networkTargets += @('WPLuaBind', 'Editor') }
        Invoke-NetworkCheck cmake (@('--build', 'project_x64', '--config', $Configuration, '--target') + $networkTargets + @('--parallel', '4'))
        Invoke-NetworkCheck ctest @('--test-dir', 'project_x64', '-C', $Configuration, '-R', '^WPNetwork\.integration$', '--no-tests=error', '--output-on-failure')
    }
    Invoke-NetworkCheck git @('diff', '--check')
} finally {
    Pop-Location
}
