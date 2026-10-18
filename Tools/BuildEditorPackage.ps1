#requires -Version 7.0
[CmdletBinding()]
param(
    [string]$BuildDirectory = 'project_x64',
    [string]$OutputDirectory,
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = if ([IO.Path]::IsPathRooted($BuildDirectory)) { [IO.Path]::GetFullPath($BuildDirectory) } else { [IO.Path]::GetFullPath((Join-Path $repo $BuildDirectory)) }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repo ('project_graphics_sdk/editor-validation-' + [guid]::NewGuid().ToString('N')) }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Use a new output directory to preserve previous evidence.' }
New-Item -ItemType Directory -Path $output | Out-Null
$evidence = [ordered]@{ schemaVersion = 1; validationPassed = $false; releaseCertified = $false
    scope = 'Relocated Windows x64 C++ Editor candidate; headless software startup'
    buildDirectory = $build; incrementalBuild = $true; buildSkipped = [bool]$SkipBuild
    interactiveValidated = $false; cleanMachineValidated = $false; licenseAuditComplete = $false }
$checks = [Collections.Generic.List[object]]::new()
$inputs = @('CMakeLists.txt','CMake/InstallWorkphoneEditor.cmake','Tools/BuildEditorPackage.ps1',
    'Tools/cpp/Editor/src/application/win32/Main.cpp','Tools/cpp/Editor/src/application/win32/EditorPackage.hpp',
    'Tools/cpp/Editor/src/EditorApplication.hpp','Tools/cpp/Editor/src/EditorApplication.cpp',
    'Engine/cpp/Source/Workphone/System/ApplicationManager.cpp','docs/WPGRAPHICS_EDITOR_PACKAGE.md',
    'docs/WPGRAPHICS_THIRD_PARTY_NOTICES.txt','docs/WPGRAPHICS_EDITOR_THIRD_PARTY_NOTICES.txt',
    'Tools/cpp/ResourcePackageTool/main.cpp','Tools/cpp/ResourcePackageTool/CMakeLists.txt',
    'Engine/cpp/Source/Workphone/System/ResourcePackageProfile.cpp',
    'Engine/cpp/Source/Workphone/System/PackageManager.cpp',
    'Engine/cpp/Source/Workphone/Jobs/ResourcePackageJob.cpp',
    'Engine/cpp/Source/WPLuaBind/Bindings/SystemBind.cpp',
    'bin/Media/Scripts/Lua/Editor/PackageEditor.lua','Tests/python/resource_package_cli.py') | ForEach-Object {
    [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath (Join-Path $repo $_)).Hash }
}
$evidence['packagingInputs'] = @($inputs)
function Invoke-CMake([string]$Name, [string[]]$Arguments) {
    & cmake @Arguments *> (Join-Path $output "$Name.log")
    if ($LASTEXITCODE -ne 0) { throw "CMake $Name failed; see $output/$Name.log" }
}
function Get-Payload([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -Recurse -File | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($Root, $_.FullName).Replace('\','/'); bytes = $_.Length
            sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
}
try {
    if (-not $SkipBuild) {
        Invoke-CMake 'configure' @('-S', $repo, '-B', $build, '-DWP_INSTALL_EDITOR_PACKAGE=ON')
        Invoke-CMake 'build' @('--build', $build, '--config', 'RelWithDebInfo', '--target', 'WorkphoneEditorPackageBinaries', '--parallel', '8')
    }
    $stage = Join-Path $output 'WorkphoneEditor'
    & cmake --install $build --config Debug --component WorkphoneEditor --prefix (Join-Path $output 'rejected-debug') *> (Join-Path $output 'reject-debug.log')
    $checks.Add([ordered]@{ name = 'reject-debug-test-harness'; exitCode = $LASTEXITCODE })
    if ($LASTEXITCODE -eq 0 -or (Test-Path (Join-Path $output 'rejected-debug/bin/Editor.exe'))) { throw 'Debug Editor packaging was not rejected' }
    Invoke-CMake 'install' @('--install', $build, '--config', 'RelWithDebInfo', '--component', 'WorkphoneEditor', '--prefix', $stage)
    $symbols = Join-Path $output 'WorkphoneEditorSymbols'
    Invoke-CMake 'install-symbols' @('--install', $build, '--config', 'RelWithDebInfo', '--component', 'WorkphoneEditorSymbols', '--prefix', $symbols)
    $exe = Join-Path $stage 'bin/Editor.exe'
    $pe = [IO.File]::ReadAllBytes($exe)
    $offset = [BitConverter]::ToInt32($pe, 0x3c)
    if ($pe[0] -ne 0x4d -or $pe[1] -ne 0x5a -or [BitConverter]::ToUInt32($pe,$offset) -ne 0x4550 -or
        [BitConverter]::ToUInt16($pe,$offset+4) -ne 0x8664) { throw 'Editor executable is not Windows x64 PE' }
    $evidence['machine'] = 'AMD64 (0x8664)'
    $plugins = @(Get-Content (Join-Path $stage 'editor-package.cfg') | Where-Object { $_ -like 'Plugin=*' } | ForEach-Object { $_.Substring(7) })
    $evidence['plugins'] = $plugins
    [ordered]@{ schemaVersion = 1; product = 'Workphone C++ Editor'; configuration = 'RelWithDebInfo'
        architecture = 'x64'; graphics = 'Claw'; scripting = 'Lua'; candidate = $true
        releaseCertified = $false; licenseAuditComplete = $false; plugins = $plugins
        requiredRuntime = 'Windows x64 and Microsoft Visual C++ x64 release Redistributable'
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'package-info.json') -Encoding utf8
    $payload = Get-Payload $stage
    $evidence['payload'] = $payload
    $archive = Join-Path $output 'WorkphoneEditor-Windows-x64.zip'
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -CompressionLevel Fastest
    $evidence['archive'] = [ordered]@{ path = $archive; sha256 = (Get-FileHash $archive -Algorithm SHA256).Hash }
    $isolated = Join-Path ([IO.Path]::GetTempPath()) ('wp-editor-package-' + [guid]::NewGuid().ToString('N'))
    $root = Join-Path $isolated 'Editor package with spaces'
    Expand-Archive -LiteralPath $archive -DestinationPath $root
    $evidence['isolatedRoot'] = $root
    foreach ($entry in $payload) {
        $file = Join-Path $root $entry.path
        if ((Get-FileHash -LiteralPath $file).Hash -ne $entry.sha256) { throw "Extracted payload mismatch: $($entry.path)" }
        (Get-Item -LiteralPath $file).IsReadOnly = $true
    }
    function Invoke-Editor([string]$Name, [int]$ExpectedExit, [string]$ExpectedText, [string]$PackageDirectory = $root) {
        $root = $PackageDirectory
        $workspace = Join-Path $isolated "$Name workspace"
        New-Item -ItemType Directory -Path $workspace | Out-Null
        $moduleReport = Join-Path $isolated "$Name-modules.txt"
        $info = [Diagnostics.ProcessStartInfo]::new()
        $info.FileName = Join-Path $root 'bin/Editor.exe'
        $info.WorkingDirectory = $workspace
        $info.UseShellExecute = $false
        $info.CreateNoWindow = $true
        $info.RedirectStandardOutput = $true
        $info.RedirectStandardError = $true
        foreach ($argument in @('--package-smoke','--workspace',$workspace)) { $info.ArgumentList.Add($argument) }
        $info.Environment.Clear()
        foreach ($key in @('SYSTEMROOT','WINDIR')) { $info.Environment[$key] = $env:SystemRoot }
        $info.Environment['PATH'] = (Join-Path $env:SystemRoot 'System32') + ';' + $env:SystemRoot
        foreach ($key in @('TEMP','TMP','LOCALAPPDATA','APPDATA','USERPROFILE')) { $info.Environment[$key] = $workspace }
        $info.Environment['WP_EDITOR_MODULE_REPORT'] = $moduleReport
        $process = [Diagnostics.Process]::Start($info)
        try {
            $stdout = $process.StandardOutput.ReadToEndAsync()
            $stderr = $process.StandardError.ReadToEndAsync()
            if (-not $process.WaitForExit(90000)) { $process.Kill($true); $process.WaitForExit(); throw "Editor timed out: $Name" }
            $text = $stdout.GetAwaiter().GetResult()
            $errors = $stderr.GetAwaiter().GetResult()
            $text | Set-Content (Join-Path $output "$Name-stdout.log")
            $errors | Set-Content (Join-Path $output "$Name-stderr.log")
            $checks.Add([ordered]@{ name = $Name; expectedExit = $ExpectedExit; exitCode = $process.ExitCode })
            if ($process.ExitCode -ne $ExpectedExit -or ($text + $errors) -notlike "*$ExpectedText*") { throw "Editor check failed: $Name ($($process.ExitCode))" }
            if ($ExpectedExit -eq 0) {
                if (($text + $errors) -match 'stack traceback:|No matching overload found') {
                    throw "Editor startup reported a Lua binding/runtime failure: $Name"
                }
                $modules = @(Get-Content -LiteralPath $moduleReport)
                if (-not $modules.Count) { throw 'Empty module inventory' }
                foreach ($module in $modules) {
                    if (-not $module.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase) -and
                        -not $module.StartsWith($env:SystemRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
                        throw "Editor loaded a module outside the package/Windows: $module"
                    }
                }
                foreach ($plugin in $plugins) {
                    if (-not ($modules | Where-Object { [IO.Path]::GetFileName($_) -ieq "$plugin.dll" })) { throw "Configured plugin not loaded: $plugin" }
                }
                Copy-Item -LiteralPath $moduleReport -Destination (Join-Path $output "$Name-loaded-modules.txt")
                $moduleKey = if ($Name -eq 'startup') { 'loadedModules' } else { "$Name-loadedModules" }
                $evidence[$moduleKey] = @($modules | ForEach-Object { [ordered]@{ path = $_; sha256 = (Get-FileHash -LiteralPath $_).Hash } })
            }
        } finally { $process.Dispose() }
    }
    # Run the real checked profile tool from the read-only relocated installation.
    # Its fixture restricts child PATH to Windows system directories, so missing
    # packaged DLLs cannot fall back to the source checkout through PATH.
    $pythonLine = Get-Content (Join-Path $build 'CMakeCache.txt') | Where-Object { $_ -match '^_Python3_EXECUTABLE:INTERNAL=' } | Select-Object -First 1
    if (-not $pythonLine) { throw 'Python interpreter is required for relocated resource package validation' }
    $python = $pythonLine.Substring($pythonLine.IndexOf('=') + 1)
    & $python (Join-Path $repo 'Tests/python/resource_package_cli.py') (Join-Path $root 'bin/WorkphoneResourcePackage.exe') *> (Join-Path $output 'relocated-resource-package.log')
    $checks.Add([ordered]@{ name = 'relocated-resource-package-profiles'; exitCode = $LASTEXITCODE })
    if ($LASTEXITCODE -ne 0) { throw 'Relocated resource package CLI validation failed' }
    Invoke-Editor 'startup' 0 'Editor package startup and teardown passed'
    # Remove a required media input in this task-owned extracted copy, then restore.
    $media = Join-Path $root 'Media/Scripts/Lua/Editor/Editor.lua'
    [IO.File]::Move($media, "$media.hidden")
    try { Invoke-Editor 'missing-media' 1 'Editor package media is missing' }
    finally { [IO.File]::Move("$media.hidden", $media) }
    $profile = Join-Path $root 'editor-package.cfg'
    (Get-Item $profile).IsReadOnly = $false
    $originalProfile = [IO.File]::ReadAllBytes($profile)
    try {
        [IO.File]::AppendAllText($profile, "Plugin=MissingPackagePlugin`n")
        Invoke-Editor 'missing-plugin' 1 'Editor package plugin is missing: MissingPackagePlugin'
    } finally { [IO.File]::WriteAllBytes($profile, $originalProfile); (Get-Item $profile).IsReadOnly = $true }
    $actual = Get-Payload $root
    if (($actual | ConvertTo-Json -Depth 4 -Compress) -cne ($payload | ConvertTo-Json -Depth 4 -Compress)) { throw 'Editor modified the installed payload' }
    # A different drive has no lexical relative path; exercise the common case
    # of a G: installation with a C: per-user workspace when available locally.
    if ([IO.Path]::GetPathRoot($stage) -ine [IO.Path]::GetPathRoot($isolated)) {
        foreach ($entry in $payload) { (Get-Item -LiteralPath (Join-Path $stage $entry.path)).IsReadOnly = $true }
        Invoke-Editor 'cross-volume-startup' 0 'Editor package startup and teardown passed' $stage
        $stageAfter = Get-Payload $stage
        if (($stageAfter | ConvertTo-Json -Depth 4 -Compress) -cne ($payload | ConvertTo-Json -Depth 4 -Compress)) { throw 'Cross-volume Editor modified the installed payload' }
    }
    $symbolArchive = Join-Path $output 'WorkphoneEditorSymbols-Windows-x64.zip'
    Compress-Archive -Path (Join-Path $symbols '*') -DestinationPath $symbolArchive -CompressionLevel Fastest
    $evidence['symbols'] = [ordered]@{ path = $symbolArchive; sha256 = (Get-FileHash $symbolArchive).Hash; payload = (Get-Payload $symbols) }
    foreach ($inputFile in $inputs) {
        if ((Get-FileHash -LiteralPath (Join-Path $repo $inputFile.path)).Hash -ne $inputFile.sha256) { throw "Packaging input changed during validation: $($inputFile.path)" }
    }
    $evidence['validationPassed'] = $true
} catch {
    $evidence['error'] = $_.Exception.Message
    throw
} finally {
    $evidence['checks'] = @($checks.ToArray())
    $evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'editor-package-evidence.json') -Encoding utf8
    Write-Host "Editor package evidence: $output"
}
