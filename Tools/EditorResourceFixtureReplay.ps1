#requires -Version 5.1
[CmdletBinding()]
param([string]$PackageRoot,[string]$FixturesRoot,[string]$CandidateManifestPath,[string]$ReportPath,
    [switch]$LibraryOnly)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest

function Assert-EditorFixtureUuid($Value) {
    Assert-Clean ($Value -is [string] -and $Value -cmatch '^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$' -and
        $Value -ne '00000000-0000-0000-0000-000000000000') 'Fixture requires a canonical non-nil UUID.'
}
function Assert-EditorResourceFixture($Entry,[string]$Root) {
    Assert-Clean ($Entry.name -in @('mixed-scene','extended-typed')) 'Unknown retained resource fixture.'
    Assert-Clean ((Test-Path -LiteralPath $Root -PathType Container) -and
        -not ((Get-Item -LiteralPath $Root).Attributes -band [IO.FileAttributes]::ReparsePoint)) 'Fixture root is missing or linked.'
    $pins=@($Entry.files);Assert-Clean ($pins.Count -gt 1 -and $pins.Count -le 4096) 'Fixture inventory exceeds bounds.'
    $seen=@{};$total=0L
    $normalized=@(foreach($pin in $pins){
        $path=[string]$pin.path
        Assert-Clean ($path.Length -le 1024 -and $path -notmatch '\\|:' -and $path.Split('/') -notcontains '..' -and
            $path -cmatch '^(fixture\.json|(?:package|overlay)/(?:runtime\.wprm|.+\.wprs))$' -and
            -not $seen.ContainsKey($path)) 'Fixture contains an unexpected, duplicate or authoring path.'
        [void](Get-ContainedPath $Root $path);$seen[$path]=$true
        Assert-Clean ([string]$pin.bytes -match '^[0-9]{1,12}$' -and [string]$pin.sha256 -match '^[a-fA-F0-9]{64}$') 'Invalid fixture size/hash pin.'
        $total += [long]$pin.bytes;Assert-Clean ($total -le 536870912) 'Fixture exceeds 512 MiB.'
        [ordered]@{path=$path;bytes=[long]$pin.bytes;sha256=([string]$pin.sha256).ToUpperInvariant()}
    })
    Assert-Clean ($seen.ContainsKey('fixture.json') -and $seen.ContainsKey('package/runtime.wprm')) 'Fixture lacks its descriptor/package manifest.'
    Assert-CleanInventory $normalized $Root
    $descriptorPath=Join-Path $Root 'fixture.json'
    Assert-Clean ((Get-Item -LiteralPath $descriptorPath).Length -le 65536) 'Fixture descriptor exceeds 64 KiB.'
    $descriptor=Get-Content -LiteralPath $descriptorPath -Raw -Encoding UTF8 | ConvertFrom-Json
    Assert-Clean ($descriptor.target -ceq 'pc') 'Fixture target must be pc.'
    if($Entry.name -eq 'extended-typed'){
        Assert-Clean ($descriptor.schema -ceq 'workphone.extended-typed-fixture.v1' -and
            @($descriptor.uuids).Count -ge 1 -and @($descriptor.uuids).Count -le 256 -and
            -not @($normalized | Where-Object path -like 'overlay/*').Count) 'Invalid extended typed fixture descriptor.'
        $ids=@{};foreach($id in $descriptor.uuids){Assert-EditorFixtureUuid $id;Assert-Clean (-not $ids.ContainsKey($id)) 'Duplicate fixture UUID.';$ids[$id]=$true}
    }else{
        Assert-Clean ($descriptor.schema -ceq 'workphone.mixed-scene-fixture.v1' -and $seen.ContainsKey('overlay/runtime.wprm') -and
            @($descriptor.scenes.PSObject.Properties).Count -eq 3) 'Invalid mixed scene fixture descriptor.'
        $ids=@{};foreach($kind in @('dx11','software','audio')){
            $scene=$descriptor.scenes.$kind;Assert-EditorFixtureUuid $scene.uuid
            Assert-Clean (-not $ids.ContainsKey($scene.uuid)) 'Mixed scenes require distinct UUIDs.';$ids[$scene.uuid]=$true
            Assert-Clean (@($scene.expected.PSObject.Properties).Count -eq 6) 'Mixed scene expected counters differ.'
            foreach($field in @('actors','components','scripts','meshes','residentMeshes','streamingMeshes')){
                Assert-Clean ([string]$scene.expected.$field -match '^[0-9]{1,4}$' -and [int]$scene.expected.$field -le 4096) 'Mixed scene counter exceeds its bound.'
            }
        }
    }
    return $descriptor
}
function Get-EditorResourceFixtureRunNames($Fixtures) {
    foreach($entry in @($Fixtures)){
        if($entry.name -eq 'extended-typed'){'extended-typed-verify'}
        elseif($entry.name -eq 'mixed-scene'){
            'mixed-scene-dx11';'mixed-scene-software';'mixed-scene-overlay-reject';'mixed-scene-overlay-identical'
            'mixed-scene-overlay-prefer-earlier';'mixed-scene-software-material';'mixed-scene-audio';'mixed-scene-missing-uuid'
        }else{throw 'Unknown resource fixture in candidate manifest.'}
    }
}
function Get-EditorFixtureBootstrap($Run) {
    Assert-Clean (-not $Run.entry.timedOut -and -not $Run.entry.error) 'Resource fixture process failed or exceeded its unchanged watchdog.'
    $lines=@($Run.output -split "`r?`n" | Where-Object {$_ -clike 'RuntimeSceneBootstrap=*'})
    Assert-Clean ($lines.Count -eq 1) 'Expected one product runtime scene report.'
    return ($lines[0].Substring('RuntimeSceneBootstrap='.Length) | ConvertFrom-Json)
}
function Assert-EditorFixtureLoaded($Run,$Expected,[int]$MountCount,[string]$RootMount,[int]$Shadowed) {
    $report=Get-EditorFixtureBootstrap $Run
    Assert-Clean ($Run.entry.exitCode -eq 0 -and $report.status -ceq 'loaded' -and
        $Run.output -match 'Runtime scene package loaded' -and
        $Run.output -notmatch 'stack traceback:|No matching overload found') 'Source-free product scene did not install cleanly.'
    foreach($field in @('actors','components','scripts','meshes','residentMeshes','streamingMeshes')){
        Assert-Clean ($report.$field -eq $Expected.$field) "Source-free scene counter differs: $field"
    }
    Assert-Clean ($report.mountCount -eq $MountCount -and $report.rootMount -ceq $RootMount -and
        $report.rootShadowedMounts -eq $Shadowed) 'Source-free scene mount provenance differs.'
}
function Invoke-EditorFixtureProcess([string]$Program,[string[]]$Arguments,[string]$WorkingDirectory,[string]$Name,[int]$Seconds=90) {
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName=$Program;$info.WorkingDirectory=$WorkingDirectory;$info.UseShellExecute=$false;$info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true;$info.RedirectStandardError=$true
    $info.Arguments=(@($Arguments | ForEach-Object {ConvertTo-WindowsArgument $_}) -join ' ')
    $info.EnvironmentVariables['PATH']="$(Split-Path $Program -Parent);$env:WINDIR\System32;$env:WINDIR"
    $watch=[Diagnostics.Stopwatch]::StartNew();$process=$null;$stdout=$null;$stderr=$null;$output='';$processError=$null;$timedOut=$false;$code=$null
    try{
        $process=[Diagnostics.Process]::Start($info);$stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
        if(-not $process.WaitForExit($Seconds*1000)){$timedOut=$true;throw "$Name exceeded the unchanged $Seconds-second process watchdog."}
        $code=$process.ExitCode
    }catch{$processError=$_.Exception.Message}finally{
        if($process){
            try{if(-not $process.HasExited){$process.Kill();if(-not $process.WaitForExit(5000)){throw 'Fixture process did not terminate within five seconds of cancellation.'}};$code=$process.ExitCode}catch{$processError=$_.Exception.Message}
            foreach($reader in @($stdout,$stderr)){if($reader){try{if($reader.Wait(5000)){$output+=$reader.GetAwaiter().GetResult()}else{$processError='Fixture output pipe did not close within five seconds.'}}catch{$processError=$_.Exception.Message}}}
            $process.Dispose()
        }
        $nativeLog=Join-Path $WorkingDirectory 'Application.log'
        if(Test-Path -LiteralPath $nativeLog){try{$target=Join-Path $EvidenceRoot ($Name+'.application.log');Move-Item -LiteralPath $nativeLog -Destination $target;$output+="`nNative Application.log:`n"+(Get-Content -LiteralPath $target -Raw)}catch{$processError=$_.Exception.Message}}
        $watch.Stop();$logPath=Join-Path $EvidenceRoot ($Name+'.log');$output | Set-Content -LiteralPath $logPath -Encoding UTF8
    }
    $exitHex=$null;if($null -ne $code){$exitHex=[BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$code),0).ToString('X8')}
    $entry=[ordered]@{name=$Name;arguments=$Arguments;exitCode=$code;exitCodeHex=$exitHex;timedOut=$timedOut;watchdogSeconds=$Seconds;seconds=$watch.Elapsed.TotalSeconds;error=$processError;logFile=($Name+'.log');logSha256=Get-CleanHash $logPath}
    $script:runs.Add($entry);return [pscustomobject]@{entry=$entry;output=$output}
}
function Invoke-EditorResourceFixtureReplay([string]$PackageRoot,[string]$FixturesRoot,$Fixtures,[string]$WorkRoot) {
    $results=New-Object 'Collections.Generic.List[object]'
    $seen=@{}
    $oldModuleReport=[Environment]::GetEnvironmentVariable('WP_EDITOR_MODULE_REPORT')
    try {
        # Preserve the guest startup's independently reconciled module report.
        [Environment]::SetEnvironmentVariable('WP_EDITOR_MODULE_REPORT',$null)
        foreach($entry in @($Fixtures)){
            Assert-Clean (-not $seen.ContainsKey($entry.name)) 'Duplicate retained resource fixture.';$seen[$entry.name]=$true
            $root=Get-ContainedPath $FixturesRoot $entry.name
            $descriptor=Assert-EditorResourceFixture $entry $root
            Assert-Clean (($descriptor | ConvertTo-Json -Depth 12 -Compress) -ceq ($entry.descriptor | ConvertTo-Json -Depth 12 -Compress)) 'Pinned fixture descriptor differs.'
            $workspace=Get-ContainedPath $WorkRoot $entry.name;New-Item -ItemType Directory -Path $workspace | Out-Null
            $package=Join-Path $root 'package'
            if($entry.name -eq 'extended-typed'){
                $arguments=@('--verify',$package,$descriptor.target)+@($descriptor.uuids)
                $run=Invoke-EditorFixtureProcess (Join-Path $PackageRoot 'bin/WorkphoneResourcePackage.exe') $arguments $workspace 'extended-typed-verify' 90
                Assert-Clean ($run.entry.exitCode -eq 0 -and -not $run.entry.error -and -not $run.entry.timedOut) 'Source-free extended typed UUID verification failed.'
                $results.Add([ordered]@{name=$entry.name;verifiedUuids=@($descriptor.uuids).Count;validationPassed=$true})
            }else{
                $exe=Join-Path $PackageRoot 'bin/Editor.exe';$overlay=Join-Path $root 'overlay'
                $base=@('--interactive','--package-root',$PackageRoot,'--workspace',$workspace,'--runtime-scene-package',$package,
                    '--runtime-scene-target',$descriptor.target,'--runtime-scene-validate')
                foreach($kind in @('dx11','software')){
                    $scene=$descriptor.scenes.$kind
                    $run=Invoke-EditorFixtureProcess $exe ($base+@('--runtime-scene-uuid',$scene.uuid,('--'+$kind))) $workspace ('mixed-scene-'+$kind) 90
                    Assert-EditorFixtureLoaded $run $scene.expected 1 'base' 0
                }
                $selected=$base+@('--runtime-scene-uuid',$descriptor.scenes.dx11.uuid,'--dx11','--runtime-scene-overlay',$overlay)
                $run=Invoke-EditorFixtureProcess $exe $selected $workspace 'mixed-scene-overlay-reject' 90;$report=Get-EditorFixtureBootstrap $run
                Assert-Clean ($run.entry.exitCode -eq 1 -and $report.status -ceq 'failed' -and $report.mountCount -eq 2 -and $report.diagnostic -match 'collision') 'Duplicate package rejection changed.'
                foreach($policy in @('identical','prefer-earlier')){
                    $run=Invoke-EditorFixtureProcess $exe ($selected+@('--runtime-scene-collisions',$policy)) $workspace ('mixed-scene-overlay-'+$policy) 90
                    Assert-EditorFixtureLoaded $run $descriptor.scenes.dx11.expected 2 'overlay-0' 1
                }
                $run=Invoke-EditorFixtureProcess $exe ($base+@('--runtime-scene-uuid',$descriptor.scenes.dx11.uuid,'--software')) $workspace 'mixed-scene-software-material' 90
                $report=Get-EditorFixtureBootstrap $run
                Assert-Clean ($run.entry.exitCode -eq 1 -and $report.status -ceq 'failed' -and $report.diagnostic -cmatch 'active DX11') 'Software material capability rejection changed.'
                $run=Invoke-EditorFixtureProcess $exe ($base+@('--runtime-scene-uuid',$descriptor.scenes.audio.uuid,'--dx11')) $workspace 'mixed-scene-audio' 90
                $report=Get-EditorFixtureBootstrap $run;$audio='unavailable'
                if($run.entry.exitCode -eq 0){Assert-EditorFixtureLoaded $run $descriptor.scenes.audio.expected 1 'base' 0;$audio='available'}
                else{Assert-Clean ($run.entry.exitCode -eq 1 -and $report.status -ceq 'failed' -and $report.diagnostic -clike '*Runtime audio output device is unavailable*') 'Audio failed for an unexpected cause.'}
                $run=Invoke-EditorFixtureProcess $exe ($base+@('--runtime-scene-uuid',[guid]::NewGuid().ToString(),'--dx11')) $workspace 'mixed-scene-missing-uuid' 90
                $report=Get-EditorFixtureBootstrap $run
                Assert-Clean ($run.entry.exitCode -eq 1 -and $report.status -ceq 'failed' -and $report.diagnostic -cmatch 'UUID') 'Missing captured scene UUID rejection changed.'
                $results.Add([ordered]@{name=$entry.name;validationPassed=$true;dx11=$true;software=$true;overlayPolicies=@('reject','identical','prefer-earlier');audio=$audio})
            }
            [void](Assert-EditorResourceFixture $entry $root)
        }
    }finally{[Environment]::SetEnvironmentVariable('WP_EDITOR_MODULE_REPORT',$oldModuleReport)}
    return [pscustomobject]@{typedFixtureCoverage=($results.Count -gt 0);fixtures=@($results.ToArray());immutableInputsVerified=$true}
}

