param(
    [ValidateSet('Debug', 'RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'
$networkRoot = Split-Path -Parent $PSScriptRoot
$networkDirectory = Join-Path $networkRoot '.codex/network'
$networkEngine = Join-Path $networkRoot 'project_x64'
$networkCache = Join-Path $networkEngine 'CMakeCache.txt'
if (-not (Test-Path -LiteralPath $networkCache -PathType Leaf)) {
    throw 'Configure the Windows x64 engine in project_x64 before running this fixture.'
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $networkDirectory ('trust-runs/' + [guid]::NewGuid().ToString('N'))
}
$networkOutput = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $networkOutput) { throw 'OutputDirectory must be new.' }
New-Item -ItemType Directory -Path $networkOutput -Force | Out-Null

function Invoke-TrustCheck {
    param([string]$File, [string[]]$Arguments)
    & $File @Arguments
    if ($LASTEXITCODE -ne 0) { throw "WPNetwork trust command failed: $File ($LASTEXITCODE)" }
}

function Publish-TrustCertificate {
    param([string]$Path, [string[]]$Arguments)
    $networkStagedCertificate = $Path + '.issuing'
    Invoke-TrustCheck $networkIssuer (@('issue', '--out', $networkStagedCertificate) + $Arguments)
    [IO.File]::Move($networkStagedCertificate, $Path)
}

function Start-TrustPeer {
    param([string]$Role, [string[]]$Arguments, [string]$Executable = $networkFixture)
    $networkProcessArguments = @($Arguments | ForEach-Object {
        if ($_.Contains('"')) { throw 'Invalid quote in fixture argument.' }
        '"' + $_ + '"'
    }) -join ' '
    $networkProcess = Start-Process -FilePath $Executable -ArgumentList $networkProcessArguments `
        -WorkingDirectory (Split-Path -Parent $Executable) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $networkCase "$Role.stdout.log") `
        -RedirectStandardError (Join-Path $networkCase "$Role.stderr.log")
    $networkChildren.Add($networkProcess)
    $null = $networkProcess.Handle
    return $networkProcess
}

function Wait-TrustFile {
    param([string]$Path, [System.Diagnostics.Process]$Process)
    $networkDeadline = [DateTime]::UtcNow.AddSeconds(20)
    while (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        if ($Process.HasExited -or [DateTime]::UtcNow -ge $networkDeadline) {
            throw "Fixture did not publish $Path (PID $($Process.Id))."
        }
        Start-Sleep -Milliseconds 25
    }
}

function Read-TrustResult {
    param([string]$Path, [System.Diagnostics.Process]$Process, [string]$Role, [bool]$Accepted,
        [string[]]$Stages)
    Wait-TrustFile $Path $Process
    if (-not $Process.WaitForExit(3000) -or $null -eq $Process.ExitCode -or $Process.ExitCode -ne 0) {
        throw "Fixture failed after publishing $Path."
    }
    $networkResult = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    $networkOutcome = if ($Accepted) { 'accepted' } else { 'rejected' }
    $networkExpectedCount = if ($Accepted) { 1 } else { 0 }
    if ($networkResult.version -ne 1 -or $networkResult.role -cne $Role -or
        $networkResult.pid -ne $Process.Id -or $networkResult.outcome -cne $networkOutcome -or
        $networkResult.stage -cnotin $Stages -or $networkResult.admissions -ne $networkExpectedCount -or
        $networkResult.packets -ne $networkExpectedCount -or -not $networkResult.closed) {
        throw "Invalid $Role evidence in $Path."
    }
    return $networkResult
}

function Get-TrustPort {
    $networkPortProbe = New-Object Net.Sockets.UdpClient ([Net.IPEndPoint]::new([Net.IPAddress]::Loopback, 0))
    try { $networkPortProbe.Client.LocalEndPoint.Port.ToString() }
    finally { $networkPortProbe.Dispose() }
}

function Write-TrustErrors {
    foreach ($networkLog in Get-ChildItem -LiteralPath $networkCase -Filter '*.stderr.log') {
        Write-Host (Get-Content -LiteralPath $networkLog.FullName -Raw)
    }
}

function Stop-TrustPeers {
    foreach ($networkChild in $networkChildren) {
        try {
            if (-not $networkChild.HasExited) {
                $networkChild.Kill()
                if (-not $networkChild.WaitForExit(3000)) { throw 'Failed to stop trust fixture child.' }
            }
        } finally { $networkChild.Dispose() }
    }
}

function Invoke-TrustCase {
    param([string]$Name)
    $networkCase = Join-Path $networkOutput $Name
    New-Item -ItemType Directory -Path $networkCase | Out-Null
    $networkChildren = New-Object 'System.Collections.Generic.List[System.Diagnostics.Process]'
    $networkAccepted = $Name -eq 'trusted'
    $networkServerRequest = Join-Path $networkCase 'server.request'
    $networkServerCertificate = Join-Path $networkCase 'server.cert'
    $networkServerReady = Join-Path $networkCase 'server.ready'
    $networkServerResult = Join-Path $networkCase 'server.json'
    $networkClientRequest = Join-Path $networkCase 'client.request'
    $networkClientCertificate = Join-Path $networkCase 'client.cert'
    $networkClientResult = Join-Path $networkCase 'client.json'
    $networkPort = Get-TrustPort
    $networkRawProof = $Name.StartsWith('proof-', [StringComparison]::Ordinal)
    try {
        $networkServerArgs = @('--mode', 'server', '--peer', 'str:wp-client', '--port', $networkPort,
            '--request', $networkServerRequest, '--certificate', $networkServerCertificate,
            '--ready', $networkServerReady, '--result', $networkServerResult, '--timeout-ms', '14000')
        if (-not $networkAccepted) { $networkServerArgs += '--expect-rejection' }
        $networkServer = Start-TrustPeer 'server' $networkServerArgs
        Wait-TrustFile $networkServerRequest $networkServer
        Publish-TrustCertificate $networkServerCertificate @('--request', $networkServerRequest,
            '--identity', 'str:wp-server', '--key', $networkTrustedKey)
        Wait-TrustFile $networkServerReady $networkServer

        $networkClientArgs = @('--mode', 'client', '--peer', 'str:wp-server', '--port', $networkPort,
            '--result', $networkClientResult, '--timeout-ms', '12000')
        $networkClientStages = @('handshake')
        if (-not $networkAccepted) { $networkClientArgs += '--expect-rejection' }
        if ($Name -eq 'no-local-certificate') {
            $networkClientArgs += '--without-certificate'
            $networkClientStages = @('startup')
        } elseif ($Name -eq 'unsigned-on-wire') {
            $networkClientArgs += '--unverified-client'
        } else {
            $networkClientArgs += @('--request', $networkClientRequest, '--certificate', $networkClientCertificate)
        }
        if ($Name -eq 'trusted') { $networkClientStages = @('gameplay') }
        if ($Name -eq 'wrong-root') { $networkClientStages += 'certificate' }
        if ($Name -eq 'expired') {
            $networkClientReady = Join-Path $networkCase 'client.ready'
            $networkClientGate = Join-Path $networkCase 'client.connect'
            $networkClientArgs += @('--ready', $networkClientReady, '--connect-gate', $networkClientGate)
            $networkClientStages += 'startup'
        }
        if ($networkRawProof) {
            $networkProofMode = $Name.Substring('proof-'.Length)
            $networkClientArgs += @('--proof-mode', $networkProofMode)
            $networkClient = Start-TrustPeer 'client' $networkClientArgs $networkProofFixture
        } else {
            $networkClient = Start-TrustPeer 'client' $networkClientArgs
        }
        if ($Name -notin @('no-local-certificate', 'unsigned-on-wire')) {
            Wait-TrustFile $networkClientRequest $networkClient
            $networkIdentity = if ($Name -eq 'wrong-identity') { 'str:wp-other-client' } else { 'str:wp-client' }
            $networkSigningKey = if ($Name -eq 'wrong-root') { $networkOtherKey } else { $networkTrustedKey }
            $networkIssuerArgs = @('--request', $networkClientRequest, '--identity', $networkIdentity,
                '--key', $networkSigningKey)
            if ($Name -eq 'expired') { $networkIssuerArgs += @('--lifetime-seconds', '3') }
            Publish-TrustCertificate $networkClientCertificate $networkIssuerArgs
            if ($Name -eq 'expired') {
                Wait-TrustFile $networkClientReady $networkClient
                Start-Sleep -Seconds 4
                New-Item -ItemType File -Path $networkClientGate | Out-Null
            }
        }
        $networkClientReport = Read-TrustResult $networkClientResult $networkClient 'client' $networkAccepted $networkClientStages
        if ($networkRawProof -and ($networkClientReport.challengeReceived -ne $true -or
            $networkClientReport.remoteClosure -ne $true -or $networkClientReport.proofMode -cne $networkProofMode)) {
            throw "Invalid proof rejection evidence in $networkClientResult."
        }
        if ($Name -eq 'proof-replay' -and $networkClientReport.freshChallengeObserved -ne $true) {
            throw "Replay probe did not observe distinct challenges in $networkClientResult."
        }
        $networkServerStages = if ($networkAccepted) { @('gameplay') } else { @('observation') }
        $networkServerReport = Read-TrustResult $networkServerResult $networkServer 'server' $networkAccepted $networkServerStages
        Write-Host "PASS: $Name ($($networkClientReport.stage))"
        [ordered]@{ name = $Name; server = $networkServerReport; client = $networkClientReport }
    } catch {
        Write-TrustErrors
        throw
    } finally { Stop-TrustPeers }
}

function Invoke-TrustTicketClient {
    param([string]$Name, [string]$Identity, [string]$TicketPath, [bool]$Accepted = $false)
    $networkRequest = Join-Path $networkCase "$Name.request"
    $networkCertificate = Join-Path $networkCase "$Name.cert"
    $networkResult = Join-Path $networkCase "$Name.json"
    $networkClientArgs = @('--mode', 'client', '--peer', 'str:wp-server', '--port', $networkPort,
        '--request', $networkRequest, '--certificate', $networkCertificate, '--result', $networkResult,
        '--timeout-ms', '12000')
    if ($TicketPath) { $networkClientArgs += @('--ticket', $TicketPath) }
    if (-not $Accepted) { $networkClientArgs += '--expect-ticket-rejection' }
    $networkClient = Start-TrustPeer $Name $networkClientArgs
    Wait-TrustFile $networkRequest $networkClient
    Publish-TrustCertificate $networkCertificate @('--request', $networkRequest, '--identity', $Identity,
        '--key', $networkTrustedKey)
    $networkStage = if ($Accepted) { 'gameplay' } else { 'ticket' }
    $networkClientReport = Read-TrustResult $networkResult $networkClient 'client' $Accepted @($networkStage)
    [ordered]@{ name = $Name; client = $networkClientReport }
}

function Invoke-TrustTicketCase {
    $networkCase = Join-Path $networkOutput 'admission-tickets'
    New-Item -ItemType Directory -Path $networkCase | Out-Null
    $networkChildren = New-Object 'System.Collections.Generic.List[System.Diagnostics.Process]'
    $networkPort = Get-TrustPort
    $networkServerRequest = Join-Path $networkCase 'server.request'
    $networkServerCertificate = Join-Path $networkCase 'server.cert'
    $networkServerReady = Join-Path $networkCase 'server.ready'
    $networkServerResult = Join-Path $networkCase 'server.json'
    $networkAdmitted = Join-Path $networkCase 'server.admitted'
    $networkFinish = Join-Path $networkCase 'server.finish'
    $networkIdentity = 'str:wp-client'
    $networkAlternateIdentity = 'str:wp-alternate-client'
    try {
        $networkServerArgs = @('--mode', 'server', '--peer', $networkIdentity, '--extra-peer', $networkAlternateIdentity,
            '--port', $networkPort, '--request', $networkServerRequest, '--certificate', $networkServerCertificate,
            '--ready', $networkServerReady, '--result', $networkServerResult, '--timeout-ms', '30000',
            '--ticket', $networkTicket, '--expiring-ticket', $networkExpiringTicket,
            '--admitted', $networkAdmitted, '--finish', $networkFinish)
        $networkServer = Start-TrustPeer 'server' $networkServerArgs
        Wait-TrustFile $networkServerRequest $networkServer
        Publish-TrustCertificate $networkServerCertificate @('--request', $networkServerRequest,
            '--identity', 'str:wp-server', '--key', $networkTrustedKey)
        Wait-TrustFile $networkServerReady $networkServer

        # Ticket bytes stay in the temporary issuer directory and are never emitted in reports.
        $networkMutated = [IO.File]::ReadAllBytes($networkTicket)
        try {
            if ($networkMutated.Length -ne 32) { throw 'Server produced an invalid ticket length.' }
            $networkMutated[0] = [byte]($networkMutated[0] -bxor 1)
            [IO.File]::WriteAllBytes($networkInvalidTicket + '.tmp', $networkMutated)
            [IO.File]::Move($networkInvalidTicket + '.tmp', $networkInvalidTicket)
        } finally { [Array]::Clear($networkMutated, 0, $networkMutated.Length) }

        $networkAttempts = @(
            Invoke-TrustTicketClient 'missing' $networkIdentity ''
            Invoke-TrustTicketClient 'invalid' $networkIdentity $networkInvalidTicket
            Invoke-TrustTicketClient 'wrong-allowed-identity' $networkAlternateIdentity $networkTicket
            Start-Sleep -Milliseconds 1200
            Invoke-TrustTicketClient 'expired' $networkIdentity $networkExpiringTicket
            Invoke-TrustTicketClient 'valid' $networkIdentity $networkTicket $true
            # The server must see the first player leave before the same identity dials again.
            Wait-TrustFile $networkAdmitted $networkServer
            Invoke-TrustTicketClient 'replay' $networkIdentity $networkTicket
        )
        New-Item -ItemType File -Path $networkFinish | Out-Null
        $networkServerReport = Read-TrustResult $networkServerResult $networkServer 'server' $true @('gameplay')
        if ($networkServerReport.disconnects -ne 1 -or $networkServerReport.peerId -ne 1) {
            throw 'Ticket listener did not admit and retire exactly its first assigned player.'
        }
        Write-Host 'PASS: admission tickets (missing, invalid, wrong identity, expired, valid, replay)'
        [ordered]@{ name = 'admission-tickets'; server = $networkServerReport; attempts = $networkAttempts }
    } catch {
        Write-TrustErrors
        throw
    } finally { Stop-TrustPeers }
}

& (Join-Path $PSScriptRoot 'Validate-WPNetworkTransport.ps1') -Configuration $Configuration
$networkIssuer = Join-Path $networkDirectory "build/$Configuration/WPNetworkGNSIssuer.exe"
$networkKeyDirectory = Join-Path ([IO.Path]::GetTempPath()) ('WPNetwork-fixture-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $networkKeyDirectory | Out-Null
$networkTrustedKey = Join-Path $networkKeyDirectory 'trusted.pem'
$networkOtherKey = Join-Path $networkKeyDirectory 'other.pem'
$networkTicket = Join-Path $networkKeyDirectory 'admission.ticket'
$networkExpiringTicket = Join-Path $networkKeyDirectory 'expiring.ticket'
$networkInvalidTicket = Join-Path $networkKeyDirectory 'invalid.ticket'
$networkPublicRoot = Join-Path $networkOutput 'root.pub'
$networkPreviousPath = $env:Path
$networkEngineChanged = $false
$networkBuildLock = $null
try {
    Invoke-TrustCheck $networkIssuer @('generate-key', '--out', $networkTrustedKey)
    Invoke-TrustCheck $networkIssuer @('generate-key', '--out', $networkOtherKey)
    Invoke-TrustCheck $networkIssuer @('root', '--key', $networkTrustedKey, '--out', $networkPublicRoot)
    $networkPackage = & (Join-Path $PSScriptRoot 'Build-WPNetworkGNSTrust.ps1') -RootKeyFile $networkPublicRoot `
        -Configuration $Configuration -PassThru
    $networkPrefix = Join-Path $networkDirectory 'installed/x64-windows-static-md'
    $networkIsolatedBin = Join-Path $networkOutput 'bin'
    $networkLockPath = Join-Path $networkEngine '.wpnetwork-trust.lock'
    $networkLockDeadline = [DateTime]::UtcNow.AddSeconds(90)
    while (-not $networkBuildLock) {
        try {
            $networkBuildLock = [IO.File]::Open($networkLockPath, 'OpenOrCreate', 'ReadWrite', 'None')
        } catch {
            $networkLockError = $_.Exception.InnerException
            if ($networkLockError -isnot [IO.IOException] -or
                ($networkLockError.HResult -band 0xffff) -notin @(32, 33) -or
                [DateTime]::UtcNow -ge $networkLockDeadline) { throw }
            Start-Sleep -Milliseconds 250
        }
    }
    $networkRestore = @('-S', $networkRoot, '-B', $networkEngine)
    $networkCacheText = Get-Content -LiteralPath $networkCache
    foreach ($networkOption in @('WP_NETWORK_GNS', 'WP_NETWORK_GNS_VALIDATION_OUTPUT', 'CMAKE_PREFIX_PATH',
            'GameNetworkingSockets_DIR', 'Protobuf_DIR', 'Protobuf_PROTOC_EXECUTABLE')) {
        $networkEntry = $networkCacheText | Where-Object { $_ -match ('^' + $networkOption + ':[^=]+=') } | Select-Object -First 1
        if ($null -ne $networkEntry) { $networkRestore += ('-D' + $networkEntry) }
        else { $networkRestore += "-U$networkOption" }
    }
    $networkEngineChanged = $true
    Invoke-TrustCheck cmake @('-S', $networkRoot, '-B', $networkEngine, '-DWP_NETWORK_GNS=ON',
        "-DWP_NETWORK_GNS_VALIDATION_OUTPUT=$networkIsolatedBin", "-DCMAKE_PREFIX_PATH=$networkPrefix",
        "-DGameNetworkingSockets_DIR=$($networkPackage.Path)/lib/cmake/GameNetworkingSockets",
        "-DProtobuf_DIR=$networkPrefix/share/protobuf",
        "-DProtobuf_PROTOC_EXECUTABLE=$networkDirectory/installed/x64-windows/tools/protobuf/protoc.exe")
    Invoke-TrustCheck cmake @('--build', $networkEngine, '--config', $Configuration, '--target',
        'WPNetworkGNSTrustTests', 'WPNetworkGNSProofTests', '--parallel', '4')
    Invoke-TrustCheck cmake $networkRestore
    $networkEngineChanged = $false
    $networkBuildLock.Dispose()
    $networkBuildLock = $null
    $networkFixture = Join-Path $networkIsolatedBin "$Configuration/WPNetworkGNSTrustTests.exe"
    $networkProofFixture = Join-Path $networkIsolatedBin "$Configuration/WPNetworkGNSProofTests.exe"
    $networkEngineBin = Join-Path $networkRoot "bin/windows/v145/x64/MD/$Configuration"
    $env:Path = $networkEngineBin + ';' + $networkPreviousPath
    $networkReports = @('trusted', 'wrong-identity', 'wrong-root', 'unsigned-on-wire',
        'expired', 'no-local-certificate', 'proof-withheld', 'proof-wrong-nonce', 'proof-early-gameplay', 'proof-replay' |
        ForEach-Object { Invoke-TrustCase $_ })
    $networkReports += Invoke-TrustTicketCase
    [ordered]@{
        version = 1
        configuration = $Configuration
        rootFingerprint = $networkPackage.RootFingerprint
        gnsSourceSHA512 = $networkPackage.SourceSHA512
        gnsPatchSHA256 = $networkPackage.PatchSHA256
        gnsLibrarySHA256 = $networkPackage.LibrarySHA256
        issuerSHA256 = (Get-FileHash -LiteralPath $networkIssuer -Algorithm SHA256).Hash
        fixtureSHA256 = (Get-FileHash -LiteralPath $networkFixture -Algorithm SHA256).Hash
        proofFixtureSHA256 = (Get-FileHash -LiteralPath $networkProofFixture -Algorithm SHA256).Hash
        wpNetworkSHA256 = (Get-FileHash -LiteralPath (Join-Path $networkIsolatedBin "$Configuration/WPNetwork.dll") -Algorithm SHA256).Hash
        scenarios = $networkReports
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $networkOutput 'report.json') -Encoding UTF8
    Write-Host "Report: $(Join-Path $networkOutput 'report.json')"
} finally {
    $env:Path = $networkPreviousPath
    try {
        foreach ($networkKey in @($networkTrustedKey, $networkOtherKey, $networkTicket, ($networkTicket + '.tmp'),
                $networkExpiringTicket, ($networkExpiringTicket + '.tmp'), $networkInvalidTicket, ($networkInvalidTicket + '.tmp'))) {
            if (Test-Path -LiteralPath $networkKey -PathType Leaf) { Remove-Item -LiteralPath $networkKey -Force }
        }
        Remove-Item -LiteralPath $networkKeyDirectory
    } finally {
        try {
            if ($networkEngineChanged) { Invoke-TrustCheck cmake $networkRestore }
        } finally {
            if ($networkBuildLock) { $networkBuildLock.Dispose() }
        }
    }
}
