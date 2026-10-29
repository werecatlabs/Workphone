# Collect native frame/Present evidence from an already paused live Editor.
# This checks redraw progress; it does not inspect rendered pixels or send input.
param(
    [Parameter(Mandatory=$true)][string]$ActivityReport,
    [string]$Report,
    [ValidateRange(2,100)][int]$SampleCount=3,
    [ValidateRange(250,10000)][int]$IntervalMilliseconds=1000,
    [switch]$RequireFrozenSimulation,
    [switch]$RequireAudioSuspended,
    [switch]$RequireRenderOriginCallback,
    [switch]$RequireEditorDebuggerUI
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version 2

$activityPath=[IO.Path]::GetFullPath($ActivityReport)
if ($Report -and [StringComparer]::OrdinalIgnoreCase.Equals($activityPath,[IO.Path]::GetFullPath($Report))) {
    throw 'The collector report must differ from the live activity report.'
}
$advanceFields=@('nativeUpdateSerial','nativeRenderSerial','completedPumps','dx11FramesLifetime','dx11PresentAttemptsLifetime')
$stableFields=@('dx11PresentFailuresLifetime','dx11PresentOccludedLifetime')
$profileFields=@('nativeUpdateSerial','nativeRenderSerial','dx11FramesLifetime','dx11PresentAttemptsLifetime')+$stableFields
$editorUIFields=@('debuggerPanelVisible','debuggerStepOverEnabled','debuggerContinueEnabled',
                 'debuggerPauseEnabled','debuggerStatusStopped','debuggerSourcePreviewAvailable')

function Get-RequiredProperty($Document,[string]$Name) {
    $property=$Document.PSObject.Properties[$Name]
    if ($null -eq $property) { throw "Activity snapshot is missing $Name." }
    return $property.Value
}

function Get-BooleanProperty($Document,[string]$Name) {
    $value=Get-RequiredProperty $Document $Name
    if ($value -isnot [bool]) { throw "Activity snapshot field $Name must be a boolean." }
    return $value
}

function Get-CounterProperty($Document,[string]$Name) {
    $value=Get-RequiredProperty $Document $Name
    if ($null -eq $value -or $value.GetType().FullName -notmatch '^System\.(Byte|SByte|Int16|UInt16|Int32|UInt32|Int64|UInt64|Decimal|Double|Single|Numerics.BigInteger)$') {
        throw "Activity snapshot field $Name must be an unsigned integer."
    }
    try { $number=[decimal]$value }
    catch { throw "Activity snapshot field $Name is outside the supported integer range." }
    if ($number -lt 0 -or $number -gt [decimal]::Parse('18446744073709551615') -or [decimal]::Truncate($number) -ne $number) {
        throw "Activity snapshot field $Name must be an unsigned 64-bit integer."
    }
    # A JSON floating point value at this magnitude cannot preserve every integer.
    if (($value -is [double] -or $value -is [single]) -and $number -gt 9007199254740991) {
        throw "Activity snapshot field $Name has lost integer precision."
    }
    return $number
}

function Get-FiniteNumberProperty($Document,[string]$Name) {
    $value=Get-RequiredProperty $Document $Name
    if ($null -eq $value -or $value.GetType().FullName -notmatch '^System\.(Byte|SByte|Int16|UInt16|Int32|UInt32|Int64|UInt64|Decimal|Double|Single|Numerics.BigInteger)$') {
        throw "Activity snapshot field $Name must be a finite number."
    }
    try { $number=[double]$value }
    catch { throw "Activity snapshot field $Name is outside the supported numeric range." }
    if ([double]::IsNaN($number) -or [double]::IsInfinity($number)) {
        throw "Activity snapshot field $Name must be finite."
    }
    return $number
}

function Read-ActivitySnapshot {
    $stream=$null
    try {
        # Delete sharing is required for the Editor's atomic MoveFileEx replacement.
        $sharing=[IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete
        $stream=[IO.FileStream]::new($activityPath,[IO.FileMode]::Open,[IO.FileAccess]::Read,$sharing)
        if ($stream.Length -lt 1 -or $stream.Length -gt 65536) { throw 'Activity snapshot size must be 1..65536 bytes.' }
        $bytes=New-Object byte[] 65537
        $length=0
        while ($length -lt $bytes.Length) {
            $read=$stream.Read($bytes,$length,$bytes.Length-$length)
            if ($read -eq 0) { break }
            $length+=$read
        }
        if ($length -lt 1 -or $length -gt 65536) { throw 'Activity snapshot size must be 1..65536 bytes.' }
        $encoding=[Text.UTF8Encoding]::new($false,$true)
        $document=$encoding.GetString($bytes,0,$length) | ConvertFrom-Json
    }
    finally { if ($null -ne $stream) { $stream.Dispose() } }
    if ($null -eq $document -or $document -isnot [pscustomobject]) { throw 'Activity snapshot must be a JSON object.' }
    # Copy only the known, validated activity schema into evidence reports.
    $sample=[ordered]@{
        capturedUtc=[DateTime]::UtcNow.ToString('o')
        paused=Get-BooleanProperty $document 'paused'
        pauseSequence=Get-CounterProperty $document 'pauseSequence'
        completedPumps=Get-CounterProperty $document 'completedPumps'
        redrawAvailable=Get-BooleanProperty $document 'redrawAvailable'
        profileAvailable=Get-BooleanProperty $document 'profileAvailable'
        dx11Available=Get-BooleanProperty $document 'dx11Available'
    }
    $reason=Get-RequiredProperty $document 'redrawUnavailableReason'
    if ($reason -isnot [string] -or $reason.Length -gt 256) { throw 'Activity redraw-unavailable reason must be a short string.' }
    $sample.redrawUnavailableReason=$reason
    foreach ($field in $profileFields) {
        $sample[$field]=if ($sample.profileAvailable) { Get-CounterProperty $document $field } else { $null }
    }
    if ($RequireFrozenSimulation) {
        $sample.executorWorkerCount=Get-CounterProperty $document 'executorWorkerCount'
        $sample.timerSuspended=Get-BooleanProperty $document 'timerSuspended'
        $sample.inputSuspended=Get-BooleanProperty $document 'inputSuspended'
        $sample.frameSimulationEnabled=Get-BooleanProperty $document 'frameSimulationEnabled'
        foreach ($field in @('engineNow','engineFixedTimeNow','engineDeltaTime','nativeDeltaTime')) {
            $sample[$field]=Get-FiniteNumberProperty $document $field
        }
    }
    if ($RequireAudioSuspended) {
        $sample.audioSuspended=Get-BooleanProperty $document 'audioSuspended'
    }
    if ($RequireRenderOriginCallback) {
        $sample.deferredRenderCallbackActive=Get-BooleanProperty $document 'deferredRenderCallbackActive'
        foreach ($field in @('deferredRenderCallbackCount','dispatchedScriptEventCount')) {
            if ($document.PSObject.Properties[$field]) { $sample[$field]=Get-CounterProperty $document $field }
        }
    }
    if ($RequireEditorDebuggerUI) {
        foreach ($field in $editorUIFields) { $sample[$field]=Get-BooleanProperty $document $field }
        $button=Get-RequiredProperty $document 'debuggerStepOverShortcut'
        $render=Get-RequiredProperty $button 'lastRendered'
        $sample.debuggerButtonRenderFrame=Get-CounterProperty $render 'frame'
        $sample.debuggerButtonRenderedEnabled=Get-BooleanProperty $render 'enabled'
        $shortcut=Get-RequiredProperty $button 'keyboardShortcut'
        if ($shortcut -isnot [string]) { throw 'Debugger shortcut must be a scalar string.' }
        $sample.debuggerButtonShortcutIsF10=($shortcut -ceq 'F10')
    }
    return [pscustomobject]$sample
}

$samples=[Collections.Generic.List[object]]::new()
$deltas=[Collections.Generic.List[object]]::new()
$failures=[Collections.Generic.List[string]]::new()
for ($index=0; $index -lt $SampleCount; ++$index) {
    if ($index -gt 0) { Start-Sleep -Milliseconds $IntervalMilliseconds }
    try {
        $sample=Read-ActivitySnapshot
        $samples.Add($sample)
        foreach ($flag in @('paused','redrawAvailable','profileAvailable','dx11Available')) {
            if (-not $sample.$flag) { $failures.Add("Sample $index requires $flag=true.") }
        }
        if ($sample.pauseSequence -lt 1) { $failures.Add("Sample $index has no active pause sequence.") }
        if ($RequireFrozenSimulation) {
            if ($sample.executorWorkerCount -ne 0) { $failures.Add("Sample $index requires executorWorkerCount=0.") }
            if (-not $sample.timerSuspended) { $failures.Add("Sample $index requires timerSuspended=true.") }
            if (-not $sample.inputSuspended) { $failures.Add("Sample $index requires inputSuspended=true.") }
            if ($sample.frameSimulationEnabled) { $failures.Add("Sample $index requires frameSimulationEnabled=false.") }
            foreach ($field in @('engineDeltaTime','nativeDeltaTime')) {
                if ($sample.$field -ne 0.0) { $failures.Add("Sample $index requires ${field}=0.") }
            }
        }
        if ($RequireAudioSuspended -and -not $sample.audioSuspended) {
            $failures.Add("Sample $index requires audioSuspended=true.")
        }
        if ($RequireRenderOriginCallback -and -not $sample.deferredRenderCallbackActive) {
            $failures.Add("Sample $index requires deferredRenderCallbackActive=true.")
        }
        if ($RequireEditorDebuggerUI) {
            if (-not $sample.debuggerButtonRenderedEnabled -or -not $sample.debuggerButtonShortcutIsF10) {
                $failures.Add("Sample $index requires a rendered, enabled native F10 button.")
            }
            foreach ($field in $editorUIFields) {
                $expected=($field -ne 'debuggerPauseEnabled')
                if ($sample.$field -ne $expected) { $failures.Add("Sample $index requires ${field}=$expected.") }
            }
        }
    }
    catch {
        # Do not include raw JSON or arbitrary file contents in an error report.
        $samples.Add([pscustomobject]@{capturedUtc=[DateTime]::UtcNow.ToString('o');error='Snapshot read or schema validation failed.'})
        $failures.Add("Sample $index could not be read or did not match the activity schema.")
    }
}

for ($index=1; $index -lt $samples.Count; ++$index) {
    $before=$samples[$index-1]
    $after=$samples[$index]
    if ($before.PSObject.Properties['error'] -or $after.PSObject.Properties['error']) { continue }
    if ($after.pauseSequence -ne $before.pauseSequence) {
        $failures.Add("Samples $($index-1) and $index belong to different pause sequences.")
    }
    $delta=[ordered]@{fromSample=$index-1;toSample=$index}
    foreach ($field in $advanceFields+$stableFields) {
        if ($null -eq $before.$field -or $null -eq $after.$field) { $delta[$field]=$null; continue }
        $difference=$after.$field-$before.$field
        $delta[$field]=$difference
        if ($field -in $advanceFields -and $difference -le 0) {
            $failures.Add("Samples $($index-1)..${index}: $field did not advance.")
        }
        if ($field -in $stableFields -and $difference -ne 0) {
            $failures.Add("Samples $($index-1)..${index}: $field changed.")
        }
    }
    if ($RequireFrozenSimulation) {
        foreach ($field in @('engineNow','engineFixedTimeNow')) {
            $delta[$field]=$after.$field-$before.$field
            if ($after.$field -ne $before.$field) {
                $failures.Add("Samples $($index-1)..${index}: $field advanced while simulation was suspended.")
            }
        }
    }
    if ($RequireEditorDebuggerUI) {
        $delta.debuggerButtonRenderFrame=$after.debuggerButtonRenderFrame-$before.debuggerButtonRenderFrame
        if ($delta.debuggerButtonRenderFrame -le 0) {
            $failures.Add("Samples $($index-1)..${index}: the debugger button was not redrawn.")
        }
    }
    $deltas.Add([pscustomobject]$delta)
}

$result=[pscustomobject][ordered]@{
    gate='Lua debugger paused native redraw'
    passed=($failures.Count -eq 0)
    requireFrozenSimulation=[bool]$RequireFrozenSimulation
    requireAudioSuspended=[bool]$RequireAudioSuspended
    requireRenderOriginCallback=[bool]$RequireRenderOriginCallback
    requireEditorDebuggerUI=[bool]$RequireEditorDebuggerUI
    intervalMilliseconds=$IntervalMilliseconds
    samples=$samples.ToArray()
    deltas=$deltas.ToArray()
    failures=$failures.ToArray()
}
if ($Report) {
    $reportPath=[IO.Path]::GetFullPath($Report)
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($reportPath)) | Out-Null
    [IO.File]::WriteAllText($reportPath,($result | ConvertTo-Json -Depth 8),[Text.UTF8Encoding]::new($false))
}
Write-Output $result
if (-not $result.passed) { throw ('Paused native redraw gate failed: '+($failures -join ' ')) }