if($LibraryOnly){return}
$requestedPackage=$PackageRoot;$requestedFixtures=$FixturesRoot;$requestedManifest=$CandidateManifestPath;$requestedReport=$ReportPath
. (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1') -LibraryOnly
$PackageRoot=$requestedPackage;$FixturesRoot=$requestedFixtures;$CandidateManifestPath=$requestedManifest;$ReportPath=$requestedReport
Assert-Clean ($PackageRoot -and $FixturesRoot -and $CandidateManifestPath -and $ReportPath) 'Supply PackageRoot, FixturesRoot, CandidateManifestPath and a fresh ReportPath.'
$ReportPath=[IO.Path]::GetFullPath($ReportPath);Assert-Clean (-not(Test-Path -LiteralPath $ReportPath)) 'Preserve earlier replay evidence; choose a fresh report.'
$EvidenceRoot=Split-Path $ReportPath -Parent;New-Item -ItemType Directory -Path $EvidenceRoot -Force | Out-Null
$script:runs=New-Object 'Collections.Generic.List[object]'
$report=[ordered]@{schema='workphone.editor-resource-fixture-host-replay.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o');
    environmentKind='host-only';cleanMachineExecuted=$false;guestIdentityVerified=$false;releaseCertified=$false;validationPassed=$false}
