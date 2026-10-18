[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Split-Path $PSScriptRoot -Parent),
    [ValidateSet('x64', 'Win32')][string]$Architecture = 'x64'
)
$ErrorActionPreference = 'Stop'
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$runRoot = Join-Path $RepositoryRoot ('project_graphics_sdk/validation-' + [guid]::NewGuid().ToString('N'))
$build = Join-Path $runRoot 'build'
$prefix = Join-Path $runRoot 'original-install'
$relocated = Join-Path $runRoot 'relocated SDK'
$consumerSource = Join-Path $runRoot 'external sample'
$consumerBuild = Join-Path $runRoot 'consumer-build'
New-Item -ItemType Directory -Path $runRoot | Out-Null
$commands = [System.Collections.Generic.List[object]]::new()
$report = [ordered]@{
    schemaVersion = 1
    architecture = $Architecture
    scope = 'Windows static native SDK; relocated C90/C++17 consumer with software rasterization'
    validationPassed = $false
    releaseCertified = $false
    commands = $commands
    startedUtc = [DateTime]::UtcNow.ToString('o')
}
function Invoke-Checked([string]$Label, [string]$Program, [string[]]$Arguments) {
    $log = Join-Path $runRoot ($Label + '.log')
    Write-Host "$Label..."
    & $Program @Arguments *> $log
    $code = $LASTEXITCODE
    $commands.Add([ordered]@{ label = $Label; program = $Program; arguments = $Arguments; exitCode = $code; log = $log })
    if ($code -ne 0) {
        Get-Content -LiteralPath $log -Tail 40 | Write-Host
        throw "$Label failed with exit code $code"
    }
}
function Get-SourceSnapshot {
    $inputs = @(Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot 'Engine/c/Source/WorkphoneCore'),
        (Join-Path $RepositoryRoot 'Engine/c/Source/WorkphoneGraphics'),
        (Join-Path $RepositoryRoot 'Engine/c/Source/WorkphonePlatformWin32'),
        (Join-Path $RepositoryRoot 'Engine/c/Include/WorkphoneCore'),
        (Join-Path $RepositoryRoot 'Engine/c/Include/WorkphoneGraphics'),
        (Join-Path $RepositoryRoot 'Engine/c/Include/WorkphonePlatformWin32'),
        (Join-Path $RepositoryRoot 'Engine/c/Include/WorkphonePhysics'),
        (Join-Path $RepositoryRoot 'Tests/GraphicsSDKConsumer') -Recurse -File)
    $inputs += foreach ($relative in @('CMakeLists.txt', 'CMake/InstallWorkphoneGraphicsSDK.cmake',
        'CMake/WorkphoneGraphicsSDKConfig.cmake.in', 'Tools/ValidateGraphicsSDK.ps1',
        'Engine/c/Project/WorkphoneCore/CMakeLists.txt', 'Engine/c/Project/WorkphoneGraphics/CMakeLists.txt',
        'Engine/c/Project/WorkphonePlatformWin32/CMakeLists.txt', 'LICENSE', 'docs/WPGRAPHICS_NATIVE_SDK.md')) {
        Get-Item -LiteralPath (Join-Path $RepositoryRoot $relative)
    }
    @($inputs | Sort-Object FullName -Unique | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($RepositoryRoot.Length + 1).Replace('\', '/');
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
}
try {
    $snapshot = Get-SourceSnapshot
    Invoke-Checked 'configure-sdk' 'cmake' @('-S', $RepositoryRoot, '-B', $build, '-A', $Architecture,
        '-DWP_BUILD_GRAPHICS_SDK_ONLY=ON')
    foreach ($config in @('Debug', 'RelWithDebInfo')) {
        Invoke-Checked "build-sdk-$config" 'cmake' @('--build', $build, '--config', $config, '--parallel', '4')
        Invoke-Checked "install-sdk-$config" 'cmake' @('--install', $build, '--config', $config, '--prefix', $prefix)
    }
    # Both absolute move targets are verified beneath this fresh, task-owned run.
    $safeRoot = [IO.Path]::GetFullPath($runRoot).TrimEnd('\') + '\'
    foreach ($path in @($prefix, $relocated)) {
        if (-not [IO.Path]::GetFullPath($path).StartsWith($safeRoot, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Relocation path escapes validation run: $path"
        }
    }
    Move-Item -LiteralPath $prefix -Destination $relocated
    if (Test-Path -LiteralPath $prefix) { throw 'Original installation still exists after relocation' }
    # The consumer source itself comes from the installed sample, not Tests/ in the checkout.
    Copy-Item -LiteralPath (Join-Path $relocated 'share/WorkphoneGraphicsSDK/sample') -Destination $consumerSource -Recurse
    foreach ($config in @('Debug', 'RelWithDebInfo')) {
        foreach ($target in @('WorkphoneCore', 'WorkphoneGraphics', 'WorkphonePlatformWin32')) {
            foreach ($extension in @('lib', 'pdb')) {
                if (-not (Test-Path -LiteralPath (Join-Path $relocated "lib/$config/$target.$extension"))) {
                    throw "Missing installed archive or symbol file: $config/$target.$extension"
                }
            }
        }
    }
    $exports = Get-ChildItem -LiteralPath (Join-Path $relocated 'lib/cmake/WorkphoneGraphicsSDK') -Filter '*.cmake'
    foreach ($file in $exports) {
        $content = Get-Content -LiteralPath $file.FullName -Raw
        foreach ($forbidden in @($RepositoryRoot, $RepositoryRoot.Replace('\', '/'), $build, $build.Replace('\', '/'))) {
            if ($content.Contains($forbidden)) { throw "Export contains local source/build path: $($file.Name)" }
        }
    }
    Invoke-Checked 'configure-consumer' 'cmake' @('-S', $consumerSource, '-B', $consumerBuild, '-A', $Architecture,
        "-DSDK_ROOT=$($relocated.Replace('\', '/'))")
    foreach ($config in @('Debug', 'RelWithDebInfo')) {
        Invoke-Checked "build-consumer-$config" 'cmake' @('--build', $consumerBuild, '--config', $config, '--parallel', '4')
        Invoke-Checked "test-consumer-$config" 'ctest' @('--test-dir', $consumerBuild, '-C', $config,
            '--no-tests=error', '-V', '--output-junit', (Join-Path $runRoot "consumer-$config.xml"))
    }
    $report.installedFiles = @(Get-ChildItem -LiteralPath $relocated -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($relocated.Length + 1).Replace('\', '/');
            bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    $after = Get-SourceSnapshot
    if (($snapshot | ConvertTo-Json -Depth 4 -Compress) -cne ($after | ConvertTo-Json -Depth 4 -Compress)) {
        throw 'SDK inputs changed during validation; rerun against stable sources'
    }
    $report.sourceFiles = $snapshot
    $archive = Join-Path $runRoot "WorkphoneGraphicsSDK-0.1.0-Windows-$Architecture.zip"
    Compress-Archive -LiteralPath $relocated -DestinationPath $archive
    $report.archive = [ordered]@{ path = $archive; sha256 = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash }
    $report.validationPassed = $true
    $report.installation = $relocated
} catch {
    $report.error = $_.Exception.Message
    throw
} finally {
    $report.finishedUtc = [DateTime]::UtcNow.ToString('o')
    $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $runRoot 'evidence.json') -Encoding UTF8
    Write-Host "SDK evidence: $(Join-Path $runRoot 'evidence.json')"
}
