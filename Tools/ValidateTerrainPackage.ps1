#requires -Version 7.0
param(
    [string]$BuildDirectory = 'project_x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release')][string]$Configuration = 'RelWithDebInfo',
    [string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
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
    $info.Environment['PATH'] = "$bin;$env:SystemRoot\System32;$env:SystemRoot"
    $process = [Diagnostics.Process]::Start($info)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit(120000)) {
        $process.Kill($true)
        $process.WaitForExit()
        throw "$Name exceeded the 120-second package process limit."
    }
    $log = $stdout.GetAwaiter().GetResult() + $stderr.GetAwaiter().GetResult()
    $log | Set-Content -LiteralPath (Join-Path $output "$Name.log") -Encoding utf8
    $exitCode = $process.ExitCode
    $process.Dispose()
    $runs.Add([ordered]@{ name = $Name; exitCode = $exitCode; expectedSuccess = $ExpectedSuccess; output = $log })
    if (($exitCode -eq 0) -ne $ExpectedSuccess) { throw "$Name returned $exitCode. See $output/$Name.log" }
    return $log
}
$passed = $false
$failure = $null
$initialBinaryHash = (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash
try {
    Invoke-PackageProcess $executable @('--terrain-package-export', $authoring) $repositoryRoot 'author' $true | Out-Null
    Copy-Item -LiteralPath (Join-Path $authoring 'terrain-package/package') -Destination (Join-Path $install 'assets') -Recurse
    Copy-Item -LiteralPath $executable -Destination $bin
    & $cmake "-DEXECUTABLE=$executable" "-DOUTPUT_DIRECTORY=$bin" "-DDUMPBIN=$dumpbin" -P (Join-Path $PSScriptRoot 'TerrainPackageRuntime.cmake') *> (Join-Path $output 'dependencies.log')
    if ($LASTEXITCODE -ne 0) { throw 'Runtime dependency collection failed; see dependencies.log.' }
    $consumer = Join-Path $bin (Split-Path $executable -Leaf)
    $assets = Join-Path $install 'assets'
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', 'assets') $install 'consume' $true
    if ($log -notmatch 'PASS: standalone source-free terrain package' -or $log -notmatch 'DX11 device:') {
        throw 'Consumer did not record package/device evidence.'
    }
    New-Item -ItemType Directory -Path (Join-Path $install 'missing-manifest') | Out-Null
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', 'missing-manifest') $install 'missing-manifest' $false
    if ($log -notmatch '(?i)FAIL:.*manifest') { throw 'Missing manifest failed for an unexpected reason.' }
    Copy-Item -LiteralPath $assets -Destination (Join-Path $install 'corrupt-assets') -Recurse
    $victim = Get-ChildItem -LiteralPath (Join-Path $install 'corrupt-assets') -Recurse -File |
        Where-Object { $_.Extension -notin @('.wprs', '.ppm') } | Sort-Object FullName | Select-Object -First 1
    if (-not $victim) { throw 'No compiled payload found for corruption test.' }
    $bytes = [IO.File]::ReadAllBytes($victim.FullName)
    $bytes[$bytes.Length - 1] = $bytes[$bytes.Length - 1] -bxor 1
    [IO.File]::WriteAllBytes($victim.FullName, $bytes)
    $log = Invoke-PackageProcess $consumer @('--terrain-package-consume', 'corrupt-assets') $install 'corrupt-payload' $false
    if ($log -notmatch '(?i)FAIL:.*(hash|corrupt|checksum)') { throw 'Corrupt payload failed for an unexpected reason.' }
    if ((Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash -ne $initialBinaryHash) { throw 'Build executable changed during packaging.' }
    $passed = $true
} catch { $failure = $_.Exception.Message; throw } finally {
    $files = @(Get-ChildItem -LiteralPath $install -Recurse -File | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($install, $_.FullName); bytes = $_.Length; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    [ordered]@{
        recordedAtUtc = [DateTime]::UtcNow.ToString('o'); sourceRevision = (& git -C $repositoryRoot rev-parse HEAD)
        configuration = $Configuration; buildDirectory = $buildPath; binarySha256 = $initialBinaryHash
        packagePassed = $passed; failure = $failure; runs = @($runs.ToArray()); files = $files
        compilerRegistryCreatedByConsumer = $false; authoringDatabaseCreatedByConsumer = $false
        sourceFreeConsumerProcess = $true; cleanMachineVerified = $false; releaseCertified = $false
        limitations = @('Small four-layer fixture; no packaged vehicle/physics scene', 'OS/CRT prerequisites provisioned on this machine', 'Interactive Editor and representative visual acceptance remain separate')
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'package-evidence.json') -Encoding utf8
}
Write-Host "Terrain package process checks passed: $output/package-evidence.json"