try{
    $manifest=Get-Content -LiteralPath $CandidateManifestPath -Raw | ConvertFrom-Json
    Assert-Clean ((Get-CleanHash $PSCommandPath) -eq $manifest.resourceFixtureRunnerSha256 -and
        (Get-CleanHash (Join-Path $PSScriptRoot 'TerrainCleanMachineGuest.ps1')) -eq $manifest.commonRunnerSha256) 'Replay tooling differs from the prepared pins.'
    Assert-CleanInventory $manifest.expectedFiles $PackageRoot
    $work=Join-Path $EvidenceRoot ('resource-fixture-replay-'+[guid]::NewGuid().ToString('N'));New-Item -ItemType Directory -Path $work | Out-Null
    $fixtures=@();if($manifest.PSObject.Properties['sourceFreeFixtures']){$fixtures=@($manifest.sourceFreeFixtures)}
    $report['result']=Invoke-EditorResourceFixtureReplay $PackageRoot $FixturesRoot $fixtures $work
    Assert-CleanInventory $manifest.expectedFiles $PackageRoot
    $report['candidateManifestSha256']=Get-CleanHash $CandidateManifestPath;$report['validationPassed']=$true
}catch{$report['error']=$_.Exception.Message;throw}finally{
    $report['runs']=@($script:runs.ToArray());$report | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath $ReportPath -Encoding UTF8
}
