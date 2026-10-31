#requires -Version 5.1
[CmdletBinding()]
param([string]$PayloadRoot='C:\WorkphoneEditorPayload', [string]$EvidenceRoot='C:\WorkphoneEditorEvidence',
    [switch]$ProvisionProvidedPrerequisite)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$requestedPayload=$PayloadRoot; $requestedEvidence=$EvidenceRoot; $requestedProvision=[bool]$ProvisionProvidedPrerequisite
. (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1') -LibraryOnly
$PayloadRoot=$requestedPayload; $EvidenceRoot=$requestedEvidence
$ProvisionProvidedPrerequisite=[switch]$requestedProvision
$script:runs=New-Object 'Collections.Generic.List[object]'
$passed=$false; $failure=$null; $baseline=$null; $inventoryVerified=$false; $guestIdentity=$false; $moduleInventory=@()
$prerequisites=[ordered]@{baselineProbePassed=$false;baselineLoaderCode=$null;missingDllObserved=$false
    providedInstallerRun=$false;installedAfterBaseline=$false;postProvisionProbePassed=$false;rebootRequired=$false}
$manifest=Get-Content -LiteralPath (Join-Path $PayloadRoot 'candidate-manifest.json') -Raw | ConvertFrom-Json
$launch=Get-Content -LiteralPath (Join-Path $PayloadRoot 'launch-host.json') -Raw | ConvertFrom-Json
Assert-Clean (-not(Test-Path -LiteralPath (Join-Path $EvidenceRoot 'guest-result.json'))) 'Preserve previous guest evidence; prepare a fresh kit.'
$work=Join-Path $env:TEMP ('Workphone Editor clean '+$manifest.runId)
$install=Join-Path $work 'install'
$moduleFile=Join-Path $EvidenceRoot 'editor-loaded-modules.txt'
try {
    Assert-Clean ([Environment]::Is64BitProcess) 'Run stock 64-bit Windows PowerShell.'
    $baseline=Get-GuestFacts
    Assert-Clean ($manifest.schema -eq 'workphone.editor-clean-machine-candidate.v1' -and $manifest.runId -eq $launch.runId) 'Candidate/launch identity mismatch.'
    Assert-Clean ($launch.environmentKind -eq 'WindowsSandbox' -and $env:USERNAME -eq 'WDAGUtilityAccount' -and
        $baseline.machineGuid -ne $launch.machineGuid -and $baseline.computerName -ne $launch.computerName) 'A separately identified fresh Sandbox guest is required.'
    Assert-Clean (@($baseline.developmentTools).Count -eq 0) 'Developer tools were found in the supposed clean guest.'
    $guestIdentity=$true
    $baseline | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'baseline-environment.json') -Encoding UTF8
    Assert-Clean ((Get-CleanHash $PSCommandPath) -eq $manifest.guestRunnerSha256 -and
        (Get-CleanHash (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1')) -eq $manifest.commonRunnerSha256) 'Guest tooling differs from the pinned kit.'
    $archive=Join-Path $PayloadRoot $manifest.archiveFileName
    Assert-Clean ((Get-CleanHash $archive) -eq $manifest.archiveSha256) 'Archive hash mismatch.'
    Assert-Clean (-not(Test-Path -LiteralPath $work)) 'Fresh guest workspace already exists.'
    New-Item -ItemType Directory -Path $work | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive)
    try {
        $seen=@{}
        foreach($entry in $zip.Entries){
            [void](Get-ContainedPath $install $entry.FullName)
            Assert-Clean (-not $seen.ContainsKey($entry.FullName.ToLowerInvariant())) 'Duplicate ZIP path.'
            $seen[$entry.FullName.ToLowerInvariant()]=$true
        }
    } finally {$zip.Dispose()}
    Expand-Archive -LiteralPath $archive -DestinationPath $install
    Assert-CleanInventory $manifest.expectedFiles $install
    $exe=Join-Path $install 'bin/Editor.exe'
    Add-Type -TypeDefinition 'using System.Runtime.InteropServices; public static class EditorGuestErrorMode { [DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode); }'
    [void][EditorGuestErrorMode]::SetErrorMode(0x8003)
    $probe=Invoke-CleanProcess $exe @('--help') $work 'baseline-loader-probe' 90
    $prerequisites.baselineProbePassed=($probe.entry.exitCode -eq 0 -and -not $probe.entry.error)
    $prerequisites.baselineLoaderCode=$probe.entry.exitCodeHex
    $prerequisites.missingDllObserved=($probe.entry.exitCodeHex -eq 'C0000135')
    if(-not $prerequisites.baselineProbePassed){
        Assert-Clean $prerequisites.missingDllObserved 'Loader failed for a cause other than a missing DLL.'
        Assert-Clean ($ProvisionProvidedPrerequisite -and $manifest.prerequisite) 'Missing runtime recorded; no prerequisite is installed without an explicitly supplied installer and opt-in.'
        $installer=Join-Path $PayloadRoot $manifest.prerequisite.fileName
        Assert-Clean ((Get-CleanHash $installer) -eq $manifest.prerequisite.sha256) 'Prerequisite bytes changed.'
        $signature=Get-AuthenticodeSignature -LiteralPath $installer
        Assert-Clean ($signature.SignerCertificate -and $signature.SignerCertificate.Thumbprint -eq $manifest.prerequisite.signerThumbprint) 'Prerequisite signer changed.'
        $provision=Invoke-CleanProcess $installer @('/install','/quiet','/norestart') $work 'provided-prerequisite-install' 180
        $prerequisites.providedInstallerRun=$true
        $prerequisites.rebootRequired=($provision.entry.exitCode -eq 3010)
        Assert-Clean ($provision.entry.exitCode -eq 0 -and -not $provision.entry.error) 'Runtime installation failed or requires restart; acceptance remains open.'
        $prerequisites.installedAfterBaseline=$true
        $probe=Invoke-CleanProcess $exe @('--help') $work 'post-provision-loader-probe' 90
        $prerequisites.postProvisionProbePassed=($probe.entry.exitCode -eq 0 -and -not $probe.entry.error)
        Assert-Clean $prerequisites.postProvisionProbePassed 'Loader still fails after the supplied prerequisite.'
    }
    Get-ChildItem -LiteralPath $install -Recurse -File | ForEach-Object {$_.IsReadOnly=$true}
    $workspace=Join-Path $work 'workspace'; New-Item -ItemType Directory -Path $workspace | Out-Null
    $env:WP_EDITOR_MODULE_REPORT=$moduleFile
    $startup=Invoke-CleanProcess $exe @('--package-smoke','--workspace',$workspace) $workspace 'editor-startup' 90
    Assert-Clean ($startup.entry.exitCode -eq 0 -and -not $startup.entry.error -and
        $startup.output -match 'Editor package startup and teardown passed' -and
        $startup.output -notmatch 'stack traceback:|No matching overload found') 'Editor startup/Lua/teardown smoke failed.'
    $modules=@(Get-Content -LiteralPath $moduleFile)
    Assert-Clean ($modules.Count -gt 0) 'Empty Editor module inventory.'
    $moduleInventory=@($modules | ForEach-Object {
        $path=[IO.Path]::GetFullPath($_)
        Assert-Clean ($path.StartsWith($install+'\',[StringComparison]::OrdinalIgnoreCase) -or
            $path.StartsWith($env:WINDIR+'\',[StringComparison]::OrdinalIgnoreCase)) "Unexpected guest module: $path"
        [ordered]@{path=$path;sha256=Get-CleanHash $path}
    })
    foreach($plugin in $manifest.plugins){Assert-Clean (@($modules | Where-Object {[IO.Path]::GetFileName($_) -ieq ($plugin+'.dll')}).Count -eq 1) "Configured plugin was not loaded: $plugin"}
    $negative=Join-Path $work 'missing-media'; Copy-Item -LiteralPath $install -Destination $negative -Recurse
    Get-ChildItem -LiteralPath $negative -Recurse -File | ForEach-Object {$_.IsReadOnly=$false}
    $missingPath=Get-ContainedPath $negative 'Media/Scripts/Lua/Editor/Editor.lua'
    Remove-Item -LiteralPath $missingPath
    $bad=Invoke-CleanProcess (Join-Path $negative 'bin/Editor.exe') @('--package-smoke','--workspace',(Join-Path $work 'negative-workspace')) $work 'missing-media' 90
    Assert-Clean ($bad.entry.exitCode -eq 1 -and -not $bad.entry.error -and $bad.output -match 'Editor package media is missing') 'Missing media did not fail for the intended reason.'
    Assert-CleanInventory $manifest.expectedFiles $install
    Assert-Clean ((Get-CleanHash $archive) -eq $manifest.archiveSha256) 'Read-only input archive changed.'
    $inventoryVerified=$true; $passed=$true
} catch {$failure=$_.Exception.Message; Write-Error -Message $failure -ErrorAction Continue} finally {
    $baselinePath=Join-Path $EvidenceRoot 'baseline-environment.json'
    $baselineHash=$null; if(Test-Path -LiteralPath $baselinePath){$baselineHash=Get-CleanHash $baselinePath}
    $moduleHash=$null; if(Test-Path -LiteralPath $moduleFile){$moduleHash=Get-CleanHash $moduleFile}
    [ordered]@{schema='workphone.editor-clean-machine-guest.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o');runId=$manifest.runId
        archiveSha256=$manifest.archiveSha256;candidateManifestSha256=Get-CleanHash (Join-Path $PayloadRoot 'candidate-manifest.json')
        guestRunnerSha256=Get-CleanHash $PSCommandPath;commonRunnerSha256=Get-CleanHash (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1')
        environmentKind='WindowsSandbox';guestIdentityVerified=$guestIdentity;environment=$baseline;baselineEnvironmentSha256=$baselineHash
        prerequisites=$prerequisites;runs=@($script:runs.ToArray());loadedModules=$moduleInventory
        loadedModuleReportSha256=$moduleHash;installRoot=$install;windowsRoot=$env:WINDIR
        installedInventoryUnchanged=$inventoryVerified;guestValidationPassed=$passed;failure=$failure
        cleanMachineAcceptanceRequiresHostReconciliation=$true;interactiveEditorVerified=$false;hardwareAcceptanceVerified=$false
        licenseAuditComplete=$false;releaseCertified=$false} | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'guest-result.json') -Encoding UTF8
    [IO.File]::WriteAllText((Join-Path $EvidenceRoot 'guest-complete.txt'),$manifest.runId)
}
if(-not $passed){exit 1}
