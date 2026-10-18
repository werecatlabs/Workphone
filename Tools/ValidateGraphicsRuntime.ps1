#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ConsumerBuild,
    [Parameter(Mandatory)][string]$SdkRoot,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet('x64', 'Win32')][string]$Architecture = 'x64'
)
$ErrorActionPreference = 'Stop'
$OutputDirectory = (Resolve-Path -LiteralPath $OutputDirectory).Path
$stage = Join-Path $OutputDirectory 'WorkphoneGraphicsRuntime'
$tests = [System.Collections.Generic.List[object]]::new()
$evidence = [ordered]@{ schemaVersion = 1; validationPassed = $false; releaseCertified = $false
    scope = 'Local source-free native console sample; offscreen software rendering'
    startedUtc = [DateTime]::UtcNow.ToString('o'); tests = $tests }

function Get-PayloadFiles([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($Root.Length + 1).Replace('\', '/'); bytes = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
}

function Assert-Payload([string]$Root, $Files) {
    if (@(Get-ChildItem -LiteralPath $Root -Recurse -File).Count -ne $Files.Count) { throw 'Runtime payload file count changed' }
    foreach ($file in $Files) {
        $path = Join-Path $Root $file.path
        if (-not (Test-Path -LiteralPath $path) -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ne $file.sha256) {
            throw "Runtime payload missing or changed: $($file.path)"
        }
    }
}

function Invoke-Sample([string]$Name, [string]$Mesh, [int]$ExpectedExit) {
    $moduleReport = Join-Path $isolatedRoot "$Name-modules.txt"
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $executable
    $info.WorkingDirectory = $runtimeRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.ArgumentList.Add($Mesh)
    $info.Environment.Clear()
    $info.Environment['SYSTEMROOT'] = $env:SystemRoot
    $info.Environment['WINDIR'] = $env:SystemRoot
    $info.Environment['PATH'] = (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
    $info.Environment['TEMP'] = $isolatedRoot
    $info.Environment['TMP'] = $isolatedRoot
    $info.Environment['WP_GRAPHICS_MODULE_REPORT'] = $moduleReport
    $process = [Diagnostics.Process]::Start($info)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(30000)) {
            $process.Kill($true)
            $process.WaitForExit()
            throw "Runtime sample timed out: $Name"
        }
        $output = $stdout.GetAwaiter().GetResult()
        $errors = $stderr.GetAwaiter().GetResult()
        $output | Set-Content -LiteralPath (Join-Path $OutputDirectory "runtime-$Name-stdout.log") -Encoding UTF8
        $errors | Set-Content -LiteralPath (Join-Path $OutputDirectory "runtime-$Name-stderr.log") -Encoding UTF8
        $tests.Add([ordered]@{ name = $Name; expectedExit = $ExpectedExit; exitCode = $process.ExitCode })
        if ($process.ExitCode -ne $ExpectedExit) { throw "Runtime $Name exited $($process.ExitCode), expected $ExpectedExit`: $errors" }
        if ($ExpectedExit -eq 0 -and $output -notmatch 'Installed SDK: cooked WPM1 mesh') { throw 'Runtime success evidence missing' }
        if ($ExpectedExit -eq 1 -and $errors -notmatch 'Cannot load cooked mesh:') { throw 'Expected mesh rejection diagnostic missing' }
    } finally { $process.Dispose() }
    return $moduleReport
}

try {
    if (Test-Path -LiteralPath $stage) { throw 'Runtime stage already exists; use a fresh validation output directory' }
    & cmake --install $ConsumerBuild --config RelWithDebInfo --prefix $stage *> (Join-Path $OutputDirectory 'install-runtime.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime install failed; see install-runtime.log' }
    $prerequisites = Get-Content -LiteralPath (Join-Path $SdkRoot 'share/WorkphoneGraphicsSDK/WorkphoneGraphicsSDK-dependencies.json') -Raw | ConvertFrom-Json
    $reader = [IO.BinaryReader]::new([IO.File]::OpenRead((Join-Path $stage 'bin/WorkphoneGraphicsSample.exe')))
    try {
        if ($reader.ReadUInt16() -ne 0x5a4d) { throw 'Runtime executable is not a PE image' }
        $reader.BaseStream.Position = 0x3c
        $peOffset = $reader.ReadInt32()
        $reader.BaseStream.Position = $peOffset
        if ($reader.ReadUInt32() -ne 0x4550) { throw 'Runtime executable has an invalid PE header' }
        $machine = $reader.ReadUInt16()
    } finally { $reader.Dispose() }
    $expectedMachine = if ($Architecture -eq 'x64') { 0x8664 } else { 0x14c }
    if ($machine -ne $expectedMachine) { throw "Runtime architecture mismatch: $Architecture package, PE machine 0x$($machine.ToString('X4'))" }
    $evidence.machine = '0x' + $machine.ToString('X4')
    $files = Get-PayloadFiles $stage
    foreach ($file in $files) {
        if ($file.path -ne 'licenses/LICENSE' -and
            [IO.Path]::GetExtension($file.path) -notin @('.exe', '.pdb', '.wpm', '.txt', '.md')) {
            throw "Unexpected runtime payload (source/SDK files are excluded): $($file.path)"
        }
    }
    foreach ($required in @('bin/WorkphoneGraphicsSample.exe', 'content/triangle.wpm',
        'symbols/WorkphoneGraphicsSample.pdb', 'licenses/LICENSE',
        'licenses/WPGRAPHICS_THIRD_PARTY_NOTICES.txt', 'WPGRAPHICS_RUNTIME_SAMPLE.md')) {
        if (-not (Test-Path -LiteralPath (Join-Path $stage $required))) { throw "Required runtime file missing: $required" }
    }
    # LICENSE has no extension and is an explicitly allowed notice.
    $manifest = [ordered]@{ schemaVersion = 1; packageVersion = '0.1.0'; architecture = $Architecture
        machine = $evidence.machine; configuration = 'RelWithDebInfo'; compiler = $prerequisites.compiler
        prerequisites = 'Windows system components and matching Microsoft Visual C++ release DLL runtime; no Microsoft DLLs bundled'
        scope = $evidence.scope; releaseCertified = $false; files = $files }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $stage 'runtime-manifest.json') -Encoding UTF8
    $payload = Get-PayloadFiles $stage
    $archive = Join-Path $OutputDirectory "WorkphoneGraphicsRuntime-0.1.0-Windows-$Architecture.zip"
    Compress-Archive -LiteralPath $stage -DestinationPath $archive
    $evidence.archive = [ordered]@{ path = $archive; sha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash }
    $isolatedRoot = Join-Path ([IO.Path]::GetTempPath()) ('wp-graphics-runtime-' + [guid]::NewGuid().ToString('N'))
    foreach ($forbiddenRoot in @($ConsumerBuild, $SdkRoot, (Split-Path $PSScriptRoot -Parent))) {
        $forbiddenPrefix = [IO.Path]::GetFullPath($forbiddenRoot).TrimEnd('\') + '\'
        if ([IO.Path]::GetFullPath($isolatedRoot).StartsWith($forbiddenPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime isolation directory is inside a source/build/SDK tree: $isolatedRoot"
        }
    }
    # The application launch and all assets live outside the build and SDK trees.
    New-Item -ItemType Directory -Path $isolatedRoot | Out-Null
    Expand-Archive -LiteralPath $archive -DestinationPath $isolatedRoot
    $runtimeRoot = Join-Path $isolatedRoot 'WorkphoneGraphicsRuntime'
    $evidence.isolatedDirectory = $isolatedRoot
    Assert-Payload $runtimeRoot $payload
    Get-ChildItem -LiteralPath $runtimeRoot -Recurse -File | ForEach-Object { $_.IsReadOnly = $true }
    $evidence.payloadFilesReadOnly = $true
    $executable = (Resolve-Path -LiteralPath (Join-Path $runtimeRoot 'bin/WorkphoneGraphicsSample.exe')).Path
    $modules = Invoke-Sample 'valid-content' (Join-Path $runtimeRoot 'content/triangle.wpm') 0
    $null = Invoke-Sample 'missing-content' (Join-Path $runtimeRoot 'content/missing.wpm') 1
    $corrupt = Join-Path $isolatedRoot 'truncated.wpm'
    [IO.File]::WriteAllBytes($corrupt, [byte[]]@(0x57, 0x50, 0x4d, 0x31))
    $null = Invoke-Sample 'corrupt-content' $corrupt 1
    $windowsPrefix = [IO.Path]::GetFullPath($env:SystemRoot).TrimEnd('\') + '\'
    $modulePaths = @(Get-Content -LiteralPath $modules -Encoding UTF8 | Sort-Object -Unique)
    if ($modulePaths -notcontains $executable) { throw 'Running application is missing from its module report' }
    $evidence.loadedModules = @($modulePaths | ForEach-Object {
        $resolved = [IO.Path]::GetFullPath($_)
        if ($resolved -ne $executable -and -not $resolved.StartsWith($windowsPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Runtime loaded a developer/non-Windows dependency: $resolved"
        }
        [ordered]@{ path = $resolved; sha256 = (Get-FileHash -LiteralPath $resolved -Algorithm SHA256).Hash }
    })
    Assert-Payload $runtimeRoot $payload
    $evidence.payloadFiles = $payload
    $evidence.payloadUnchanged = $true
    $evidence.restrictedEnvironmentKeys = @('SYSTEMROOT', 'WINDIR', 'PATH', 'TEMP', 'TMP', 'WP_GRAPHICS_MODULE_REPORT')
    $evidence.validationPassed = $true
} catch {
    $evidence.error = $_.Exception.Message
    throw
} finally {
    $evidence.finishedUtc = [DateTime]::UtcNow.ToString('o')
    $evidence | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $OutputDirectory 'runtime-evidence.json') -Encoding UTF8
    Write-Host "Runtime evidence: $(Join-Path $OutputDirectory 'runtime-evidence.json')"
}
