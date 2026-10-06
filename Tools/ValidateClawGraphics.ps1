param(
    [string]$BuildDirectory = 'project_x64',
    [ValidateSet('Debug', 'Release', 'RelWithDebInfo')][string]$Configuration = 'RelWithDebInfo',
    [switch]$RequireExternalAssets
)
$ErrorActionPreference = 'Stop'
$repositoryRoot = Split-Path $PSScriptRoot -Parent
$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory))
$reportPath = Join-Path $buildPath 'claw-graphics-results.xml'
& ctest --test-dir $buildPath -C $Configuration -R '^(WPGraphics|WorkphoneGraphics)\.' --output-on-failure --no-tests=error --output-junit $reportPath
$testExit = $LASTEXITCODE
if (-not (Test-Path -LiteralPath $reportPath)) { throw 'CTest did not produce a report.' }
[xml]$report = Get-Content -LiteralPath $reportPath -Raw
$skipped = @($report.testsuite.testcase | Where-Object { $_.status -eq 'notrun' -or $_.skipped })
$unexpectedSkips = @($skipped | Where-Object { $RequireExternalAssets -or $_.name -ne 'WorkphoneGraphics.mesh_import_assets' })
foreach ($skip in $skipped) { Write-Warning "Coverage unavailable: $($skip.name)" }
if ($testExit -ne 0 -or $unexpectedSkips.Count -gt 0) { throw 'Graphics validation failed or mandatory tests were skipped.' }
Write-Host "Graphics checks passed. Report: $reportPath"
if ($skipped.Count -gt 0) { Write-Host 'External-media coverage remains unavailable; this is not a release certification.' }
