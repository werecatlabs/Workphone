#requires -Version 7.0
param(
    [string]$BuildDirectory = 'project_wpgraphics_baseline',
    [ValidateSet('Debug', 'RelWithDebInfo')][string[]]$Configuration = @('Debug', 'RelWithDebInfo'),
    [string]$CMakeExecutable,
    [string]$Generator,
    [switch]$IncludeEditor,
    [switch]$RequireExternalAssets,
    [switch]$CaptureCrashDumps,
    [switch]$IncludeTerrainPackage,
    [switch]$IncludeTerrainGameplayPackage,
    [switch]$TerrainOnly
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'TerrainValidationEvidence.ps1')
. (Join-Path $PSScriptRoot 'GraphicsValidationEvidence.ps1')
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDirectory)) { [System.IO.Path]::GetFullPath($BuildDirectory) } else { [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory)) }
New-Item -ItemType Directory -Path $buildPath -Force | Out-Null
if (-not $CMakeExecutable) {
    $installedCMake = Join-Path $env:ProgramFiles 'CMake/bin/cmake.exe'
    $CMakeExecutable = if (Test-Path -LiteralPath $installedCMake) { $installedCMake } else { (Get-Command cmake -ErrorAction Stop).Source }
}
$CMakeExecutable = (Get-Command $CMakeExecutable -ErrorAction Stop).Source
$ctestExecutable = Join-Path (Split-Path $CMakeExecutable -Parent) 'ctest.exe'

function Invoke-BaselineCMake([string[]]$Arguments, [string]$LogPath) {
    $info = [System.Diagnostics.ProcessStartInfo]::new()
    $info.FileName = $CMakeExecutable
    $info.WorkingDirectory = $repositoryRoot
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    # Windows environment names are case-insensitive. Some launchers supply both
    # PATH and Path, which MSBuild's compiler tasks reject as duplicate keys.
    $info.Environment.Clear()
    foreach ($entry in Get-ChildItem Env:) { $info.Environment[$entry.Name.ToUpperInvariant()] = $entry.Value }
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $log = [System.IO.StreamWriter]::new($LogPath, $false)
    $log.AutoFlush = $true
    $process = $null
    try {
        $process = [System.Diagnostics.Process]::Start($info)
        $stderr = $process.StandardError.ReadToEndAsync()
        while (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            $log.WriteLine($line)
            if ($line -match 'error |CMake Error|FAILED|Configuring done|Generating done') { Write-Host $line }
        }
        $process.WaitForExit()
        $errors = $stderr.GetAwaiter().GetResult()
        $log.WriteLine($errors)
        if ($errors) { Write-Host $errors }
        if ($process.ExitCode -ne 0) { throw "CMake exited with $($process.ExitCode). See $LogPath" }
    } finally { $log.Dispose(); if ($process) { $process.Dispose() } }
}

