#requires -Version 5.1
[CmdletBinding()]
param(
    [ValidateSet('Prepare','Launch','Collect')][string]$Mode = 'Prepare',
    [string]$KitDirectory,
    [string]$ArchivePath,
    [string]$PackageEvidencePath,
    [ValidatePattern('^[A-Fa-f0-9]{64}$')][string]$ExpectedArchiveSha256 = 'AF0735BA0B51140D71FD5893569F4DA0674A6C150A16898A870B9D1F7995AFB9',
    [string]$ProvidedRedistributable,
    [switch]$ProvisionProvidedPrerequisite,
    [switch]$VisibleSandbox
)
# Prepare is offline and read-only with respect to host features/prerequisites.
# Launch never enables features or installs software on the host. Collect accepts
# only a separately identified fresh Sandbox run with exact candidate identities.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repositoryRoot = Split-Path $PSScriptRoot -Parent
$requestedProvision=[bool]$ProvisionProvidedPrerequisite
. (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1') -LibraryOnly
$ProvisionProvidedPrerequisite=[switch]$requestedProvision
if (-not $KitDirectory) { $KitDirectory = Join-Path $repositoryRoot 'artifacts/terrain/gt0-20261029/clean-machine-kit-01' }
$kit = [IO.Path]::GetFullPath($KitDirectory)
$payload = Join-Path $kit 'payload'; $evidence = Join-Path $kit 'evidence'
$candidatePath = Join-Path $payload 'candidate-manifest.json'
$guestScript = Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1'

function Get-CleanHostFacts {
    $features=@(Get-CimInstance Win32_OptionalFeature | Where-Object {$_.Name -in @('Containers-DisposableClientVM','Microsoft-Hyper-V-All','Microsoft-Hyper-V-Management-PowerShell','VirtualMachinePlatform','Containers')} | ForEach-Object {
        $names=@{1='Enabled';2='Disabled';3='Absent';4='Unknown'}
        [ordered]@{name=$_.Name;installState=$_.InstallState;state=$names[[int]$_.InstallState]}
    })
    [ordered]@{recordedAtUtc=[DateTime]::UtcNow.ToString('o');computerName=$env:COMPUTERNAME;machineGuid=(Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Cryptography').MachineGuid
        os=Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,OSArchitecture
        computer=Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer,Model,HypervisorPresent,TotalPhysicalMemory
        cpu=Get-CimInstance Win32_Processor | Select-Object Name,VirtualizationFirmwareEnabled,SecondLevelAddressTranslationExtensions,VMMonitorModeExtensions
        elevated=([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
        optionalFeatures=$features;sandboxExecutable=Test-Path -LiteralPath "$env:WINDIR\System32\WindowsSandbox.exe"
        guestTools=@(Get-Command WindowsSandbox.exe,Get-VM,vmconnect.exe,VBoxManage.exe,vmrun.exe,qemu-system-x86_64.exe,docker.exe -ErrorAction SilentlyContinue | Select-Object Name,Source)
        guestServices=@(Get-Service vmcompute,vmms,hns -ErrorAction SilentlyContinue | Select-Object Name,Status,StartType)
        cleanMachineVerified=$false;hostFeaturesModified=$false;hostSoftwareInstalled=$false
    }
}
function Write-CleanJson($Object, [string]$Path) { $Object | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $Path -Encoding UTF8 }
function Assert-CandidateInputs($Manifest) {
    Assert-Clean ($Manifest.schema -eq 'workphone.terrain-clean-machine-candidate.v1') 'Unsupported candidate manifest.'
    Assert-Clean ($Manifest.runId -match '^[a-f0-9]{32}$') 'Invalid fresh-run identifier.'
    $prepared=Get-Content -LiteralPath (Join-Path $kit 'kit-evidence.json') -Raw | ConvertFrom-Json
    Assert-Clean ($Manifest.archiveSha256 -eq $ExpectedArchiveSha256 -and $prepared.archiveSha256 -eq $ExpectedArchiveSha256 -and $prepared.runId -eq $Manifest.runId -and (Get-CleanHash $candidatePath) -eq $prepared.manifestSha256) 'Prepared kit identity differs from the explicitly pinned candidate.'
    Assert-Clean ((Get-CleanHash (Join-Path $payload $Manifest.archiveFileName)) -eq $Manifest.archiveSha256) 'Prepared archive hash mismatch.'
    Assert-Clean ((Get-CleanHash (Join-Path $payload 'TerrainCleanMachineGuest.ps1')) -eq $Manifest.guestRunnerSha256) 'Prepared guest runner changed.'
    if($Manifest.prerequisite){Assert-Clean ((Get-CleanHash (Join-Path $payload $Manifest.prerequisite.fileName)) -eq $Manifest.prerequisite.sha256) 'Prepared prerequisite installer changed.'}
    Assert-Clean ($Manifest.configuration -in @('RelWithDebInfo','Release')) 'A clean-machine kit must not redistribute Debug CRT-dependent builds.'
}
function Write-SandboxConfiguration([bool]$Provision) {
    $command='powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File C:\TerrainGT0Payload\TerrainCleanMachineGuest.ps1'
    if($Provision) { $command += ' -ProvisionProvidedPrerequisite' }
    $safePayload=[Security.SecurityElement]::Escape($payload); $safeEvidence=[Security.SecurityElement]::Escape($evidence); $safeCommand=[Security.SecurityElement]::Escape($command)
    @"
<Configuration>
  <VGpu>Disable</VGpu>
  <Networking>Disable</Networking>
  <AudioInput>Disable</AudioInput>
  <VideoInput>Disable</VideoInput>
  <PrinterRedirection>Disable</PrinterRedirection>
  <ClipboardRedirection>Disable</ClipboardRedirection>
  <MemoryInMB>8192</MemoryInMB>
  <MappedFolders>
    <MappedFolder><HostFolder>$safePayload</HostFolder><SandboxFolder>C:\TerrainGT0Payload</SandboxFolder><ReadOnly>true</ReadOnly></MappedFolder>
    <MappedFolder><HostFolder>$safeEvidence</HostFolder><SandboxFolder>C:\TerrainGT0Evidence</SandboxFolder><ReadOnly>false</ReadOnly></MappedFolder>
  </MappedFolders>
  <LogonCommand><Command>$safeCommand</Command></LogonCommand>
</Configuration>
"@ | Set-Content -LiteralPath (Join-Path $kit 'TerrainGameplay.wsb') -Encoding UTF8
}

if ($Mode -eq 'Prepare') {
    if (-not $ArchivePath) { $ArchivePath=Join-Path $repositoryRoot 'artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/WorkphoneTerrainGameplay-RelWithDebInfo.zip' }
    if (-not $PackageEvidencePath) { $PackageEvidencePath=Join-Path $repositoryRoot 'artifacts/terrain/gt0-20261029/gameplay-RelWithDebInfo-package-01/gameplay-package-evidence.json' }
    $archive=(Resolve-Path -LiteralPath $ArchivePath).Path; $reportPath=(Resolve-Path -LiteralPath $PackageEvidencePath).Path
    Assert-Clean (-not (Test-Path -LiteralPath $kit)) 'Choose a fresh kit directory; earlier evidence is preserved.'
    Assert-Clean ((Get-CleanHash $archive) -eq $ExpectedArchiveSha256) 'Release ZIP differs from the explicitly pinned candidate.'
    $report=Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    Assert-Clean ($report.packagePassed -and $report.archiveExtractedAndExecuted -and $report.runtimeInputsUnchangedVerified -and $report.assetFilesUnchangedVerified -and @($report.changedSources).Count -eq 0) 'Candidate package evidence did not pass all local gates.'
    Assert-Clean ($report.archiveSha256 -eq $ExpectedArchiveSha256 -and $report.configuration -in @('RelWithDebInfo','Release')) 'Report configuration/archive identity does not match the pinned release ZIP.'
    Assert-Clean ($report.sourceBefore -eq $report.sourceAfter) 'Candidate sources drifted during local package validation.'
    New-Item -ItemType Directory -Path $kit,$payload,$evidence | Out-Null
    Copy-Item -LiteralPath $archive -Destination (Join-Path $payload 'WorkphoneTerrainGameplay.zip')
    Copy-Item -LiteralPath $guestScript -Destination $payload
    # The copied host tool plus guest helper let this kit move to another host.
    Copy-Item -LiteralPath $PSCommandPath -Destination (Join-Path $kit 'ValidateTerrainCleanMachine.ps1')
    Copy-Item -LiteralPath $guestScript -Destination (Join-Path $kit 'TerrainCleanMachineGuest.ps1')
    $prerequisite=$null
    if ($ProvidedRedistributable) {
        $installer=(Resolve-Path -LiteralPath $ProvidedRedistributable).Path
        $signature=Get-AuthenticodeSignature -LiteralPath $installer
        Assert-Clean ($signature.Status -eq 'Valid' -and $signature.SignerCertificate -and $signature.SignerCertificate.Subject -match 'Microsoft Corporation') 'Only a valid Microsoft-signed operator-provided x64 redistributable is accepted.'
        $installerVersion=(Get-Item -LiteralPath $installer).VersionInfo
        $installerDescription=([string]$installerVersion.ProductName)+' '+([string]$installerVersion.FileDescription)
        Assert-Clean ($installerDescription -match 'Microsoft.*Visual C\+\+.*Redistributable' -and $installerDescription -match '\(x64\)') 'Provided Microsoft installer is not identified as the x64 Visual C++ Redistributable.'
        Copy-Item -LiteralPath $installer -Destination (Join-Path $payload 'vc_redist.x64.exe')
        $prerequisite=[ordered]@{fileName='vc_redist.x64.exe';sha256=Get-CleanHash $installer;signerThumbprint=$signature.SignerCertificate.Thumbprint;signerSubject=$signature.SignerCertificate.Subject;hostSignatureStatus=[string]$signature.Status;productName=$installerVersion.ProductName;fileDescription=$installerVersion.FileDescription;fileVersion=$installerVersion.FileVersion;originalFilename=$installerVersion.OriginalFilename}
    }
    Assert-Clean (-not $ProvisionProvidedPrerequisite -or $prerequisite) 'Explicit provisioning requires an operator-provided signed redistributable.'
    $cameraReports=@($report.captures | ForEach-Object {Get-Content -LiteralPath $_.evidencePath -Raw | ConvertFrom-Json})
    Assert-Clean ($cameraReports.Count -eq 2 -and @($cameraReports | Where-Object {$_.seed -ne $cameraReports[0].seed -or $_.quality -ne $cameraReports[0].quality}).Count -eq 0) 'Accepted capture generation inputs do not agree.'
    $manifest=[ordered]@{schema='workphone.terrain-clean-machine-candidate.v1';runId=[guid]::NewGuid().ToString('N');preparedAtUtc=[DateTime]::UtcNow.ToString('o');configuration=$report.configuration;sourceFingerprint=$report.sourceAfter;seed=$cameraReports[0].seed;quality=$cameraReports[0].quality
        archiveFileName='WorkphoneTerrainGameplay.zip';archiveSha256=$ExpectedArchiveSha256;acceptedPackageReportSha256=Get-CleanHash $reportPath;guestRunnerSha256=Get-CleanHash (Join-Path $payload 'TerrainCleanMachineGuest.ps1');recipeHash=$report.recipeHash;expectedFiles=@($report.files);prerequisite=$prerequisite;provisionProvidedPrerequisite=[bool]$ProvisionProvidedPrerequisite;diagnosticWatchdogSeconds=600;originalGateTimeoutsChanged=$false
        hardwareAcceptanceVerified=$false;independentVisualAcceptanceVerified=$false;cleanMachineVerified=$false}
    Write-CleanJson $manifest $candidatePath
    $facts=Get-CleanHostFacts; Write-CleanJson $facts (Join-Path $kit 'host-environment.json')
    # Do not trust ZIP path spellings; verify every entry against the accepted inventory.
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive); $observed=@{}
    try {
        foreach($entry in $zip.Entries) {
            [void](Get-ContainedPath (Join-Path $kit 'unexpanded-placeholder') $entry.FullName)
            if($entry.FullName.EndsWith('/')) { continue }
            $name=$entry.FullName.Replace('\','/'); Assert-Clean (-not $observed.ContainsKey($name)) 'Duplicate ZIP entry.'
            $stream=$entry.Open(); $sha=[Security.Cryptography.SHA256]::Create()
            try {$hash=[BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-','')} finally {$sha.Dispose();$stream.Dispose()}
            $observed[$name]=[ordered]@{path=$name;bytes=$entry.Length;sha256=$hash}
        }
    } finally {$zip.Dispose()}
    Assert-Clean ($observed.Count -eq @($manifest.expectedFiles).Count) 'ZIP has missing or extra files.'
    foreach($file in $manifest.expectedFiles) { $name=$file.path.Replace('\','/'); Assert-Clean ($observed.ContainsKey($name) -and $observed[$name].sha256 -eq $file.sha256 -and $observed[$name].bytes -eq $file.bytes) "ZIP entry differs from accepted evidence: $name" }
    Assert-Clean (@($manifest.expectedFiles | Where-Object {$_.path -match '^licenses[/\\]'}).Count -eq 12) 'The established notice inventory changed.'
    Write-SandboxConfiguration ([bool]$ProvisionProvidedPrerequisite)
    $kitReadme = @'
Workphone terrain gameplay clean-Windows validation kit

The payload contains only the pinned optimized package, guest runner, candidate
manifest and (only when supplied explicitly) a Microsoft-signed redistributable.
The .wsb maps payload read-only and evidence writable; it never maps the repository,
build directories, installed SDK, user profile or host runtime folders. Networking,
clipboard, audio/video input, printers and vGPU are disabled. vGPU-off uses WARP;
hardware and independent visual acceptance remain open.

Nothing has been launched, downloaded, enabled, rebooted or installed by Prepare.
On a host with Windows Sandbox ALREADY enabled and no existing Sandbox session:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ValidateTerrainCleanMachine.ps1 -Mode Launch -KitDirectory . -ExpectedArchiveSha256 __ARCHIVE_HASH__
Use -VisibleSandbox only when you want to see/control the guest. Launch returns
without waiting. Native smoke retains every original phase, threshold and 600s
process watchdog. Capture processes are separate. Expect several minutes on WARP.
After evidence\guest-complete.txt appears:
  powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\ValidateTerrainCleanMachine.ps1 -Mode Collect -KitDirectory . -ExpectedArchiveSha256 __ARCHIVE_HASH__
Collect never equates prepared files, a fresh user, or restricted PATH with a clean
OS. It requires the matching Sandbox launch record, distinct guest identity,
baseline software/CRT inventory and passing native/hash evidence.

If baseline-loader-probe records 0xC0000135, no prerequisite is installed by default.
Provide a valid Microsoft-signed x64 VC redistributable to Prepare in a NEW kit,
with -ProvidedRedistributable PATH and -ProvisionProvidedPrerequisite only when
explicit provisioning is intended. The guest records the unprovisioned failure
before installing those exact pinned bytes. It never downloads or installs on the
host. A restart-required installer result leaves acceptance open.

If Windows Sandbox is absent/disabled or nested virtualization is unavailable,
use another already provisioned Sandbox-capable Windows host. Enabling host
features/rebooting is a separate administrator action, not performed by this kit.
The fresh guest and mapped evidence remain for review; no automatic cleanup or
guest shutdown occurs. Close the guest after collecting evidence.

Official configuration reference:
https://learn.microsoft.com/en-us/windows/security/application-security/application-isolation/windows-sandbox/windows-sandbox-configure-using-wsb-file
Optional feature state reference (1 enabled, 2 disabled, 3 absent, 4 unknown):
https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-optionalfeature
'@
    $kitReadme.Replace('__ARCHIVE_HASH__', $ExpectedArchiveSha256) |
        Set-Content -LiteralPath (Join-Path $kit 'README.txt') -Encoding UTF8
    Write-CleanJson ([ordered]@{schema='workphone.terrain-clean-machine-kit.v1';preparedAtUtc=[DateTime]::UtcNow.ToString('o');runId=$manifest.runId;archiveSha256=$manifest.archiveSha256;manifestSha256=Get-CleanHash $candidatePath;guestRunnerSha256=$manifest.guestRunnerSha256;sandboxConfigurationSha256=Get-CleanHash (Join-Path $kit 'TerrainGameplay.wsb');sourceFingerprint=$manifest.sourceFingerprint;preparedOnly=$true;guestLaunched=$false;cleanMachineVerified=$false;releaseCertified=$false;limitations=@('Preparation does not execute a clean guest.','No host feature, prerequisite or reboot changes are performed.','WARP does not certify hardware, live Editor interaction or independent visual quality.','License audit and wider graphics soak remain open.')}) (Join-Path $kit 'kit-evidence.json')
    Write-Output "Prepared offline kit; clean-machine acceptance remains unexecuted: $kit"
    return
}

Assert-Clean (Test-Path -LiteralPath $candidatePath) 'Prepared candidate manifest is missing.'
$manifest=Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json
Assert-CandidateInputs $manifest
if($Mode -eq 'Launch') {
    Assert-Clean (-not (Test-Path -LiteralPath (Join-Path $kit 'host-launch.json'))) 'A launch record already exists; prepare a fresh kit for a fresh guest.'
    Assert-Clean (@(Get-Process WindowsSandbox -ErrorAction SilentlyContinue).Count -eq 0) 'An existing Sandbox session cannot count as a fresh OS; close it first.'
    $facts=Get-CleanHostFacts
    $enabled=@($facts.optionalFeatures | Where-Object {$_.name -eq 'Containers-DisposableClientVM' -and $_.installState -eq 1}).Count -eq 1
    Assert-Clean ($facts.sandboxExecutable -and $enabled) 'Windows Sandbox is not already enabled/available. No feature was changed; use an existing clean guest host or an administrator-provisioned Sandbox.'
    Assert-Clean (@(Get-ChildItem -LiteralPath $evidence -Force).Count -eq 0) 'Evidence mapping is not empty; prepare a fresh kit.'
    Write-SandboxConfiguration ([bool]$manifest.provisionProvidedPrerequisite)
    $launch=[ordered]@{schema='workphone.terrain-clean-machine-launch.v1';runId=$manifest.runId;environmentKind='WindowsSandbox';launchedAtUtc=[DateTime]::UtcNow.ToString('o');machineGuid=$facts.machineGuid;computerName=$facts.computerName;hostFacts=$facts;manifestSha256=Get-CleanHash $candidatePath;configurationSha256=Get-CleanHash (Join-Path $kit 'TerrainGameplay.wsb');sandboxExecutableSha256=Get-CleanHash "$env:WINDIR\System32\WindowsSandbox.exe";networking='Disable';vGPU='Disable';freshSessionChecked=$true}
    Write-CleanJson $launch (Join-Path $payload 'launch-host.json')
    $windowStyle='Hidden'; if($VisibleSandbox){$windowStyle='Normal'}
    $process=Start-Process -FilePath "$env:WINDIR\System32\WindowsSandbox.exe" -ArgumentList (ConvertTo-WindowsArgument (Join-Path $kit 'TerrainGameplay.wsb')) -WindowStyle $windowStyle -PassThru
    $launch.processId=$process.Id
    Write-CleanJson $launch (Join-Path $kit 'host-launch.json')
    Write-Output "Sandbox launch requested (PID $($process.Id)); collect only after evidence/guest-complete.txt."
    return
}

$launch=Get-Content -LiteralPath (Join-Path $kit 'host-launch.json') -Raw | ConvertFrom-Json
Assert-Clean ($launch.runId -eq $manifest.runId -and $launch.freshSessionChecked -and $launch.networking -eq 'Disable' -and $launch.vGPU -eq 'Disable') 'Fresh Sandbox launch identity/configuration is missing.'
Assert-Clean ((Get-CleanHash $candidatePath) -eq $launch.manifestSha256 -and (Get-CleanHash (Join-Path $kit 'TerrainGameplay.wsb')) -eq $launch.configurationSha256) 'Candidate/launch configuration changed.'
Assert-Clean ((Get-Content -LiteralPath (Join-Path $evidence 'guest-complete.txt') -Raw).Trim() -eq $manifest.runId) 'Matching completed guest marker is absent.'
$resultPath=Join-Path $evidence 'guest-result.json'; $result=Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
Assert-Clean ($result.schema -eq 'workphone.terrain-clean-machine-guest.v1' -and $result.runId -eq $manifest.runId -and $result.archiveSha256 -eq $manifest.archiveSha256 -and $result.candidateManifestSha256 -eq $launch.manifestSha256 -and $result.guestRunnerSha256 -eq $manifest.guestRunnerSha256) 'Guest evidence identity mismatch.'
Assert-Clean ($result.environmentKind -eq 'WindowsSandbox' -and $result.guestIdentityVerified -and $result.environment.machineGuid -ne $launch.machineGuid -and $result.environment.computerName -ne $launch.computerName -and @($result.environment.developmentTools).Count -eq 0) 'Clean guest origin/baseline was not established.'
Assert-Clean ($result.guestValidationPassed -and -not $result.failure -and $result.installedInventoryUnchanged -and $result.diagnosticWatchdogSeconds -eq 600 -and $result.originalGateTimeoutsChanged -eq $false) 'Guest validation failed or changed its gate scope.'
$baselinePath=Join-Path $evidence 'baseline-environment.json'
Assert-Clean ((Get-CleanHash $baselinePath) -eq $result.baselineEnvironmentSha256) 'Original guest baseline environment changed.'
$originalBaseline=Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json
Assert-Clean (($originalBaseline | ConvertTo-Json -Depth 12 -Compress) -ceq ($result.environment | ConvertTo-Json -Depth 12 -Compress)) 'Final guest report differs from its pre-launch environment inventory.'
$required=@('gameplay-smoke','capture-track','capture-car','missing-manifest','corrupt-payload')
$expectedRunNames=@('baseline-loader-probe')+$required
if($result.prerequisites.providedInstallerRun){$expectedRunNames+=@('provided-prerequisite-install','post-provision-loader-probe')}
Assert-Clean ((@($result.runs.name | Sort-Object) -join "`n") -ceq (@($expectedRunNames | Sort-Object) -join "`n")) 'Guest run set has missing, duplicate or unexpected processes.'
foreach($run in $result.runs){
    Assert-Clean (-not $run.timedOut -and -not $run.error -and $run.watchdogSeconds -eq 600) "Guest process did not finish normally: $($run.name)"
    Assert-Clean ((Get-CleanHash (Get-ContainedPath $evidence $run.logFile)) -eq $run.logSha256) "Guest process log changed: $($run.name)"
}
$baselineProbe=@($result.runs | Where-Object {$_.name -eq 'baseline-loader-probe'})[0]
if($result.prerequisites.baselineProbePassed){
    Assert-Clean ($baselineProbe.exitCode -eq 0 -and -not $result.prerequisites.providedInstallerRun -and -not $result.prerequisites.installedAfterBaseline -and -not $result.prerequisites.missingDllObserved) 'Unprovisioned baseline transition is inconsistent.'
} else {
    Assert-Clean ($baselineProbe.exitCodeHex -eq 'C0000135' -and $result.prerequisites.baselineLoaderCode -eq 'C0000135' -and $result.prerequisites.missingDllObserved -and $manifest.provisionProvidedPrerequisite -and $manifest.prerequisite) 'A prerequisite was installed without the recorded missing-DLL baseline/explicit opt-in.'
    $installerRun=@($result.runs | Where-Object {$_.name -eq 'provided-prerequisite-install'})[0]
    $postProbe=@($result.runs | Where-Object {$_.name -eq 'post-provision-loader-probe'})[0]
    Assert-Clean ($installerRun.exitCode -eq 0 -and $postProbe.exitCode -eq 0 -and $result.prerequisites.installedAfterBaseline -and $result.prerequisites.postProvisionProbePassed -and -not $result.prerequisites.rebootRequired) 'Approved prerequisite transition did not complete.'
}
foreach($name in $required) {
    $run=@($result.runs | Where-Object {$_.name -eq $name}); Assert-Clean ($run.Count -eq 1) "Missing/duplicate guest run: $name"
    $run=$run[0]; Assert-Clean (-not $run.timedOut -and -not $run.error -and $run.watchdogSeconds -eq 600) "Guest run failed/timed out: $name"
    $logPath=Get-ContainedPath $evidence $run.logFile; Assert-Clean ((Get-CleanHash $logPath) -eq $run.logSha256) "Guest log changed: $name"
    $log=Get-Content -LiteralPath $logPath -Raw
    if($name -in @('gameplay-smoke','capture-track','capture-car')) {
        Assert-Clean ($run.exitCode -eq 0 -and $log -match 'Terrain gameplay surfaces: PASS' -and $log -match ('Terrain gameplay recipe: PASS hash='+[regex]::Escape($manifest.recipeHash)+'(?:\s|$)') -and $log -match 'Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false') "Guest gameplay gate missing: $name"
        if($name -eq 'gameplay-smoke'){foreach($phase in 0..4){Assert-Clean ($log -match "Vehicle smoke phase ${phase}: PASS") "Original phase $phase missing in clean guest."}}
    } elseif($name -eq 'missing-manifest') {Assert-Clean ($run.exitCode -ne 0 -and $log -match 'Failed to open compiled resource:[^\r\n]*runtime\.wprm') 'Missing manifest cause differs.'}
    else {Assert-Clean ($run.exitCode -ne 0 -and $log -match 'Compiled resource payload hash mismatch') 'Corrupt payload cause differs.'}
}
Assert-Clean ((@($result.captures.view | Sort-Object) -join "`n") -ceq "car`ntrack") 'Both unique native guest captures are required.'
$decimalRecipe=[UInt64]::Parse($manifest.recipeHash,[Globalization.NumberStyles]::AllowHexSpecifier,[Globalization.CultureInfo]::InvariantCulture).ToString([Globalization.CultureInfo]::InvariantCulture)
foreach($capture in $result.captures) {
    Assert-Clean ($capture.imageFile -eq ('terrain-gameplay-'+$capture.view+'.bmp') -and $capture.reportFile -eq ('terrain-gameplay-'+$capture.view+'.json')) 'Guest capture filenames differ from requested views.'
    Assert-Clean ((Get-CleanHash (Get-ContainedPath $evidence $capture.imageFile)) -eq $capture.imageSha256 -and (Get-CleanHash (Get-ContainedPath $evidence $capture.reportFile)) -eq $capture.reportSha256) 'Guest capture files changed.'
    $captureReport=Get-Content -LiteralPath (Get-ContainedPath $evidence $capture.reportFile) -Raw | ConvertFrom-Json
    Assert-Clean ($captureReport.validationPassed -and $captureReport.terrainGameplaySourceFree -and $captureReport.terrainGameplaySurfacesValidated -and $captureReport.device.adapter -and $captureReport.device.captureTarget.width -eq 640 -and $captureReport.device.captureTarget.height -eq 360) 'Guest native capture did not validate.'
    Assert-Clean ($captureReport.camera -eq $capture.view -and $captureReport.seed -eq $manifest.seed -and $captureReport.quality -eq $manifest.quality -and [string]$captureReport.vehicleRecipeHash -eq $decimalRecipe) 'Guest capture camera or generation recipe differs from candidate.'
    Assert-Clean ($captureReport.device.vendorId -eq 5140 -and $captureReport.device.deviceId -eq 140) 'Actual guest adapter is not the expected Microsoft WARP device.'
}
Assert-Clean ($result.prerequisites.baselineProbePassed -or ($result.prerequisites.missingDllObserved -and $result.prerequisites.installedAfterBaseline -and $result.prerequisites.postProvisionProbePassed -and -not $result.prerequisites.rebootRequired)) 'Prerequisite baseline/provisioning transition is unresolved.'
$acceptance=[ordered]@{schema='workphone.terrain-clean-machine-acceptance.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o');runId=$manifest.runId;sourceFingerprint=$manifest.sourceFingerprint;archiveSha256=$manifest.archiveSha256;candidateManifestSha256=$launch.manifestSha256;guestResultSha256=Get-CleanHash $resultPath;cleanMachineValidationPassed=$true;environmentKind='fresh Windows Sandbox';prerequisiteBaseline=$result.prerequisites;nativeCaptures=$result.captures;hardwareAcceptanceVerified=$false;interactiveEditorVerified=$false;independentVisualAcceptanceVerified=$false;licenseAuditComplete=$false;releaseCertified=$false;limitations=@('WARP guest execution does not certify physical GPU performance/recovery.','Source-free gameplay and installed inventory were tested; filesystem write denial and interactive Editor behavior were not.','Independent visual approval, full license audit and wider graphics soak remain open.')}
Write-CleanJson $acceptance (Join-Path $kit 'clean-machine-acceptance.json')
Write-Output "Clean guest package evidence reconciled: $kit/clean-machine-acceptance.json"
