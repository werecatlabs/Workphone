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
    [ValidateSet('follow', 'car', 'track', 'corner', 'junction', 'hillside', 'verge', 'street', 'boulevard', 'traffic', 'frontage', 'facade', 'building', 'street-lamp', 'street-bin', 'utilities', 'road-profile', 'park', 'shopfront', 'entry-route', 'rooftop', 'pitched-roof', 'road-edit-before', 'road-edit-after', 'imposter-front', 'imposter-diagonal', 'imposter-back', 'imposter-high')][string[]]$Views = @('car', 'track', 'corner', 'follow'),
    [string]$Replay,
    [switch]$NoEffects,
    [switch]$FoliageWorld,
    [ValidateSet('auto', '0', '1', '2')][string]$FoliageLOD = 'auto',
    [switch]$FoliageLODSmokeTest,
    [switch]$FoliageMotionCapture,
    [switch]$RoadAuthoringShowcase,
    [switch]$CityTerrainRoads,
    [ValidateRange(0, 2)][double]$CityTerrainRelief = 1,
    [switch]$ExportMaterials,
    [ValidateRange(30, 7200)][int]$TimeoutSeconds = 600,
    [ValidateRange(0, 100)][int]$CityStreetDetail = 100,
    [ValidateRange(0, 200)][int]$CityStreetPropDensity = 150,
    [ValidateRange(0, 100)][int]$CityTrafficDensity = 35,
    [switch]$CityTrafficDriveOnLeft,
    [switch]$CityDebugBlocks,
    [switch]$CityDebugLots,
    [ValidateSet('on', 'off')][string]$CityStreaming = 'off',
    [ValidateRange(128, 4096)][int]$CitySourceMemoryMiB = 1024,
    [switch]$CityStreamingSmokeTest,
    [ValidateSet('district', 'masonry', 'civic', 'modern', 'terrace', 'parisian', 'warehouse', 'mediterranean', 'artdeco', 'timber')][string]$CityBuildingStyle,
    [hashtable]$CityBuildingMeshes = @{}
)
$ErrorActionPreference = 'Stop'
$streetDetailExplicit = $PSBoundParameters.ContainsKey('CityStreetDetail')
$streetPropDensityExplicit = $PSBoundParameters.ContainsKey('CityStreetPropDensity')
if ($RoadAuthoringShowcase) { $Scene = 'city' }
if ($CityTerrainRoads) { $Scene = 'city' }
if ($FoliageLOD -ne 'auto' -and !($FoliageWorld -or $CityTerrainRoads)) { throw '-FoliageLOD requires -FoliageWorld or -CityTerrainRoads.' }
if ($FoliageLODSmokeTest -and (!($FoliageWorld -or $CityTerrainRoads) -or $CityStreamingSmokeTest -or $RoadAuthoringShowcase -or $Replay)) {
    throw '-FoliageLODSmokeTest requires paged foliage, without another smoke test, road showcase or replay.'
}
if ($CityStreamingSmokeTest -and ($Scene -ne 'city' -or $CityStreaming -ne 'on')) { throw 'CityStreamingSmokeTest requires city and CityStreaming on.' }
$imposterViews = @($Views | Where-Object { $_ -in @('imposter-front', 'imposter-diagonal', 'imposter-back', 'imposter-high') })
if ($imposterViews.Count -and ($Scene -ne 'city' -or $RoadAuthoringShowcase -or $CityStreaming -ne 'on' -or $CityStreamingSmokeTest)) {
    throw 'Imposter views require a generated city, CityStreaming on and no streaming smoke test.'
}
if ($streetDetailExplicit -and !$CityTerrainRoads) { throw '-CityStreetDetail requires -CityTerrainRoads.' }
if ($streetPropDensityExplicit -and !$CityTerrainRoads) { throw '-CityStreetPropDensity requires -CityTerrainRoads for capture validation.' }
if ($PSBoundParameters.ContainsKey('CityTrafficDensity') -and !$CityTerrainRoads) { throw '-CityTrafficDensity requires -CityTerrainRoads.' }
if ($CityTrafficDriveOnLeft -and !$CityTerrainRoads) { throw '-CityTrafficDriveOnLeft requires -CityTerrainRoads.' }
if ($Views -contains 'traffic' -and (!$CityTerrainRoads -or $CityTrafficDensity -eq 0 -or $Seconds -lt 2)) {
    throw '-Views traffic requires -CityTerrainRoads, nonzero -CityTrafficDensity and at least two benchmark seconds.'
}
if (($CityDebugBlocks -or $CityDebugLots) -and !$CityTerrainRoads) { throw 'City debug overlay capture requires -CityTerrainRoads.' }
if ($Views -contains 'frontage' -and !$CityTerrainRoads) { throw '-Views frontage requires -CityTerrainRoads.' }
if ($Views -contains 'street-lamp' -and !$CityTerrainRoads) { throw '-Views street-lamp requires -CityTerrainRoads.' }
if ($Views -contains 'street-bin' -and (!$CityTerrainRoads -or $CityStreetDetail -eq 0 -or $CityStreetPropDensity -eq 0)) {
    throw '-Views street-bin requires -CityTerrainRoads and nonzero street detail and prop density.'
}
if ($Views -contains 'utilities' -and !$CityTerrainRoads) { throw '-Views utilities requires -CityTerrainRoads.' }
if ($Views -contains 'road-profile' -and !$CityTerrainRoads) { throw '-Views road-profile requires -CityTerrainRoads.' }
if ($Views -contains 'park' -and !$CityTerrainRoads) { throw '-Views park requires -CityTerrainRoads.' }
if ($Views -contains 'shopfront' -and !$CityTerrainRoads) { throw '-Views shopfront requires -CityTerrainRoads.' }
if ($Views -contains 'entry-route' -and !$CityTerrainRoads) { throw '-Views entry-route requires -CityTerrainRoads.' }
if ($Views -contains 'rooftop' -and ($Scene -ne 'city' -or !$CityTerrainRoads)) { throw '-Views rooftop requires -Scene city and -CityTerrainRoads.' }
if ($Views -contains 'pitched-roof' -and ($Scene -ne 'city' -or !$CityTerrainRoads)) { throw '-Views pitched-roof requires -Scene city and -CityTerrainRoads.' }
if ($Views -contains 'pitched-roof' -and $CityBuildingStyle -and $CityBuildingStyle -notin @('district', 'masonry', 'terrace', 'parisian', 'mediterranean', 'timber')) { throw '-Views pitched-roof requires a pitched building style.' }
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
if ($FoliageLODSmokeTest -and !$PSBoundParameters.ContainsKey('Views')) { $Views = @($(if ($CityTerrainRoads) { 'park' } else { 'corner' })) }
if ($FoliageLODSmokeTest -and @($Views | Where-Object { $_ -eq 'follow' -or $_ -in $imposterViews -or $_ -in $roadEditViews }).Count) {
    throw '-FoliageLODSmokeTest requires a fixed camera outside road-edit and building imposter review views.'
}
if ($FoliageMotionCapture -and (!$CityTerrainRoads -or $Seconds -lt 10 -or $FoliageLODSmokeTest -or
    @($Views | Where-Object { $_ -ne 'car' }).Count)) {
    throw '-FoliageMotionCapture requires terrain-city roads, 10+ benchmark seconds and only the car view.'
}
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
$sourceRoots += @('Engine/cpp/Include/Workphone/Procedural', 'Engine/cpp/Source/Workphone/Procedural')
$sources = @($sourceRoots | ForEach-Object { Get-ChildItem -LiteralPath (Join-Path $repoRoot $_) -Recurse -File | Where-Object { $_.Extension -in '.cpp', '.c', '.hpp', '.h', '.txt' } } | Sort-Object FullName -Unique | ForEach-Object { Get-Identity $_.FullName })
$sources += @('Engine/cpp/Source/Workphone/System/FSM.cpp', 'Engine/cpp/Source/Workphone/System/FSMManager.cpp', 'Engine/cpp/Include/Workphone/System/FSM.hpp', 'Engine/cpp/Include/Workphone/System/FSMManager.hpp') | ForEach-Object { Get-Identity (Join-Path $repoRoot $_) }
$sources += @('Engine/cpp/Source/Workphone/Mesh/MeshImposterGenerator.cpp', 'Engine/cpp/Include/Workphone/Mesh/MeshImposterGenerator.hpp') | ForEach-Object { Get-Identity (Join-Path $repoRoot $_) }
$sources += @('Engine/cpp/Source/Workphone/WorkphonePlugin.cpp', 'Tools/powershell/CaptureProceduralRacing.ps1') | ForEach-Object { Get-Identity (Join-Path $repoRoot $_) }
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
    cityStreetPropDensity = $(if ($streetPropDensityExplicit) { $CityStreetPropDensity } else { $null })
    cityTrafficDensity = $(if ($CityTerrainRoads) { $CityTrafficDensity } else { $null })
    cityTrafficDriveOnLeft = [bool]$CityTrafficDriveOnLeft
    cityDebugBlocks = [bool]$CityDebugBlocks
    cityDebugLots = [bool]$CityDebugLots
    cityStreaming = $(if ($Scene -eq 'city') { $CityStreaming } else { $null })
    citySourceMemoryMiB = $(if ($Scene -eq 'city') { $CitySourceMemoryMiB } else { $null })
    cityStreamingSmokeTest = [bool]$CityStreamingSmokeTest
    foliageLOD = $FoliageLOD; foliageLODSmokeTest = [bool]$FoliageLODSmokeTest; foliageMotionCapture = [bool]$FoliageMotionCapture
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
        if ($Scene -eq 'city') { $arguments += @('--city-streaming', $CityStreaming, '--city-source-memory-mib', "$CitySourceMemoryMiB") }
        if ($CityStreamingSmokeTest) { $arguments += '--city-streaming-smoke-test' }
        if ($SceneAA -ne 'auto') { $arguments += @('--scene-aa', $SceneAA) }
        if ($RoadAuthoringShowcase) { $arguments += '--road-authoring-showcase' }
        if ($CityTerrainRoads) { $arguments += @('--city-terrain-roads', '--city-terrain-relief', $CityTerrainRelief.ToString([Globalization.CultureInfo]::InvariantCulture)) }
        if ($streetDetailExplicit) { $arguments += @('--city-street-detail', "$CityStreetDetail") }
        if ($streetPropDensityExplicit) { $arguments += @('--city-street-prop-density', "$CityStreetPropDensity") }
        if ($CityTerrainRoads) { $arguments += @('--city-traffic-density', "$CityTrafficDensity") }
        if ($CityTrafficDriveOnLeft) { $arguments += '--city-traffic-drive-on-left' }
        if ($CityDebugBlocks) { $arguments += '--city-debug-blocks' }
        if ($CityDebugLots) { $arguments += '--city-debug-lots' }
        if ($CityBuildingStyle) { $arguments += @('--city-building-style', $CityBuildingStyle.ToLowerInvariant()) }
        foreach ($role in $buildingMeshRoles) {
            if ($buildingMeshPaths.Contains($role)) {
                $arguments += @('--city-building-mesh', $role, (Quote-ProcessArgument $buildingMeshPaths[$role]))
            }
        }
        if ($Replay) { $arguments += @('--replay', (Quote-ProcessArgument $Replay)) }
        if ($NoEffects) { $arguments += '--no-effects' }
        if ($FoliageWorld) { $arguments += '--foliage-world' }
        $arguments += @('--foliage-lod', $FoliageLOD)
        if ($FoliageLODSmokeTest) { $arguments += '--foliage-lod-smoke-test' }
        if ($FoliageMotionCapture) { $arguments += @('--orbit', '--foliage-motion-capture') }
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
            if ($CityTerrainRoads -or $FoliageWorld) {
                $species = $run.evidence.foliageSpecies
                $run.passed = $run.passed -and $null -ne $species -and $species.valid -eq $true -and
                    $species.woodySpecies -gt 0 -and $species.realisedWoodyAtlases -eq $species.woodySpecies
                if ($CityTerrainRoads) { $run.passed = $run.passed -and $species.woodySpecies -eq 6 }
            }
            if ($FoliageLODSmokeTest) {
                $probe = $run.evidence.foliageLODReview
                $stages = @($probe.stages)
                $run.foliageLODCaptures = @()
                $run.foliageAtlasSheets = @()
                $run.passed = $run.passed -and $null -ne $probe -and $probe.passed -eq $true -and
                    $probe.restored -eq $true -and $stages.Count -eq 4
                if ($stages.Count -eq 4) {
                    $first = $stages[0]
                    $expectedLevels = @(0, 1, 2, -1)
                    $previousSerial = 0
                    for ($stageIndex = 0; $stageIndex -lt 4; ++$stageIndex) {
                        $stage = $stages[$stageIndex]
                        $label = if ($stageIndex -eq 3) { 'auto' } else { "$stageIndex" }
                        $expectedCapture = Join-Path $output "$view-foliage-lod$label.bmp"
                        $run.passed = $run.passed -and $stage.level -eq $expectedLevels[$stageIndex] -and
                            $stage.recorded -eq $true -and $stage.capturePassed -eq $true -and
                            $stage.completedFrames -ge 8 -and $stage.nativeRenderSerial -gt $previousSerial -and
                            $stage.actualDraws -gt 0 -and $stage.submittedTriangles -gt 0 -and
                            $stage.population -gt 0 -and $stage.population -eq $first.population -and
                            $stage.populationHash -ceq $first.populationHash -and $stage.worldIdentity -ceq $first.worldIdentity -and
                            $stage.sourceRevision -eq $first.sourceRevision -and $stage.residentPages -eq $first.residentPages -and
                            $stage.residentInstances -eq $first.residentInstances -and
                            ($stage.cameraPosition -join ',') -ceq ($first.cameraPosition -join ',') -and
                            ($stage.cameraOrientation -join ',') -ceq ($first.cameraOrientation -join ',')
                        if ($stageIndex -lt 3) {
                            for ($level = 0; $level -lt 3; ++$level) {
                                $run.passed = $run.passed -and $(if ($level -eq $stageIndex) {
                                    $stage.lodInstances[$level] -gt 0
                                } else { $stage.lodInstances[$level] -eq 0 })
                            }
                        }
                        if ($stage.capturePath -and [IO.Path]::GetFullPath($stage.capturePath) -ieq $expectedCapture -and
                            (Test-Path -LiteralPath $expectedCapture -PathType Leaf)) {
                            $identity = Get-Identity $expectedCapture
                            $run.foliageLODCaptures += [ordered]@{ level = $stage.level; image = $identity }
                        } else { $run.passed = $false }
                        $previousSerial = $stage.nativeRenderSerial
                    }
                    $run.passed = $run.passed -and $stages[0].submittedTriangles -gt $stages[1].submittedTriangles -and
                        $stages[1].submittedTriangles -gt $stages[2].submittedTriangles
                }
                foreach ($kind in @('tree', 'bush')) {
                    $kindValue = if ($kind -eq 'tree') { 0 } else { 1 }
                    $expectedSpecies = @($run.evidence.foliageSpecies.species | Where-Object { $_.kind -eq $kindValue })
                    if (!$expectedSpecies.Count) { continue }
                    $sheet = $expectedSpecies[0].atlas.maps[0].atlasExport
                    $expectedSheet = Join-Path $output "$view-foliage-$kind-sheet.tga"
                    if ($sheet.path -and [IO.Path]::GetFullPath($sheet.path) -ieq $expectedSheet -and
                        $sheet.losslessRepack -eq $true -and $sheet.columns -eq 5 -and $sheet.rows -eq 5 -and
                        (Test-Path -LiteralPath $expectedSheet -PathType Leaf)) {
                        $run.foliageAtlasSheets += [ordered]@{ kind = $kind; image = Get-Identity $expectedSheet }
                    } else { $run.passed = $false }
                }
            }
            if ($FoliageMotionCapture) {
                $trace = Join-Path $output "$view-motion.csv"
                $rows = @(Import-Csv -LiteralPath $trace)
                $run.motionTrace = Get-Identity $trace
                $run.motionCaptures = @()
                $run.passed = $run.passed -and $rows.Count -eq 6
                for ($stage = 0; $stage -lt $rows.Count; ++$stage) {
                    $row = $rows[$stage]
                    $image = Join-Path $output "$view-motion-$stage.bmp"
                    $run.passed = $run.passed -and [int]$row.stage -eq $stage -and
                        [int]$row.residentPages -gt 0 -and [int]$row.residentInstances -gt 0 -and
                        [int]$row.submittedTriangles -gt 0 -and (Test-Path -LiteralPath $image -PathType Leaf)
                    if (Test-Path -LiteralPath $image -PathType Leaf) { $run.motionCaptures += Get-Identity $image }
                }
                $eyes = @($rows | ForEach-Object { "$($_.eyeX),$($_.eyeY),$($_.eyeZ)" } | Sort-Object -Unique)
                $bins = @($rows | ForEach-Object { "$($_.lod0),$($_.lod1),$($_.lod2)" } | Sort-Object -Unique)
                $revisions = @($rows | ForEach-Object sourceRevision | Sort-Object -Unique)
                $populations = @($rows | ForEach-Object sourcePopulationHash | Sort-Object -Unique)
                $firstEye = $rows[0]; $lastEye = $rows[-1]
                $displacement = [Math]::Sqrt([Math]::Pow([double]$firstEye.eyeX - [double]$lastEye.eyeX, 2) +
                    [Math]::Pow([double]$firstEye.eyeY - [double]$lastEye.eyeY, 2) +
                    [Math]::Pow([double]$firstEye.eyeZ - [double]$lastEye.eyeZ, 2))
                $run.motionDisplacementMetres = $displacement
                $run.passed = $run.passed -and $eyes.Count -eq 6 -and $bins.Count -gt 1 -and
                    $revisions.Count -eq 1 -and $populations.Count -eq 1 -and $displacement -gt 110
            }
            if ($Scene -eq 'city' -and !$RoadAuthoringShowcase) {
                $buildings = $run.evidence.cityBuildingGeometry
                $run.passed = $run.passed -and $null -ne $buildings -and $buildings.ready -eq $true -and
                    $buildings.meshes -gt 0 -and ($buildings.readyMeshes + [int]$buildings.streamedOutMeshes) -eq $buildings.meshes
                $run.passed = $run.passed -and $run.evidence.cityImposters.ready -eq $true
            }
            if ($CityStreamingSmokeTest) {
                $smoke = $run.evidence.cityStreamingSmoke
                $initial = $smoke.initialResidency
                $cold = $initial.streaming
                $first = $smoke.firstCameraLoad
                $run.passed = $run.passed -and $null -ne $smoke -and $smoke.passed -eq $true -and
                    $initial.recorded -eq $true -and $cold.initialized -eq $true -and
                    $cold.patches -gt 0 -and $cold.fallbackMeshes -eq $cold.patches -and
                    $cold.residentPatches -eq 0 -and $cold.residentMeshes -eq 0 -and
                    $cold.residentSourceBytes -eq 0 -and $cold.pendingPatches -eq 0 -and
                    $cold.installedPatches -eq 0 -and $cold.nativeMeshLoads -eq 0 -and
                    $null -ne $first -and $first.cameraViewMatches -eq $true -and
                    $first.streaming.residentPatches -gt 0 -and $first.streaming.residentMeshes -gt 0 -and
                    $first.readyMeshes -gt $first.readyImposters -and
                    $first.readyImposters -eq $first.imposterMeshes -and
                    $first.imposterMeshes -eq $cold.patches -and
                    $smoke.cameraOnly.passed -eq $true
            }
            if ($view -in $imposterViews) {
                $imposters = $run.evidence.cityImposters
                $sheet = $imposters.atlasExport
                $run.passed = $run.passed -and $imposters.cameraOnlySelectionChanged -eq $true -and
                    $imposters.review.forcedLastLOD -eq $true -and $imposters.review.forcedTargetOnly -eq $true -and
                    $imposters.review.radiusMetres -eq 300 -and
                    $imposters.patchCount -gt 0 -and @($imposters.patches | Where-Object {
                        $_.ready -ne $true -or $_.viewCount -ne 49 -or $_.nativeVertices -ne 4 -or $_.nativeTriangles -ne 2
                    }).Count -eq 0 -and $sheet.losslessRepack -eq $true -and $sheet.columns -eq 7 -and $sheet.rows -eq 7
                if ($sheet.path -and (Test-Path -LiteralPath $sheet.path -PathType Leaf)) {
                    $run.imposterSheet = Get-Identity $sheet.path
                } else { $run.passed = $false }
            }
            if ($streetDetailExplicit) {
                $street = $run.evidence.terrainCity
                $run.passed = $run.passed -and $null -ne $street -and $street.streetDetailValid -eq $true -and
                    $null -ne $street.cityStreetDetailCount -and $street.cityStreetDetail -eq $CityStreetDetail
            }
            if ($streetPropDensityExplicit) {
                $street = $run.evidence.terrainCity
                $run.passed = $run.passed -and $null -ne $street -and $street.cityStreetPropDensity -eq $CityStreetPropDensity
            }
            if ($Scene -eq 'city' -and $CityTerrainRoads -and !$RoadAuthoringShowcase) {
                $traffic = $run.evidence.cityTraffic
                $run.passed = $run.passed -and $null -ne $traffic -and $traffic.valid -eq $true -and
                    $traffic.densityPercent -eq $CityTrafficDensity -and
                    $traffic.driveOnLeft -eq [bool]$CityTrafficDriveOnLeft -and
                    $traffic.sourceRevisionMatches -eq $true -and
                    $(if ($CityTrafficDensity -eq 0) { $traffic.active -eq 0 } else { $traffic.active -gt 0 })
            }
            if ($view -eq 'traffic') {
                $traffic = $run.evidence.cityTraffic
                $camera = $run.evidence.cityTrafficCamera
                $run.passed = $run.passed -and $null -ne $camera -and $camera.valid -eq $true -and
                    $camera.focusDisplacementMetres -gt 0.5 -and
                    $traffic.moving -gt 0 -and $traffic.travelledMetres -gt 0 -and
                    @($camera.eye).Count -eq 3 -and @($camera.target).Count -eq 3
            }
            if ($CityDebugBlocks -or $CityDebugLots) {
                $debug = $run.evidence.terrainCity
                $run.passed = $run.passed -and $null -ne $debug -and $debug.cityDebugValid -eq $true -and
                    $debug.cityDebugBlocks -eq [bool]$CityDebugBlocks -and $debug.cityDebugLots -eq [bool]$CityDebugLots
            }
            if ($view -eq 'street-lamp') {
                $lamp = $run.evidence.cityStreetLampCamera
                $run.passed = $run.passed -and $lamp.valid -eq $true -and $lamp.lensMeshReady -eq $true -and
                    [bool]$lamp.roadId -and @($lamp.foundation).Count -eq 3 -and
                    @($lamp.eye).Count -eq 3 -and @($lamp.target).Count -eq 3
            }
            if ($view -eq 'street-bin') {
                $bin = $run.evidence.cityStreetBinCamera
                $run.passed = $run.passed -and $bin.valid -eq $true -and $bin.meshesReady -eq $true -and
                    @($bin.base).Count -eq 3 -and @($bin.eye).Count -eq 3 -and @($bin.target).Count -eq 3
            }
            if ($view -eq 'utilities') {
                $utility = $run.evidence.cityUtilitiesCamera
                $run.passed = $run.passed -and $null -ne $utility -and $utility.valid -eq $true -and
                    $utility.primaryPresent -eq $true -and $utility.nativeReady -eq $true
            }
            if ($view -eq 'road-profile') {
                $profile = $run.evidence.cityRoadProfileCamera
                $run.passed = $run.passed -and $null -ne $profile -and $profile.valid -eq $true -and
                    $profile.roadSamplesDriveable -eq $true -and $profile.roadWidthMetres -ge 9 -and
                    $profile.crownMetres -ge 0.02 -and $profile.crossSlope -le 0.035 -and
                    $profile.candidateSamples -gt 0 -and
                    @($profile.eye).Count -eq 3 -and @($profile.target).Count -eq 3 -and
                    $run.evidence.terrainCity.footwayCollisionReady -eq $true
            }
            if ($view -eq 'park') {
                $park = $run.evidence.cityParkCamera
                $run.passed = $run.passed -and $null -ne $park -and $park.valid -eq $true -and [bool]$park.lotId
            }
            if ($view -eq 'shopfront') {
                $run.passed = $run.passed -and $run.evidence.cityShopfrontCameraValid -eq $true -and
                    [bool]$run.evidence.cityFacadeCamera.lotId
            }
            if ($view -eq 'entry-route') {
                $run.passed = $run.passed -and $run.evidence.cityEntryRouteCamera.valid -eq $true -and
                    [bool]$run.evidence.cityFacadeCamera.lotId
            }
            if ($view -in @('rooftop', 'pitched-roof')) {
                $run.passed = $run.passed -and $run.evidence.cityRooftopCameraValid -eq $true -and
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
    if ($imposterViews.Count -gt 1) {
        $reviews = @($manifest.runs | Where-Object { $_.view -in $imposterViews })
        $first = $reviews[0]
        $manifest.matchedImposterReview = @($reviews | Where-Object {
            $_.evidence.cityImposters.review.targetPatch -ne $first.evidence.cityImposters.review.targetPatch -or
            ($_.evidence.cityImposters.review.target -join ',') -cne ($first.evidence.cityImposters.review.target -join ',') -or
            $_.imposterSheet.sha256 -cne $first.imposterSheet.sha256
        }).Count -eq 0
        if (!$manifest.matchedImposterReview) { $failed = $true }
    }
} finally {
    $manifest.completedUtc = [DateTime]::UtcNow.ToString('o')
    $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'manifest.json') -Encoding utf8
}
if ($failed) { throw "Capture validation failed; inspect $output" }
Write-Host "Captured evidence: $output"
