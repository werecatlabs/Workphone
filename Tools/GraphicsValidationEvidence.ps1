# Shared source boundary for the full build and test gate. Include plugin and
# Editor/package-tool sources that contribute to the tested runtime, not only
# WPGraphics, so a stable test executable cannot mask a concurrent source edit.
function Get-GraphicsValidationSourceSnapshot([string]$RepositoryRoot) {
    $directories=@('Engine/c','Engine/cpp','Tests','Tools/cpp/Editor','Tools/cpp/ResourcePackageTool','CMake')
    $files=@(foreach($directory in $directories) {
        Get-ChildItem -LiteralPath (Join-Path $RepositoryRoot $directory) -File -Recurse |
            Where-Object { $_.Extension -in @('.c','.cpp','.h','.hpp','.inl','.cmake','.py','.lua','.gltf','.json','.txt') }
    })
    $files+=@('CMakeLists.txt','CMakePresets.json','Tools/ValidateClawGraphics.ps1',
        'Tools/BuildWPGraphicsBaseline.ps1','Tools/GraphicsValidationEvidence.ps1') |
        ForEach-Object { Get-Item -LiteralPath (Join-Path $RepositoryRoot $_) }
    $snapshot=@{}
    foreach($file in $files | Sort-Object FullName -Unique) {
        $snapshot[$file.FullName]=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    }
    $snapshot
}
function Compare-GraphicsValidationSourceSnapshots($Before,$After) {
    @(@($Before.Keys)+@($After.Keys) | Sort-Object -Unique |
        Where-Object { $Before[$_] -ne $After[$_] })
}
