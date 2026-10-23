<#
.SYNOPSIS
Read-only PMU preflight, or an explicitly requested, uniquely owned WPR trace.
.DESCRIPTION
PreflightOnly is the default and never starts/stops/cancels a trace. Every call
requires a NEW output directory. No existing evidence is overwritten. Start is
conservative: all three generic HAL sources, token profile privilege, valid WPRP,
and unambiguous idle PMC/kernel status are required. Start/Stop never change
privileges, boot/security settings, drivers, counter mappings, or other sessions.

Start example (only on an eligible machine):
  .\CollectMaterialPmu.ps1 -Mode Start -InstanceName ('M4Material_' + [guid]::NewGuid().ToString('N')) -OutputDirectory G:\Evidence\pmu_start_001
Run the production sample, then explicitly stop that receipt's unique instance:
  .\CollectMaterialPmu.ps1 -Mode Stop -OwnerFile G:\Evidence\pmu_start_001\owner.json -OutputDirectory G:\Evidence\pmu_stop_001

An ETL is NOT verified PMU evidence. Inspect lost events, actual counters, target
PID/TID, ROI begin/end markers and process scope before computing CPI/MPKI.
CacheMisses is not labelled L1/L2/LLC. CPU timer measurements are not PMU evidence.
Official sources and exact local toolkit schema are recorded in preflight.json.
#>
[CmdletBinding()]
param(
    [ValidateSet('PreflightOnly', 'Start', 'Stop')]
    [string]$Mode = 'PreflightOnly',
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [string]$InstanceName,
    [string]$OwnerFile,
    [string]$WptDirectory
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$script:Commands = New-Object System.Collections.Generic.List[object]
$script:OutputRoot = $null
$script:Utf8 = New-Object System.Text.UTF8Encoding($false)

function Write-NewText([string]$Path, [string]$Value) {
    $stream = [System.IO.File]::Open($Path, [System.IO.FileMode]::CreateNew,
        [System.IO.FileAccess]::Write, [System.IO.FileShare]::Read)
    try {
        $bytes = $script:Utf8.GetBytes($Value)
        $stream.Write($bytes, 0, $bytes.Length)
    } finally { $stream.Dispose() }
}

function Write-NewJson([string]$Path, $Value) {
    Write-NewText $Path (($Value | ConvertTo-Json -Depth 16) + [Environment]::NewLine)
}

function Get-FileRecord([string]$Path) {
    if (-not $Path -or -not [System.IO.File]::Exists($Path)) { return $null }
    $item = Get-Item -LiteralPath $Path
    return [ordered]@{
        path = $item.FullName; file_version = $item.VersionInfo.FileVersion
        product_version = $item.VersionInfo.ProductVersion
        bytes = $item.Length; sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
    }
}

function ConvertTo-NativeArgument([string]$Value) {
    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    # Windows CommandLineToArgv/CRT escaping, not PowerShell/shell escaping.
    $quoted = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $quoted = [regex]::Replace($quoted, '(\\+)$', '$1$1')
    return '"' + $quoted + '"'
}

function Invoke-Logged([string]$Name, [string]$Executable, [string[]]$Arguments) {
    $started = [DateTime]::UtcNow
    $stdout = ''; $stderr = ''; $exitCode = $null; $errorText = $null
    try {
        # No shell is involved; stdout/stderr remain exact text rather than
        # PowerShell's wrapped native-stderr ErrorRecord rendering.
        $info = New-Object System.Diagnostics.ProcessStartInfo
        $info.FileName = $Executable
        $info.Arguments = (($Arguments | ForEach-Object { ConvertTo-NativeArgument $_ }) -join ' ')
        $info.UseShellExecute = $false; $info.CreateNoWindow = $true
        $info.RedirectStandardOutput = $true; $info.RedirectStandardError = $true
        $process = New-Object System.Diagnostics.Process
        $process.StartInfo = $info
        try {
            $null = $process.Start()
            $outTask = $process.StandardOutput.ReadToEndAsync()
            $errTask = $process.StandardError.ReadToEndAsync()
            $process.WaitForExit()
            $stdout = $outTask.GetAwaiter().GetResult()
            $stderr = $errTask.GetAwaiter().GetResult()
            $exitCode = $process.ExitCode
        } finally { $process.Dispose() }
    } catch { $errorText = $_.Exception.Message }
    $output = $stdout + $stderr
    $log = Join-Path $script:OutputRoot ($Name + '.log')
    $outLog = Join-Path $script:OutputRoot ($Name + '.stdout.log')
    $errLog = Join-Path $script:OutputRoot ($Name + '.stderr.log')
    Write-NewText $log $output
    Write-NewText $outLog $stdout
    Write-NewText $errLog $stderr
    $result = [pscustomobject][ordered]@{
        name = $Name; executable = $Executable; arguments = @($Arguments)
        started_utc = $started.ToString('o'); finished_utc = [DateTime]::UtcNow.ToString('o')
        exit_code = $exitCode; error = $errorText; raw_log = $log; raw = $output
        stdout_log = $outLog; stderr_log = $errLog
        combined_log_order = 'stdout then stderr; separate streams are also preserved'
    }
    $script:Commands.Add($result)
    Write-NewJson (Join-Path $script:OutputRoot ($Name + '.command.json')) $result
    return $result
}

function Get-NormalizedSources($Result) {
    $names = @(); $capacity = $null
    if ($null -ne $Result) {
        foreach ($line in ($Result.raw -split '\r?\n')) {
            if ($line -match '^\s*\d+\s+(\S+)\s+\d+\s+\d+\s+\d+') { $names += $Matches[1] }
            if ($line -match 'Maximum selectable profile sources:\s*(\d+)') { $capacity = [int]$Matches[1] }
        }
    }
    return [ordered]@{ names = @($names); maximum_selectable_sources = $capacity }
}

function Get-OptionalCpuId {
    if (-not ('System.Runtime.Intrinsics.X86.X86Base' -as [type])) {
        return [ordered]@{ available = $false; reason = 'CpuId API unavailable in this PowerShell runtime; HAL source enumeration is authoritative.' }
    }
    try {
        if (-not [System.Runtime.Intrinsics.X86.X86Base]::IsSupported) { throw 'X86Base is not supported.' }
        $pmu = [System.Runtime.Intrinsics.X86.X86Base]::CpuId(10, 0)
        $hv = [System.Runtime.Intrinsics.X86.X86Base]::CpuId(1073741824, 0)
        $vendor = [Text.Encoding]::ASCII.GetString(
            [BitConverter]::GetBytes($hv.Item2) + [BitConverter]::GetBytes($hv.Item3) + [BitConverter]::GetBytes($hv.Item4))
        $result = [ordered]@{
            available = $true; architectural_pmu_version = ($pmu.Item1 -band 255)
            general_counters = (($pmu.Item1 -shr 8) -band 255)
            fixed_counters = ($pmu.Item4 -band 31); hypervisor_vendor = $vendor
            warning = 'CPUID advertisement alone does not establish Windows PMU availability.'
        }
        if ($vendor -eq 'Microsoft Hv' -and $hv.Item1 -ge 1073741827) {
            $features = [System.Runtime.Intrinsics.X86.X86Base]::CpuId(1073741827, 0)
            $result.hyperv_architectural_pmu_available = (($features.Item3 -band 128) -ne 0)
            $result.hyperv_performance_monitor_available = (($features.Item4 -band 4) -ne 0)
        }
        return $result
    } catch { return [ordered]@{ available = $false; reason = $_.Exception.Message } }
}

function Assert-InstanceName([string]$Name) {
    # Receipt names contain an explicit random GUID; never use WPR's default instance.
    if ($Name -notmatch '^M4Material_[0-9a-fA-F]{32}$') {
        throw 'InstanceName must be M4Material_<32 hexadecimal GUID digits>, explicitly supplied for Start.'
    }
}

try {
    if ($env:OS -ne 'Windows_NT') { throw 'This collector requires Windows.' }
    $newOutputRoot = [System.IO.Path]::GetFullPath($OutputDirectory)
    if (Test-Path -LiteralPath $newOutputRoot) { throw 'OutputDirectory already exists; choose a new evidence directory.' }
    $null = New-Item -ItemType Directory -Path $newOutputRoot -ErrorAction Stop
    $script:OutputRoot = $newOutputRoot
    $required = @('InstructionRetired', 'TotalCycles', 'CacheMisses')
    $blockers = New-Object System.Collections.Generic.List[string]
    $toolkit = if ($WptDirectory) { [System.IO.Path]::GetFullPath($WptDirectory) } else {
        Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Windows Performance Toolkit'
    }
    $wpr = Join-Path $toolkit 'wpr.exe'; $xperf = Join-Path $toolkit 'xperf.exe'
    $schemaPath = Join-Path $toolkit 'WPRControlProfiles.xsd'
    $savedProfile = Join-Path $script:OutputRoot 'MaterialScenePmu.wprp'
    $profileSource = Join-Path $PSScriptRoot 'MaterialScenePmu.wprp'
    [System.IO.File]::Copy($profileSource, $savedProfile, $false)
    [System.IO.File]::Copy($PSCommandPath, (Join-Path $script:OutputRoot 'CollectMaterialPmu.ps1'), $false)

    $hostErrors = New-Object System.Collections.Generic.List[string]
    $os = $null; $cpu = @(); $computer = $null; $deviceGuard = $null
    try { $os = Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, BuildNumber, OSArchitecture }
    catch { $hostErrors.Add($_.Exception.Message) }
    try { $cpu = @(Get-CimInstance Win32_Processor | Select-Object Name, Manufacturer, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed, ProcessorId) }
    catch { $hostErrors.Add($_.Exception.Message) }
    try { $computer = Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer, Model, HypervisorPresent, NumberOfProcessors, NumberOfLogicalProcessors }
    catch { $hostErrors.Add($_.Exception.Message) }
    try { $deviceGuard = Get-CimInstance -Namespace 'root/Microsoft/Windows/DeviceGuard' Win32_DeviceGuard |
        Select-Object VirtualizationBasedSecurityStatus, SecurityServicesRunning, SecurityServicesConfigured }
    catch { $hostErrors.Add('DeviceGuard query: ' + $_.Exception.Message) }
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    $whoami = Join-Path $env:SystemRoot 'System32\whoami.exe'
    $privileges = Invoke-Logged 'token_privileges' $whoami @('/priv')
    $groups = Invoke-Logged 'token_groups' $whoami @('/groups')
    $profilePrivilege = ($privileges.exit_code -eq 0 -and $privileges.raw -match '(?m)^\s*SeSystemProfilePrivilege\s+')
    if (-not $profilePrivilege) { $blockers.Add('The current token does not expose SeSystemProfilePrivilege; this collector never changes privileges or elevates itself.') }

    $schema = [ordered]@{ available = $false; validated = $false; errors = @(); file = $null }
    if ([System.IO.File]::Exists($schemaPath)) {
        $savedSchema = Join-Path $script:OutputRoot 'WPRControlProfiles.xsd'
        [System.IO.File]::Copy($schemaPath, $savedSchema, $false)
        $schema.available = $true; $schema.file = Get-FileRecord $savedSchema
        try {
            $settings = New-Object System.Xml.XmlReaderSettings
            $null = $settings.Schemas.Add($null, $savedSchema)
            $settings.ValidationType = [System.Xml.ValidationType]::Schema
            $reader = [System.Xml.XmlReader]::Create($savedProfile, $settings)
            try { while ($reader.Read()) { } } finally { $reader.Dispose() }
            $schema.validated = $true
        } catch { $schema.errors = @($_.Exception.Message) }
    }
    if (-not $schema.validated) { $blockers.Add('The copied WPR profile has not passed the selected toolkit XSD schema validation.') }

    $wprSources = $null; $xperfSources = $null; $wprSessions = $null
    $xperfSessions = $null; $status = $null; $loggers = $null; $profiles = $null
    if ([System.IO.File]::Exists($wpr)) {
        $wprSources = Invoke-Logged 'wpr_pmcsources' $wpr @('-pmcsources')
        $wprSessions = Invoke-Logged 'wpr_pmcsessions' $wpr @('-pmcsessions')
        $status = Invoke-Logged 'wpr_status' $wpr @('-status', 'profiles')
        $profiles = Invoke-Logged 'wpr_profile_validation' $wpr @('-profiles', $savedProfile)
    } else { $blockers.Add('Selected Windows Performance Toolkit wpr.exe is missing.') }
    if ([System.IO.File]::Exists($xperf)) {
        $xperfSources = Invoke-Logged 'xperf_pmcsources' $xperf @('-pmcsources')
        $xperfSessions = Invoke-Logged 'xperf_pmcsessions' $xperf @('-pmcsessions')
        $loggers = Invoke-Logged 'xperf_loggers' $xperf @('-loggers')
    } else { $blockers.Add('Selected Windows Performance Toolkit xperf.exe is missing.') }
    $wprNormalized = Get-NormalizedSources $wprSources
    $xperfNormalized = Get-NormalizedSources $xperfSources
    $hardwareAvailable = $true
    foreach ($source in $required) {
        if ($null -eq $wprSources -or $wprSources.exit_code -ne 0 -or $wprNormalized.names -notcontains $source -or
            $null -eq $xperfSources -or $xperfSources.exit_code -ne 0 -or $xperfNormalized.names -notcontains $source) {
            $hardwareAvailable = $false
            $blockers.Add('Required hardware source is not enumerated by both selected tools: ' + $source)
        }
    }
    $pmcIdle = ($null -ne $wprSessions -and $wprSessions.exit_code -eq 0 -and $wprSessions.raw -match 'No PMC profile sources are being used' -and
        $null -ne $xperfSessions -and $xperfSessions.exit_code -eq 0 -and $xperfSessions.raw -match 'No PMC profile sources are being used')
    $wprIdle = ($null -ne $status -and $status.exit_code -eq 0 -and $status.raw -match 'WPR is not recording')
    $loggerKnown = ($null -ne $loggers -and $loggers.exit_code -eq 0 -and -not [string]::IsNullOrWhiteSpace($loggers.raw))
    $kernelConflict = ($loggerKnown -and $loggers.raw -match '(?im)NT Kernel Logger|PMC_PROFILE|PMC_PROFILE_SOURCE')
    $profileParsed = ($null -ne $profiles -and $profiles.exit_code -eq 0 -and $profiles.raw -match 'MaterialScenePmu')
    if (-not $pmcIdle) { $blockers.Add('PMC session status is active, failed, or unrecognized; foreign sessions will not be modified.') }
    if (-not $wprIdle) { $blockers.Add('The default WPR instance is active, failed, or unrecognized; foreign recordings will not be modified.') }
    if (-not $loggerKnown -or $kernelConflict) { $blockers.Add('Kernel/PMC logger state is unavailable or conflicting; foreign loggers will not be modified.') }
    if (-not $profileParsed) {
        $parseReason = 'The selected WPR tool did not successfully enumerate the exact copied custom profile.'
        if ($null -ne $profiles -and $profiles.raw -match 'Error code:\s*(0x[0-9a-fA-F]+)') {
            $parseReason += ' WPR error ' + $Matches[1] + '.'
        }
        if ($null -ne $profiles -and $profiles.raw -match 'number of hardware counters.+maximum allowable limit') {
            $parseReason += ' The requested strict counter count exceeds the maximum allowed by this OS/host.'
        }
        $blockers.Add($parseReason)
    }

    $report = [ordered]@{
        schema = 'workphone.material_scene_pmu.preflight.v1'; mode = $Mode
        created_utc = [DateTime]::UtcNow.ToString('o'); output_directory = $script:OutputRoot
        hardware_counters_available = [bool]$hardwareAvailable
        kernel_profile_privilege_available = [bool]$profilePrivilege
        capture_preflight_passed = ($blockers.Count -eq 0)
        blockers = @($blockers.ToArray()); never_started_by_preflight = $true
        capture_started = $false; hardware_metrics_available = $false
        capture_validation = 'Unvalidated: no actual PMU counter capture or export is established by preflight/profile parsing.'
        required_counters = $required; counter_scope = 'Generic Windows HAL counters sampled on CSwitch; no cache level is asserted.'
        roi_provider_guid = 'BB4D8875-7B47-4EDD-A325-06385E59DF1C'
        roi_markers = @('scene_profile_measurement_begin', 'scene_profile_measurement_end')
        host = [ordered]@{ computer_name = $env:COMPUTERNAME; os = $os; processors = $cpu; computer = $computer
            device_guard = $deviceGuard; cpuid = Get-OptionalCpuId; query_errors = @($hostErrors.ToArray()) }
        token = [ordered]@{ user = $identity.Name; user_sid = $identity.User.Value
            is_administrator = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
            profile_privilege_present = [bool]$profilePrivilege; privileges = $privileges; groups = $groups }
        toolkit = [ordered]@{ selected_directory = $toolkit; wpr = Get-FileRecord $wpr; xperf = Get-FileRecord $xperf
            wpa = Get-FileRecord (Join-Path $toolkit 'wpa.exe')
            wpaexporter = Get-FileRecord (Join-Path $toolkit 'wpaexporter.exe')
            system_wpr = Get-FileRecord (Join-Path $env:SystemRoot 'System32\wpr.exe') }
        collector_script = Get-FileRecord (Join-Path $script:OutputRoot 'CollectMaterialPmu.ps1')
        profile = [ordered]@{ file = Get-FileRecord $savedProfile; schema = $schema; wpr_parse_passed = $profileParsed; parser_result = $profiles }
        sources = [ordered]@{ wpr = [ordered]@{ normalized = $wprNormalized; result = $wprSources }
            xperf = [ordered]@{ normalized = $xperfNormalized; result = $xperfSources } }
        active_sessions = [ordered]@{ pmc_idle_confirmed = $pmcIdle; default_wpr_idle_confirmed = $wprIdle
            kernel_logger_status_known = $loggerKnown; kernel_or_pmc_conflict = $kernelConflict
            wpr = $wprSessions; xperf = $xperfSessions; default_wpr_status = $status; xperf_loggers = $loggers }
        cpu_fallback = 'Use scene QPC/Stopwatch stage/frame durations and aggregate process user/kernel CPU time. These are software-DX11/VM diagnostics where applicable, not cache-miss/instruction/CPI evidence.'
        references = @(
            'https://learn.microsoft.com/windows-hardware/test/wpt/recording-pmu-events',
            'https://learn.microsoft.com/windows-hardware/test/wpt/wpr-command-line-options',
            'https://learn.microsoft.com/windows-hardware/test/wpt/wprcontrolprofiles-schema',
            'https://learn.microsoft.com/windows-hardware/test/wpt/exporter',
            'https://learn.microsoft.com/virtualization/hyper-v-on-windows/tlfs/feature-discovery')
        commands = @($script:Commands.ToArray())
    }
    Write-NewJson (Join-Path $script:OutputRoot 'preflight.json') $report

    if ($Mode -eq 'PreflightOnly') {
        if ($InstanceName -or $OwnerFile) { throw 'PreflightOnly does not accept InstanceName or OwnerFile.' }
        Write-Output (Join-Path $script:OutputRoot 'preflight.json')
        exit 0
    }
    if ($Mode -eq 'Start') {
        Assert-InstanceName $InstanceName
        if ($OwnerFile) { throw 'Start does not accept OwnerFile.' }
        if (-not $report.capture_preflight_passed) {
            Write-NewJson (Join-Path $script:OutputRoot 'capture.json') ([ordered]@{
                started = $false; state = 'blocked_by_preflight'; blockers = $report.blockers })
            Write-Output (Join-Path $script:OutputRoot 'preflight.json'); exit 2
        }
        $ownStatus = Invoke-Logged 'own_instance_status_before_start' $wpr @('-status', 'profiles', '-instancename', $InstanceName)
        if ($ownStatus.exit_code -ne 0 -or $ownStatus.raw -notmatch 'WPR is not recording') {
            throw 'Requested unique WPR instance is active or its state is ambiguous; no trace was started.'
        }
        $request = [ordered]@{
            schema = 'workphone.material_scene_pmu.owner.v1'; owner_id = [guid]::NewGuid().ToString('N')
            instance_name = $InstanceName; computer_name = $env:COMPUTERNAME; user_sid = $identity.User.Value
            wpr = Get-FileRecord $wpr; profile = Get-FileRecord $savedProfile
            start_requested_utc = [DateTime]::UtcNow.ToString('o'); start_succeeded = $false
        }
        Write-NewJson (Join-Path $script:OutputRoot 'start_request.json') $request
        $start = Invoke-Logged 'own_instance_start' $wpr @('-start', ($savedProfile + '!MaterialScenePmu'), '-filemode', '-recordtempto', $script:OutputRoot, '-instancename', $InstanceName)
        if ($start.exit_code -ne 0 -or $start.error) {
            Write-NewJson (Join-Path $script:OutputRoot 'capture.json') ([ordered]@{
                started = $false; state = 'start_failed_or_uncertain'; result = $start
                note = 'No cleanup or foreign session operation was attempted. Inspect the exact named instance manually before retrying.' })
            exit 3
        }
        $request.start_succeeded = $true; $request.start_result = $start
        $request.started_utc = [DateTime]::UtcNow.ToString('o')
        Write-NewJson (Join-Path $script:OutputRoot 'owner.json') $request
        Write-NewJson (Join-Path $script:OutputRoot 'capture.json') ([ordered]@{
            started = $true; state = 'recording_owned_instance'; instance_name = $InstanceName
            owner_file = (Join-Path $script:OutputRoot 'owner.json'); hardware_metrics_available = $false
            note = 'Recording started; counter coverage, dropped events, ROI, process filtering and export remain unvalidated.' })
        Write-Output (Join-Path $script:OutputRoot 'owner.json'); exit 0
    }

    # Stop is allowed only through a successful immutable receipt from this collector.
    if (-not $OwnerFile) { throw 'Stop requires OwnerFile from this collector''s successful Start.' }
    $ownerPath = (Resolve-Path -LiteralPath $OwnerFile).ProviderPath
    $owner = Get-Content -LiteralPath $ownerPath -Raw | ConvertFrom-Json
    if ($owner.schema -ne 'workphone.material_scene_pmu.owner.v1' -or -not $owner.start_succeeded -or
        $owner.computer_name -ne $env:COMPUTERNAME -or $owner.user_sid -ne $identity.User.Value) {
        throw 'Owner receipt is not a successful Start by this user on this machine.'
    }
    Assert-InstanceName $owner.instance_name
    if ($InstanceName -and $InstanceName -ne $owner.instance_name) { throw 'InstanceName does not match the owner receipt.' }
    $actualWpr = Get-FileRecord $wpr
    if ($null -eq $actualWpr -or $actualWpr.path -ne $owner.wpr.path -or $actualWpr.sha256 -ne $owner.wpr.sha256) {
        throw 'Stop requires the exact WPR executable used for the owned Start; supply its WptDirectory.'
    }
    $ownerProfile = Get-FileRecord $owner.profile.path
    if ($null -eq $ownerProfile -or $ownerProfile.sha256 -ne $owner.profile.sha256) { throw 'The owned Start profile is missing or changed.' }
    $ownStatus = Invoke-Logged 'own_instance_status_before_stop' $wpr @('-status', 'profiles', '-instancename', $owner.instance_name)
    if ($ownStatus.exit_code -ne 0 -or $ownStatus.raw -notmatch 'WPR recording is in progress' -or $ownStatus.raw -notmatch 'MaterialScenePmu') {
        throw 'The receipt''s exact named instance is not confirmed active with MaterialScenePmu; no stop was attempted.'
    }
    # A create-new claim prevents concurrent/repeated Stop. It is intentionally
    # retained on uncertain failure; do not blindly repeat or cancel any session.
    Write-NewJson ($ownerPath + '.stop_claim.json') ([ordered]@{
        owner_id = $owner.owner_id; requested_utc = [DateTime]::UtcNow.ToString('o'); output_directory = $script:OutputRoot })
    $etl = Join-Path $script:OutputRoot 'MaterialScenePmu.etl'
    $stop = Invoke-Logged 'own_instance_stop' $wpr @('-stop', $etl, '-instancename', $owner.instance_name)
    $saved = ($stop.exit_code -eq 0 -and -not $stop.error -and [System.IO.File]::Exists($etl))
    $traceStats = $null
    if ($saved -and [System.IO.File]::Exists($xperf)) {
        $traceStats = Invoke-Logged 'owned_trace_statistics' $xperf @('-i', $etl, '-a', 'tracestats', '-detail')
    }
    Write-NewJson (Join-Path $script:OutputRoot 'capture.json') ([ordered]@{
        schema = 'workphone.material_scene_pmu.stop.v1'; owner_file = $ownerPath
        owner_file_sha256 = (Get-FileHash -LiteralPath $ownerPath -Algorithm SHA256).Hash
        instance_name = $owner.instance_name; saved = $saved; result = $stop
        trace = Get-FileRecord $etl; trace_statistics = $traceStats; hardware_metrics_available = $false
        note = 'Saved ETL still requires dropped-event, real counter, PID/TID/ROI and exporter validation. No numeric PMU metrics are invented.'
    })
    Write-Output (Join-Path $script:OutputRoot 'capture.json')
    if (-not $saved) { exit 3 }
    exit 0
} catch {
    $failure = [ordered]@{ schema = 'workphone.material_scene_pmu.error.v1'; mode = $Mode
        error = $_.Exception.Message; created_utc = [DateTime]::UtcNow.ToString('o')
        note = 'No automatic cancellation, foreign-session cleanup, security or boot configuration change is performed.' }
    if ($script:OutputRoot -and [System.IO.Directory]::Exists($script:OutputRoot) -and
        -not [System.IO.File]::Exists((Join-Path $script:OutputRoot 'error.json'))) {
        Write-NewJson (Join-Path $script:OutputRoot 'error.json') $failure
    }
    Write-Error $failure.error -ErrorAction Continue
    exit 1
}
