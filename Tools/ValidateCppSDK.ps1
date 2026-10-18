#requires -Version 7.0
[CmdletBinding()]
param([string]$BuildDirectory = 'project_x64', [switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = [IO.Path]::GetFullPath((Join-Path $repo $BuildDirectory))
if ([IO.Path]::IsPathRooted($BuildDirectory)) { $build = $BuildDirectory }
$run = Join-Path $repo ('project_graphics_sdk/cpp-sdk-validation-' + [guid]::NewGuid().ToString('N'))
$external = Join-Path ([IO.Path]::GetTempPath()) ('Workphone Cpp SDK ' + [guid]::NewGuid().ToString('N'))
$stage = Join-Path $run 'WorkphoneCppSDK'
$report = [ordered]@{schemaVersion=1;validationPassed=$false;releaseCertified=$false
    scope='Windows x64 C++17 shared Core/Claw installed consumer; release DLL CRT'
    buildSkipped=[bool]$SkipBuild;incrementalBuild=$true;externalDirectory=$external}
New-Item -ItemType Directory -Path $run,$external | Out-Null
function Invoke-Checked([string]$Label, [string[]]$Arguments) {
    & cmake @Arguments *> (Join-Path $run "$Label.log")
    if ($LASTEXITCODE -ne 0) {
        Get-Content (Join-Path $run "$Label.log") -Tail 30 | Write-Host
        throw "$Label failed ($LASTEXITCODE)"
    }
}
function Get-Payload([string]$Root) {
    @(Get-ChildItem -LiteralPath $Root -File -Recurse | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=[IO.Path]::GetRelativePath($Root,$_.FullName).Replace('\','/');
            bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
    })
}
try {
    $inputFiles = @(Get-ChildItem (Join-Path $repo 'Engine/cpp/Include/Workphone'),
        (Join-Path $repo 'Engine/cpp/Include/WPGraphics'), (Join-Path $repo 'Tests/CppSDKConsumer') -Recurse -File)
    $inputFiles += @('CMake/InstallWorkphoneCppSDK.cmake','CMake/WorkphoneCppSDKConfig.cmake.in',
        'CMake/InstallWorkphoneEditor.cmake','Tools/ValidateCppSDK.ps1','docs/WPGRAPHICS_CPP_SDK.md') |
        ForEach-Object { Get-Item -LiteralPath (Join-Path $repo $_) }
    $report['inputs'] = @($inputFiles | Sort-Object FullName -Unique | ForEach-Object {
        [ordered]@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
    })
    if (-not $SkipBuild) {
        Invoke-Checked 'configure' @('-S',$repo,'-B',$build,'-DWP_INSTALL_EDITOR_PACKAGE=ON')
        Invoke-Checked 'build' @('--build',$build,'--config','RelWithDebInfo','--target','WorkphoneEditorPackageBinaries','--parallel','8')
    }
    Invoke-Checked 'install-runtime' @('--install',$build,'--config','RelWithDebInfo','--component','WorkphoneEditor','--prefix',$stage)
    Invoke-Checked 'install-sdk' @('--install',$build,'--config','RelWithDebInfo','--component','WorkphoneCppSDK','--prefix',$stage)
    & cmake --install $build --config Debug --component WorkphoneCppSDK --prefix (Join-Path $run 'rejected-debug') *> (Join-Path $run 'reject-debug-install.log')
    if ($LASTEXITCODE -eq 0) { throw 'Debug SDK install was not rejected' }
    $report['debugInstallRejected'] = $true
    $payload = Get-Payload $stage
    $report['payload'] = $payload
    $archive = Join-Path $run 'WorkphoneCppSDK-Windows-x64.zip'
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($stage,$archive,[IO.Compression.CompressionLevel]::Fastest,$false)
    $report['archive'] = [ordered]@{path=$archive;sha256=(Get-FileHash -LiteralPath $archive).Hash}
    $extracted = Join-Path $external 'extracted SDK'
    [IO.Compression.ZipFile]::ExtractToDirectory($archive,$extracted)
    $extractedPayload = Get-Payload $extracted
    if (($payload | ConvertTo-Json -Depth 4 -Compress) -ne ($extractedPayload | ConvertTo-Json -Depth 4 -Compress)) {
        throw 'Extracted payload differs from the installation'
    }
    $sample = Join-Path $external 'external sample'
    Copy-Item -LiteralPath (Join-Path $extracted 'share/WorkphoneCppSDK/sample') -Destination $sample -Recurse
    Get-ChildItem -LiteralPath $extracted -File -Recurse | ForEach-Object { $_.IsReadOnly=$true }
    $consumerBuild = Join-Path $external 'consumer-build'
    Invoke-Checked 'consumer-configure' @('-S',$sample,'-B',$consumerBuild,'-A','x64',"-DCMAKE_PREFIX_PATH=$extracted")
    & cmake -S $sample -B (Join-Path $external 'rejected-debug-consumer') -A x64 "-DCMAKE_PREFIX_PATH=$extracted" '-DCMAKE_CONFIGURATION_TYPES=Debug' *> (Join-Path $run 'reject-debug-consumer.log')
    if ($LASTEXITCODE -eq 0) { throw 'Debug consumer was not rejected' }
    $report['debugConsumerRejected'] = $true
    & cmake -S $sample -B (Join-Path $external 'rejected-static-crt') -A x64 "-DCMAKE_PREFIX_PATH=$extracted" '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded' *> (Join-Path $run 'reject-static-crt.log')
    if ($LASTEXITCODE -eq 0) { throw 'Static CRT consumer was not rejected' }
    $report['staticCrtConsumerRejected'] = $true
    $results = [Collections.Generic.List[object]]::new()
    foreach ($config in @('RelWithDebInfo','Release')) {
        Invoke-Checked "consumer-build-$config" @('--build',$consumerBuild,'--config',$config,'--parallel','4')
        $exe = Join-Path $consumerBuild "$config/cpp-sdk-consumer.exe"
        $start = [Diagnostics.ProcessStartInfo]::new($exe)
        $start.UseShellExecute=$false
        $start.RedirectStandardOutput=$true
        $start.RedirectStandardError=$true
        $start.WorkingDirectory=$external
        $start.Environment.Clear()
        $start.Environment['SystemRoot']=$env:SystemRoot
        $start.Environment['PATH']=(Join-Path $extracted 'bin') + ';' + (Join-Path $env:SystemRoot 'System32')
        $start.Environment['TEMP']=$external
        $start.Environment['TMP']=$external
        $process=[Diagnostics.Process]::Start($start)
        $stdout=$process.StandardOutput.ReadToEndAsync()
        $stderr=$process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(60000)) { $process.Kill($true);throw "Consumer timed out: $config" }
        $out=$stdout.GetAwaiter().GetResult()
        $err=$stderr.GetAwaiter().GetResult()
        $out | Set-Content (Join-Path $run "consumer-$config.stdout.log")
        $err | Set-Content (Join-Path $run "consumer-$config.stderr.log")
        if ($process.ExitCode -ne 0 -or $out -notlike '*Installed C++ SDK:*passed*') { throw "Consumer failed: $config ($($process.ExitCode))" }
        $modules = @($out -split '\r?\n' | Where-Object { $_.StartsWith('module=') } | ForEach-Object { $_.Substring(7).Trim() })
        foreach ($module in $modules) {
            if ($module -ieq $exe) { continue }
            if (-not $module.StartsWith($extracted.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -and
                -not $module.StartsWith($env:SystemRoot.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
                throw "Unexpected consumer module: $module"
            }
        }
        foreach ($required in @('Workphone.dll','WPGraphics.dll')) {
            if (-not ($modules | Where-Object { [IO.Path]::GetFileName($_) -ieq $required })) { throw "Required DLL not observed: $required" }
        }
        $results.Add([ordered]@{configuration=$config;exitCode=$process.ExitCode;executableSha256=(Get-FileHash $exe).Hash;loadedModules=$modules})
        & ctest --test-dir $consumerBuild -C $config --output-on-failure --output-junit (Join-Path $run "consumer-$config.xml") *> (Join-Path $run "consumer-$config.log")
        if ($LASTEXITCODE -ne 0) { throw "Consumer CTest failed: $config" }
    }
    foreach ($inputEntry in $report.inputs) {
        if ((Get-FileHash -LiteralPath $inputEntry.path).Hash -ne $inputEntry.sha256) { throw "Input changed: $($inputEntry.path)" }
    }
    $after = Get-Payload $extracted
    if (($payload | ConvertTo-Json -Depth 4 -Compress) -ne ($after | ConvertTo-Json -Depth 4 -Compress)) { throw 'SDK payload changed during validation' }
    $report['consumerResults']=@($results)
    $report['readOnlyPayloadUnchanged']=$true
    $report['sourceInputsUnchanged']=$true
    $report['validationPassed']=$true
} catch {
    $report['error']=$_.Exception.Message
    throw
} finally {
    $report | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $run 'cpp-sdk-evidence.json') -Encoding UTF8
    Write-Host "C++ SDK evidence: $run/cpp-sdk-evidence.json"
}
