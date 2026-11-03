#requires -Version 7.0
[CmdletBinding()]
param(
    [string]$BuildDirectory = 'project_x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release')][string]$Configuration = 'RelWithDebInfo',
    [string]$BinaryDirectory,
    [string]$OutputDirectory,
    # This watchdog belongs to the relocated gameplay package diagnostic. It
    # does not replace the existing road/city or GT0 performance gate limits.
    [ValidateRange(30, 900)][int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'TerrainValidationEvidence.ps1')
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) { [IO.Path]::GetFullPath($BuildDirectory) } else { [IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory)) }
$cmake = Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
if (-not (Test-Path -LiteralPath $cmake -PathType Leaf)) { throw "CMake unavailable: $cmake" }
if ($BinaryDirectory) {
    $binaryRoot = (Resolve-Path -LiteralPath $BinaryDirectory).Path
} else {
    $project = Join-Path $buildPath 'Samples/cpp/VehicleAdvanced/SampleVehicleAdvanced.vcxproj'
    if (-not (Test-Path -LiteralPath $project -PathType Leaf)) { throw 'Configure the VehicleAdvanced sample or supply -BinaryDirectory.' }
    [xml]$projectXml = Get-Content -LiteralPath $project -Raw
    $outDirs = @($projectXml.SelectNodes('//*[local-name()="OutDir"]') | Where-Object {
        $_.Condition -match "==\s*'$([regex]::Escape($Configuration))\|[^']+'"
    } | ForEach-Object { $_.InnerText } | Sort-Object -Unique)
    if ($outDirs.Count -ne 1 -or $outDirs[0] -match '\$\(') { throw 'Cannot identify one concrete sample output directory; supply -BinaryDirectory.' }
    $binaryRoot = [IO.Path]::GetFullPath($outDirs[0])
}
$executable = Join-Path $binaryRoot 'SampleVehicleAdvanced.exe'
$authorPlugins = Join-Path $binaryRoot 'wp_plugins_samples.cfg'
$pluginNames = @('WPGraphics', 'WPPhysics', 'WPVehiclePhysics', 'WPProcedural')
$pluginRoots = @($pluginNames | ForEach-Object { Join-Path $binaryRoot ($_ + '.dll') })
foreach ($required in @($executable, $authorPlugins) + $pluginRoots) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) { throw "Build SampleVehicleAdvanced and its plugin targets first: $required" }
}
$pluginFolders = @(Get-Content -LiteralPath $authorPlugins | Where-Object { $_ -match '^\s*PluginFolder\s*=' })
if ($pluginFolders.Count -ne 1) { throw 'Author configuration must specify exactly one PluginFolder.' }
$authorPluginFolder = ($pluginFolders[0] -split '=', 2)[1].Trim()
if (-not [IO.Path]::IsPathRooted($authorPluginFolder)) { $authorPluginFolder = Join-Path $binaryRoot $authorPluginFolder }
if ([IO.Path]::GetFullPath($authorPluginFolder).TrimEnd('\', '/') -ine $binaryRoot.TrimEnd('\', '/')) {
    throw 'Author PluginFolder must resolve to BinaryDirectory so package validation cannot mix plugin builds.'
}
$compilerFiles = @(Get-ChildItem -Path (Join-Path $buildPath 'CMakeFiles/*/CMakeCXXCompiler.cmake'))
if (-not $compilerFiles.Count) { throw 'Configured compiler evidence is missing.' }
$compilerText = Get-Content -LiteralPath ($compilerFiles | Select-Object -Last 1).FullName -Raw
if ($compilerText -notmatch 'set\(CMAKE_CXX_COMPILER "([^"]+)"\)') { throw 'Compiler path missing from CMake evidence.' }
$dumpbin = Join-Path (Split-Path $Matches[1] -Parent) 'dumpbin.exe'
if (-not (Test-Path -LiteralPath $dumpbin -PathType Leaf)) { throw "PE dependency resolver unavailable: $dumpbin" }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $repositoryRoot ('artifacts/terrain/gameplay-package-' + [guid]::NewGuid().ToString('N')) }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Choose a new output directory to preserve earlier evidence: $output" }
$authoring = Join-Path $output 'authoring-assets'
$install = Join-Path $output 'relocated-install'
$bin = Join-Path $install 'bin'
$assets = Join-Path $install 'assets'
New-Item -ItemType Directory -Path $output, $bin -Force | Out-Null
$runs = [Collections.Generic.List[object]]::new()
$runtimeInputs = [Collections.Generic.List[object]]::new()
$captures = [Collections.Generic.List[object]]::new()

