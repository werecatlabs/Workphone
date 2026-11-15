[CmdletBinding()]
param(
    [ValidateSet('Debug', 'RelWithDebInfo')][string]$Configuration = 'Debug',
    [string]$BinaryDirectory,
    [string]$OutputDirectory,
    [ValidateRange(0, 4294967295)][long]$Seed = 42,
    [ValidateSet('low', 'medium', 'high', 'cinematic')][string]$Quality = 'high',
    [ValidateSet('circuit', 'city')][string]$Scene = 'circuit',
    [ValidateSet(6, 8, 10)][int]$CityBlocks = 8,
    [ValidateRange(0, 2)][int]$CityRoute = 0,
    [ValidateSet('grid', 'lsystem')][string]$CityLayout = 'grid',
    [ValidatePattern('^\d+x\d+$')][string]$Resolution = '1280x720',
    [ValidateSet('auto', 'none', 'msaa2', 'msaa4')][string]$SceneAA = 'auto',
    [ValidateRange(0.1, 3600)][double]$Seconds = 10,
    [ValidateSet('follow', 'car', 'track', 'corner', 'junction', 'hillside', 'verge', 'street', 'boulevard', 'frontage', 'facade', 'building', 'utilities', 'park', 'shopfront', 'road-edit-before', 'road-edit-after')][string[]]$Views = @('car', 'track', 'corner', 'follow'),
    [string]$Replay,
    [switch]$NoEffects,
    [switch]$FoliageWorld,
    [switch]$RoadAuthoringShowcase,
    [switch]$CityTerrainRoads,
    [ValidateRange(0, 2)][double]$CityTerrainRelief = 1,
    [switch]$ExportMaterials,
    [ValidateRange(30, 7200)][int]$TimeoutSeconds = 600,
    [ValidateRange(0, 100)][int]$CityStreetDetail = 100,
    [ValidateSet('district', 'masonry', 'civic', 'modern', 'terrace', 'parisian', 'warehouse', 'mediterranean', 'artdeco', 'timber')][string]$CityBuildingStyle,
    [hashtable]$CityBuildingMeshes = @{}
)
$ErrorActionPreference = 'Stop'
$streetDetailExplicit = $PSBoundParameters.ContainsKey('CityStreetDetail')
if ($RoadAuthoringShowcase) { $Scene = 'city' }
if ($CityTerrainRoads) { $Scene = 'city' }
if ($streetDetailExplicit -and !$CityTerrainRoads) { throw '-CityStreetDetail requires -CityTerrainRoads.' }
if ($Views -contains 'frontage' -and !$CityTerrainRoads) { throw '-Views frontage requires -CityTerrainRoads.' }
if ($Views -contains 'utilities' -and !$CityTerrainRoads) { throw '-Views utilities requires -CityTerrainRoads.' }
if ($Views -contains 'park' -and !$CityTerrainRoads) { throw '-Views park requires -CityTerrainRoads.' }
if ($Views -contains 'shopfront' -and !$CityTerrainRoads) { throw '-Views shopfront requires -CityTerrainRoads.' }
if ($Views -contains 'junction' -and !$CityTerrainRoads) { throw '-Views junction requires -CityTerrainRoads.' }
if ($Views -contains 'verge' -and !$CityTerrainRoads) { throw '-Views verge requires -CityTerrainRoads.' }
if (($Views -contains 'hillside' -or $PSBoundParameters.ContainsKey('CityTerrainRelief')) -and !$CityTerrainRoads) { throw 'The hillside view and terrain relief require -CityTerrainRoads.' }
$roadEditViews = @($Views | Where-Object { $_ -in @('road-edit-before', 'road-edit-after') })
if ($roadEditViews.Count -and !$CityTerrainRoads) { throw 'Road edit views require -CityTerrainRoads.' }
if (($CityBuildingStyle -or $CityBuildingMeshes.Count -or $Views -contains 'facade' -or $Views -contains 'building') -and
    ($Scene -ne 'city' -or $RoadAuthoringShowcase)) { throw 'Building styles, mesh overrides, and the facade/building views require a generated city.' }
