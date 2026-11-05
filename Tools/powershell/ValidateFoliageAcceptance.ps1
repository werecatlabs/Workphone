#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release')][string]$Configuration = 'RelWithDebInfo',
    [ValidateSet('GF0', 'GF1')][string]$Tier = 'GF0',
    [switch]$Diagnostic,
    [ValidateRange(2, 10000)][int]$Frames = 120,
    [string]$CTestExecutable = 'ctest'
)

$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Use a new evidence directory: $output" }
New-Item -ItemType Directory -Path $output | Out-Null
. (Join-Path $PSScriptRoot 'GraphicsValidationEvidence.ps1')

$testNames = @(
    'WPGraphics.claw_production_render', 'WPGraphics.foliage_authoring',
    'WPGraphics.foliage_plant_geometry', 'WPGraphics.foliage_contracts',
    'WPGraphics.terrain_lua_workflow', 'WPGraphics.cooked_material_resource',
    'WorkphoneAssets.typed_package_families'
)
$selection = '^(' + (($testNames | ForEach-Object { [Regex]::Escape($_) }) -join '|') + ')$'
$ctestArgs = @('--test-dir', $build, '-C', $Configuration, '-R', $selection)
$inventoryText = & $CTestExecutable @ctestArgs --show-only=json-v1
if ($LASTEXITCODE) { throw 'CTest could not enumerate foliage acceptance dependencies.' }
$inventory = ($inventoryText -join "`n") | ConvertFrom-Json
$missing = @($testNames | Where-Object { $_ -notin $inventory.tests.name })
if ($missing.Count) { throw "Missing foliage acceptance tests: $($missing -join ', ')" }
$hostPath = ($inventory.tests | Where-Object name -eq 'WPGraphics.claw_production_render').command[0]
$hostDirectory = Split-Path -Parent $hostPath
$runtimePaths = @($inventory.tests.command | Where-Object {
    [IO.Path]::IsPathRooted($_) -and (Test-Path -LiteralPath $_ -PathType Leaf)
}) + @(Get-ChildItem -LiteralPath $hostDirectory -Filter '*.dll' -File | ForEach-Object FullName)

function Get-RuntimeSnapshot {
    $result = @{}
    foreach ($file in ($runtimePaths | Sort-Object -Unique)) {
        $result[$file] = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
    }
    $result
}

function Get-Percentiles($Values) {
    $sorted = @($Values | Sort-Object)
    if (!$sorted.Count) { return $null }
    [ordered]@{
        samples = $sorted.Count
        p50 = $sorted[[Math]::Ceiling(.50 * $sorted.Count) - 1]
        p95 = $sorted[[Math]::Ceiling(.95 * $sorted.Count) - 1]
        p99 = $sorted[[Math]::Ceiling(.99 * $sorted.Count) - 1]
        maximum = $sorted[-1]
    }
}

$sourceBefore = Get-GraphicsValidationSourceSnapshot $repository
foreach ($extra in @($PSCommandPath, (Join-Path $repository 'bin/Media/Scripts/Lua/Editor/TerrainEditor.lua'))) {
    $sourceBefore[$extra] = (Get-FileHash -LiteralPath $extra -Algorithm SHA256).Hash
}
$runtimeBefore = Get-RuntimeSnapshot
$revision = & git -C $repository rev-parse HEAD
$sourceStatus = @(& git -C $repository status --short)
$junitPath = Join-Path $output 'contracts.xml'
& $CTestExecutable @ctestArgs --output-on-failure --output-junit $junitPath 2>&1 |
    Tee-Object -FilePath (Join-Path $output 'contracts.log')
$contractsExit = $LASTEXITCODE
$contracts = @{}
if (Test-Path -LiteralPath $junitPath -PathType Leaf) {
    $junit = [xml](Get-Content -Raw -LiteralPath $junitPath)
    foreach ($test in $junit.SelectNodes('//testcase')) {
        $contracts[$test.GetAttribute('name')] = if ($test.SelectSingleNode('failure|error')) { 'failed' }
            elseif ($test.SelectSingleNode('skipped') -or $test.GetAttribute('status') -ne 'run') { 'unavailable' }
            else { 'passed' }
    }
}
$contractsPassed = @($testNames | Where-Object { $contracts[$_] -eq 'passed' }).Count -eq $testNames.Count

