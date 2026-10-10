#requires -Version 7.0
param(
    [string]$BuildDirectory = 'project_x64',
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [string]$CTestExecutable,
    [switch]$RequireExternalAssets
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDirectory)) { [System.IO.Path]::GetFullPath($BuildDirectory) } else { [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory)) }
if (-not $CTestExecutable) {
    $installedCTest = Join-Path $env:ProgramFiles 'CMake/bin/ctest.exe'
    $CTestExecutable = if (Test-Path -LiteralPath $installedCTest) { $installedCTest } else { (Get-Command ctest -ErrorAction Stop).Source }
}
$CTestExecutable = (Get-Command $CTestExecutable -ErrorAction Stop).Source
$selection = '^(WPGraphics|WorkphoneGraphics|WorkphoneAssets)\.|^WPResourceTests$'
$requiredTests = @(
    'WPGraphics.terrain_contracts', 'WPGraphics.terrain_lua_workflow',
    'WPGraphics.production_Skinning', 'WPGraphics.production_ParticleSimulation',
    'WorkphoneGraphics.mesh_serializer', 'WorkphoneGraphics.mesh_import_assets',
    'WorkphoneGraphics.renderer_contract', 'WorkphoneGraphics.shader_contract',
    'WorkphoneGraphics.aaa_pipeline', 'WPGraphics.cubemap_pbr',
    'WorkphoneAssets.catalog_contracts', 'WPGraphics.claw_production_render', 'WPGraphics.cooked_material_resource',
    'WPGraphics.claw_text_contract', 'WPGraphics.claw_dx11_targets',
    'WPGraphics.claw_ui_destruction', 'WPResourceTests'
)
$inventoryText = & $CTestExecutable --test-dir $buildPath -C $Configuration -R $selection --show-only=json-v1
if ($LASTEXITCODE -ne 0) { throw 'CTest inventory failed.' }
$inventory = ($inventoryText -join "`n") | ConvertFrom-Json
$missing = @($requiredTests | Where-Object { $_ -notin $inventory.tests.name })
if ($missing.Count -gt 0) { throw "Required tests are not registered: $($missing -join ', '). Configure the wpgraphics-baseline preset." }

$reportPath = Join-Path $buildPath "claw-graphics-$Configuration-results.xml"
$evidencePath = Join-Path $buildPath "claw-graphics-$Configuration-evidence.json"
# Do not accidentally certify an old report if this CTest invocation fails.
if (Test-Path -LiteralPath $reportPath) { Remove-Item -LiteralPath $reportPath }
& $CTestExecutable --test-dir $buildPath -C $Configuration -R $selection --output-on-failure --no-tests=error --test-output-size-passed 65536 --test-output-size-failed 65536 --output-junit $reportPath
$testExit = $LASTEXITCODE
if (-not (Test-Path -LiteralPath $reportPath)) { throw 'CTest did not produce a report.' }
[xml]$report = Get-Content -LiteralPath $reportPath -Raw
$skipped = @($report.testsuite.testcase | Where-Object { $_.status -eq 'notrun' -or $_.skipped })
$unexpectedSkips = @($skipped | Where-Object { $RequireExternalAssets -or $_.name -ne 'WorkphoneGraphics.mesh_import_assets' })
foreach ($skip in $skipped) { Write-Warning "Coverage unavailable: $($skip.name)" }
$missingResults = @($requiredTests | Where-Object { $_ -notin $report.testsuite.testcase.name })
$fixturePaths = @(
    'bin/Media/Ogre/models/Barrel.mesh', 'bin/Media/Ogre/models/athene.mesh',
    'bin/Media/Ogre/models/ninja.mesh', 'bin/Media/Ogre/models/ogrehead.mesh',
    'bin/Media/Ogre/mygui/Common/Scene/Mikki.mesh', 'bin/Media/OgreNext/models/char_feet.mesh'
)
$testSources = @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Tests/GraphicsProduction') -Filter '*Tests.c') +
    @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Tests/c') -Filter 'WorkphoneGraphics*Tests.c') +
    @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Tests/cpp') -Filter 'Claw*Tests.cpp') +
    @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Tests/cpp') -Filter '*Contracts.hpp') +
    @(Get-Item -LiteralPath (Join-Path $repositoryRoot 'Tests/cpp/AssetCatalogTests.cpp'), (Join-Path $repositoryRoot 'Tests/cpp/ResourceSystem/ResourceSystemSmoke.cpp'))
