#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$PayloadRoot = 'C:\TerrainGT0Payload',
    [string]$EvidenceRoot = 'C:\TerrainGT0Evidence',
    [switch]$ProvisionProvidedPrerequisite,
    [switch]$LibraryOnly
)
# Stock Windows PowerShell 5.1 is intentional: no developer runtime is required.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Get-CleanHash([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
function Assert-Clean([bool]$Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Get-ContainedPath([string]$Root, [string]$Relative) {
    Assert-Clean (-not [IO.Path]::IsPathRooted($Relative) -and $Relative -notmatch ':') 'Absolute/stream archive path rejected.'
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    $path = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    Assert-Clean ($path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) 'Archive path escapes the installation.'
    return $path
}
function Get-CleanInventory([string]$Root) {
    $absolute = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $all = @(Get-ChildItem -LiteralPath $absolute -Force -Recurse)
    Assert-Clean (@($all | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }).Count -eq 0) 'Reparse point found in package.'
    @($all | Where-Object { -not $_.PSIsContainer } | Sort-Object FullName | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($absolute.Length + 1).Replace('\', '/'); bytes = $_.Length; sha256 = Get-CleanHash $_.FullName }
    })
}
function Assert-CleanInventory($Expected, [string]$Root) {
    $actual = @(Get-CleanInventory $Root)
    $expectedSorted = @($Expected | Sort-Object path | ForEach-Object { [ordered]@{path=$_.path.Replace('\','/');bytes=$_.bytes;sha256=$_.sha256} })
    $actualSorted = @($actual | Sort-Object path)
    Assert-Clean (($expectedSorted | ConvertTo-Json -Depth 4 -Compress) -ceq ($actualSorted | ConvertTo-Json -Depth 4 -Compress)) 'Installed package inventory/bytes changed.'
}
function ConvertTo-WindowsArgument([string]$Value) {
    if ($Value.Length -gt 0 -and $Value -notmatch '[\s"]') { return $Value }
    # CommandLineToArgvW/MSVC quoting, including quotes and trailing backslashes.
    $escaped = [regex]::Replace($Value, '(\\*)"', '$1$1\"')
    $escaped = [regex]::Replace($escaped, '(\\+)$', '$1$1')
    return '"' + $escaped + '"'
}
function Get-GuestFacts {
    $crt = @('vcruntime140.dll','vcruntime140_1.dll','msvcp140.dll','msvcp140_atomic_wait.dll','concrt140.dll','ucrtbase.dll')
    $apps = @('HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*', 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*')
    [ordered]@{
        recordedAtUtc = [DateTime]::UtcNow.ToString('o'); computerName = $env:COMPUTERNAME; userName = $env:USERNAME
        machineGuid = (Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Cryptography').MachineGuid
        os = Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber,OSArchitecture,LastBootUpTime
        computer = Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer,Model,HypervisorPresent,TotalPhysicalMemory
        adapters = @(Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion,AdapterRAM)
        installedApplications = @(Get-ItemProperty -Path $apps -ErrorAction SilentlyContinue | Where-Object { $_.PSObject.Properties['DisplayName'] } | Select-Object DisplayName,DisplayVersion,Publisher | Sort-Object DisplayName)
        crt = @($crt | ForEach-Object { $file=Join-Path "$env:WINDIR\System32" $_; if(Test-Path -LiteralPath $file) { $item=Get-Item -LiteralPath $file; [ordered]@{name=$_;present=$true;version=$item.VersionInfo.FileVersion;sha256=Get-CleanHash $file} } else {[ordered]@{name=$_;present=$false}} })
        developmentTools = @(Get-Item 'C:\Program Files\Microsoft Visual Studio','C:\Program Files (x86)\Microsoft Visual Studio','C:\Program Files (x86)\Windows Kits','C:\Program Files\CMake' -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName)
        path = $env:PATH; powerShellVersion = $PSVersionTable.PSVersion.ToString()
    }
}
function Invoke-CleanProcess([string]$Program, [string[]]$Arguments, [string]$WorkingDirectory, [string]$Name, [int]$Seconds = 600) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $Program; $info.WorkingDirectory = $WorkingDirectory
    $info.UseShellExecute = $false; $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true; $info.RedirectStandardError = $true
    $info.Arguments = (@($Arguments | ForEach-Object { ConvertTo-WindowsArgument $_ }) -join ' ')
    $info.EnvironmentVariables['PATH'] = "$(Split-Path $Program -Parent);$env:WINDIR\System32;$env:WINDIR"
    $watch = [Diagnostics.Stopwatch]::StartNew(); $process = $null; $stdout=$null; $stderr=$null; $output = ''; $processError = $null; $timedOut = $false; $code = $null
    try {
        $process = [Diagnostics.Process]::Start($info)
        $stdout = $process.StandardOutput.ReadToEndAsync(); $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($Seconds * 1000)) { $timedOut = $true; $process.Kill(); $process.WaitForExit(); throw "$Name exceeded the unchanged $Seconds-second process watchdog." }
        $code = $process.ExitCode
    } catch { $processError = $_.Exception.Message } finally {
        if ($process) {
            try { if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }; $code = $process.ExitCode } catch { $processError=$_.Exception.Message }
            foreach ($reader in @($stdout, $stderr)) { if($reader) { try { if ($reader.Wait(5000)) { $output += $reader.GetAwaiter().GetResult() } else { $processError = 'Native output pipe did not close.' } } catch {$processError=$_.Exception.Message} } }
            $process.Dispose()
        }
        $nativeLog = Join-Path $WorkingDirectory 'Application.log'
        if (Test-Path -LiteralPath $nativeLog) { try { $nativeTarget=Join-Path $EvidenceRoot ($Name+'.application.log'); Move-Item -LiteralPath $nativeLog -Destination $nativeTarget; $output += "`nNative Application.log:`n" + (Get-Content -LiteralPath $nativeTarget -Raw) } catch {$processError=$_.Exception.Message} }
        $watch.Stop(); $logPath=Join-Path $EvidenceRoot ($Name+'.log'); $output | Set-Content -LiteralPath $logPath -Encoding UTF8
    }
    $exitHex = $null
    if ($null -ne $code) { $exitHex = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$code),0).ToString('X8') }
    $entry=[ordered]@{name=$Name;arguments=$Arguments;exitCode=$code;exitCodeHex=$exitHex;timedOut=$timedOut;watchdogSeconds=$Seconds;seconds=$watch.Elapsed.TotalSeconds;error=$processError;logFile=($Name+'.log');logSha256=Get-CleanHash $logPath}
    $script:runs.Add($entry)
    return [pscustomobject]@{entry=$entry;output=$output}
}
function Assert-GameplayResult($Result, [bool]$Smoke) {
    Assert-Clean ($Result.entry.exitCode -eq 0 -and -not $Result.entry.timedOut -and -not $Result.entry.error) 'Native gameplay process failed.'
    Assert-Clean ($Result.output -match 'Terrain gameplay surfaces: PASS') 'Terrain publication/query/collision gate did not pass.'
    Assert-Clean ($Result.output -match ('Terrain gameplay recipe: PASS hash=' + [regex]::Escape($script:manifest.recipeHash) + '(?:\s|$)')) 'Pinned vehicle recipe did not match.'
    Assert-Clean ($Result.output -match 'Terrain gameplay package: PASS sourceFree=true compilerRegistry=false authoringDatabase=false') 'Consumer did not verify source-free services.'
    if ($Smoke) { foreach ($phase in 0..4) { Assert-Clean ($Result.output -match "Vehicle smoke phase ${phase}: PASS") "Original vehicle phase $phase did not pass." } }
}

if ($LibraryOnly) { return }
Assert-Clean ([Environment]::Is64BitProcess) 'Run stock 64-bit Windows PowerShell.'
Assert-Clean (Test-Path -LiteralPath $PayloadRoot -PathType Container) 'Read-only payload mapping is missing.'
Assert-Clean (Test-Path -LiteralPath $EvidenceRoot -PathType Container) 'Evidence mapping is missing.'
Assert-Clean (-not (Test-Path -LiteralPath (Join-Path $EvidenceRoot 'guest-result.json'))) 'Existing guest evidence is preserved; prepare a fresh kit.'
$script:manifest = Get-Content -LiteralPath (Join-Path $PayloadRoot 'candidate-manifest.json') -Raw | ConvertFrom-Json
$launch = Get-Content -LiteralPath (Join-Path $PayloadRoot 'launch-host.json') -Raw | ConvertFrom-Json
$baseline = Get-GuestFacts
$script:runs = New-Object 'Collections.Generic.List[object]'
$captures = New-Object 'Collections.Generic.List[object]'
$prerequisite = [ordered]@{baselineProbePassed=$false;baselineLoaderCode=$null;missingDllObserved=$false;providedInstallerRun=$false;installedAfterBaseline=$false;rebootRequired=$false;postProvisionProbePassed=$false}
$passed=$false; $failure=$null; $install=$null; $inventoryVerified=$false; $guestIdentityVerified=$false
$work = Join-Path $env:LOCALAPPDATA ('WorkphoneTerrainGT0-'+$manifest.runId)
try {
    Assert-Clean ($manifest.schema -eq 'workphone.terrain-clean-machine-candidate.v1' -and $manifest.runId -match '^[a-f0-9]{32}$' -and $manifest.runId -eq $launch.runId) 'Candidate/launch identity mismatch.'
    Assert-Clean ($baseline.machineGuid -ne $launch.machineGuid -and $baseline.computerName -ne $launch.computerName) 'Guest runner must not run on its launch host.'
    Assert-Clean ($env:USERNAME -eq 'WDAGUtilityAccount' -and $launch.environmentKind -eq 'WindowsSandbox') 'This automatic acceptance path requires a fresh Windows Sandbox guest.'
    Assert-Clean (@($baseline.developmentTools).Count -eq 0) 'Developer tool installation found in the supposed clean guest.'
    $guestIdentityVerified=$true
    $baseline | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'baseline-environment.json') -Encoding UTF8
    Assert-Clean ((Get-CleanHash $PSCommandPath) -eq $manifest.guestRunnerSha256) 'Guest runner bytes differ from prepared candidate.'
    $archive=Join-Path $PayloadRoot $manifest.archiveFileName
    Assert-Clean ((Get-CleanHash $archive) -eq $manifest.archiveSha256) 'Archive SHA256 mismatch.'
    Assert-Clean (-not (Test-Path -LiteralPath $work)) 'Fresh guest work directory already exists.'
    New-Item -ItemType Directory -Path $work | Out-Null
    $install=Join-Path $work 'install'
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip=[IO.Compression.ZipFile]::OpenRead($archive)
    try { $names=@{}; foreach($entry in $zip.Entries) { $safe=Get-ContainedPath $install $entry.FullName; Assert-Clean (-not $names.ContainsKey($entry.FullName.ToLowerInvariant())) 'Duplicate archive entry.'; $names[$entry.FullName.ToLowerInvariant()]=$true } } finally {$zip.Dispose()}
    Expand-Archive -LiteralPath $archive -DestinationPath $install
    Assert-CleanInventory $manifest.expectedFiles $install
    $bin=Join-Path $install 'bin'; $exe=Join-Path $bin 'SampleVehicleAdvanced.exe'; $assets=Join-Path $install 'assets'; $plugins=Join-Path $bin 'wp_plugins_terrain_gameplay.cfg'
    # Suppress loader error dialogs so missing CRT can be recorded unattended.
    Add-Type -TypeDefinition 'using System.Runtime.InteropServices; public static class TerrainGuestErrorMode { [DllImport("kernel32.dll")] public static extern uint SetErrorMode(uint mode); }'
    [void][TerrainGuestErrorMode]::SetErrorMode(0x8003)
    $probe=Invoke-CleanProcess $exe @('--help') $bin 'baseline-loader-probe'
    $prerequisite.baselineProbePassed=($probe.entry.exitCode -eq 0 -and -not $probe.entry.error)
    $prerequisite.baselineLoaderCode=$probe.entry.exitCodeHex
    $prerequisite.missingDllObserved=($probe.entry.exitCodeHex -eq 'C0000135')
    if (-not $prerequisite.baselineProbePassed) {
        Assert-Clean $prerequisite.missingDllObserved 'Baseline loader failed for a reason other than missing DLL (0xC0000135).'
        Assert-Clean ($ProvisionProvidedPrerequisite -and $manifest.prerequisite) 'Missing DLL observed before provisioning. Supply an approved x64 redistributable and explicitly opt in; nothing was installed.'
        $installer=Join-Path $PayloadRoot $manifest.prerequisite.fileName
        Assert-Clean ((Get-CleanHash $installer) -eq $manifest.prerequisite.sha256) 'Provided prerequisite installer hash mismatch.'
        $signature=Get-AuthenticodeSignature -LiteralPath $installer
        Assert-Clean ($signature.SignerCertificate -and $signature.SignerCertificate.Thumbprint -eq $manifest.prerequisite.signerThumbprint) 'Provided Microsoft installer signer mismatch.'
        $provision=Invoke-CleanProcess $installer @('/install','/quiet','/norestart') $work 'provided-prerequisite-install'
        $prerequisite.providedInstallerRun=$true
        $prerequisite.rebootRequired=($provision.entry.exitCode -eq 3010)
        Assert-Clean ($provision.entry.exitCode -eq 0 -and -not $provision.entry.error) 'Prerequisite installation failed or requires restart; acceptance remains open.'
        $prerequisite.installedAfterBaseline=$true
        $probe=Invoke-CleanProcess $exe @('--help') $bin 'post-provision-loader-probe'
        $prerequisite.postProvisionProbePassed=($probe.entry.exitCode -eq 0 -and -not $probe.entry.error)
        Assert-Clean $prerequisite.postProvisionProbePassed 'Loader still fails after approved prerequisite provisioning.'
    }
    $common=@('--terrain-gameplay-package',$assets,'--plugins',$plugins,'--no-audio','--no-effects','--no-hud','--resolution','640x360')
    $smoke=Invoke-CleanProcess $exe ($common+'--smoke-test') $bin 'gameplay-smoke'
    Assert-GameplayResult $smoke $true
    foreach($view in @('track','car')) {
        $capture=Join-Path $EvidenceRoot ('terrain-gameplay-'+$view+'.bmp'); $report=Join-Path $EvidenceRoot ('terrain-gameplay-'+$view+'.json')
        $result=Invoke-CleanProcess $exe ($common+@('--capture',$capture,'--view',$view,'--evidence',$report)) $bin ('capture-'+$view)
        Assert-GameplayResult $result $false
        $captureEvidence=Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
        Assert-Clean ($captureEvidence.validationPassed -and $captureEvidence.terrainGameplaySurfacesValidated -and $captureEvidence.terrainGameplaySourceFree -and $captureEvidence.device.adapter) 'Native capture evidence did not validate source-free terrain gameplay.'
        $decimalRecipe=[UInt64]::Parse($manifest.recipeHash,[Globalization.NumberStyles]::AllowHexSpecifier,[Globalization.CultureInfo]::InvariantCulture).ToString([Globalization.CultureInfo]::InvariantCulture)
        Assert-Clean ($captureEvidence.camera -eq $view -and $captureEvidence.seed -eq $manifest.seed -and $captureEvidence.quality -eq $manifest.quality -and [string]$captureEvidence.vehicleRecipeHash -eq $decimalRecipe) 'Capture camera or pinned generation inputs differ.'
        Assert-Clean ($captureEvidence.device.vendorId -eq 5140 -and $captureEvidence.device.deviceId -eq 140) 'vGPU-off Sandbox did not report the expected Microsoft Basic Render Driver/WARP device.'
        Assert-Clean ($captureEvidence.device.captureTarget.width -eq 640 -and $captureEvidence.device.captureTarget.height -eq 360) 'Capture dimensions differ from the fixture.'
        $captures.Add([ordered]@{view=$view;imageFile=('terrain-gameplay-'+$view+'.bmp');imageSha256=Get-CleanHash $capture;reportFile=('terrain-gameplay-'+$view+'.json');reportSha256=Get-CleanHash $report;device=$captureEvidence.device})
    }
    $missing=Join-Path $work 'missing-manifest'; Copy-Item -LiteralPath $assets -Destination $missing -Recurse; Remove-Item -LiteralPath (Join-Path $missing 'runtime.wprm')
    $negative=Invoke-CleanProcess $exe (@('--terrain-gameplay-package',$missing,'--plugins',$plugins,'--smoke-test','--no-audio','--no-effects','--no-hud')) $bin 'missing-manifest'
    Assert-Clean ($negative.entry.exitCode -ne 0 -and -not $negative.entry.timedOut -and $negative.output -match 'Failed to open compiled resource:[^\r\n]*runtime\.wprm') 'Missing manifest failed for an unexpected reason.'
    $corrupt=Join-Path $work 'corrupt-payload'; Copy-Item -LiteralPath $assets -Destination $corrupt -Recurse
    $victim=Get-ChildItem -LiteralPath $corrupt -Recurse -File | Where-Object Extension -eq '.wprs' | Sort-Object FullName | Select-Object -First 1
    $bytes=[IO.File]::ReadAllBytes($victim.FullName); $bytes[$bytes.Length-1]=$bytes[$bytes.Length-1] -bxor 1; [IO.File]::WriteAllBytes($victim.FullName,$bytes)
    $negative=Invoke-CleanProcess $exe (@('--terrain-gameplay-package',$corrupt,'--plugins',$plugins,'--smoke-test','--no-audio','--no-effects','--no-hud')) $bin 'corrupt-payload'
    Assert-Clean ($negative.entry.exitCode -ne 0 -and -not $negative.entry.timedOut -and $negative.output -match 'Compiled resource payload hash mismatch') 'Corrupt payload failed for an unexpected reason.'
    Assert-CleanInventory $manifest.expectedFiles $install
    Assert-Clean ((Get-CleanHash $archive) -eq $manifest.archiveSha256) 'Read-only input archive changed.'
    $inventoryVerified=$true; $passed=$true
} catch { $failure=$_.Exception.Message; Write-Error -Message $failure -ErrorAction Continue } finally {
    $baselineFile=Join-Path $EvidenceRoot 'baseline-environment.json'
    $baselineHash=$null; if(Test-Path -LiteralPath $baselineFile){$baselineHash=Get-CleanHash $baselineFile}
    [ordered]@{schema='workphone.terrain-clean-machine-guest.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o');runId=$manifest.runId;archiveSha256=$manifest.archiveSha256;candidateManifestSha256=Get-CleanHash (Join-Path $PayloadRoot 'candidate-manifest.json');guestRunnerSha256=Get-CleanHash $PSCommandPath;environmentKind='WindowsSandbox';guestIdentityVerified=$guestIdentityVerified;environment=$baseline;baselineEnvironmentSha256=$baselineHash;prerequisites=$prerequisite;postRunEnvironment=Get-GuestFacts;runs=@($script:runs.ToArray());captures=@($captures.ToArray());installedInventoryUnchanged=$inventoryVerified;guestValidationPassed=$passed;failure=$failure;cleanMachineAcceptanceRequiresHostReconciliation=$true;hardwareAcceptanceVerified=$false;interactiveEditorVerified=$false;independentVisualAcceptanceVerified=$false;licenseAuditComplete=$false;releaseCertified=$false;diagnosticWatchdogSeconds=600;originalGateTimeoutsChanged=$false} | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'guest-result.json') -Encoding UTF8
    [IO.File]::WriteAllText((Join-Path $EvidenceRoot 'guest-complete.txt'),$manifest.runId)
}
if (-not $passed) { exit 1 }