if ($RoadAuthoringShowcase -and $CityTerrainRoads) { throw 'Choose -RoadAuthoringShowcase or -CityTerrainRoads.' }
if ($CityTerrainRoads -and $ExportMaterials) { throw 'Terrain-city source material export is unavailable.' }
if ($RoadAuthoringShowcase -and $FoliageWorld) { throw 'Road authoring showcase does not provide foliage-world generation.' }
if ($RoadAuthoringShowcase -and !$PSBoundParameters.ContainsKey('Views')) { $Views = @('street', 'track') }
if ($CityTerrainRoads -and !$PSBoundParameters.ContainsKey('Views')) { $Views = @('street', 'track') }
if ($Scene -ne 'city' -and ($Views -contains 'street' -or $Views -contains 'boulevard' -or $PSBoundParameters.ContainsKey('CityBlocks') -or $PSBoundParameters.ContainsKey('CityRoute') -or $PSBoundParameters.ContainsKey('CityLayout'))) {
    throw 'Street-level views and city layout options require -Scene city.'
}
$repoRoot = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$buildingMeshRoles = @('wall', 'window', 'entrance', 'corner', 'roof')
$buildingMeshPaths = [ordered]@{}
foreach ($entry in $CityBuildingMeshes.GetEnumerator()) {
    if ($entry.Key -isnot [string] -or $entry.Key -notin $buildingMeshRoles -or $entry.Value -isnot [string] -or
        [string]::IsNullOrWhiteSpace($entry.Value)) {
        throw '-CityBuildingMeshes requires wall, window, entrance, corner, or roof keys and nonempty file paths.'
    }
    $role = $entry.Key.ToLowerInvariant()
    if ($buildingMeshPaths.Contains($role)) { throw "Duplicate city building mesh role: $role" }
    $buildingMeshPaths[$role] = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($entry.Value)
}
$sampleWorkingDirectory = Join-Path $repoRoot "bin/windows/v145/x64/MD/$Configuration"
$binaryRoot = $sampleWorkingDirectory
if ($BinaryDirectory) {
    $binaryRoot = (Resolve-Path -LiteralPath $BinaryDirectory).Path
    if (!(Test-Path -LiteralPath $binaryRoot -PathType Container)) { throw 'BinaryDirectory must be an existing build snapshot directory.' }
}
$executable = Join-Path $binaryRoot 'SampleVehicleAdvanced.exe'
$plugins = Join-Path $binaryRoot 'wp_plugins_samples.cfg'
if (!(Test-Path -LiteralPath $executable)) { throw "Build SampleVehicleAdvanced $Configuration first." }
if (!(Test-Path -LiteralPath $plugins)) { throw "Missing sample plugin configuration: $plugins" }
if ($BinaryDirectory) {
    $pluginFolders = @(Get-Content -LiteralPath $plugins | Where-Object { $_ -match '^\s*PluginFolder\s*=' })
    if ($pluginFolders.Count -ne 1) { throw 'Retained binaries require one explicit PluginFolder.' }
    $pluginFolder = ($pluginFolders[0] -split '=', 2)[1].Trim()
    if (![IO.Path]::IsPathRooted($pluginFolder)) { $pluginFolder = Join-Path $sampleWorkingDirectory $pluginFolder }
    if ([IO.Path]::GetFullPath($pluginFolder).TrimEnd('\', '/') -ine $binaryRoot.TrimEnd('\', '/')) {
        throw 'Retained PluginFolder must resolve to BinaryDirectory so captures cannot mix plugin builds.'
    }
}
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
$sourceRoots = @('Engine/cpp/Include/WPProcedural', 'Engine/cpp/Source/WPProcedural', 'Engine/cpp/Include/WPGraphics', 'Engine/cpp/Source/WPGraphics', 'Engine/cpp/Include/WPImGui', 'Engine/cpp/Source/WPImGui', 'Engine/cpp/Include/Workphone/Graphics', 'Engine/cpp/Source/Workphone/Graphics', 'Engine/cpp/Include/Workphone/Interface/Graphics', 'Engine/cpp/Include/Workphone/Interface/Procedural', 'Engine/cpp/Source/Workphone/Interface/Graphics', 'Engine/cpp/Include/Workphone/Scene', 'Engine/cpp/Source/Workphone/Scene', 'Engine/c/Include/WorkphoneGraphics', 'Engine/c/Source/WorkphoneGraphics', 'Engine/c/Include/WorkphonePlatformWin32', 'Engine/c/Source/WorkphonePlatformWin32', 'Samples/cpp/VehicleAdvanced')
$sourceRoots += @('Engine/c/Include/WorkphonePhysics', 'Engine/c/Source/WorkphonePhysics')
$sources = @($sourceRoots | ForEach-Object { Get-ChildItem -LiteralPath (Join-Path $repoRoot $_) -Recurse -File | Where-Object { $_.Extension -in '.cpp', '.c', '.hpp', '.h', '.txt' } } | Sort-Object FullName -Unique | ForEach-Object { Get-Identity $_.FullName })
$sources += @('Engine/cpp/Source/Workphone/System/FSM.cpp', 'Engine/cpp/Source/Workphone/System/FSMManager.cpp', 'Engine/cpp/Include/Workphone/System/FSM.hpp', 'Engine/cpp/Include/Workphone/System/FSMManager.hpp') | ForEach-Object { Get-Identity (Join-Path $repoRoot $_) }
$binaries = @(Get-ChildItem -LiteralPath $binaryRoot -File | Where-Object { $_.Extension -eq '.dll' -or $_.Name -eq 'SampleVehicleAdvanced.exe' } | Sort-Object Name | ForEach-Object { Get-Identity $_.FullName })
$assets = @(Get-ChildItem -LiteralPath (Join-Path $repoRoot 'bin/Media') -Recurse -File | Where-Object { $_.Extension -in '.png', '.jpg', '.jpeg', '.tga', '.dds', '.mesh', '.hdr', '.texres', '.matres', '.folspec', '.folworld' } | Sort-Object FullName | ForEach-Object { Get-Identity $_.FullName })
$manifest = [ordered]@{
    schema = 'workphone.procedural-capture-set.v1'; startedUtc = [DateTime]::UtcNow.ToString('o')
    revision = $revision; configuration = $Configuration; seed = $Seed; quality = $Quality
    binaryDirectory = $binaryRoot; retainedBinaries = [bool]$BinaryDirectory; assetWorkingDirectory = $sampleWorkingDirectory
    scene = $Scene; cityBlocks = $(if ($Scene -eq 'city' -and !$RoadAuthoringShowcase) { $CityBlocks } else { $null }); cityRoute = $(if ($Scene -eq 'city' -and !$RoadAuthoringShowcase) { $CityRoute } else { $null })
    cityLayout = $(if ($RoadAuthoringShowcase) { 'authored-splines' } elseif ($Scene -eq 'city') { $CityLayout } else { $null })
    roadAuthoringShowcase = [bool]$RoadAuthoringShowcase
    cityTerrainRoads = [bool]$CityTerrainRoads
    cityTerrainRelief = $(if ($CityTerrainRoads) { $CityTerrainRelief } else { $null })
    cityStreetDetail = $(if ($streetDetailExplicit) { $CityStreetDetail } else { $null })
    cityBuildingStyle = $(if ($CityBuildingStyle) { $CityBuildingStyle.ToLowerInvariant() } else { $null })
    cityBuildingMeshes = $buildingMeshPaths
    cityBuildingMeshInputs = [ordered]@{}
    requestedResolution = $Resolution; sceneAA = $SceneAA; benchmarkSeconds = $Seconds; visualAcceptance = 'unreviewed'
    diff = Get-Identity (Join-Path $output 'working-tree.patch'); sources = $sources; binaries = $binaries
    plugins = Get-Identity $plugins; assets = $assets; replay = $null; runs = @()
    limitations = @('Fresh build must be performed separately; hashes identify artifacts, not freshness.', 'Control replay applies one row per physics-task tick; the task targets 120 Hz but physics uses measured deltas.', 'Asset hashes inventory the media tree; they are not a runtime dependency trace.', 'No blind comparison or visual acceptance is implied.')
}
foreach ($role in $buildingMeshRoles) {
    if ($buildingMeshPaths.Contains($role) -and (Test-Path -LiteralPath $buildingMeshPaths[$role] -PathType Leaf)) {
        $manifest.cityBuildingMeshInputs[$role] = Get-Identity $buildingMeshPaths[$role]
    }
}
if ($Replay) { $manifest.replay = Get-Identity $Replay }
$failed = $false
try {
    foreach ($view in $Views) {
        $capture = Join-Path $output "$view.bmp"
        $evidence = Join-Path $output "$view.json"
        $arguments = @('--seed', "$Seed", '--quality', $Quality, '--plugins', (Quote-ProcessArgument $plugins), '--capture', (Quote-ProcessArgument $capture), '--view', $view, '--no-hud', '--resolution', $Resolution, '--no-audio', '--benchmark', $Seconds.ToString([Globalization.CultureInfo]::InvariantCulture), '--evidence', (Quote-ProcessArgument $evidence))
        if ($Scene -eq 'city') { $arguments += @('--city', '--city-blocks', "$CityBlocks", '--city-route', "$CityRoute", '--city-layout', $CityLayout) }
        if ($SceneAA -ne 'auto') { $arguments += @('--scene-aa', $SceneAA) }
        if ($RoadAuthoringShowcase) { $arguments += '--road-authoring-showcase' }
        if ($CityTerrainRoads) { $arguments += @('--city-terrain-roads', '--city-terrain-relief', $CityTerrainRelief.ToString([Globalization.CultureInfo]::InvariantCulture)) }
        if ($streetDetailExplicit) { $arguments += @('--city-street-detail', "$CityStreetDetail") }
        if ($CityBuildingStyle) { $arguments += @('--city-building-style', $CityBuildingStyle.ToLowerInvariant()) }
        foreach ($role in $buildingMeshRoles) {
            if ($buildingMeshPaths.Contains($role)) {
                $arguments += @('--city-building-mesh', $role, (Quote-ProcessArgument $buildingMeshPaths[$role]))
            }
        }
        if ($Replay) { $arguments += @('--replay', (Quote-ProcessArgument $Replay)) }
        if ($NoEffects) { $arguments += '--no-effects' }
        if ($FoliageWorld) { $arguments += '--foliage-world' }
        if ($ExportMaterials) { $arguments += @('--export-materials', (Quote-ProcessArgument (Join-Path $output "$view-materials"))) }
        Write-Host "Capturing $Scene/$view ($Quality, seed $Seed)"
        $process = Start-Process -FilePath $executable -ArgumentList $arguments -WorkingDirectory $sampleWorkingDirectory -WindowStyle Hidden -RedirectStandardOutput (Join-Path $output "$view.stdout.log") -RedirectStandardError (Join-Path $output "$view.stderr.log") -PassThru
        $timedOut = !$process.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) { $process.Kill(); $process.WaitForExit() }
        $process.Refresh()
        $run = [ordered]@{ view = $view; arguments = $arguments; exitCode = $process.ExitCode; timedOut = $timedOut; capture = $null; evidence = $null; passed = $false }
        if (Test-Path -LiteralPath $capture) { $run.capture = Get-Identity $capture }
        if (Test-Path -LiteralPath $evidence) {
            $run.evidence = Get-Content -LiteralPath $evidence -Raw | ConvertFrom-Json
            $run.passed = !$timedOut -and $process.ExitCode -eq 0 -and $null -ne $run.capture -and $run.evidence.validationPassed
            if ($Scene -eq 'city' -and !$RoadAuthoringShowcase) {
                $buildings = $run.evidence.cityBuildingGeometry
                $run.passed = $run.passed -and $null -ne $buildings -and $buildings.ready -eq $true -and
                    $buildings.meshes -gt 0 -and $buildings.readyMeshes -eq $buildings.meshes
            }
            if ($streetDetailExplicit) {
                $street = $run.evidence.terrainCity
                $run.passed = $run.passed -and $null -ne $street -and $street.streetDetailValid -eq $true -and
                    $null -ne $street.cityStreetDetailCount -and $street.cityStreetDetail -eq $CityStreetDetail
            }
            if ($view -eq 'utilities') {
                $utility = $run.evidence.cityUtilitiesCamera
                $run.passed = $run.passed -and $null -ne $utility -and $utility.valid -eq $true -and
                    $utility.primaryPresent -eq $true -and $utility.nativeReady -eq $true
            }
            if ($view -eq 'park') {
                $park = $run.evidence.cityParkCamera
                $run.passed = $run.passed -and $null -ne $park -and $park.valid -eq $true -and [bool]$park.lotId
            }
            if ($view -eq 'shopfront') {
                $run.passed = $run.passed -and $run.evidence.cityShopfrontCameraValid -eq $true -and
                    [bool]$run.evidence.cityFacadeCamera.lotId
            }
            if ($view -in $roadEditViews) {
                $edit = $run.evidence.roadTerrainEdit
                $expectedStage = if ($view -eq 'road-edit-before') { 'undo' } else { 'redo' }
                $run.passed = $run.passed -and $null -ne $edit -and $edit.valid -eq $true -and
                    $edit.stage -eq $expectedStage -and $edit.changedSamples -gt 0 -and $edit.collisionMatched -eq $true -and
                    $edit.moveRestoredSamples -gt 0 -and $edit.deleteRestoredSamples -gt 0
            }
            $kit = $run.evidence.cityBuildingKit
            if ($CityBuildingStyle) {
                $run.passed = $run.passed -and $null -ne $kit -and $kit.styleApplied -eq $true -and $kit.style -ieq $CityBuildingStyle
            }
            if ($buildingMeshPaths.Count) {
                $run.passed = $run.passed -and $null -ne $kit -and $kit.overridesApplied -eq $true -and
                    $kit.requestedOverrides -eq $buildingMeshPaths.Count -and $kit.moduleCount -gt 0
                foreach ($role in $buildingMeshPaths.Keys) {
                    $appliedRole = $kit.roles.$role
                    $run.passed = $run.passed -and $null -ne $appliedRole -and
                        $appliedRole.path -ieq $buildingMeshPaths[$role] -and $appliedRole.placements -gt 0
                }
            }
        }
        $manifest.runs += $run
        if (!$run.passed) { $failed = $true }
    }
    if ($roadEditViews -contains 'road-edit-before' -and $roadEditViews -contains 'road-edit-after') {
        $before = ($manifest.runs | Where-Object { $_.view -eq 'road-edit-before' }).evidence.roadTerrainEdit
        $after = ($manifest.runs | Where-Object { $_.view -eq 'road-edit-after' }).evidence.roadTerrainEdit
        $manifest.matchedRoadEdit = $null -ne $before -and $null -ne $after -and
            $before.roadIndex -eq $after.roadIndex -and $before.changedSamples -eq $after.changedSamples -and
            $before.moveRestoredSamples -eq $after.moveRestoredSamples -and
            $before.deleteRestoredSamples -eq $after.deleteRestoredSamples -and
            ($before.eye -join ',') -eq ($after.eye -join ',') -and ($before.target -join ',') -eq ($after.target -join ',') -and
            ($before.oldKnot -join ',') -eq ($after.oldKnot -join ',') -and ($before.newKnot -join ',') -eq ($after.newKnot -join ',') -and
            ($before.probe -join ',') -eq ($after.probe -join ',')
        if (!$manifest.matchedRoadEdit) { $failed = $true }
    }
} finally {
    $manifest.completedUtc = [DateTime]::UtcNow.ToString('o')
    $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding utf8
}
if ($failed) { throw "Capture validation failed; inspect $output" }
Write-Host "Captured evidence: $output"