$verificationPaths = @(
    'Tests/cpp/TerrainContractsTests.cpp', 'Tests/cpp/TerrainLuaBindingTests.cpp',
    'Engine/cpp/Include/Workphone/Graphics/TerrainData.hpp',
    'Engine/cpp/Include/Workphone/Graphics/Terrain.hpp',
    'Engine/cpp/Include/WPGraphics/ClawTerrain.hpp',
    'Engine/cpp/Include/Workphone/Scene/Components/Terrain/TerrainSystem.hpp',
    'Engine/cpp/Include/Workphone/Scene/TerrainEditing.hpp',
    'Engine/cpp/Include/Workphone/System/CommandManager.hpp',
    'Engine/cpp/Include/Workphone/System/CommandManagerMT.hpp',
    'Engine/cpp/Source/Workphone/Graphics/TerrainData.cpp',
    'Engine/cpp/Source/Workphone/Graphics/Terrain.cpp',
    'Engine/cpp/Source/Workphone/Scene/TerrainEditing.cpp',
    'Engine/cpp/Source/Workphone/System/CommandManager.cpp',
    'Engine/cpp/Source/Workphone/System/CommandManagerMT.cpp',
    'Engine/cpp/Project/Workphone/CMakeLists.txt', 'Dependencies/cJSON/cJSON.c',
    'Engine/cpp/Source/Workphone/Scene/Components/Terrain/TerrainSystem.cpp',
    'Engine/cpp/Source/WPGraphics/ClawTerrain.cpp',
    'Engine/cpp/Source/WPLuabind/Bindings/ComponentBind.cpp',
    'bin/Media/Scripts/Lua/Editor/TerrainEditor.lua',
    'Tests/cpp/DX11TestEvidence.hpp', 'Tools/ValidateClawGraphics.ps1',
    'Tools/BuildWPGraphicsBaseline.ps1', 'CMakePresets.json',
    'Engine/cpp/Project/WPGraphics/CMakeLists.txt', 'Tests/cpp/CMakeLists.txt',
    'Engine/cpp/Include/Workphone/Database/AssetCatalogPath.hpp',
    'Engine/cpp/Source/Workphone/Database/AssetCatalogPath.cpp',
    'Engine/cpp/Include/Workphone/Database/CatalogResourceAdapter.hpp',
    'Engine/cpp/Source/Workphone/Database/CatalogResourceAdapter.cpp'
)
$testSources += @($verificationPaths | ForEach-Object { Get-Item -LiteralPath (Join-Path $repositoryRoot $_) })
$testSources += @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Engine/cpp/Include/WPGraphics/Resources') -Filter '*.hpp') +
    @(Get-ChildItem -LiteralPath (Join-Path $repositoryRoot 'Engine/cpp/Source/WPGraphics/Resources') -Filter '*.cpp')