Write-Host "Using $CMakeExecutable"
$configureArguments = @('--preset', 'wpgraphics-baseline', '-B', $buildPath)
if ($IncludeEditor) { $configureArguments += @('-DWP_BUILD_TOOLS=ON', '-DWP_BUILD_EDITOR=ON') }
if ($IncludeTerrainGameplayPackage) { $configureArguments += '-DWP_BUILD_SAMPLES=ON' }
if ($Generator) { $configureArguments += @('-G', $Generator) }
Invoke-BaselineCMake $configureArguments (Join-Path $buildPath 'baseline-configure.log')
$presets = Get-Content -LiteralPath (Join-Path $repositoryRoot 'CMakePresets.json') -Raw | ConvertFrom-Json
foreach ($config in $Configuration) {
    $fullSourceBefore = if (-not $TerrainOnly) { Get-GraphicsValidationSourceSnapshot $repositoryRoot } else { $null }
    $sourceBefore = if ($TerrainOnly) { Get-TerrainSourceSnapshot $repositoryRoot } else { $null }
    $presetName = if ($config -eq 'Debug') { 'wpgraphics-baseline-debug' } else { 'wpgraphics-baseline' }
    $preset = $presets.buildPresets | Where-Object { $_.name -eq $presetName }
    Write-Host "Building WPGraphics baseline: $config"
    $buildArguments = @('--build', $buildPath, '--config', $config, '--parallel', '4', '--target') + $preset.targets
    if ($TerrainOnly) { $buildArguments = @('--build', $buildPath, '--config', $config, '--parallel', '2', '--target', 'WPTerrainValidation') }
    if (-not $TerrainOnly) {
        # The preset is a minimum, not the complete evolving test inventory.
        # Resolve Python-driven host/tool arguments too; checking only command[0]
        # would silently leave resource recovery/package executables stale.
        $selection = '^(WPGraphics|WorkphoneGraphics|WorkphoneAssets|WPAnimation|WPResource)\.|^WPResourceTests$'
        $inventoryText = & $ctestExecutable --test-dir $buildPath -C $config -R $selection --show-only=json-v1
        if ($LASTEXITCODE -ne 0) { throw 'CTest build inventory failed.' }
        $inventory = ($inventoryText -join "`n") | ConvertFrom-Json
        $mapping = @{}
        foreach ($line in Get-Content -LiteralPath (Join-Path $buildPath "graphics-validation/$config/targets.txt")) {
            $target, $path = $line -split '\|', 2
            $mapping[[IO.Path]::GetFullPath($path)] = $target
        }
        $resolvedTargets = @($preset.targets) + @(foreach ($test in $inventory.tests) {
            foreach ($argument in $test.command) {
                if ([IO.Path]::IsPathRooted($argument) -and $mapping.ContainsKey([IO.Path]::GetFullPath($argument))) {
                    $mapping[[IO.Path]::GetFullPath($argument)]
                }
            }
        })
        $resolvedTargets = @($resolvedTargets | Sort-Object -Unique)
        $resolvedTargets | Set-Content -LiteralPath (Join-Path $buildPath "baseline-$config-targets.txt")
        $buildArguments = @('--build', $buildPath, '--config', $config, '--parallel', '4', '--target') + $resolvedTargets
    }
    if ($IncludeEditor) { $buildArguments += 'Editor' }
    if ($IncludeTerrainGameplayPackage) { $buildArguments += @('SampleVehicleAdvanced', 'WPGraphics', 'WPPhysics', 'WPVehiclePhysics', 'WPProcedural') }
    Invoke-BaselineCMake ($buildArguments + @('--', '/nodeReuse:false', '/verbosity:minimal')) (Join-Path $buildPath "baseline-$config-build.log")
    if (-not $TerrainOnly) {
        $fullSourceAfter = Get-GraphicsValidationSourceSnapshot $repositoryRoot
        $changed = @(Compare-GraphicsValidationSourceSnapshots $fullSourceBefore $fullSourceAfter)
        [ordered]@{stable=($changed.Count -eq 0);changedFiles=$changed;
            inputs=@($fullSourceBefore.Keys | Sort-Object | ForEach-Object { [ordered]@{path=$_;sha256=$fullSourceBefore[$_]} })} |
            ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $buildPath "baseline-$config-build-source.json") -Encoding utf8
        if ($changed.Count) { throw "Graphics inputs changed during $config build. Rebuild against stable inputs; see baseline-$config-build-source.json." }
    }
    if ($TerrainOnly) {
        $sourceAfter = Get-TerrainSourceSnapshot $repositoryRoot
        $changed = @(Compare-TerrainSourceSnapshots $sourceBefore $sourceAfter)
        [ordered]@{ sourceBefore = $sourceBefore.sha256; sourceAfter = $sourceAfter.sha256; changedFiles = $changed; stable = ($changed.Count -eq 0) } |
            ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $buildPath "terrain-gt0-$config-build-source.json") -Encoding utf8
        if ($changed.Count) { throw "Terrain sources changed during $config build. Repeat the build against stable inputs; see terrain-gt0-$config-build-source.json." }
    }
    & (Join-Path $PSScriptRoot 'ValidateClawGraphics.ps1') -BuildDirectory $buildPath -Configuration $config -CTestExecutable $ctestExecutable -RequireExternalAssets:$RequireExternalAssets -CaptureCrashDumps:$CaptureCrashDumps -TerrainOnly:$TerrainOnly
    if ($IncludeTerrainPackage) {
        & (Join-Path $PSScriptRoot 'ValidateTerrainPackage.ps1') -BuildDirectory $buildPath -Configuration $config
    }
    if ($IncludeTerrainGameplayPackage) {
        & (Join-Path $PSScriptRoot 'ValidateTerrainGameplayPackage.ps1') -BuildDirectory $buildPath -Configuration $config
    }
}
