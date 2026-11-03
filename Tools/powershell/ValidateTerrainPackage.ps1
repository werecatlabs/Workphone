#requires -Version 7.0
param(
    [string]$BuildDirectory = 'project_x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release')][string]$Configuration = 'RelWithDebInfo',
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'TerrainValidationEvidence.ps1')
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) { [IO.Path]::GetFullPath($BuildDirectory) } else { [IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory)) }
$cmake = Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
$ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
$inventoryText = & $ctest --test-dir $buildPath -C $Configuration -R '^WPGraphics.cooked_material_resource$' --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'Unable to inventory cooked resource executable.' }
$inventory = ($inventoryText -join "`n") | ConvertFrom-Json
if ($inventory.tests.Count -ne 1 -or -not $inventory.tests[0].command) { throw 'Cooked resource test is not built/registered.' }
$executable = $inventory.tests[0].command[0]
if (-not (Test-Path -LiteralPath $executable)) { throw "Build WPGraphicsCookedResourceTests first: $executable" }
$compilerFile = Get-ChildItem -Path (Join-Path $buildPath 'CMakeFiles/*/CMakeCXXCompiler.cmake') | Select-Object -Last 1
$compilerText = Get-Content -LiteralPath $compilerFile.FullName -Raw
if ($compilerText -notmatch 'set\(CMAKE_CXX_COMPILER "([^"]+)"\)') { throw 'Compiler path missing from CMake evidence.' }
$dumpbin = Join-Path (Split-Path $Matches[1] -Parent) 'dumpbin.exe'
if (-not (Test-Path -LiteralPath $dumpbin)) { throw "PE dependency resolver unavailable: $dumpbin" }
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $repositoryRoot ('artifacts/terrain/package-' + [guid]::NewGuid().ToString('N'))
}
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Output must be a new directory: $output" }
$authoring = Join-Path $output 'authoring-process'
$install = Join-Path $output 'relocated-install'
$bin = Join-Path $install 'bin'
New-Item -ItemType Directory -Path $authoring, $bin -Force | Out-Null
$runs = [Collections.Generic.List[object]]::new()
function Invoke-PackageProcess([string]$Program, [string[]]$Arguments, [string]$WorkingDirectory, [string]$Name, [bool]$ExpectedSuccess) {
    $info = [Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $Program
    $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    # No source/build directories in DLL search PATH. OS/CRT prerequisites remain external.
    $info.Environment['PATH'] = "$(Split-Path $Program -Parent);$env:SystemRoot\System32;$env:SystemRoot"
    $process = $null
    $stdout = $null
    $stderr = $null
    $exitCode = $null
    $timedOut = $false
    $processError = $null
    $log = ''
    $elapsed = [Diagnostics.Stopwatch]::StartNew()
    try {
        $process = [Diagnostics.Process]::Start($info)
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(120000)) {
            $timedOut = $true
            throw "$Name exceeded the 120-second package process limit."
        }
        $exitCode = $process.ExitCode
    } catch {
        $processError = $_.Exception.Message
        throw
    } finally {
        if ($process) {
            try {
                if (-not $process.HasExited) {
                    $process.Kill($true)
                    if (-not $process.WaitForExit(5000)) { $log += "Process tree did not terminate within five seconds.`n" }
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
        if ($processError) { $log += "`nPackage process error: $processError`n" }
        $elapsed.Stop()
        try {
            $log | Set-Content -LiteralPath (Join-Path $output "$Name.log") -Encoding utf8
            $runs.Add([ordered]@{ name = $Name; exitCode = $exitCode; expectedSuccess = $ExpectedSuccess
                timedOut = $timedOut; seconds = $elapsed.Elapsed.TotalSeconds; error = $processError; output = $log })
        } finally { if ($process) { $process.Dispose() } }
    }
    if (($exitCode -eq 0) -ne $ExpectedSuccess) { throw "$Name returned $exitCode. See $output/$Name.log" }
    return $log
}
$passed = $false
$failure = $null
$archive = $null
$archiveHash = $null
$archiveVerified = $false
$sourceBefore = $null
$sourceAfter = $null
$changedSources = @()
$runtimeInputs = [Collections.Generic.List[object]]::new()
$assetFilesUnchangedVerified = $false
$initialBinaryHash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
try {
    $sourceBefore = Get-TerrainSourceSnapshot $repositoryRoot
    # Discover a closure before authoring, then freeze each source DLL's bytes.
    # The second collection below must match this exact closure and fingerprint.
    $discovery = Join-Path $output 'dependency-discovery'
    New-Item -ItemType Directory -Path $discovery | Out-Null
    & $cmake "-DEXECUTABLE=$executable" "-DOUTPUT_DIRECTORY=$discovery" "-DDUMPBIN=$dumpbin" -P (Join-Path $PSScriptRoot 'TerrainPackageRuntime.cmake') *> (Join-Path $output 'dependency-discovery.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime dependency discovery failed; see dependency-discovery.log.' }
    $dependencyPaths = @((Get-Content -LiteralPath (Join-Path $discovery 'runtime-dependencies.txt') -Raw) -split '[;\r\n]+' |
        Where-Object { $_ } | ForEach-Object { [IO.Path]::GetFullPath($_) } | Sort-Object -Unique)
    foreach ($dependency in @($executable) + $dependencyPaths) {
        $hash = (Get-FileHash -LiteralPath $dependency -Algorithm SHA256).Hash
        if ($dependency -eq $executable) {
            if ($hash -ne $initialBinaryHash) { throw 'Build executable changed before package dependency collection.' }
        } elseif ((Get-FileHash -LiteralPath (Join-Path $discovery (Split-Path $dependency -Leaf)) -Algorithm SHA256).Hash -ne $hash) {
            throw "Runtime dependency changed during discovery: $dependency"
        }
        $runtimeInputs.Add([ordered]@{ source = $dependency; fileName = (Split-Path $dependency -Leaf)
            sha256Before = $hash; sha256Collected = $null; sha256After = $null })
    }
    Invoke-PackageProcess $executable @('--terrain-package-export', $authoring) $repositoryRoot 'author' $true | Out-Null
    Copy-Item -LiteralPath (Join-Path $authoring 'terrain-package/package') -Destination (Join-Path $install 'assets') -Recurse
    $authoringLeaks = @(Get-ChildItem -LiteralPath (Join-Path $install 'assets') -Recurse -Force -File |
        Where-Object { $_.Name -match '(?i)\.(db|db-wal|db-shm|sqlite|terrain|texres|tga)$' })
    if ($authoringLeaks.Count) { throw 'Authoring data or compilation database leaked into the terrain install.' }
    Copy-Item -LiteralPath $executable -Destination $bin
    & $cmake "-DEXECUTABLE=$executable" "-DOUTPUT_DIRECTORY=$bin" "-DDUMPBIN=$dumpbin" -P (Join-Path $PSScriptRoot 'TerrainPackageRuntime.cmake') *> (Join-Path $output 'dependencies.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime dependency collection failed; see dependencies.log.' }
    $collectedPaths = @((Get-Content -LiteralPath (Join-Path $bin 'runtime-dependencies.txt') -Raw) -split '[;\r\n]+' |
        Where-Object { $_ } | ForEach-Object { [IO.Path]::GetFullPath($_) } | Sort-Object -Unique)
    if ($dependencyPaths.Count -ne $collectedPaths.Count -or
        ($dependencyPaths -join "`n") -ne ($collectedPaths -join "`n")) {
        throw 'Runtime dependency closure changed during package collection.'
    }
    foreach ($inputFile in $runtimeInputs) {
        $inputFile.sha256Collected = (Get-FileHash -LiteralPath (Join-Path $bin $inputFile.fileName) -Algorithm SHA256).Hash
        if ($inputFile.sha256Collected -ne $inputFile.sha256Before -or
            (Get-FileHash -LiteralPath $inputFile.source -Algorithm SHA256).Hash -ne $inputFile.sha256Before) {
            throw "Runtime binary changed during package collection: $($inputFile.source)"
        }
    }
    Move-Item -LiteralPath (Join-Path $bin 'runtime-dependencies.txt') -Destination (Join-Path $output 'runtime-dependencies.txt')
    # Reuse the established Editor notice sources. This retains their original
    # texts; it does not assert that the overall dependency license audit is closed.
    $noticeInstaller = Join-Path $repositoryRoot 'CMake/InstallWorkphoneEditorNotices.cmake'
    $noticeEntries = [regex]::Matches((Get-Content -LiteralPath $noticeInstaller -Raw), '"(Dependencies/[^"|]+)\|([^"|]+)"')
    if ($noticeEntries.Count -ne 12) { throw 'Established dependency notice inventory changed; reconcile package notices.' }
    $licenses = Join-Path $install 'licenses'
    New-Item -ItemType Directory -Path $licenses | Out-Null
    foreach ($entry in $noticeEntries) {
        Copy-Item -LiteralPath (Join-Path $repositoryRoot $entry.Groups[1].Value) -Destination (Join-Path $licenses $entry.Groups[2].Value)
    }
    $consumer = Join-Path $bin (Split-Path $executable -Leaf)
    $assets = Join-Path $install 'assets'
    $assetHashes = @{}
    foreach ($assetFile in (Get-ChildItem -LiteralPath $assets -Recurse -Force -File)) {
        $assetHashes[$assetFile.FullName] = (Get-FileHash -LiteralPath $assetFile.FullName -Algorithm SHA256).Hash
    }
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', 'assets', (Join-Path $output 'terrain.ppm')) $install 'consume' $true
    if ($log -notmatch 'PASS: standalone source-free terrain package' -or $log -notmatch 'DX11 device:') {
        throw 'Consumer did not record package/device evidence.'
    }
    $negativeRoot = Join-Path $output 'negative-inputs'
    $missingManifest = Join-Path $negativeRoot 'missing-manifest'
    New-Item -ItemType Directory -Path $missingManifest -Force | Out-Null
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', $missingManifest) $install 'missing-manifest' $false
    if ($log -notmatch '(?i)FAIL:.*manifest') { throw 'Missing manifest failed for an unexpected reason.' }
    $corruptAssets = Join-Path $negativeRoot 'corrupt-assets'
    Copy-Item -LiteralPath $assets -Destination $corruptAssets -Recurse
    $victim = Get-ChildItem -LiteralPath $corruptAssets -Recurse -File |
        Where-Object { $_.Name -ne 'terrain.wprs' } | Sort-Object FullName | Select-Object -First 1
    if (-not $victim) { throw 'No compiled payload found for corruption test.' }
    $bytes = [IO.File]::ReadAllBytes($victim.FullName)
    $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 1
    [IO.File]::WriteAllBytes($victim.FullName, $bytes)
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', $corruptAssets) $install 'corrupt-payload' $false
    if ($log -notmatch '(?i)FAIL:.*(hash|corrupt|checksum)') { throw 'Corrupt payload failed for an unexpected reason.' }
    $assetFilesAfter = @(Get-ChildItem -LiteralPath $assets -Recurse -Force -File)
    if ($assetFilesAfter.Count -ne $assetHashes.Count) { throw 'Consumer changed the installed asset inventory.' }
    foreach ($assetFile in $assetFilesAfter) {
        if ($assetHashes[$assetFile.FullName] -ne (Get-FileHash -LiteralPath $assetFile.FullName -Algorithm SHA256).Hash) {
            throw "Consumer changed an installed asset: $($assetFile.FullName)"
        }
    }
    $assetFilesUnchangedVerified = $true
    $archive = Join-Path $output "WorkphoneTerrainFixture-$Configuration.zip"
    Compress-Archive -Path (Join-Path $install '*') -DestinationPath $archive -CompressionLevel Fastest
    $archiveHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    $extracted = Join-Path $output 'archive-install'
    Expand-Archive -LiteralPath $archive -DestinationPath $extracted
    $installFiles = @(Get-ChildItem -LiteralPath $install -Recurse -Force -File)
    $extractedFiles = @(Get-ChildItem -LiteralPath $extracted -Recurse -Force -File)
    if ($installFiles.Count -ne $extractedFiles.Count) { throw 'Archive extraction changed the install inventory.' }
    foreach ($file in $installFiles) {
        $relative = [IO.Path]::GetRelativePath($install, $file.FullName)
        if ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -ne
            (Get-FileHash -LiteralPath (Join-Path $extracted $relative) -Algorithm SHA256).Hash) {
            throw "Archive extraction changed $relative"
        }
    }
    $extractedConsumer = Join-Path $extracted ('bin/' + (Split-Path $executable -Leaf))
    $log = Invoke-PackageProcess $extractedConsumer @('--terrain-package-consume', 'assets', (Join-Path $output 'archive-terrain.ppm')) $extracted 'archive-consume' $true
    if ($log -notmatch 'PASS: standalone source-free terrain package' -or $log -notmatch 'DX11 device:') {
        throw 'Extracted archive consumer did not record package/device evidence.'
    }
    $archiveVerified = $true
    foreach ($inputFile in $runtimeInputs) {
        $inputFile.sha256After = (Get-FileHash -LiteralPath $inputFile.source -Algorithm SHA256).Hash
        if ($inputFile.sha256After -ne $inputFile.sha256Before -or
            (Get-FileHash -LiteralPath (Join-Path $bin $inputFile.fileName) -Algorithm SHA256).Hash -ne $inputFile.sha256Before) {
            throw "Runtime binary changed during package validation: $($inputFile.source)"
        }
    }
    $sourceAfter = Get-TerrainSourceSnapshot $repositoryRoot
    $changedSources = @(Compare-TerrainSourceSnapshots $sourceBefore $sourceAfter)
    if ($changedSources.Count) { throw 'Terrain sources changed during package validation; repeat against stable inputs.' }
    $passed = $true
} catch { $failure = $_.Exception.Message; throw } finally {
    $files = @(Get-ChildItem -LiteralPath $install -Recurse -File | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($install, $_.FullName); bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    [ordered]@{
        recordedAtUtc = [DateTime]::UtcNow.ToString('o'); sourceRevision = (& git -C $repositoryRoot rev-parse HEAD)
        configuration = $Configuration; buildDirectory = $buildPath; binarySha256 = $initialBinaryHash
        sourceBefore = $sourceBefore.sha256; sourceAfter = $sourceAfter.sha256; changedSources = $changedSources
        runtimeInputs = @($runtimeInputs.ToArray()); assetFilesUnchangedVerified = $assetFilesUnchangedVerified
        packagePassed = $passed; failure = $failure; runs = @($runs.ToArray()); files = $files
        archive = $archive; archiveSha256 = $archiveHash; archiveExtractedAndExecuted = $archiveVerified; licenseAuditComplete = $false
        compilerRegistryCreatedByConsumer = $false; authoringDatabaseCreatedByConsumer = $false
        sourceFreeConsumerProcess = $true; cleanMachineVerified = $false; releaseCertified = $false
        limitations = @('Small four-layer fixture; no packaged vehicle/physics scene', 'OS/CRT prerequisites provisioned on this machine', 'Interactive Editor and representative visual acceptance remain separate')
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'package-evidence.json') -Encoding utf8
}
Write-Host "Terrain package process checks passed: $output/package-evidence.json"
