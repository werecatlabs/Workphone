[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo')][string]$Configuration = 'Debug',
    [string]$OutputDirectory,
    [ValidateRange(0, 4294967295)][long]$Seed = 42,
    [ValidateSet('low', 'medium', 'high', 'cinematic')][string]$Quality = 'high',
    [ValidatePattern('^\d+x\d+$')][string]$Resolution = '1280x720',
    [ValidateRange(0.1, 3600)][double]$Seconds = 10,
    [ValidateSet('follow', 'car', 'track', 'corner')][string[]]$Views = @('car', 'track', 'corner', 'follow'),
    [string]$Replay,
    [switch]$NoEffects,
    [switch]$FoliageWorld,
    [switch]$ExportMaterials,
    [ValidateRange(30, 7200)][int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$binaryRoot = Join-Path $repoRoot "bin/windows/v145/x64/MD/$Configuration"
$executable = Join-Path $binaryRoot 'SampleVehicleAdvanced.exe'
$plugins = Join-Path $binaryRoot 'wp_plugins_samples.cfg'
if (!(Test-Path -LiteralPath $executable)) { throw "Build SampleVehicleAdvanced $Configuration first." }
if (!(Test-Path -LiteralPath $plugins)) { throw "Missing sample plugin configuration: $plugins" }
if (!$OutputDirectory) { $OutputDirectory = Join-Path $repoRoot ('artifacts/procedural/' + (Get-Date -Format 'yyyyMMdd-HHmmss')) }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Choose a fresh output directory to preserve previous evidence.' }
if ($Replay) { $Replay = (Resolve-Path -LiteralPath $Replay).Path }
New-Item -ItemType Directory -Path $output -Force | Out-Null

function Get-Identity([string]$Path) {
    $item = Get-Item -LiteralPath $Path
    [ordered]@{ path = $item.FullName; bytes = $item.Length; sha256 = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
}
function Quote-ProcessArgument([string]$Value) {
    if ($Value.Contains('"')) { throw 'Quotes are not supported inside capture paths.' }
    # All file arguments here are absolute paths to files, so no trailing slash.
    '"' + $Value + '"'
}

$revision = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source revision.' }
$diffPath = Join-Path $output 'working-tree.patch'
& git -C $repoRoot diff --binary HEAD "--output=$diffPath"
if ($LASTEXITCODE -ne 0) { throw 'Cannot capture local diff.' }
$sourceRoots = @('Engine/cpp/Include/WPProcedural', 'Engine/cpp/Source/WPProcedural', 'Engine/cpp/Include/WPGraphics', 'Engine/cpp/Source/WPGraphics', 'Engine/cpp/Include/Workphone/Graphics', 'Engine/cpp/Include/Workphone/Scene/Components', 'Engine/cpp/Source/Workphone/Scene/Components', 'Samples/cpp/VehicleAdvanced')
$sources = @($sourceRoots | ForEach-Object { Get-ChildItem -LiteralPath (Join-Path $repoRoot $_) -Recurse -File | Where-Object { $_.Extension -in '.cpp', '.hpp', '.h', '.txt' } } | Sort-Object FullName | ForEach-Object { Get-Identity $_.FullName })
$binaries = @(Get-ChildItem -LiteralPath $binaryRoot -File | Where-Object { $_.Extension -eq '.dll' -or $_.Name -eq 'SampleVehicleAdvanced.exe' } | Sort-Object Name | ForEach-Object { Get-Identity $_.FullName })
$assets = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'bin/Media') -Recurse -File | Where-Object { $_.Extension -in '.png', '.jpg', '.jpeg', '.tga', '.dds', '.mesh', '.hdr', '.texres', '.matres', '.folspec', '.folworld' } | Sort-Object FullName | ForEach-Object { Get-Identity $_.FullName })
$manifest = [ordered]@{
    schema = 'workphone.procedural-capture-set.v1'; startedUtc = [DateTime]::UtcNow.ToString('o')
    revision = $revision; configuration = $Configuration; seed = $Seed; quality = $Quality
    requestedResolution = $Resolution; benchmarkSeconds = $Seconds; visualAcceptance = 'unreviewed'
    diff = Get-Identity (Join-Path $output 'working-tree.patch'); sources = $sources; binaries = $binaries
    plugins = Get-Identity $plugins; assets = $assets; replay = $null; runs = @()
    limitations = @('Fresh build must be performed separately; hashes identify artifacts, not freshness.', 'Control replay applies one row per physics-task tick; the task targets 120 Hz but physics uses measured deltas.', 'Asset hashes inventory the media tree; they are not a runtime dependency trace.', 'No blind comparison or visual acceptance is implied.')
}
if ($Replay) { $manifest.replay = Get-Identity $Replay }
$failed = $false
try {
    foreach ($view in $Views) {
        $capture = Join-Path $output "$view.bmp"
        $evidence = Join-Path $output "$view.json"
        $arguments = @('--seed', "$Seed", '--quality', $Quality, '--plugins', (Quote-ProcessArgument $plugins), '--capture', (Quote-ProcessArgument $capture), '--view', $view, '--no-hud', '--resolution', $Resolution, '--no-audio', '--benchmark', $Seconds.ToString([Globalization.CultureInfo]::InvariantCulture), '--evidence', (Quote-ProcessArgument $evidence))
        if ($Replay) { $arguments += @('--replay', (Quote-ProcessArgument $Replay)) }
        if ($NoEffects) { $arguments += '--no-effects' }
        if ($FoliageWorld) { $arguments += '--foliage-world' }
        if ($ExportMaterials) { $arguments += @('--export-materials', (Quote-ProcessArgument (Join-Path $output "$view-materials"))) }
        Write-Host "Capturing $view ($Quality, seed $Seed)"
        $process = Start-Process -FilePath $executable -ArgumentList $arguments -WorkingDirectory $binaryRoot -WindowStyle Hidden -RedirectStandardOutput (Join-Path $output "$view.stdout.log") -RedirectStandardError (Join-Path $output "$view.stderr.log") -PassThru
        $timedOut = !$process.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) { $process.Kill(); $process.WaitForExit() }
        $process.Refresh()
        $run = [ordered]@{ view = $view; arguments = $arguments; exitCode = $process.ExitCode; timedOut = $timedOut; capture = $null; evidence = $null; passed = $false }
        if (Test-Path -LiteralPath $capture) { $run.capture = Get-Identity $capture }
        if (Test-Path -LiteralPath $evidence) {
            $run.evidence = Get-Content -LiteralPath $evidence -Raw | ConvertFrom-Json
            $run.passed = !$timedOut -and $process.ExitCode -eq 0 -and $null -ne $run.capture -and $run.evidence.validationPassed
        }
        $manifest.runs += $run
        if (!$run.passed) { $failed = $true }
    }
} finally {
    $manifest.completedUtc = [DateTime]::UtcNow.ToString('o')
    $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding utf8
}
if ($failed) { throw "Capture validation failed; inspect $output" }
Write-Host "Captured evidence: $output"
