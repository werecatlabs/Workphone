#requires -Version 7.0
param(
    [string]$BuildDirectory = 'project_wpgraphics_baseline',
    [ValidateSet('Debug', 'RelWithDebInfo')][string[]]$Configuration = @('Debug', 'RelWithDebInfo'),
    [string]$CMakeExecutable,
    [string]$Generator,
    [switch]$RequireExternalAssets
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
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
    $process = [System.Diagnostics.Process]::Start($info)
    $stderr = $process.StandardError.ReadToEndAsync()
    $log = [System.IO.StreamWriter]::new($LogPath, $false)
    $log.AutoFlush = $true
    try {
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
    } finally { $log.Dispose(); $process.Dispose() }
}

Write-Host "Using $CMakeExecutable"
$configureArguments = @('--preset', 'wpgraphics-baseline', '-B', $buildPath)
if ($Generator) { $configureArguments += @('-G', $Generator) }
Invoke-BaselineCMake $configureArguments (Join-Path $buildPath 'baseline-configure.log')
$presets = Get-Content -LiteralPath (Join-Path $repositoryRoot 'CMakePresets.json') -Raw | ConvertFrom-Json
foreach ($config in $Configuration) {
    $presetName = if ($config -eq 'Debug') { 'wpgraphics-baseline-debug' } else { 'wpgraphics-baseline' }
    $preset = $presets.buildPresets | Where-Object { $_.name -eq $presetName }
    Write-Host "Building WPGraphics baseline: $config"
    $buildArguments = @('--build', $buildPath, '--config', $config, '--parallel', '4', '--target') + $preset.targets
    Invoke-BaselineCMake ($buildArguments + @('--', '/nodeReuse:false', '/verbosity:minimal')) (Join-Path $buildPath "baseline-$config-build.log")
    & (Join-Path $PSScriptRoot 'ValidateClawGraphics.ps1') -BuildDirectory $buildPath -Configuration $config -CTestExecutable $ctestExecutable -RequireExternalAssets:$RequireExternalAssets
}
