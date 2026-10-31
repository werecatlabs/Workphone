#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SdkEvidence,
    [Parameter(Mandatory)][string]$EditorEvidence,
    [Parameter(Mandatory)][string[]]$GraphicsEvidence,
    [string]$ExhaustiveHeaderEvidence,
    [Parameter(Mandatory)][string]$OutputPath,
    [switch]$RequireComplete
)
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$output=[IO.Path]::GetFullPath($OutputPath)
if(Test-Path -LiteralPath $output) { throw 'Use a new release evidence path to preserve previous results.' }
$report=[ordered]@{schemaVersion=2;recordedAtUtc=[DateTime]::UtcNow.ToString('o');
    validationPassed=$false;releaseReady=$false;releaseCertified=$false;
    scope='Consistency of current Windows x64 Core/Claw/SQLite SDK and Editor release evidence';gates=@()}
function Read-Evidence([string]$Path) {
    $resolved=(Resolve-Path -LiteralPath $Path).Path
    [pscustomobject]@{path=$resolved;sha256=(Get-FileHash -LiteralPath $resolved).Hash;
        value=(Get-Content -LiteralPath $resolved -Raw | ConvertFrom-Json)}
}
function Assert-Hash([string]$Path,[string]$Hash) {
    if(-not $Hash -or -not(Test-Path -LiteralPath $Path -PathType Leaf) -or
        (Get-FileHash -LiteralPath $Path).Hash -ne $Hash) { throw "Release evidence input changed or missing: $Path" }
}
function Assert-Inputs($Inputs,[string]$Root='') {
    foreach($inputEntry in $Inputs) {
        $path=if([IO.Path]::IsPathRooted($inputEntry.path)){$inputEntry.path}else{Join-Path $Root $inputEntry.path}
        Assert-Hash $path $inputEntry.sha256
    }
}
function Add-Gate([string]$Name,[bool]$Passed,[string]$Detail) {
    $report.gates+=@([ordered]@{name=$Name;status=$(if($Passed){'passed'}else{'open'});detail=$Detail})
}
try {
    $sdk=Read-Evidence $SdkEvidence
    $editor=Read-Evidence $EditorEvidence
    if(-not $sdk.value.validationPassed -or -not $editor.value.validationPassed) { throw 'A package validation report did not pass.' }
    if(-not $sdk.value.inputs.Count -or -not $editor.value.packagingInputs.Count -or
        $sdk.value.headerResults.Count -ne 2 -or @($sdk.value.headerResults | Where-Object { -not $_.validationPassed }).Count) {
        throw 'Package reports lack current source or required standalone-header evidence.'
    }
    if(-not $sdk.value.abi.validationPassed -or $sdk.value.abi.nativeLayoutDefinitions.Count -ne 5 -or
        @($sdk.value.consumerResults | Where-Object { -not $_.nativeUiLayoutPassed }).Count -or
        $sdk.value.consumerResults.Count -ne 2) { throw 'SDK native/C++ ABI or caller-owned UI layout evidence is incomplete.' }
    if($sdk.value.buildArtifacts.Count -ne 15 -or $sdk.value.cookedContent.files.Count -ne 7 -or
        $sdk.value.cookedConsumerResults.Count -ne 2 -or
        @($sdk.value.cookedConsumerResults | Where-Object { -not $_.validationPassed -or $_.cases.Count -ne 17 }).Count -or
        @($sdk.value.headerResults.configuration | Sort-Object -Unique).Count -ne 2 -or
        @($sdk.value.cookedConsumerResults.configuration | Sort-Object -Unique).Count -ne 2 -or
        @($sdk.value.headerResults.configuration | Where-Object { $_ -notin @('Release','RelWithDebInfo') }).Count -or
        @($sdk.value.cookedConsumerResults.configuration | Where-Object { $_ -notin @('Release','RelWithDebInfo') }).Count) {
        throw 'SDK report lacks the complete runtime, typed-family or consumer-configuration coverage.'
    }
    foreach($package in @($sdk,$editor)) {
        Assert-Hash $package.value.archive.path $package.value.archive.sha256
    }
    Assert-Hash $editor.value.symbols.path $editor.value.symbols.sha256
    Assert-Inputs $sdk.value.inputs
    Assert-Inputs $editor.value.packagingInputs $repo
    $report['sdk']=[ordered]@{evidence=$sdk.path;evidenceSha256=$sdk.sha256;archive=$sdk.value.archive;
        cookedConfigurations=$sdk.value.cookedConsumerResults;headers=$sdk.value.headerResults}
    $report['editor']=[ordered]@{evidence=$editor.path;evidenceSha256=$editor.sha256;archive=$editor.value.archive;
        symbols=$editor.value.symbols.path;checks=$editor.value.checks.Count}
    # Both candidates must contain the exact same runtime, not just binaries
    # bearing the same filenames or a successful report from an earlier build.
    $shared=@(foreach($entry in $sdk.value.buildArtifacts | Where-Object path -like 'bin/*') {
        $match=@($editor.value.payload | Where-Object path -eq $entry.path)
        if($match.Count -ne 1 -or $match[0].sha256 -ne $entry.sha256) { throw "SDK/Editor runtime mismatch: $($entry.path)" }
        Assert-Hash $entry.buildPath $entry.sha256
        [ordered]@{path=$entry.path;sha256=$entry.sha256}
    })
    $report['sharedRuntime']=$shared
    $baseline=@(foreach($path in $GraphicsEvidence) {
        $item=Read-Evidence $path
        if($item.value.testExitCode -ne 0) { throw "Graphics tests failed: $path" }
        $missing=@($item.value.requiredTests | Where-Object { $_ -notin $item.value.tests.name })
        if(-not $item.value.sourceInputs.Count -or -not $item.value.executableInputs.Count -or
            -not $item.value.tests.Count -or -not $item.value.runtimeLibraries.Count -or $missing.Count -or
            -not $item.value.requiredTests.Count -or
            @($item.value.tests | Where-Object { $_.status -notin @('passed','unavailable') }).Count -or
            $item.value.changedSourceInputs.Count -or $item.value.changedExecutableInputs.Count -or
            $item.value.changedRuntimeLibraries.Count -or $item.value.changedTestBinaries.Count) {
            throw "Graphics evidence is incomplete or changed during execution: $path"
        }
        Assert-Inputs $item.value.sourceInputs
        Assert-Inputs $item.value.executableInputs
        Assert-Inputs $item.value.runtimeLibraries
        foreach($binary in $shared | Where-Object { $item.value.configuration -eq 'RelWithDebInfo' -and $_.path -like '*.dll' }) {
            $runtime=@($item.value.runtimeLibraries | Where-Object { [IO.Path]::GetFileName($_.path) -ieq [IO.Path]::GetFileName($binary.path) })
            if(-not $runtime.Count -or @($runtime | Where-Object sha256 -ne $binary.sha256).Count) { throw "Tested runtime is absent or differs from candidates: $($binary.path)" }
        }
        [ordered]@{evidence=$item.path;evidenceSha256=$item.sha256;configuration=$item.value.configuration;
            tests=$item.value.tests.Count;passed=@($item.value.tests | Where-Object status -eq 'passed').Count;
            unavailable=@($item.value.tests | Where-Object status -eq 'unavailable').name;
            coverageComplete=$item.value.coverageComplete;devices=$item.value.gpuDevices}
    })
    if(@($baseline.configuration | Sort-Object -Unique).Count -ne $baseline.Count -or
        'RelWithDebInfo' -notin $baseline.configuration -or
        @($baseline.configuration | Where-Object { $_ -notin @('Debug','RelWithDebInfo') }).Count) { throw 'Supply unique Debug/RelWithDebInfo baseline configurations including the candidate RelWithDebInfo runtime.' }
    $report['graphics']=$baseline
    Add-Gate 'Installed SDK and Editor distribution' $true 'Archive identities, source inputs and shared producing runtime agree.'
    Add-Gate 'Graphics/resource baseline including recovery and bounded soaks' (-not @($baseline | Where-Object { -not $_.coverageComplete }).Count) 'External-media skips remain unavailable coverage; bounded contract soaks do not certify the long mixed-scene gate.'
    $headers=if($ExhaustiveHeaderEvidence){Read-Evidence $ExhaustiveHeaderEvidence}else{$null}
    if($headers) {
        $candidates=@($headers.value.inventory | Where-Object kind -eq 'standaloneCandidate')
        if(-not $headers.value.exhaustive -or -not $headers.value.inventory.Count -or
            @($headers.value.inventory.header | Sort-Object -Unique).Count -ne $headers.value.inventory.Count -or
            $candidates.Count -ne $headers.value.checkedHeaders -or
            ($headers.value.passedHeaders + $headers.value.failedHeaders) -ne $headers.value.checkedHeaders -or
            ($headers.value.fullPublicSurfacePassed -and $headers.value.failedHeaders -ne 0)) {
            throw 'Exhaustive header evidence has incomplete or inconsistent coverage.'
        }
        foreach($header in $headers.value.inventory) {
            Assert-Hash (Join-Path (Split-Path $sdk.path -Parent) "WorkphoneCppSDK/include/$($header.header)") $header.sha256
        }
        $report['exhaustiveHeaders']=[ordered]@{evidence=$headers.path;evidenceSha256=$headers.sha256;
            checked=$headers.value.checkedHeaders;passed=$headers.value.passedHeaders;failed=$headers.value.failedHeaders}
    }
    Add-Gate 'Complete installed public header surface' ([bool]($headers -and $headers.value.fullPublicSurfacePassed)) 'Required supported headers and the exhaustive legacy header audit have distinct acceptance scopes.'
    Add-Gate 'Broader engine typed assets and optional-plugin SDKs' $false 'The six existing graphics formats are covered. Imported mesh/skeleton/clip cooking and SDK link surfaces for the other runtime plugins are outside this candidate.'
    $audits=@(foreach($package in @($sdk,$editor)) {
        $audit=Read-Evidence (Join-Path (Split-Path $package.path -Parent) 'dependency-media-audit.json')
        if(-not $audit.value.inventoryComplete -or -not $audit.value.noticeBytesVerified) { throw 'Dependency/media inventory or required notice verification is incomplete.' }
        $auditPayload=@($audit.value.payload | Sort-Object path | ForEach-Object { "$($_.path)|$($_.bytes)|$($_.sha256)" })
        $packagePayload=@($package.value.payload | Sort-Object path | ForEach-Object { "$($_.path)|$($_.bytes)|$($_.sha256)" })
        if(-not $auditPayload.Count -or (Compare-Object $auditPayload $packagePayload)) { throw 'Dependency audit does not describe the exact validated package payload.' }
        [ordered]@{evidence=$audit.path;evidenceSha256=$audit.sha256;openReviewItems=$audit.value.openReviewItems}
    })
    $report['dependencyAudits']=$audits
    Add-Gate 'Bundled required dependency notices and payload inventory' $true 'Original notice bytes and actual binaries/media inventoried; permission approval is separate.'
    Add-Gate 'Complete dependency/media licensing review' $false 'Review the candidate audit openReviewItems, including media, Native File Dialog and ZZipLib terms.'
    Add-Gate 'Interactive Editor authoring matrix' $false 'No complete candidate-bound interactive authoring acceptance is supplied by these automated reports.'
    Add-Gate 'Fresh Windows prerequisite execution' $false 'An offline kit is preparation evidence; actual clean guest execution is required.'
    Add-Gate 'Representative physical GPU performance and recovery' $false 'Software/basic-adapter execution cannot certify physical hardware.'
    Add-Gate 'Long mixed-scene and memory-pressure soak' $false 'The current bounded lifecycle/material/terrain contracts do not establish this gate.'
    $report['validationPassed']=$true
    $report['releaseReady']=@($report.gates | Where-Object status -ne 'passed').Count -eq 0
} catch {
    $report['error']=$_.Exception.Message
    throw
} finally {
    $parent=Split-Path $output -Parent
    if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent | Out-Null}
    $report | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $output -Encoding UTF8
}
if($RequireComplete -and -not $report.releaseReady) {
    throw "Release gates remain open: $((@($report.gates | Where-Object status -ne 'passed').name) -join ', ')"
}
[pscustomobject]$report
