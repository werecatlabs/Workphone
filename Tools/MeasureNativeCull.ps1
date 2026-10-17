#requires -Version 7.0
[CmdletBinding()]
param(
    [string]$BuildDirectory = 'project_graphics_contracts',
    [ValidateSet('Release','RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [string]$OutputDirectory,
    [ValidateRange(64,50000)][int]$Objects = 10000,
    [ValidateRange(0,60)][double]$WarmupSeconds = 5,
    [ValidateRange(.01,120)][double]$MeasurementSeconds = 30,
    [ValidateRange(1,10)][int]$Pairs = 5,
    [ValidateRange(1,4294967295)][long]$Seed = 1337,
    [ValidateRange(0,16)][int]$Workers = 0,
    [switch]$Smoke,
    [switch]$ProfilePhases,
    [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
if (-not $IsWindows) { throw 'The native cull benchmark requires Windows QPC.' }
$root = Split-Path $PSScriptRoot -Parent
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
if ([IO.Path]::IsPathRooted($BuildDirectory)) { $build = [IO.Path]::GetFullPath($BuildDirectory) }
if ($Smoke) {
    if (-not $PSBoundParameters.ContainsKey('Objects')) { $Objects=128 }
    if (-not $PSBoundParameters.ContainsKey('WarmupSeconds')) { $WarmupSeconds=.05 }
    if (-not $PSBoundParameters.ContainsKey('MeasurementSeconds')) { $MeasurementSeconds=.1 }
    if (-not $PSBoundParameters.ContainsKey('Pairs')) { $Pairs=2 }
} elseif ($WarmupSeconds -lt 5 -or $MeasurementSeconds -lt 30 -or $Pairs -lt 5) {
    throw 'Evidence runs require >=5 seconds warmup, >=30 seconds measurement and >=5 pairs. Use -Smoke for harness checks.'
}
if (-not $OutputDirectory) { $OutputDirectory=Join-Path $build ('benchmarks/cull-'+[DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ')) }
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Output directory already exists: $output" }
[void][IO.Directory]::CreateDirectory($output)
$manifestPath=Join-Path $output 'manifest.json'
$csvPath=Join-Path $output 'frames.csv'
$summaryPath=Join-Path $output 'summary.json'
$manifest=[ordered]@{
    schema_version=3
    status='RUNNING'; started_utc=[DateTime]::UtcNow.ToString('o'); scope='Headless native CPU preparation and result retirement; no GPU, draw, Present, Editor or image timings.'
    protocol_eligible=(-not $Smoke); fixture='seeded_static_interleaved_10_clusters_10_percent_visible'
    objects=$Objects; expected_visible=[int][Math]::Ceiling($Objects/10.0); seed=$Seed; workers=$Workers
    warmup_seconds=$WarmupSeconds; measurement_seconds=$MeasurementSeconds; pairs=$Pairs
    order='Alternating flat/spatial then spatial/flat by pair'; configuration=$Configuration
    timing='Windows QPC; lifecycle = preparation + retirement; counter extraction excluded; CSV and parity checks outside timed intervals'
    quantiles='Nearest rank per run; paired comparison uses median lifecycle per run; no pooled-frame speed claim'
    outliers='lifecycle > 2 times the run median'; platform=[Runtime.InteropServices.RuntimeInformation]::OSDescription
    processor=$env:PROCESSOR_IDENTIFIER; logical_processors=[Environment]::ProcessorCount
    gpu='none'; build_directory=$build; csv=$csvPath
    profile_phases=[bool]$ProfilePhases
}
function Save-Manifest { $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $manifestPath -Encoding utf8 }
$sourcePaths=@(
    'Tests/c/SceneJobContracts.cmake','Tests/c/WorkphoneGraphicsCullBenchmark.h',
    'Engine/c/Source/WorkphoneGraphics/graphics_cull_bounds.h','Tools/MeasureNativeCull.ps1'
)
$contractText=Get-Content -LiteralPath (Join-Path $root 'Tests/c/SceneJobContracts.cmake') -Raw
$sourcePaths += [regex]::Matches($contractText,'\$\{WP_JOB_ROOT\}/([^\s\)]+)') | ForEach-Object { $_.Groups[1].Value.Trim('"') }
foreach($directory in @('Engine/c/Include/WorkphoneCore','Engine/c/Include/WorkphoneGraphics')) {
    $sourcePaths += Get-ChildItem -LiteralPath (Join-Path $root $directory) -Filter '*.h' -File | ForEach-Object { $directory+'/'+$_.Name }
}
$sourcePaths=@($sourcePaths | Where-Object { Test-Path -LiteralPath (Join-Path $root $_) -PathType Leaf } | Sort-Object -Unique)
function Get-SourceHashes {
    @($sourcePaths | ForEach-Object { [ordered]@{ path=$_; sha256=(Get-FileHash -LiteralPath (Join-Path $root $_) -Algorithm SHA256).Hash } })
}
function Get-Distribution($values) {
    [double[]]$sorted=@($values); [Array]::Sort($sorted)
    if (-not $sorted.Count) { throw 'Empty measurement run.' }
    $median=$sorted[[Math]::Max(0,[int][Math]::Ceiling(.5*$sorted.Count)-1)]
    [ordered]@{
        p50_ms=$median/1e6
        p95_ms=$sorted[[Math]::Max(0,[int][Math]::Ceiling(.95*$sorted.Count)-1)]/1e6
        p99_ms=$sorted[[Math]::Max(0,[int][Math]::Ceiling(.99*$sorted.Count)-1)]/1e6
        outliers_above_2x_median=@($sorted | Where-Object { $_ -gt 2*$median }).Count
    }
}
try {
    $manifest.git_head_before=(& git -C $root rev-parse HEAD) -join ''
    $manifest.git_status_before=@(& git -C $root status --short)
    $before=Get-SourceHashes
    Save-Manifest
    if (-not $SkipBuild) {
        & cmake --build $build --config $Configuration --target WorkphoneGraphicsSceneJobTests -- /m:4 /verbosity:minimal *> (Join-Path $output 'build.log')
        if ($LASTEXITCODE -ne 0) { throw 'Native benchmark target build failed; see build.log.' }
    }
    $manifest.source_hashes=Get-SourceHashes
    if (($before | ConvertTo-Json -Compress) -ne ($manifest.source_hashes | ConvertTo-Json -Compress)) { throw 'Benchmark sources changed during build.' }
    $candidates=@(
        (Join-Path $build "Tests/cpp/GraphicsProduction/$Configuration/WorkphoneGraphicsSceneJobTests.exe"),
        (Join-Path $root "bin/windows/v145/x64/MD/$Configuration/WorkphoneGraphicsSceneJobTests.exe")
    )
    $exe=@($candidates | Where-Object { Test-Path -LiteralPath $_ }) | Select-Object -First 1
    if (-not $exe) { throw 'Native benchmark executable not found.' }
    $manifest.executable=$exe
    $manifest.executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    $manifest.compiler_flags=@(Get-Content -LiteralPath (Join-Path $build 'CMakeCache.txt') | Where-Object { $_ -match '^CMAKE_C_(COMPILER|FLAGS[^:]*):' })
    $manifest.skip_build=[bool]$SkipBuild
    $manifest.build_verified=(-not $SkipBuild)
    if ($SkipBuild) { $manifest.protocol_eligible=$false }
    Save-Manifest
    $culture=[Globalization.CultureInfo]::InvariantCulture
    $arguments=@('--cull-benchmark',$csvPath,$Objects.ToString($culture),$WarmupSeconds.ToString($culture),$MeasurementSeconds.ToString($culture),$Pairs.ToString($culture),$Seed.ToString($culture),$Workers.ToString($culture),([int][bool]$ProfilePhases).ToString($culture))
    Write-Host "Running $Pairs alternating CPU-only pairs: $Objects objects, ${WarmupSeconds}s warmup + ${MeasurementSeconds}s per variant. Evidence: $output"
    & $exe @arguments 2> (Join-Path $output 'run.log')
    if ($LASTEXITCODE -ne 0) { throw 'Native measurement or parity check failed; see run.log. Partial CSV is not valid evidence.' }
    $after=Get-SourceHashes
    if (($manifest.source_hashes | ConvertTo-Json -Compress) -ne ($after | ConvertTo-Json -Compress)) { throw 'Benchmark sources changed during measurement.' }
    if ($manifest.executable_sha256 -ne (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash) { throw 'Benchmark executable changed during measurement.' }
    $rows=@(Import-Csv -LiteralPath $csvPath)
    if (-not $rows.Count) { throw 'No raw samples produced.' }
    $runs=@(foreach($group in ($rows | Group-Object pair,mode)) {
        $first=$group.Group[0]
        if (@($group.Group | Where-Object { [int]$_.candidates -ne $Objects -or [int]$_.visible -ne $manifest.expected_visible }).Count) { throw 'Unexpected work/visibility in raw samples.' }
        $phases=[ordered]@{}
        if (@($group.Group | Where-Object { [int]$_.phases_available -ne [int][bool]$ProfilePhases }).Count) { throw 'Unexpected phase timing availability.' }
        if ($ProfilePhases) {
            foreach($name in @('capture','transform','bounds','groups','view','visibility','sort','publish',
                'transform_hierarchy','transform_unique','transform_locals','transform_prefixes','transform_submit','transform_wait')) {
                $phases[$name]=Get-Distribution ($group.Group | ForEach-Object { [double]::Parse($_.($name+'_ns'),$culture) })
            }
        }
        [ordered]@{
            pair=[int]$first.pair; order=[int]$first.order; mode=$first.mode; frames=$group.Count
            phases=$phases
            prepare=(Get-Distribution ($group.Group | ForEach-Object { [double]::Parse($_.prepare_ns,$culture) }))
            retire=(Get-Distribution ($group.Group | ForEach-Object { [double]::Parse($_.retire_ns,$culture) }))
            lifecycle=(Get-Distribution ($group.Group | ForEach-Object { [double]::Parse($_.lifecycle_ns,$culture) }))
            work=$first | Select-Object candidates,visible,multiplies,sphere_tests,box_tests,sat_tests,group_tests,group_rejections,group_inside,publication_bytes,view_peak_bytes,map_bytes,class_bytes
        }
    })
    if ($runs.Count -ne 2*$Pairs) { throw 'Incomplete paired run set.' }
    $paired=@(for($pair=1;$pair -le $Pairs;++$pair) {
        $flat=@($runs | Where-Object { $_.pair -eq $pair -and $_.mode -eq 'flat' })[0]
        $spatial=@($runs | Where-Object { $_.pair -eq $pair -and $_.mode -eq 'spatial' })[0]
        if ($flat.order -ne (($pair-1)%2) -or $spatial.order -ne (1-(($pair-1)%2))) { throw 'Unexpected A/B order.' }
        [ordered]@{ pair=$pair; flat_median_ms=$flat.lifecycle.p50_ms; spatial_median_ms=$spatial.lifecycle.p50_ms; spatial_change_percent=100*($spatial.lifecycle.p50_ms/$flat.lifecycle.p50_ms-1) }
    })
    $report=[ordered]@{ schema_version=3; scope=$manifest.scope; profile_phases=[bool]$ProfilePhases; protocol_eligible=$manifest.protocol_eligible; runs=$runs; paired=$paired; raw_samples=$rows.Count; interpretation='CPU fixture evidence only. Lower geometry work does not establish GPU/Editor or representative frame-time improvement.' }
    $report | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $summaryPath -Encoding utf8
    $manifest.git_head_after=(& git -C $root rev-parse HEAD) -join ''
    $manifest.git_status_after=@(& git -C $root status --short)
    $manifest.csv_sha256=(Get-FileHash -LiteralPath $csvPath -Algorithm SHA256).Hash
    $manifest.summary_sha256=(Get-FileHash -LiteralPath $summaryPath -Algorithm SHA256).Hash
    $manifest.status='COMPLETE'; $manifest.completed_utc=[DateTime]::UtcNow.ToString('o'); Save-Manifest
    $paired | ForEach-Object { Write-Host ("pair {0}: flat {1:F3} ms, spatial {2:F3} ms ({3:+0.00;-0.00;0.00}%)" -f $_.pair,$_.flat_median_ms,$_.spatial_median_ms,$_.spatial_change_percent) }
    Write-Host "Summary: $summaryPath"
} catch {
    $manifest.status='FAILED'; $manifest.error=$_.Exception.Message; $manifest.completed_utc=[DateTime]::UtcNow.ToString('o'); Save-Manifest
    throw
}
