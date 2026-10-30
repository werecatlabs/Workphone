param(
    [string]$Executable,
    [ValidateRange(2, 8)][int]$Clients = 2,
    [ValidateRange(5, 30)][int]$TimeoutSeconds = 20,
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$networkRoot = Split-Path -Parent $PSScriptRoot
if (-not $Executable) {
    $Executable = Join-Path $networkRoot 'bin/windows/v145/x64/MD/RelWithDebInfo/WPNetworkProcessTests.exe'
}
$networkExecutable = (Resolve-Path -LiteralPath $Executable).Path
if (-not (Test-Path -LiteralPath $networkExecutable -PathType Leaf)) { throw 'Network process fixture is missing.' }
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path ([IO.Path]::GetTempPath()) ('WPNetwork-local-' + [guid]::NewGuid().ToString('N'))
}
$networkOutput = [IO.Path]::GetFullPath($OutputDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
if (Test-Path -LiteralPath $networkOutput) {
    if (-not (Test-Path -LiteralPath $networkOutput -PathType Container) -or
        @(Get-ChildItem -LiteralPath $networkOutput -Force).Count -ne 0) {
        throw 'OutputDirectory must be a new or empty directory; existing reports are preserved.'
    }
} else {
    New-Item -ItemType Directory -Path $networkOutput | Out-Null
}

$networkProcesses = New-Object 'System.Collections.Generic.List[System.Diagnostics.Process]'
$networkDeadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
function Start-NetworkChild {
    param([string]$Name, [string[]]$ChildArguments)
    # Windows paths cannot contain a quote; avoid shell interpolation entirely.
    # Quote every argument for Start-Process's command-line serialization.
    $networkArguments = @($ChildArguments | ForEach-Object {
        if ($_.Contains('"')) { throw 'A child argument contains a quote.' }
        '"' + $_ + '"'
    }) -join ' '
    $networkChild = Start-Process -FilePath $networkExecutable -ArgumentList $networkArguments `
        -WorkingDirectory (Split-Path -Parent $networkExecutable) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $networkOutput ($Name + '.stdout.log')) `
        -RedirectStandardError (Join-Path $networkOutput ($Name + '.stderr.log'))
    $networkProcesses.Add($networkChild)
    return $networkChild
}
function Assert-NetworkDeadline {
    if ([DateTime]::UtcNow -ge $networkDeadline) { throw 'Local network process scenario exceeded its deadline.' }
}

try {
    Write-Output 'Running development UDP smoke test with separate server/client processes; no encryption or reliable-delivery guarantee.'
    $networkServer = Start-NetworkChild 'server' @('--mode', 'server', '--output', $networkOutput,
        '--clients', $Clients.ToString(), '--timeout', $TimeoutSeconds.ToString())
    $networkReady = Join-Path $networkOutput 'server.ready'
    while (-not (Test-Path -LiteralPath $networkReady -PathType Leaf)) {
        Assert-NetworkDeadline
        $networkServer.Refresh()
        if ($networkServer.HasExited) { throw "Server exited before readiness ($($networkServer.ExitCode))." }
        Start-Sleep -Milliseconds 25
    }
    $networkPort = 0
    if (-not [int]::TryParse([IO.File]::ReadAllText($networkReady), [ref]$networkPort) -or $networkPort -lt 1 -or $networkPort -gt 65535) {
        throw 'Server published an invalid port.'
    }
    for ($networkIndex = 1; $networkIndex -le $Clients; ++$networkIndex) {
        Start-NetworkChild ('client-' + $networkIndex) @('--mode', 'client', '--output', $networkOutput,
            '--client', $networkIndex.ToString(), '--port', $networkPort.ToString(),
            '--timeout', $TimeoutSeconds.ToString()) | Out-Null
    }
    do {
        Assert-NetworkDeadline
        $networkRunning = $false
        foreach ($networkProcess in $networkProcesses) {
            $networkProcess.Refresh()
            if (-not $networkProcess.HasExited) { $networkRunning = $true }
            elseif ($networkProcess.ExitCode -ne 0) { throw "Network process $($networkProcess.Id) failed ($($networkProcess.ExitCode))." }
        }
        if ($networkRunning) { Start-Sleep -Milliseconds 25 }
    } while ($networkRunning)

    $networkServerReport = Get-Content -LiteralPath (Join-Path $networkOutput 'server.json') -Raw | ConvertFrom-Json
    if ($networkServerReport.version -ne 1 -or $networkServerReport.role -ne 'server' -or
        $networkServerReport.pid -ne $networkServer.Id -or -not $networkServerReport.closed -or
        $networkServerReport.boundHost -ne 2130706433 -or -not $networkServerReport.loopback -or
        @($networkServerReport.clients).Count -ne $Clients) { throw 'Server report is incomplete or invalid.' }
    $networkClientReports = @()
    for ($networkIndex = 1; $networkIndex -le $Clients; ++$networkIndex) {
        $networkClientReport = Get-Content -LiteralPath (Join-Path $networkOutput ('client-' + $networkIndex + '.json')) -Raw | ConvertFrom-Json
        $networkPeer = @($networkServerReport.clients | Where-Object { $_.token -eq $networkIndex })
        if ($networkClientReport.version -ne 1 -or $networkClientReport.role -ne 'client' -or
            $networkClientReport.pid -ne $networkProcesses[$networkIndex].Id -or $networkClientReport.token -ne $networkIndex -or
            $networkClientReport.boundHost -ne 2130706433 -or
            -not $networkClientReport.closed -or -not $networkClientReport.helloAcknowledged -or -not $networkClientReport.finishAcknowledged -or
            $networkClientReport.playerId -lt 1 -or $networkClientReport.playerId -gt 65534 -or
            $networkPeer.Count -ne 1 -or $networkPeer[0].playerId -ne $networkClientReport.playerId -or
            -not $networkPeer[0].finished -or -not $networkPeer[0].disconnected) { throw "Client $networkIndex report or server identity mapping is invalid." }
        $networkClientReports += $networkClientReport
    }
    if (@($networkClientReports.playerId | Sort-Object -Unique).Count -ne $Clients -or
        @($networkProcesses.Id | Sort-Object -Unique).Count -ne ($Clients + 1)) {
        throw 'The scenario did not use distinct process IDs and player identities.'
    }
    [ordered]@{
        version = 1; scenario = 'local-multiprocess'; backend = 'native_udp_development'; secure = $false
        clientsUseLoopback = (@($networkClientReports | Where-Object { $_.boundHost -ne 2130706433 }).Count -eq 0)
        listenerBindsAllInterfaces = ($networkServerReport.boundHost -eq 0)
        listenerUsesLoopback = ($networkServerReport.boundHost -eq 2130706433)
        completed = $true; server = $networkServerReport; clients = $networkClientReports
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $networkOutput 'report.json') -Encoding UTF8
    Write-Output "PASS: one server and $Clients separate clients; distinct IDs, message exchange, disconnects and socket teardown."
    Write-Output "Report: $(Join-Path $networkOutput 'report.json')"
} catch {
    Write-Output "Scenario failed. Logs retained in $networkOutput"
    foreach ($networkLog in @(Get-ChildItem -LiteralPath $networkOutput -Filter '*.stderr.log')) {
        $networkLogText = Get-Content -LiteralPath $networkLog.FullName -Raw
        if ($networkLogText) { Write-Output "$($networkLog.Name): $networkLogText" }
    }
    throw
} finally {
    $networkCleanupFailures = @()
    foreach ($networkProcess in $networkProcesses) {
        try {
            $networkProcess.Refresh()
            if (-not $networkProcess.HasExited) {
                try { $networkProcess.Kill() } catch {
                    $networkProcess.Refresh()
                    if (-not $networkProcess.HasExited) { throw }
                }
                if (-not $networkProcess.WaitForExit(2000)) {
                    throw "Process $($networkProcess.Id) did not exit after termination."
                }
            }
        } catch {
            $networkCleanupFailures += $_.Exception.Message
        } finally { $networkProcess.Dispose() }
    }
    if ($networkCleanupFailures.Count -ne 0) {
        throw ('Local network process cleanup failed: ' + ($networkCleanupFailures -join '; '))
    }
}