function Get-GameplaySourceSnapshot {
    $snapshot = Get-TerrainSourceSnapshot $repositoryRoot
    $extraPaths = @(& git -C $repositoryRoot ls-files --cached --others --exclude-standard -- 'Samples/cpp/VehicleAdvanced' 'Tools/ValidateTerrainGameplayPackage.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Cannot inventory gameplay sample source inputs.' }
    $extraPaths = @($extraPaths | Sort-Object -Unique)
    $identities = @($extraPaths | & git -C $repositoryRoot hash-object --no-filters --stdin-paths)
    if ($LASTEXITCODE -ne 0 -or $identities.Count -ne $extraPaths.Count) { throw 'Cannot hash gameplay sample source inputs.' }
    for ($index = 0; $index -lt $extraPaths.Count; ++$index) { $snapshot.files[$extraPaths[$index]] = $identities[$index] }
    $text = ($snapshot.files.GetEnumerator() | Sort-Object Key | ForEach-Object { $_.Value + ' ' + $_.Key }) -join "`n"
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $snapshot.sha256 = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))).Replace('-', '') }
    finally { $sha.Dispose() }
    return $snapshot
}

function Get-FileInventory([string]$Root) {
    if (-not (Test-Path -LiteralPath $Root -PathType Container)) { return @() }
    $items = @(Get-ChildItem -LiteralPath $Root -Recurse -Force)
    if (@($items | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count) {
        throw "Package tree contains a reparse point: $Root"
    }
    @($items | Where-Object { -not $_.PSIsContainer } | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($Root, $_.FullName); bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
}

function Assert-InventoryUnchanged($Before, [string]$Root, [string]$Context) {
    $after = @(Get-FileInventory $Root)
    if (($Before | ConvertTo-Json -Depth 4 -Compress) -cne ($after | ConvertTo-Json -Depth 4 -Compress)) {
        throw "$Context changed the file inventory or bytes: $Root"
    }
}

function Invoke-GameplayProcess([string]$Program, [string[]]$Arguments, [string]$WorkingDirectory, [string]$Name, [bool]$ExpectedSuccess, [bool]$OwnedDirectory = $true) {
    # Application.log is a native application output. Keep it outside the
    # immutable install/archive rather than accidentally shipping diagnostics.
    $nativeLog = Join-Path $WorkingDirectory 'Application.log'
    $nativeLogBefore = if (Test-Path -LiteralPath $nativeLog -PathType Leaf) { (Get-FileHash -LiteralPath $nativeLog -Algorithm SHA256).Hash } else { $null }
    if ($OwnedDirectory -and $nativeLogBefore) { throw "Previous native log was not collected: $nativeLog" }
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $Program
    $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $info.Environment['PATH'] = "$(Split-Path $Program -Parent);$env:SystemRoot\System32;$env:SystemRoot"
    $process = $null; $stdout = $null; $stderr = $null; $exitCode = $null
    $timedOut = $false; $processError = $null; $log = ''; $nativeLogCollected = $false
    $elapsed = [Diagnostics.Stopwatch]::StartNew()
    try {
        $process = [Diagnostics.Process]::Start($info)
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
            $timedOut = $true
            throw "$Name exceeded the $TimeoutSeconds-second gameplay package diagnostic watchdog."
        }
        $exitCode = $process.ExitCode
    } catch { $processError = $_.Exception.Message; throw } finally {
        if ($process) {
            try {
                if (-not $process.HasExited) {
                    $process.Kill($true)
                    if (-not $process.WaitForExit(5000)) { $log += "Owned process tree did not terminate within five seconds.`n" }
                }
                if ($process.HasExited) { $exitCode = $process.ExitCode }
            } catch { $log += "Process cleanup failed: $($_.Exception.Message)`n" }
        }
        foreach ($read in @($stdout, $stderr)) {
            if ($read) {
                try {
                    if ($read.Wait(5000)) { $log += $read.GetAwaiter().GetResult() }
                    else { $log += "Output pipe did not close within five seconds.`n" }
                } catch { $log += "Output capture failed: $($_.Exception.Message)`n" }
            }
        }
        if (Test-Path -LiteralPath $nativeLog -PathType Leaf) {
            try {
                $nativeHash = (Get-FileHash -LiteralPath $nativeLog -Algorithm SHA256).Hash
                if ($OwnedDirectory -or $nativeHash -ne $nativeLogBefore) {
                    $nativeOutput = Join-Path $output "$Name.application.log"
                    if ($OwnedDirectory) { Move-Item -LiteralPath $nativeLog -Destination $nativeOutput }
                    else { Copy-Item -LiteralPath $nativeLog -Destination $nativeOutput }
                    $log += "`nNative Application.log:`n" + (Get-Content -LiteralPath $nativeOutput -Raw)
                    $nativeLogCollected = $true
                }
            } catch { $log += "Native log collection failed: $($_.Exception.Message)`n" }
        }
        if ($processError) { $log += "`nGameplay package process error: $processError`n" }
        $elapsed.Stop()
        try {
            $log | Set-Content -LiteralPath (Join-Path $output "$Name.log") -Encoding utf8
            $runs.Add([ordered]@{ name = $Name; program = $Program; arguments = $Arguments; workingDirectory = $WorkingDirectory
                exitCode = $exitCode; expectedSuccess = $ExpectedSuccess; timedOut = $timedOut; watchdogSeconds = $TimeoutSeconds
                seconds = $elapsed.Elapsed.TotalSeconds; error = $processError; nativeLogCollected = $nativeLogCollected; output = $log })
        } finally { if ($process) { $process.Dispose() } }
    }
    if (($exitCode -eq 0) -ne $ExpectedSuccess) { throw "$Name returned $exitCode. See $output/$Name.log" }
    return $log
}

function Assert-GameplayLog([string]$Log, [bool]$Smoke = $false, [bool]$RequireSurfaces = $true) {
    if ($Log -notmatch 'Terrain gameplay recipe: PASS hash=([0-9a-fA-F]+)') { throw 'Gameplay process did not record pinned vehicle recipe validation.' }
    $recipeHash = $Matches[1]
    if ($script:expectedRecipeHash -and $recipeHash -cne $script:expectedRecipeHash) { throw 'Packaged gameplay recipe hash differs from its authoring run.' }
    if (-not $script:expectedRecipeHash) { $script:expectedRecipeHash = $recipeHash }
    if ($RequireSurfaces -and $Log -notmatch 'Terrain gameplay surfaces: PASS') { throw 'Gameplay process did not record terrain surface validation.' }
    if ($Smoke) {
        foreach ($phase in 0..4) {
            if ($Log -notmatch "Vehicle smoke phase ${phase}: PASS") { throw "Gameplay driving did not pass phase $phase." }
        }
    }
}

$common = @('--no-audio', '--no-effects', '--no-hud', '--resolution', '640x360')
$passed = $false; $failure = $null; $archive = $null; $archiveHash = $null; $archiveVerified = $false
$sourceBefore = $null; $sourceAfter = $null; $changedSources = @(); $assetInventory = @(); $assetFilesUnchangedVerified = $false
$runtimeInputsUnchangedVerified = $false; $consumerAuthorServicesAbsentVerified = $false; $expectedRecipeHash = $null
$initialBinaryHash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
$initialPluginsHash = (Get-FileHash -LiteralPath $authorPlugins -Algorithm SHA256).Hash
try {
    $sourceBefore = Get-GameplaySourceSnapshot
    $sourceBefore | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'source-before.json') -Encoding utf8
    $discovery = Join-Path $output 'dependency-discovery'
    New-Item -ItemType Directory -Path $discovery | Out-Null
    $runtimeScript = Join-Path $PSScriptRoot 'TerrainPackageRuntime.cmake'
    $rootLibraries = $pluginRoots -join ';'
    & $cmake "-DEXECUTABLE=$executable" "-DROOT_LIBRARIES=$rootLibraries" "-DOUTPUT_DIRECTORY=$discovery" "-DDUMPBIN=$dumpbin" -P $runtimeScript *> (Join-Path $output 'dependency-discovery.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime/plugin dependency discovery failed; see dependency-discovery.log.' }
    $dependencyPaths = @((Get-Content -LiteralPath (Join-Path $discovery 'runtime-dependencies.txt') -Raw) -split '[;\r\n]+' |
        Where-Object { $_ } | ForEach-Object { [IO.Path]::GetFullPath($_) } | Sort-Object -Unique)
    foreach ($rootLibrary in $pluginRoots) {
        if ($dependencyPaths -notcontains [IO.Path]::GetFullPath($rootLibrary)) { throw "Dynamic plugin root missing from dependency closure: $rootLibrary" }
    }
    $duplicateNames = @($dependencyPaths | Group-Object { Split-Path $_ -Leaf } | Where-Object Count -gt 1)
    if ($duplicateNames.Count) { throw 'Dependency closure contains colliding DLL names; reconcile before packaging.' }
    foreach ($dependency in @($executable) + $dependencyPaths) {
        $hash = (Get-FileHash -LiteralPath $dependency -Algorithm SHA256).Hash
        if ($dependency -eq $executable) {
            if ($hash -ne $initialBinaryHash) { throw 'Build executable changed before dependency collection.' }
        } elseif ((Get-FileHash -LiteralPath (Join-Path $discovery (Split-Path $dependency -Leaf)) -Algorithm SHA256).Hash -ne $hash) {
            throw "Runtime dependency changed during discovery: $dependency"
        }
        $runtimeInputs.Add([ordered]@{ source = $dependency; fileName = (Split-Path $dependency -Leaf)
            sha256Before = $hash; sha256Collected = $null; sha256After = $null })
    }
    Write-Host 'Authoring the representative terrain/vehicle runtime package.'
    $log = Invoke-GameplayProcess $executable (@('--terrain-gameplay', '--export-terrain-gameplay', $authoring, '--plugins', $authorPlugins, '--quality', 'low') + $common) $binaryRoot 'author' $true $false
    Assert-GameplayLog $log $false $false
    if (-not (Test-Path -LiteralPath (Join-Path $authoring 'runtime.wprm') -PathType Leaf)) { throw 'Gameplay exporter did not produce runtime.wprm.' }
    Copy-Item -LiteralPath $authoring -Destination $assets -Recurse
    $assetInventory = @(Get-FileInventory $assets)
    if ($assetInventory.Count -lt 3 -or @($assetInventory | Where-Object { [IO.Path]::GetExtension($_.path) -notin '.wprm', '.wprs' }).Count) {
        throw 'Gameplay assets must contain only the cooked runtime manifest and compiled resource containers.'
    }
    Copy-Item -LiteralPath $executable -Destination $bin
    & $cmake "-DEXECUTABLE=$executable" "-DROOT_LIBRARIES=$rootLibraries" "-DOUTPUT_DIRECTORY=$bin" "-DDUMPBIN=$dumpbin" -P $runtimeScript *> (Join-Path $output 'dependencies.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime/plugin dependency collection failed; see dependencies.log.' }
    $collectedPaths = @((Get-Content -LiteralPath (Join-Path $bin 'runtime-dependencies.txt') -Raw) -split '[;\r\n]+' |
        Where-Object { $_ } | ForEach-Object { [IO.Path]::GetFullPath($_) } | Sort-Object -Unique)
    if (($dependencyPaths -join "`n") -cne ($collectedPaths -join "`n")) { throw 'Runtime/plugin dependency closure changed during package collection.' }
    foreach ($inputFile in $runtimeInputs) {
        $inputFile.sha256Collected = (Get-FileHash -LiteralPath (Join-Path $bin $inputFile.fileName) -Algorithm SHA256).Hash
        if ($inputFile.sha256Collected -ne $inputFile.sha256Before -or (Get-FileHash -LiteralPath $inputFile.source -Algorithm SHA256).Hash -ne $inputFile.sha256Before) {
            throw "Runtime binary changed during package collection: $($inputFile.source)"
        }
    }
    Move-Item -LiteralPath (Join-Path $bin 'runtime-dependencies.txt') -Destination (Join-Path $output 'runtime-dependencies.txt')
    $plugins = Join-Path $bin 'wp_plugins_terrain_gameplay.cfg'
    (@('PluginFolder=.') + @($pluginNames | ForEach-Object { 'Plugin=' + $_ })) | Set-Content -LiteralPath $plugins -Encoding utf8
    @'
@echo off
cd /d "%~dp0bin"
"SampleVehicleAdvanced.exe" --terrain-gameplay-package "%~dp0assets" --plugins "%~dp0bin\wp_plugins_terrain_gameplay.cfg" --no-audio --no-effects %*
exit /b %errorlevel%
'@ | Set-Content -LiteralPath (Join-Path $install 'RunTerrainGameplay.cmd') -Encoding ascii
    @'
Workphone terrain gameplay diagnostic package

Run RunTerrainGameplay.cmd to open the packaged terrain and vehicle scene.
The package pins its seed, quality and vehicle recipe; do not override them.
The launcher disables audio and effects. Optional arguments such as --smoke-test
are forwarded to the sample. Captures and smoke tests require separate runs.

This archive was tested by relocating and extracting it on the build machine.
It does not certify a clean-machine installation, hardware performance, image
quality, or interactive Editor authoring. OS/CRT prerequisites remain external.
The vehicle is regenerated from a compiled recipe and checked against its pinned
content hash. Assets contain compiled resource containers and a runtime manifest.
See the validation evidence retained alongside the archive for actual adapters,
process results, runtime/source hashes, captures, and outstanding validation gates.
'@ | Set-Content -LiteralPath (Join-Path $install 'README.txt') -Encoding utf8
    # Preserve the established twelve original notice texts. Their inclusion is
    # not a claim that the overall dependency license audit has been completed.
    $noticeInstaller = Join-Path $repositoryRoot 'CMake/InstallWorkphoneEditorNotices.cmake'
    $noticeEntries = [regex]::Matches((Get-Content -LiteralPath $noticeInstaller -Raw), '"(Dependencies/[^"|]+)\|([^"|]+)"')
    if ($noticeEntries.Count -ne 12) { throw 'Established dependency notice inventory changed; reconcile package notices.' }
    $licenses = Join-Path $install 'licenses'
    New-Item -ItemType Directory -Path $licenses | Out-Null
    foreach ($entry in $noticeEntries) { Copy-Item -LiteralPath (Join-Path $repositoryRoot $entry.Groups[1].Value) -Destination (Join-Path $licenses $entry.Groups[2].Value) }
    $installedInventory = @(Get-FileInventory $install)
    $consumer = Join-Path $bin (Split-Path $executable -Leaf)
    $consumerArgs = @('--terrain-gameplay-package', $assets, '--plugins', $plugins) + $common
    Write-Host 'Running the copied package through the five real vehicle driving phases.'
    $log = Invoke-GameplayProcess $consumer ($consumerArgs + '--smoke-test') $bin 'consume-smoke' $true
    Assert-GameplayLog $log $true
    if ($log -notmatch 'Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false') {
        throw 'Consumer did not record its source-free authoring-service boundary.'
    }
    $consumerAuthorServicesAbsentVerified = $true
    # Capture independently: capture control policy intentionally differs from
    # the driving smoke and must never suppress its throttle/brake phases.
    foreach ($view in @('track', 'car')) {
        $capture = Join-Path $output ("terrain-gameplay-$view.bmp")
        $evidence = Join-Path $output ("terrain-gameplay-$view.json")
        Write-Host "Capturing the copied terrain gameplay package: $view."
        $log = Invoke-GameplayProcess $consumer ($consumerArgs + @('--capture', $capture, '--view', $view, '--evidence', $evidence)) $bin ("capture-$view") $true
        Assert-GameplayLog $log
        if (-not (Test-Path -LiteralPath $capture -PathType Leaf) -or (Get-Item -LiteralPath $capture).Length -lt 54 -or -not (Test-Path -LiteralPath $evidence -PathType Leaf)) {
            throw "The $view capture or its evidence was not produced."
        }
        $captureEvidence = Get-Content -LiteralPath $evidence -Raw | ConvertFrom-Json
        if (-not $captureEvidence.validationPassed -or -not $captureEvidence.device.adapter) { throw "The $view capture did not pass native rendering/device validation." }
        $captures.Add([ordered]@{ view = $view; path = $capture; sha256 = (Get-FileHash -LiteralPath $capture -Algorithm SHA256).Hash
            evidencePath = $evidence; device = $captureEvidence.device; validationPassed = $captureEvidence.validationPassed; visualAcceptance = 'unreviewed' })
    }
    $negativeRoot = Join-Path $output 'negative-inputs'
    New-Item -ItemType Directory -Path $negativeRoot | Out-Null
    $missingManifest = Join-Path $negativeRoot 'missing-manifest'
    Copy-Item -LiteralPath $assets -Destination $missingManifest -Recurse
    Remove-Item -LiteralPath (Join-Path $missingManifest 'runtime.wprm')
    $log = Invoke-GameplayProcess $consumer (@('--terrain-gameplay-package', $missingManifest, '--plugins', $plugins, '--smoke-test') + $common) $bin 'missing-manifest' $false
    if ($log -notmatch '(?i)(fail|error|missing|unable|cannot|could not|open)[^\r\n]*manifest|manifest[^\r\n]*(fail|error|missing|unable|cannot|could not|open)') { throw 'Missing manifest failed for an unexpected reason.' }
    $corruptAssets = Join-Path $negativeRoot 'corrupt-assets'
    Copy-Item -LiteralPath $assets -Destination $corruptAssets -Recurse
    $victim = Get-ChildItem -LiteralPath $corruptAssets -Recurse -File | Where-Object Extension -eq '.wprs' | Sort-Object FullName | Select-Object -First 1
    if (-not $victim -or $victim.Length -eq 0) { throw 'No compiled payload found for corruption test.' }
    $bytes = [IO.File]::ReadAllBytes($victim.FullName)
    $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 1
    [IO.File]::WriteAllBytes($victim.FullName, $bytes)
    $log = Invoke-GameplayProcess $consumer (@('--terrain-gameplay-package', $corruptAssets, '--plugins', $plugins, '--smoke-test') + $common) $bin 'corrupt-payload' $false
    if ($log -notmatch '(?i)(hash|corrupt|checksum|integrity)') { throw 'Corrupt payload failed for an unexpected reason.' }
    Assert-InventoryUnchanged $assetInventory $assets 'Consumer'
    Assert-InventoryUnchanged $installedInventory $install 'Consumer'
    $assetFilesUnchangedVerified = $true
    $archive = Join-Path $output "WorkphoneTerrainGameplay-$Configuration.zip"
    Compress-Archive -Path (Join-Path $install '*') -DestinationPath $archive -CompressionLevel Fastest
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    $extracted = Join-Path $output 'archive-install'
    Expand-Archive -LiteralPath $archive -DestinationPath $extracted
    Assert-InventoryUnchanged $installedInventory $extracted 'Archive extraction'
    $extractedBin = Join-Path $extracted 'bin'
    $extractedConsumer = Join-Path $extractedBin (Split-Path $executable -Leaf)
    Write-Host 'Running the extracted archive through the five vehicle driving phases.'
    $log = Invoke-GameplayProcess $extractedConsumer (@('--terrain-gameplay-package', (Join-Path $extracted 'assets'), '--plugins', (Join-Path $extractedBin 'wp_plugins_terrain_gameplay.cfg'), '--smoke-test') + $common) $extractedBin 'archive-smoke' $true
    Assert-GameplayLog $log $true
    if ($log -notmatch 'Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false') { throw 'Extracted archive did not record its source-free authoring-service boundary.' }
    Assert-InventoryUnchanged $installedInventory $extracted 'Extracted archive consumer'
    $archiveVerified = $true
    foreach ($inputFile in $runtimeInputs) {
        $inputFile.sha256After = (Get-FileHash -LiteralPath $inputFile.source -Algorithm SHA256).Hash
        if ($inputFile.sha256After -ne $inputFile.sha256Before -or (Get-FileHash -LiteralPath (Join-Path $bin $inputFile.fileName) -Algorithm SHA256).Hash -ne $inputFile.sha256Before) {
            throw "Runtime binary changed during gameplay package validation: $($inputFile.source)"
        }
    }
    if ((Get-FileHash -LiteralPath $authorPlugins -Algorithm SHA256).Hash -ne $initialPluginsHash) { throw 'Authoring plugin configuration changed during gameplay validation.' }
    $runtimeInputsUnchangedVerified = $true
    $sourceAfter = Get-GameplaySourceSnapshot
    $sourceAfter | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'source-after.json') -Encoding utf8
    $changedSources = @(Compare-TerrainSourceSnapshots $sourceBefore $sourceAfter)
    if ($changedSources.Count) { throw 'Terrain/gameplay source inputs changed during validation; evidence is limited to captured artifacts and needs a stable rerun.' }
    $passed = $true
} catch { $failure = $_.Exception.Message; throw } finally {
    [ordered]@{
        schema = 'workphone.terrain-gameplay-package.v1'; recordedAtUtc = [DateTime]::UtcNow.ToString('o')
        sourceRevision = (& git -C $repositoryRoot rev-parse HEAD); configuration = $Configuration; buildDirectory = $buildPath
        binaryDirectory = $binaryRoot; binarySha256 = $initialBinaryHash; authorPluginsSha256 = $initialPluginsHash
        sourceBefore = $sourceBefore.sha256; sourceAfter = $sourceAfter.sha256; changedSources = $changedSources
        runtimeInputs = @($runtimeInputs.ToArray()); runtimeInputsUnchangedVerified = $runtimeInputsUnchangedVerified
        dynamicPluginRoots = $pluginRoots; consumerPlugins = $pluginNames; recipeHash = $expectedRecipeHash
        gameplayRootResource = 'data://terrain-gameplay.gameplay'; target = 'terrain-gameplay-dx11'
        assetFiles = $assetInventory; assetFilesUnchangedVerified = $assetFilesUnchangedVerified
        packagePassed = $passed; failure = $failure; runs = @($runs.ToArray()); captures = @($captures.ToArray())
        files = @(Get-FileInventory $install); archive = $archive; archiveSha256 = $archiveHash; archiveExtractedAndExecuted = $archiveVerified
        consumerAuthorServicesAbsentVerified = $consumerAuthorServicesAbsentVerified
        sourceFreeConsumerProcess = $consumerAuthorServicesAbsentVerified; vehicleRepresentation = 'pinned runtime procedural recipe with regenerated content-hash check'
        diagnosticProcessWatchdogSeconds = $TimeoutSeconds; originalGateTimeoutsChanged = $false
        hardwareAcceptanceVerified = $false; interactiveEditorVerified = $false; visualAcceptance = 'unreviewed'
        cleanMachineVerified = $false; releaseCertified = $false; licenseAuditComplete = $false
        limitations = @('A software adapter/WARP run is diagnostic only and does not certify hardware performance or recovery.',
            'Capture validation does not certify visual quality or interactive Editor authoring, undo/reload, or mouse interaction.',
            'Freshness requires a separately reconciled build; source and binary hashes identify the inputs observed during this run.',
            'The vehicle is regenerated from a pinned compiled gameplay recipe; this does not claim baked vehicle mesh-resource support.',
            'OS/CRT prerequisites are provisioned on this machine; archive relocation is not a clean-machine installation test.',
            'The executable may import WPSQLite for its author mode; the runtime package does not initialize the SQLite authoring plugin or compilation services.')
    } | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'gameplay-package-evidence.json') -Encoding utf8
}
Write-Host "Terrain gameplay package process checks passed: $output/gameplay-package-evidence.json"
