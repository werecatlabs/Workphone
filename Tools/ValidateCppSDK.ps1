#requires -Version 7.0
[CmdletBinding()]
param([string]$BuildDirectory = 'project_x64', [switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = if ([IO.Path]::IsPathRooted($BuildDirectory)) { [IO.Path]::GetFullPath($BuildDirectory) } else { [IO.Path]::GetFullPath((Join-Path $repo $BuildDirectory)) }
$run = Join-Path $repo ('project_graphics_sdk/cpp-sdk-validation-' + [guid]::NewGuid().ToString('N'))
$external = Join-Path ([IO.Path]::GetTempPath()) ('Workphone Cpp SDK é ' + [guid]::NewGuid().ToString('N'))
$stage = Join-Path $run 'WorkphoneCppSDK'
$report = [ordered]@{schemaVersion=3;validationPassed=$false;releaseCertified=$false
    scope='Windows x64 C++17 installed Core/Claw typed material/schema/publication/record consumer and catalog-cooked material/texture runtime closure; release DLL CRT'
    buildSkipped=[bool]$SkipBuild;incrementalBuild=(-not [bool]$SkipBuild);externalDirectory=$external}
$report['windowsLongPathsEnabled']=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name LongPathsEnabled -ErrorAction SilentlyContinue
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
function Assert-InputHashes([object[]]$Inputs) {
    foreach ($inputEntry in $Inputs) {
        if ((Get-FileHash -LiteralPath $inputEntry.path).Hash -ne $inputEntry.sha256) {
            throw "Input changed: $($inputEntry.path)"
        }
    }
}
function Invoke-IsolatedTool([string]$Label, [string]$Executable, [string[]]$Arguments, [string]$BinDirectory) {
    $start = [Diagnostics.ProcessStartInfo]::new($Executable)
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.RedirectStandardOutput=$true
    $start.RedirectStandardError=$true
    $start.WorkingDirectory=$external
    foreach($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $start.Environment.Clear()
    $start.Environment['SystemRoot']=$env:SystemRoot
    $start.Environment['PATH']=$BinDirectory+';'+(Join-Path $env:SystemRoot 'System32')
    $start.Environment['TEMP']=$external
    $start.Environment['TMP']=$external
    $process=[Diagnostics.Process]::Start($start)
    $stdout=$process.StandardOutput.ReadToEndAsync()
    $stderr=$process.StandardError.ReadToEndAsync()
    if(-not $process.WaitForExit(60000)) { $process.Kill($true);throw "$Label timed out" }
    $out=$stdout.GetAwaiter().GetResult()
    $err=$stderr.GetAwaiter().GetResult()
    $out | Set-Content (Join-Path $run "$Label.stdout.log")
    $err | Set-Content (Join-Path $run "$Label.stderr.log")
    if($process.ExitCode -ne 0) { throw "$Label failed ($($process.ExitCode)): $err" }
    [ordered]@{executableSha256=(Get-FileHash -LiteralPath $Executable).Hash;exitCode=$process.ExitCode;stdout=$out}
}
try {
    $inputFiles = @(Get-ChildItem (Join-Path $repo 'Engine/cpp/Include/Workphone'),
        (Join-Path $repo 'Engine/cpp/Include/WPGraphics'), (Join-Path $repo 'Engine/cpp/Include/WPSQLite'),
        (Join-Path $repo 'Engine/cpp/Source/Workphone'),
        (Join-Path $repo 'Engine/cpp/Source/WPGraphics'),
        (Join-Path $repo 'Engine/c/Include'), (Join-Path $repo 'Engine/c/Source'),
        (Join-Path $repo 'Engine/cpp/Source/WPSQLite'), (Join-Path $repo 'Tests/CppSDKConsumer') -Recurse -File)
    $inputFiles += @('CMakeLists.txt','CMake/WorkphoneWindows.manifest','CMake/InstallWorkphoneCppSDK.cmake','CMake/WorkphoneCppSDKConfig.cmake.in',
        'CMake/InstallWorkphoneEditor.cmake','CMake/InstallWorkphoneEditorNotices.cmake','Tools/ValidateCppSDK.ps1','Tools/ValidateCookedCppContent.ps1',
        'docs/WPGRAPHICS_CPP_SDK.md','docs/WPGRAPHICS_EDITOR_PACKAGE.md',
        'Engine/cpp/Source/Workphone/System/ResourceSystem.cpp','Engine/cpp/Source/Workphone/System/CompiledResource.cpp',
        'Engine/cpp/Source/Workphone/System/ResourceCompilerRegistry.cpp',
        'Engine/cpp/Source/Workphone/Database/AssetDatabaseManager.cpp','Engine/cpp/Source/Workphone/Database/CatalogResourceAdapter.cpp',
        'Engine/cpp/Source/Workphone/Database/AssetPackagePolicy.cpp','Engine/cpp/Project/Workphone/CMakeLists.txt',
        'Engine/cpp/Project/WPGraphics/CMakeLists.txt','Engine/cpp/Project/WPSQLite/CMakeLists.txt') |
        ForEach-Object { Get-Item -LiteralPath (Join-Path $repo $_) }
    $noticeEntries=[regex]::Matches((Get-Content (Join-Path $repo 'CMake/InstallWorkphoneEditorNotices.cmake') -Raw),'"(Dependencies/[^"|]+)\|([^"|]+)"')
    $inputFiles += @($noticeEntries | ForEach-Object { Get-Item -LiteralPath (Join-Path $repo $_.Groups[1].Value) })
    $report['inputs'] = @($inputFiles | Sort-Object FullName -Unique | ForEach-Object {
        [ordered]@{path=$_.FullName;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash}
    })
    if (-not $SkipBuild) {
        Invoke-Checked 'configure' @('-S',$repo,'-B',$build,'-DWP_INSTALL_EDITOR_PACKAGE=ON')
        Invoke-Checked 'build' @('--build',$build,'--config','RelWithDebInfo','--target','WorkphoneEditorPackageBinaries','--parallel','8')
    }
    # Reject a concurrent source edit before spending time packaging a binary
    # whose inputs no longer match this run. Check again after all consumers.
    Assert-InputHashes $report.inputs
    Invoke-Checked 'install-runtime' @('--install',$build,'--config','RelWithDebInfo','--component','WorkphoneEditor','--prefix',$stage)
    Invoke-Checked 'install-sdk' @('--install',$build,'--config','RelWithDebInfo','--component','WorkphoneCppSDK','--prefix',$stage)
    $report['dependencyNotices']=@($noticeEntries | ForEach-Object {
        $source=$_.Groups[1].Value; $name='licenses/'+$_.Groups[2].Value
        $hash=(Get-FileHash -LiteralPath (Join-Path $repo $source)).Hash
        if($hash -ne (Get-FileHash -LiteralPath (Join-Path $stage $name)).Hash) { throw "Dependency notice differs: $name" }
        [ordered]@{source=$source;path=$name;sha256=$hash}
    })
    & cmake --install $build --config Debug --component WorkphoneCppSDK --prefix (Join-Path $run 'rejected-debug') *> (Join-Path $run 'reject-debug-install.log')
    if ($LASTEXITCODE -eq 0 -or (Get-Content (Join-Path $run 'reject-debug-install.log') -Raw) -notlike '*SDK requires RelWithDebInfo*') { throw 'Debug SDK install was not rejected for the expected reason' }
    $report['debugInstallRejected'] = $true
    $buildMatches = foreach($line in Get-Content (Join-Path $build 'cpp-sdk/RelWithDebInfo/build-artifacts.txt')) {
        $relative,$binary = $line -split '\|',2
        $hash=(Get-FileHash -LiteralPath $binary).Hash
        if($hash -ne (Get-FileHash -LiteralPath (Join-Path $stage $relative)).Hash) { throw "Installed artifact differs from build: $relative" }
        [ordered]@{path=$relative;buildPath=$binary;sha256=$hash}
    }
    $report['buildArtifacts']=$buildMatches
    # Build the authoring example through the installed targets, then export only
    # its checked cooked closure into the distribution. Authoring state is private.
    $bootstrap=Join-Path $external 'authoring-build'
    $content=Join-Path $stage 'share/WorkphoneCppSDK/content'
    Invoke-Checked 'cooker-configure' @('-S',(Join-Path $stage 'share/WorkphoneCppSDK/sample'),'-B',$bootstrap,
        '-A','x64',"-DCMAKE_PREFIX_PATH=$stage",'-DCPP_SDK_BUILD_COOKER=ON')
    Invoke-Checked 'cooker-build' @('--build',$bootstrap,'--config','RelWithDebInfo','--parallel','4')
    $authoring=Join-Path $run 'sample-authoring'
    $cooking=Invoke-IsolatedTool 'cook-sample' (Join-Path $bootstrap 'RelWithDebInfo/cpp-sdk-cook-sample.exe') @($content,$authoring) (Join-Path $stage 'bin')
    if($cooking.stdout -notlike '*catalogAdmission=passed*') { throw 'Cooker did not confirm checked catalog admission' }
    $contentFiles=Get-Payload $content
    if($contentFiles.Count -ne 3 -or @($contentFiles | Where-Object { $_.path -notmatch '\.(wprs|wprm)$' }).Count) { throw 'Unexpected files in cooked runtime sample' }
    # Delete only this run's generated source/catalog/cooking scratch after the
    # producer exits. No source root or authoring database remains for runtime use.
    $ownedRun=[IO.Path]::GetFullPath($run).TrimEnd('\')+'\'
    if(-not [IO.Path]::GetFullPath($authoring).StartsWith($ownedRun,[StringComparison]::OrdinalIgnoreCase)) { throw 'Authoring cleanup escapes owned run' }
    Remove-Item -LiteralPath $authoring -Recurse -Force
    if(Test-Path -LiteralPath $authoring) { throw 'Private authoring inputs remain' }
    $report['cookedContent']=[ordered]@{files=$contentFiles;catalogAdmissionPassed=$true;authoringInputsRemoved=$true
        materialUUID='2a6d7957-45ab-4bbb-9d96-91f7a9c1a001';textureUUID='2a6d7957-45ab-4bbb-9d96-91f7a9c1a002';
        target='installed-cpp-sample';producer=$cooking}
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
    $extractedContent=Join-Path $extracted 'share/WorkphoneCppSDK/content'
    Invoke-Checked 'consumer-configure' @('-S',$sample,'-B',$consumerBuild,'-A','x64',"-DCMAKE_PREFIX_PATH=$extracted", "-DCPP_SDK_CONTENT_DIRECTORY=$extractedContent")
    & cmake -S $sample -B (Join-Path $external 'rejected-debug-consumer') -A x64 "-DCMAKE_PREFIX_PATH=$extracted" '-DCMAKE_CONFIGURATION_TYPES=Debug' *> (Join-Path $run 'reject-debug-consumer.log')
    if ($LASTEXITCODE -eq 0 -or (Get-Content (Join-Path $run 'reject-debug-consumer.log') -Raw) -notlike '*contains release CRT DLLs*') { throw 'Debug consumer was not rejected for the expected reason' }
    $report['debugConsumerRejected'] = $true
    & cmake -S $sample -B (Join-Path $external 'rejected-static-crt') -A x64 "-DCMAKE_PREFIX_PATH=$extracted" '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded' *> (Join-Path $run 'reject-static-crt.log')
    if ($LASTEXITCODE -eq 0 -or (Get-Content (Join-Path $run 'reject-static-crt.log') -Raw) -notlike '*requires CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL*') { throw 'Static CRT consumer was not rejected for the expected reason' }
    $report['staticCrtConsumerRejected'] = $true
    $results = [Collections.Generic.List[object]]::new()
    $cookedResults = [Collections.Generic.List[object]]::new()
    foreach ($config in @('RelWithDebInfo','Release')) {
        Invoke-Checked "consumer-build-$config" @('--build',$consumerBuild,'--config',$config,'--parallel','4')
        $exe = Join-Path $consumerBuild "$config/cpp-sdk-consumer.exe"
        $start = [Diagnostics.ProcessStartInfo]::new($exe)
        $start.UseShellExecute=$false
        $start.CreateNoWindow=$true
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
        if ($out -notlike '*material layouts/typed handles/schema migration, immutable publication, backend record generations*') {
            throw "Installed material API contract evidence missing: $config"
        }
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
        $results.Add([ordered]@{configuration=$config;exitCode=$process.ExitCode;executableSha256=(Get-FileHash $exe).Hash;
            materialContractsPassed=$true;loadedModules=$modules})
        & ctest --test-dir $consumerBuild -C $config --output-on-failure --output-junit (Join-Path $run "consumer-$config.xml") *> (Join-Path $run "consumer-$config.log")
        if ($LASTEXITCODE -ne 0) { throw "Consumer CTest failed: $config" }
        $cookedResult = & (Join-Path $PSScriptRoot 'ValidateCookedCppContent.ps1') -Executable (Join-Path $consumerBuild "$config/cpp-sdk-cooked-consumer.exe") `
            -ContentDirectory $extractedContent -SdkRoot $extracted -EvidenceDirectory (Join-Path $run "cooked-$config") -Configuration $config
        $cookedResults.Add($cookedResult)
    }
    $report['consumerResults']=$results.ToArray()
    $report['cookedConsumerResults']=$cookedResults.ToArray()
    Assert-InputHashes $report.inputs
    $after = Get-Payload $extracted
    if (($payload | ConvertTo-Json -Depth 4 -Compress) -ne ($after | ConvertTo-Json -Depth 4 -Compress)) { throw 'SDK payload changed during validation' }
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