$soakSeconds = if ($Tier -eq 'GF0') { 0 } elseif ($Diagnostic) { 10 } else { 28800 }
$forestLog = Join-Path $output 'forest.log'
& $hostPath --foliage-acceptance $Frames $soakSeconds $output 2>&1 | Tee-Object -FilePath $forestLog
$forestExit = $LASTEXITCODE
$lines = Get-Content -LiteralPath $forestLog
$columns = @('frame','prepareCpuMs','submitCpuMs','gpuMs','colourDraws','shadowDraws','reflectionDraws',
    'triangles','uploadBytes','mainPixelShaderInvocations','mainCoveredPixels','residentPages','residentInstances','splits','pagingSettleMs')
$rows = @($lines | Where-Object { $_ -like 'FOLIAGE_FRAME,*' } | ForEach-Object { $_.Substring(14) } |
    ConvertFrom-Csv -Header $columns)
if ($rows.Count) { $rows | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $output 'forest-frames.csv') }
$matchedColumns = @('mode','draws','triangles','geometryBytes','instanceUploadBytes','coveredPixels','p50CpuSubmitMs','setupMs')
$matched = @($lines | Where-Object { $_ -like 'FOLIAGE_MATCHED,*' } | ForEach-Object { $_.Substring(16) } |
    ConvertFrom-Csv -Header $matchedColumns)
if ($matched.Count) { $matched | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $output 'forest-matched.csv') }
$grass = @($lines | Where-Object { $_ -like 'FOLIAGE_GRASS,*' } | ForEach-Object { $_.Substring(14) } |
    ConvertFrom-Csv -Header @('frame','gpuMs','draws','pixelShaderInvocations','coveredPixels'))
if ($grass.Count) { $grass | Export-Csv -NoTypeInformation -LiteralPath (Join-Path $output 'grass-frames.csv') }

$sourceAfter = Get-GraphicsValidationSourceSnapshot $repository
foreach ($extra in @($PSCommandPath, (Join-Path $repository 'bin/Media/Scripts/Lua/Editor/TerrainEditor.lua'))) {
    $sourceAfter[$extra] = (Get-FileHash -LiteralPath $extra -Algorithm SHA256).Hash
}
$sourceChanges = @(Compare-GraphicsValidationSourceSnapshots $sourceBefore $sourceAfter)
$runtimeChanges = @(Compare-GraphicsValidationSourceSnapshots $runtimeBefore (Get-RuntimeSnapshot))
$flight = @($lines | Where-Object { $_ -like 'FOLIAGE_FLIGHT,*' } | Select-Object -Last 1)
$flightFields = if ($flight.Count) { $flight[0].Split(',') } else { @() }
$physicalGpu = $lines -contains 'FOLIAGE_ADAPTER,physical'
$flightMetrics = if ($flightFields.Count -eq 7) {
    [ordered]@{
        frames = [long]$flightFields[1]; durationSeconds = [double]$flightFields[2]
        peakSampledSchedulerCpuBytes = [long]$flightFields[3]
        peakSampledSchedulerGpuBytes = [long]$flightFields[4]
        peakFrameUploadBytes = [long]$flightFields[5]
        peakSampledLocalVideoMemoryBytes = if ($physicalGpu -and [long]$flightFields[6] -gt 0) {
            [long]$flightFields[6]
        } else { $null }
    }
} else { $null }
$gpu = Get-Percentiles @($rows | ForEach-Object { [double]$_.gpuMs } | Where-Object { $_ -ge 0 })
$cpu = Get-Percentiles @($rows | ForEach-Object { [double]$_.prepareCpuMs + [double]$_.submitCpuMs })
$validEvidence = !$sourceChanges.Count -and !$runtimeChanges.Count
$automatedPassed = $contractsExit -eq 0 -and $contractsPassed -and $forestExit -eq 0 -and $validEvidence -and
    $rows.Count -ge $Frames -and $matched.Count -eq 3 -and $grass.Count -eq 24 -and $lines -contains 'FOLIAGE_RECOVERY,passed'