$hashes = @($testSources | ForEach-Object { [ordered]@{ path = [System.IO.Path]::GetRelativePath($repositoryRoot, $_.FullName); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash } })
$fixtures = @($fixturePaths | ForEach-Object {
    $path = Join-Path $repositoryRoot $_
    [ordered]@{ path = $_; available = (Test-Path -LiteralPath $path); sha256 = if (Test-Path -LiteralPath $path) { (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash } else { $null } }
})
$cache = @{}
Get-Content -LiteralPath (Join-Path $buildPath 'CMakeCache.txt') | ForEach-Object {
    if ($_ -match '^([^#/][^:]*):[^=]+=(.*)$') { $cache[$Matches[1]] = $Matches[2] }
}
$compilerFiles = @(Get-ChildItem -Path (Join-Path $buildPath 'CMakeFiles/*/CMakeCXXCompiler.cmake'))
$compiler = @{}
foreach ($file in $compilerFiles) {
    Get-Content -LiteralPath $file.FullName | ForEach-Object {
        if ($_ -match '^set\((CMAKE_CXX_COMPILER(?:_VERSION)?) "([^"]*)"\)') { $compiler[$Matches[1]] = $Matches[2] }
    }
}
$adapters = @()
$adapterError = $null
try { $adapters = @(Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, PNPDeviceID) } catch { $adapterError = $_.Exception.Message }
$sourceRevision = & git -C $repositoryRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Unable to record source revision.' }
$sourceStatus = @(& git -C $repositoryRoot status --short)
$diff = (& git -C $repositoryRoot diff HEAD --binary --no-ext-diff) -join "`n"
$sha = [System.Security.Cryptography.SHA256]::Create()
try { $diffHash = [System.BitConverter]::ToString($sha.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($diff))).Replace('-', '') } finally { $sha.Dispose() }
$binaries = @($inventory.tests | ForEach-Object {
    $executable = if ($_.command) { $_.command[0] } else { $null }
    [ordered]@{ test = $_.name; path = $executable; sha256 = if ($executable -and (Test-Path -LiteralPath $executable)) { (Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash } else { $null } }
})
$results = @($report.testsuite.testcase | ForEach-Object {
    [ordered]@{ name = $_.name; status = if ($_.skipped -or $_.status -eq 'notrun') { 'unavailable' } elseif ($_.failure) { 'failed' } else { 'passed' }; seconds = $_.time }
})
$gpuDevices = @($report.testsuite.testcase | ForEach-Object {
    $lines = @(([string]$_.'system-out') -split "`n" | Where-Object { $_ -match '^DX11 (device|driver)' })
    if ($lines.Count -gt 0) { [ordered]@{ test = $_.name; deviceEvidence = $lines } }
})
$sdk = $cache['CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION']
if (-not $sdk -and (Test-Path -LiteralPath (Join-Path $buildPath 'ZERO_CHECK.vcxproj'))) {
    [xml]$checkProject = Get-Content -LiteralPath (Join-Path $buildPath 'ZERO_CHECK.vcxproj') -Raw
    $sdk = @($checkProject.Project.PropertyGroup.WindowsTargetPlatformVersion | Where-Object { $_ }) | Select-Object -First 1
}
$cmakeExecutable = Join-Path (Split-Path $CTestExecutable -Parent) 'cmake.exe'
$evidence = [ordered]@{
    recordedAtUtc = [DateTime]::UtcNow.ToString('o'); sourceRevision = $sourceRevision
    workingTreeStatus = $sourceStatus; trackedDiffSha256 = $diffHash; configuration = $Configuration
    buildDirectory = $buildPath; cmakeVersion = (& $cmakeExecutable --version | Select-Object -First 1)
    ctestVersion = (& $CTestExecutable --version | Select-Object -First 1)
    generator = $cache['CMAKE_GENERATOR']; generatorInstance = $cache['CMAKE_GENERATOR_INSTANCE']
    architecture = $cache['CMAKE_GENERATOR_PLATFORM']; staticLibraries = $cache['WP_STATIC_LIB']; staticCrt = $cache['WP_STATIC_LINK_CRT']
    windowsSdk = $sdk; compiler = $compiler; gpuDevices = $gpuDevices
    systemAdapters = $adapters; adapterQueryError = $adapterError
    testSources = $hashes; externalFixtures = $fixtures; testBinaries = $binaries; tests = $results
    testExitCode = $testExit; requiredTests = $requiredTests; requireExternalAssets = [bool]$RequireExternalAssets
    coverageComplete = ($testExit -eq 0 -and $skipped.Count -eq 0 -and $missingResults.Count -eq 0)
    validationPassed = ($testExit -eq 0 -and $unexpectedSkips.Count -eq 0 -and $missingResults.Count -eq 0)
    releaseCertified = $false
    ciWorkflowAvailable = (Test-Path -LiteralPath (Join-Path $repositoryRoot '.github/workflows/graphics-contracts.yml'))
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $evidencePath -Encoding utf8
if ($testExit -ne 0 -or $unexpectedSkips.Count -gt 0 -or $missingResults.Count -gt 0) { throw "Graphics validation failed, mandatory tests were skipped, or required results are absent. See $reportPath and $evidencePath" }
Write-Host "Graphics, catalog and resource checks passed. Report: $reportPath; evidence: $evidencePath"
if ($skipped.Count -gt 0) { Write-Host 'External-media coverage remains unavailable; this is not a release certification.' }
