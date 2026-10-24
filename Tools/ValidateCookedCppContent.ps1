#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$ContentDirectory,
    [Parameter(Mandatory)][string]$SdkRoot,
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [Parameter(Mandatory)][string]$Configuration
)
$ErrorActionPreference = 'Stop'
$evidence = [IO.Path]::GetFullPath($EvidenceDirectory)
$requestedGood = [IO.Path]::GetFullPath($ContentDirectory)
if ($evidence -ieq $requestedGood -or
    $evidence.StartsWith($requestedGood.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
    $requestedGood.StartsWith($evidence.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Cooked evidence and good content directories must not overlap.'
}
if (Test-Path -LiteralPath $evidence) {
    if (-not (Test-Path -LiteralPath $evidence -PathType Container) -or
        @(Get-ChildItem -LiteralPath $evidence -Force).Count) {
        throw 'Cooked-content evidence directory must be new or empty.'
    }
} else {
    New-Item -ItemType Directory -Path $evidence | Out-Null
}
$cases = [Collections.Generic.List[object]]::new()
$report = [ordered]@{
    schemaVersion = 1
    validationPassed = $false
    releaseCertified = $false
    scope = 'Installed C++ cooked material/texture runtime; fresh restricted processes and integrity rejection'
    configuration = $Configuration
    startedUtc = [DateTime]::UtcNow.ToString('o')
    cases = $cases
}
function Get-CookedPayload([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -File -Recurse -Force | Sort-Object FullName | ForEach-Object {
        [ordered]@{
            path = [IO.Path]::GetRelativePath($Root, $_.FullName).Replace('\', '/')
            bytes = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
        }
    })
}
function Assert-OwnedPath([string]$Path) {
    $resolved = [IO.Path]::GetFullPath($Path)
    if (-not $resolved.StartsWith($evidence.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw "Cooked validation mutation escapes its evidence directory: $resolved"
    }
    $resolved
}
function New-NegativePackage([string]$Name) {
    $clone = Assert-OwnedPath (Join-Path $evidence "cases/$Name")
    if (Test-Path -LiteralPath $clone) { throw "Negative package already exists: $clone" }
    New-Item -ItemType Directory -Path $clone | Out-Null
    foreach ($item in Get-ChildItem -LiteralPath $good -Force) {
        Copy-Item -LiteralPath $item.FullName -Destination $clone -Recurse
    }
    Get-ChildItem -LiteralPath $clone -File -Recurse -Force | ForEach-Object { $_.IsReadOnly = $false }
    $clone
}
function Damage-LastByte([string]$Path) {
    $owned = Assert-OwnedPath $Path
    $stream = [IO.File]::Open($owned, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        if ($stream.Length -lt 1) { throw "Cannot corrupt empty cooked file: $owned" }
        $stream.Position = $stream.Length - 1
        $value = $stream.ReadByte()
        $stream.Position = $stream.Length - 1
        $stream.WriteByte([byte]($value -bxor 0x80))
    } finally {
        $stream.Dispose()
    }
}
function Invoke-CookedCase([string]$Name, [string]$Package, [bool]$Reject, [string[]]$ExtraArguments = @()) {
    $stdoutPath = Join-Path $evidence "$Name.stdout.log"
    $stderrPath = Join-Path $evidence "$Name.stderr.log"
    $case = [ordered]@{
        name = $Name
        expectedExitCode = $(if ($Reject) { 10 } else { 0 })
        validationPassed = $false
        contentDirectory = $Package
        inputPayload = Get-CookedPayload $Package
        arguments = @($Package) + $ExtraArguments
        stdout = [IO.Path]::GetFileName($stdoutPath)
        stderr = [IO.Path]::GetFileName($stderrPath)
    }
    $cases.Add($case)
    $start = [Diagnostics.ProcessStartInfo]::new($exe)
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.StandardOutputEncoding = [Text.Encoding]::UTF8
    $start.StandardErrorEncoding = [Text.Encoding]::UTF8
    $start.WorkingDirectory = $evidence
    $start.ArgumentList.Add($Package)
    foreach ($argument in $ExtraArguments) { $start.ArgumentList.Add($argument) }
    $start.Environment.Clear()
    $start.Environment['SystemRoot'] = $env:SystemRoot
    $start.Environment['PATH'] = (Join-Path $sdk 'bin') + ';' + (Join-Path $env:SystemRoot 'System32')
    $start.Environment['TEMP'] = $processTemp
    $start.Environment['TMP'] = $processTemp
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        if (-not $process.Start()) { throw "Failed to start cooked consumer: $Name" }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        $timedOut = -not $process.WaitForExit(60000)
        if ($timedOut) {
            $process.Kill($true)
            $process.WaitForExit()
        }
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        $stdout | Set-Content -LiteralPath $stdoutPath -Encoding UTF8
        $stderr | Set-Content -LiteralPath $stderrPath -Encoding UTF8
        $case['exitCode'] = $process.ExitCode
        $case['timedOut'] = $timedOut
        $case['durationSeconds'] = $watch.Elapsed.TotalSeconds
        $modules = @($stdout -split '\r?\n' | Where-Object { $_.StartsWith('module=') } |
            ForEach-Object { $_.Substring(7).Trim() })
        $case['loadedModules'] = $modules
        foreach ($module in $modules) {
            $modulePath = [IO.Path]::GetFullPath($module)
            if ($modulePath -ieq $exe) { continue }
            if (-not $modulePath.StartsWith($sdk.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -and
                -not $modulePath.StartsWith($env:SystemRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
                throw "Unexpected cooked consumer module: $module"
            }
        }
        if ($modules | Where-Object { [IO.Path]::GetFileName($_) -ieq 'WPSQLite.dll' }) {
            throw "Runtime-only cooked consumer loaded WPSQLite: $Name"
        }
        if ($timedOut) { throw "Cooked consumer timed out: $Name" }
        if ($process.ExitCode -ne $case.expectedExitCode) {
            throw "Cooked case $Name returned $($process.ExitCode), expected $($case.expectedExitCode)."
        }
        if ($Reject) {
            if ($stderr -notmatch '(?m)^Cooked content rejected:\s*\S') {
                throw "Cooked case $Name did not report its expected rejection diagnostic."
            }
        } else {
            if ($stdout -notlike '*Installed cooked content:*passed*') {
                throw 'Cooked consumer did not report successful typed runtime validation.'
            }
            foreach ($required in @('Workphone.dll', 'WPGraphics.dll')) {
                if (-not ($modules | Where-Object { [IO.Path]::GetFileName($_) -ieq $required })) {
                    throw "Required cooked consumer DLL not observed: $required"
                }
            }
        }
        $case['validationPassed'] = $true
    } finally {
        $watch.Stop()
        $process.Dispose()
    }
}
try {
    $exe = (Resolve-Path -LiteralPath $Executable).Path
    $good = (Resolve-Path -LiteralPath $ContentDirectory).Path
    $sdk = (Resolve-Path -LiteralPath $SdkRoot).Path
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf) -or
        -not (Test-Path -LiteralPath $good -PathType Container) -or
        -not (Test-Path -LiteralPath (Join-Path $sdk 'bin') -PathType Container)) {
        throw 'Cooked validation requires an executable, content directory and SDK bin directory.'
    }
    $manifest = Join-Path $good 'runtime.wprm'
    if ((Get-Item -LiteralPath $good).Attributes -band [IO.FileAttributes]::ReparsePoint -or
        (Get-ChildItem -LiteralPath $good -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint })) {
        throw 'Cooked validation requires ordinary files/directories, not reparse points.'
    }
    $allFiles = @(Get-ChildItem -LiteralPath $good -File -Recurse -Force)
    $artifacts = @($allFiles | Where-Object Extension -ieq '.wprs' | Sort-Object FullName)
    if (-not (Test-Path -LiteralPath $manifest -PathType Leaf) -or $artifacts.Count -ne 2 -or $allFiles.Count -ne 3) {
        throw 'Cooked sample must contain runtime.wprm and exactly two WPRS artifacts.'
    }
    if ($evidence -ieq $good -or $evidence.StartsWith($good.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) -or
        $good.StartsWith($evidence.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Cooked evidence and good content directories must not overlap.'
    }
    $processTemp = Assert-OwnedPath (Join-Path $evidence 'process-temp')
    New-Item -ItemType Directory -Path $processTemp | Out-Null
    $before = Get-CookedPayload $good
    $report['executable'] = $exe
    $report['executableSha256'] = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    $report['contentDirectory'] = $good
    $report['sdkRoot'] = $sdk
    $report['payload'] = $before
    Invoke-CookedCase 'good' $good $false
    $missingManifest = New-NegativePackage 'missing-manifest'
    $removePath = Assert-OwnedPath (Join-Path $missingManifest 'runtime.wprm')
    Remove-Item -LiteralPath $removePath -Force
    Invoke-CookedCase 'missing-manifest' $missingManifest $true
    $corruptManifest = New-NegativePackage 'corrupt-manifest'
    Damage-LastByte (Join-Path $corruptManifest 'runtime.wprm')
    Invoke-CookedCase 'corrupt-manifest' $corruptManifest $true
    Invoke-CookedCase 'wrong-target' $good $true @('--target', 'wrong-cooked-validation-target')
    for ($index = 0; $index -lt $artifacts.Count; ++$index) {
        $relative = [IO.Path]::GetRelativePath($good, $artifacts[$index].FullName)
        $missingName = "missing-artifact-$index"
        $missingPackage = New-NegativePackage $missingName
        $removePath = Assert-OwnedPath (Join-Path $missingPackage $relative)
        Remove-Item -LiteralPath $removePath -Force
        Invoke-CookedCase $missingName $missingPackage $true
        $corruptName = "corrupt-artifact-$index"
        $corruptPackage = New-NegativePackage $corruptName
        Damage-LastByte (Join-Path $corruptPackage $relative)
        Invoke-CookedCase $corruptName $corruptPackage $true
    }
    $after = Get-CookedPayload $good
    if (($before | ConvertTo-Json -Depth 4 -Compress) -ne ($after | ConvertTo-Json -Depth 4 -Compress)) {
        throw 'Good cooked payload changed during validation.'
    }
    $report['goodPayloadUnchanged'] = $true
    $report['validationPassed'] = $true
} catch {
    $report['error'] = $_.Exception.Message
    throw
} finally {
    $report['completedUtc'] = [DateTime]::UtcNow.ToString('o')
    $report['cases'] = $cases.ToArray()
    $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $evidence 'cooked-content-evidence.json') -Encoding UTF8
}
[pscustomobject]$report
