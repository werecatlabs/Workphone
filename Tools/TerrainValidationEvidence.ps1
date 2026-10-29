# Source identity for terrain builds and tests, including dirty/untracked inputs.
# Keep this separate from Git HEAD: another chat can commit without changing bytes.
function Get-TerrainSourceSnapshot([string]$RepositoryRoot) {
    $sourcePaths = @(
        'Engine/c', 'Engine/cpp', 'Tools/cpp/Editor', 'bin/Media/Scripts/Lua/Editor',
        # Include inputs of the terrain aggregate and its existing helper fixtures.
        # An unrelated standalone test edit (e.g. LuaDebuggerTests.cpp) cannot
        # change these binaries and must not invalidate their source identity.
        'Tests/CMakeLists.txt', 'Tests/cpp/CMakeLists.txt', 'Tests/cpp/*.hpp',
        'Tests/cpp/TerrainContractsTests.cpp', 'Tests/cpp/TerrainPhysicsContractsTests.cpp',
        'Tests/cpp/TerrainLuaBindingTests.cpp', 'Tests/cpp/ClawProductionTests.cpp',
        'Tests/cpp/ClawCookedResourceTests.cpp', 'Tests/cpp/GraphicsProduction', 'Tests/cpp/Fixtures',
        'CMakeLists.txt', 'CMakePresets.json', 'CMake', 'Dependencies/CMakeLists.txt',
        'Dependencies/cJSON/CMakeLists.txt', 'Tools/ValidateClawGraphics.ps1',
        'Tools/BuildWPGraphicsBaseline.ps1', 'Tools/ValidateTerrainPackage.ps1',
        'Tools/TerrainPackageRuntime.cmake', 'Tools/TerrainValidationEvidence.ps1'
        'Samples/cpp/VehicleAdvanced', 'Tools/ValidateTerrainGameplayPackage.ps1'
    )
    $paths = @(& git -C $RepositoryRoot ls-files --cached --others --exclude-standard -- @sourcePaths)
    if ($LASTEXITCODE -ne 0) { throw 'Unable to inventory terrain source inputs.' }
    $paths = @($paths | Sort-Object -Unique)
    # One Git process avoids thousands of managed filesystem opens and preserves
    # physical bytes (--no-filters), including dirty/untracked inputs.
    $objects = @($paths | & git -C $RepositoryRoot hash-object --no-filters --stdin-paths)
    if ($LASTEXITCODE -ne 0 -or $objects.Count -ne $paths.Count) { throw 'Terrain source hashing failed.' }
    $hashes = [ordered]@{}
    for ($index = 0; $index -lt $paths.Count; ++$index) { $hashes[$paths[$index]] = $objects[$index] }
    $text = ($hashes.GetEnumerator() | ForEach-Object { $_.Value + ' ' + $_.Key }) -join "`n"
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $digest = [BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text))).Replace('-', '') }
    finally { $sha.Dispose() }
    return @{ files = $hashes; fileHashAlgorithm = 'git-blob-sha1'; sha256 = $digest }
}

function Compare-TerrainSourceSnapshots($Before, $After) {
    @(@($Before.files.Keys) + @($After.files.Keys) | Sort-Object -Unique | Where-Object {
        $Before.files[$_] -ne $After.files[$_]
    })
}
