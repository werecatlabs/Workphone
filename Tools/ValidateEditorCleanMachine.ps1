#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('Prepare','Launch','Collect')][string]$Mode='Prepare',
    [Parameter(Mandatory)][string]$KitDirectory,
    [Parameter(Mandatory)][ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$ExpectedArchiveSha256,
    [string]$ArchivePath, [string]$PackageEvidencePath, [string]$ProvidedRedistributable,
    [switch]$ProvisionProvidedPrerequisite, [switch]$VisibleSandbox
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$requestedProvision=[bool]$ProvisionProvidedPrerequisite
. (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1') -LibraryOnly
$ProvisionProvidedPrerequisite=[switch]$requestedProvision
$kit=[IO.Path]::GetFullPath($KitDirectory)
$payload=Join-Path $kit 'payload'; $evidence=Join-Path $kit 'evidence'
$candidatePath=Join-Path $payload 'candidate-manifest.json'
function Write-EditorJson($Object,[string]$Path){$Object | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $Path -Encoding UTF8}
function Get-EditorHostCapability {
    [ordered]@{recordedAtUtc=[DateTime]::UtcNow.ToString('o');computerName=$env:COMPUTERNAME
        machineGuid=(Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Cryptography').MachineGuid
        os=Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,OSArchitecture
        computer=Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer,Model,HypervisorPresent
        optionalFeatures=@(Get-CimInstance Win32_OptionalFeature | Where-Object {$_.Name -in @('Containers-DisposableClientVM','Microsoft-Hyper-V-All','VirtualMachinePlatform')} | Select-Object Name,InstallState)
        sandboxExecutablePresent=(Test-Path -LiteralPath "$env:WINDIR\System32\WindowsSandbox.exe")
        hostFeaturesModified=$false;hostPrerequisitesInstalled=$false;cleanMachineExecuted=$false}
}
function Write-EditorSandbox([bool]$Provision) {
    $command='powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File C:\WorkphoneEditorPayload\EditorCleanMachineGuest.ps1'
    if($Provision){$command+=' -ProvisionProvidedPrerequisite'}
    $p=[Security.SecurityElement]::Escape($payload); $e=[Security.SecurityElement]::Escape($evidence); $c=[Security.SecurityElement]::Escape($command)
    @"
<Configuration>
  <VGpu>Disable</VGpu><Networking>Disable</Networking><AudioInput>Disable</AudioInput><VideoInput>Disable</VideoInput>
  <PrinterRedirection>Disable</PrinterRedirection><ClipboardRedirection>Disable</ClipboardRedirection><MemoryInMB>8192</MemoryInMB>
  <MappedFolders>
    <MappedFolder><HostFolder>$p</HostFolder><SandboxFolder>C:\WorkphoneEditorPayload</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>
    <MappedFolder><HostFolder>$e</HostFolder><SandboxFolder>C:\WorkphoneEditorEvidence</SandboxFolder><ReadOnly>false</ReadOnly></MappedFolder>
  </MappedFolders>
  <LogonCommand><Command>$c</Command></LogonCommand>
</Configuration>
"@ | Set-Content -LiteralPath (Join-Path $kit 'WorkphoneEditor.wsb') -Encoding UTF8
}
if($Mode -eq 'Prepare'){
    Assert-Clean ($ArchivePath -and $PackageEvidencePath) 'Prepare requires the candidate archive and its package evidence.'
    Assert-Clean (-not(Test-Path -LiteralPath $kit)) 'Choose a fresh kit directory; earlier evidence is preserved.'
    $archive=(Resolve-Path -LiteralPath $ArchivePath).Path
    $packageReport=(Resolve-Path -LiteralPath $PackageEvidencePath).Path
    $accepted=Get-Content -LiteralPath $packageReport -Raw | ConvertFrom-Json
    Assert-Clean ($accepted.validationPassed -and $accepted.archive.sha256 -eq $ExpectedArchiveSha256 -and
        (Get-CleanHash $archive) -eq $ExpectedArchiveSha256) 'Candidate/archive identity does not match passing package evidence.'
    Assert-Clean (@($accepted.payload | Where-Object path -eq 'bin/Editor.exe').Count -eq 1) 'Candidate does not contain the Editor runtime.'
    New-Item -ItemType Directory -Path $kit,$payload,$evidence | Out-Null
    Copy-Item -LiteralPath $archive -Destination (Join-Path $payload 'WorkphoneEditor.zip')
    foreach($name in @('EditorCleanMachineGuest.ps1','TerrainCleanMachineGuest.ps1')){
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $payload
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $name) -Destination $kit
    }
    Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $kit 'ValidateEditorCleanMachine.ps1')
    Copy-Item -LiteralPath $packageReport -Destination (Join-Path $kit 'accepted-package-evidence.json')
    $prerequisite=$null
    if($ProvidedRedistributable){
        $installer=(Resolve-Path -LiteralPath $ProvidedRedistributable).Path
        $signature=Get-AuthenticodeSignature -LiteralPath $installer
        $version=(Get-Item -LiteralPath $installer).VersionInfo
        Assert-Clean ($signature.Status -eq 'Valid' -and $signature.SignerCertificate -and
            $signature.SignerCertificate.Subject -match 'Microsoft Corporation' -and
            ($version.ProductName+' '+$version.FileDescription) -match 'Microsoft.*Visual C\+\+.*Redistributable' -and
            ($version.ProductName+' '+$version.FileDescription) -match '\(x64\)') 'Supply a valid Microsoft-signed x64 Visual C++ Redistributable.'
        Copy-Item -LiteralPath $installer -Destination (Join-Path $payload 'vc_redist.x64.exe')
        $prerequisite=[ordered]@{fileName='vc_redist.x64.exe';sha256=Get-CleanHash $installer
            signerThumbprint=$signature.SignerCertificate.Thumbprint;fileVersion=$version.FileVersion}
    }
    Assert-Clean (-not $ProvisionProvidedPrerequisite -or $prerequisite) 'Provisioning requires an explicitly supplied signed prerequisite.'
    $manifest=[ordered]@{schema='workphone.editor-clean-machine-candidate.v1';runId=[guid]::NewGuid().ToString('N')
        preparedAtUtc=[DateTime]::UtcNow.ToString('o');archiveFileName='WorkphoneEditor.zip';archiveSha256=$ExpectedArchiveSha256
        acceptedPackageReportSha256=Get-CleanHash $packageReport;expectedFiles=@($accepted.payload);plugins=@($accepted.plugins)
        guestRunnerSha256=Get-CleanHash (Join-Path $payload 'EditorCleanMachineGuest.ps1')
        commonRunnerSha256=Get-CleanHash (Join-Path $payload 'TerrainCleanMachineGuest.ps1')
        prerequisite=$prerequisite;provisionProvidedPrerequisite=[bool]$ProvisionProvidedPrerequisite}
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive); $observed=@{}
    try{
        foreach($entry in $zip.Entries){
            [void](Get-ContainedPath (Join-Path $kit 'unexpanded-placeholder') $entry.FullName)
            if($entry.FullName.EndsWith('/')){continue}
            $name=$entry.FullName.Replace('\','/').ToLowerInvariant()
            Assert-Clean (-not $observed.ContainsKey($name)) 'Duplicate archive entry.'
            $stream=$entry.Open();$sha=[Security.Cryptography.SHA256]::Create()
            try{$hash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')}finally{$stream.Dispose();$sha.Dispose()}
            $observed[$name]=[ordered]@{bytes=$entry.Length;sha256=$hash}
        }
    }finally{$zip.Dispose()}
    Assert-Clean ($observed.Count -eq @($manifest.expectedFiles).Count) 'ZIP inventory differs from accepted package evidence.'
    foreach($file in $manifest.expectedFiles){
        $name=$file.path.Replace('\','/').ToLowerInvariant()
        Assert-Clean ($observed.ContainsKey($name) -and $observed[$name].sha256 -eq $file.sha256 -and $observed[$name].bytes -eq $file.bytes) "ZIP entry differs: $name"
    }
    Write-EditorJson $manifest $candidatePath
    Write-EditorJson (Get-EditorHostCapability) (Join-Path $kit 'host-capability.json')
    Write-EditorSandbox ([bool]$ProvisionProvidedPrerequisite)
    Write-EditorJson ([ordered]@{schema='workphone.editor-clean-machine-kit.v1';runId=$manifest.runId;archiveSha256=$ExpectedArchiveSha256
        manifestSha256=Get-CleanHash $candidatePath;guestRunnerSha256=$manifest.guestRunnerSha256;commonRunnerSha256=$manifest.commonRunnerSha256
        sandboxConfigurationSha256=Get-CleanHash (Join-Path $kit 'WorkphoneEditor.wsb');preparedOnly=$true;guestLaunched=$false
        cleanMachineExecuted=$false;cleanMachineValidationPassed=$false;licenseAuditComplete=$false;releaseCertified=$false}) (Join-Path $kit 'kit-evidence.json')
    @"
Workphone Editor offline clean-Windows kit

Preparation has not launched a guest, enabled host features or installed software.
Only the pinned release ZIP and runner files are mapped read-only; the evidence
directory is mapped writable. Networking, vGPU and clipboard are disabled.
The guest checks loader prerequisites, headless Editor/Lua teardown, required
plugins, missing-media rejection and immutable installed hashes. It does not
certify interactive authoring, physical graphics hardware or media permissions.

On a host where Windows Sandbox is ALREADY enabled, with no existing session:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ValidateEditorCleanMachine.ps1 -Mode Launch -KitDirectory . -ExpectedArchiveSha256 $ExpectedArchiveSha256
After evidence\guest-complete.txt appears:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ValidateEditorCleanMachine.ps1 -Mode Collect -KitDirectory . -ExpectedArchiveSha256 $ExpectedArchiveSha256

No prerequisite is downloaded or installed automatically. A missing-DLL baseline
remains a failure unless preparation explicitly supplied a Microsoft-signed x64
redistributable and opted into guest-only provisioning. Restart-required results
remain unaccepted. No host feature enablement/reboot is performed.
"@ | Set-Content -LiteralPath (Join-Path $kit 'README.txt') -Encoding UTF8
    Write-Output "Prepared only; clean-machine execution remains open: $kit"
    return
}
$manifest=Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
$prepared=Get-Content -LiteralPath (Join-Path $kit 'kit-evidence.json') -Raw | ConvertFrom-Json
Assert-Clean ($manifest.schema -eq 'workphone.editor-clean-machine-candidate.v1' -and $manifest.runId -eq $prepared.runId -and
    $manifest.archiveSha256 -eq $ExpectedArchiveSha256 -and $prepared.archiveSha256 -eq $ExpectedArchiveSha256 -and
    (Get-CleanHash $candidatePath) -eq $prepared.manifestSha256) 'Prepared candidate identity changed.'
Assert-Clean ((Get-CleanHash (Join-Path $payload $manifest.archiveFileName)) -eq $ExpectedArchiveSha256 -and
    (Get-CleanHash (Join-Path $payload 'EditorCleanMachineGuest.ps1')) -eq $manifest.guestRunnerSha256 -and
    (Get-CleanHash (Join-Path $payload 'TerrainCleanMachineGuest.ps1')) -eq $manifest.commonRunnerSha256) 'Prepared payload or runners changed.'
if($Mode -eq 'Launch'){
    Assert-Clean (-not(Test-Path -LiteralPath (Join-Path $kit 'host-launch.json'))) 'A launch record already exists; prepare a fresh kit.'
    $facts=Get-EditorHostCapability
    Write-EditorJson $facts (Join-Path $kit 'launch-host-capability.json')
    $enabled=@($facts.optionalFeatures | Where-Object {$_.Name -eq 'Containers-DisposableClientVM' -and $_.InstallState -eq 1}).Count -eq 1
    Assert-Clean ($facts.sandboxExecutablePresent -and $enabled) 'Windows Sandbox is absent/disabled; use an already provisioned host. No feature was changed.'
    Assert-Clean (@(Get-Process WindowsSandbox -ErrorAction SilentlyContinue).Count -eq 0 -and
        @(Get-ChildItem -LiteralPath $evidence -Force).Count -eq 0) 'A fresh Sandbox session and empty evidence directory are required.'
    Assert-Clean ((Get-CleanHash (Join-Path $kit 'WorkphoneEditor.wsb')) -eq $prepared.sandboxConfigurationSha256) 'Sandbox configuration changed.'
    $launch=[ordered]@{runId=$manifest.runId;environmentKind='WindowsSandbox';machineGuid=$facts.machineGuid;computerName=$facts.computerName
        manifestSha256=Get-CleanHash $candidatePath;configurationSha256=$prepared.sandboxConfigurationSha256;freshSessionChecked=$true
        networking='Disable';vGPU='Disable';launchedAtUtc=[DateTime]::UtcNow.ToString('o')}
    Write-EditorJson $launch (Join-Path $payload 'launch-host.json')
    $style='Hidden';if($VisibleSandbox){$style='Normal'}
    $process=Start-Process -FilePath "$env:WINDIR\System32\WindowsSandbox.exe" -ArgumentList (ConvertTo-WindowsArgument (Join-Path $kit 'WorkphoneEditor.wsb')) -WindowStyle $style -PassThru
    $launch['processId']=$process.Id;Write-EditorJson $launch (Join-Path $kit 'host-launch.json')
    Write-Output "Sandbox launch requested; collect after the matching completion marker: $kit"
    return
}
$launch=Get-Content -LiteralPath (Join-Path $kit 'host-launch.json') -Raw | ConvertFrom-Json
$resultPath=Join-Path $evidence 'guest-result.json';$result=Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
Assert-Clean ($launch.runId -eq $manifest.runId -and $launch.freshSessionChecked -and $launch.networking -eq 'Disable' -and $launch.vGPU -eq 'Disable' -and
    $launch.manifestSha256 -eq (Get-CleanHash $candidatePath) -and $launch.configurationSha256 -eq (Get-CleanHash (Join-Path $kit 'WorkphoneEditor.wsb'))) 'Launch identity/configuration is inconsistent.'
Assert-Clean ((Get-Content -LiteralPath (Join-Path $evidence 'guest-complete.txt') -Raw).Trim() -eq $manifest.runId -and
    $result.schema -eq 'workphone.editor-clean-machine-guest.v1' -and $result.runId -eq $manifest.runId -and
    $result.archiveSha256 -eq $ExpectedArchiveSha256 -and $result.candidateManifestSha256 -eq $launch.manifestSha256 -and
    $result.guestRunnerSha256 -eq $manifest.guestRunnerSha256 -and $result.commonRunnerSha256 -eq $manifest.commonRunnerSha256) 'Guest evidence identity changed.'
Assert-Clean ($result.guestIdentityVerified -and $result.environment.machineGuid -ne $launch.machineGuid -and
    $result.environment.computerName -ne $launch.computerName -and @($result.environment.developmentTools).Count -eq 0 -and
    $result.guestValidationPassed -and $result.installedInventoryUnchanged -and -not $result.failure) 'Clean guest validation did not pass.'
$baselinePath=Join-Path $evidence 'baseline-environment.json'
Assert-Clean ((Get-CleanHash $baselinePath) -eq $result.baselineEnvironmentSha256) 'Guest baseline inventory changed.'
$baseline=Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json
Assert-Clean (($baseline | ConvertTo-Json -Depth 12 -Compress) -ceq ($result.environment | ConvertTo-Json -Depth 12 -Compress)) 'Guest baseline differs between reports.'
$expectedNames=@('baseline-loader-probe','editor-startup','missing-media')
if($result.prerequisites.providedInstallerRun){$expectedNames+=@('provided-prerequisite-install','post-provision-loader-probe')}
Assert-Clean ((@($result.runs.name | Sort-Object) -join "`n") -ceq (@($expectedNames | Sort-Object) -join "`n")) 'Guest process set changed.'
foreach($run in $result.runs){
    $watchdog=90;if($run.name -eq 'provided-prerequisite-install'){$watchdog=180}
    Assert-Clean (-not $run.timedOut -and -not $run.error -and $run.watchdogSeconds -eq $watchdog -and
        (Get-CleanHash (Get-ContainedPath $evidence $run.logFile)) -eq $run.logSha256) "Guest process/log failed: $($run.name)"
    $log=Get-Content -LiteralPath (Get-ContainedPath $evidence $run.logFile) -Raw
    if($run.name -eq 'missing-media'){Assert-Clean ($run.exitCode -eq 1 -and $log -match 'Editor package media is missing') 'Missing-media rejection changed.'}
    elseif($run.name -eq 'editor-startup'){Assert-Clean ($run.exitCode -eq 0 -and $log -match 'Editor package startup and teardown passed' -and $log -notmatch 'stack traceback:|No matching overload found') 'Editor smoke failed.'}
    elseif($run.name -ne 'baseline-loader-probe' -or $result.prerequisites.baselineProbePassed){Assert-Clean ($run.exitCode -eq 0) 'Prerequisite/loader process failed.'}
}
$modulePath=Join-Path $evidence 'editor-loaded-modules.txt'
Assert-Clean ((Get-CleanHash $modulePath) -eq $result.loadedModuleReportSha256) 'Guest module inventory changed.'
$modulePaths=@(Get-Content -LiteralPath $modulePath)
Assert-Clean (($modulePaths -join "`n") -ceq (@($result.loadedModules.path) -join "`n")) 'Guest module reports disagree.'
foreach($module in $result.loadedModules){
    if($module.path.StartsWith($result.installRoot+'\',[StringComparison]::OrdinalIgnoreCase)){
        $relative=$module.path.Substring($result.installRoot.Length+1).Replace('\','/')
        $pin=@($manifest.expectedFiles | Where-Object path -ieq $relative)
        Assert-Clean ($pin.Count -eq 1 -and $pin[0].sha256 -eq $module.sha256) "Guest package module differs: $relative"
    }else{Assert-Clean ($module.path.StartsWith($result.windowsRoot+'\',[StringComparison]::OrdinalIgnoreCase)) 'Guest loaded a module outside package/Windows.'}
}
foreach($plugin in $manifest.plugins){Assert-Clean (@($modulePaths | Where-Object {[IO.Path]::GetFileName($_) -ieq ($plugin+'.dll')}).Count -eq 1) "Guest configured plugin absent: $plugin"}
Assert-Clean ($result.prerequisites.baselineProbePassed -or ($result.prerequisites.missingDllObserved -and
    $result.prerequisites.baselineLoaderCode -eq 'C0000135' -and $manifest.provisionProvidedPrerequisite -and $manifest.prerequisite -and
    $result.prerequisites.installedAfterBaseline -and $result.prerequisites.postProvisionProbePassed -and -not $result.prerequisites.rebootRequired)) 'Prerequisite transition is unverified.'
Write-EditorJson ([ordered]@{schema='workphone.editor-clean-machine-acceptance.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o')
    runId=$manifest.runId;archiveSha256=$ExpectedArchiveSha256;guestResultSha256=Get-CleanHash $resultPath
    cleanMachineValidationPassed=$true;environmentKind='fresh Windows Sandbox';interactiveEditorVerified=$false
    hardwareAcceptanceVerified=$false;licenseAuditComplete=$false;releaseCertified=$false}) (Join-Path $kit 'clean-machine-acceptance.json')
Write-Output "Fresh guest headless Editor acceptance reconciled: $kit/clean-machine-acceptance.json"