$performance = if (!$physicalGpu -or !$gpu -or !$cpu -or $gpu.samples -ne $rows.Count) { 'unavailable' }
    elseif ($gpu.p95 -le 16.7 -and $cpu.p95 -le 4.0) { 'within-parent-budget' } else { 'over-parent-budget' }
$soak = if ($Tier -eq 'GF0') { 'not-required' }
    elseif (!$Diagnostic -and $physicalGpu -and $forestExit -eq 0 -and $flightMetrics -and
            $flightMetrics.durationSeconds -ge 28800) { 'completed' } else { 'incomplete' }

[ordered]@{
    schemaVersion = 1; tier = $Tier; diagnostic = [bool]$Diagnostic; configuration = $Configuration
    revision = $revision; sourceStatus = $sourceStatus; stable = $validEvidence
    sourceChanges = $sourceChanges; runtimeChanges = $runtimeChanges
    fixture = @($lines | Where-Object { $_ -like 'FOLIAGE_FIXTURE,*' -or $_ -like 'FOLIAGE_MATCHED_FIXTURE,*' })
    device = @($lines | Where-Object { $_ -match '^DX11 (device|driver)' } | Select-Object -Unique)
    physicalGpu = $physicalGpu; contractsExit = $contractsExit; contracts = $contracts; forestExit = $forestExit
    automatedPassed = $automatedPassed; releaseCertified = $false
    performance = $performance; scope = '1080p offscreen foliage passes, excluding the full game scene'
    cpuPrepareMs = (Get-Percentiles @($rows | ForEach-Object { [double]$_.prepareCpuMs }))
    cpuSubmitMs = (Get-Percentiles @($rows | ForEach-Object { [double]$_.submitCpuMs }))
    cpuCombinedMs = $cpu; gpuMs = $gpu
    pagingSettleMs = (Get-Percentiles @($rows | ForEach-Object { [double]$_.pagingSettleMs } | Where-Object { $_ -gt 0 }))
    grassGpuMs = (Get-Percentiles @($grass | ForEach-Object { [double]$_.gpuMs } | Where-Object { $_ -ge 0 }))
    grassOverdrawEstimate = @($grass | Where-Object { [long]$_.coveredPixels -gt 0 } | ForEach-Object {
        [ordered]@{ frame = [long]$_.frame; pixelShaderInvocationsPerCoveredPixel =
            [double]$_.pixelShaderInvocations / [long]$_.coveredPixels }
    })
    grazingCoverageEstimate = @($rows | Where-Object { [long]$_.mainCoveredPixels -gt 0 } | ForEach-Object {
        [ordered]@{ frame = [long]$_.frame; pixelShaderInvocationsPerCoveredPixel =
            [double]$_.mainPixelShaderInvocations / [long]$_.mainCoveredPixels }
    })
    flight = $flightMetrics; soak = $soak
    captures = @(Get-ChildItem -LiteralPath $output -Filter '*.bmp' -File | ForEach-Object {
        [ordered]@{ path = $_.Name; sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
    })
    remainingReleaseEvidence = @('interactive Editor and visual review', 'representative installed scene on target Windows',
        'full mixed-scene whole-renderer release gate')
    sources = @($sourceBefore.Keys | Sort-Object | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($repository, $_); sha256 = $sourceBefore[$_] }
    })
    runtime = @($runtimeBefore.Keys | Sort-Object | ForEach-Object {
        [ordered]@{ path = [IO.Path]::GetRelativePath($repository, $_); sha256 = $runtimeBefore[$_] }
    })
} | ConvertTo-Json -Depth 7 | Set-Content -Encoding utf8 -LiteralPath (Join-Path $output 'foliage-acceptance.json')

if (!$automatedPassed) { throw "Foliage acceptance checks failed or evidence is incomplete. See $output" }
if (!$Diagnostic -and ($performance -ne 'within-parent-budget' -or $soak -eq 'incomplete')) {
    throw "Foliage release measurements did not meet the requested tier. See $output"
}
