<#
.SYNOPSIS
Verifies real Lua Apply/Save actions and restoration in a second Editor process.
.DESCRIPTION
Requires an in-tree test-enabled Editor build. Omit -Run to prepare only.
The writer starts with no preferences file; both launches share a new private
startup folder. The reader observes startup state and normal remembered-project
opening without applying or saving. Evidence and disposable files are retained.
.EXAMPLE
pwsh -NoProfile -File Tests/powershell/ProjectSettingsRestart.ps1 -Run
#>
param(
    [string]$RepoRoot=(Join-Path $PSScriptRoot '../..'),
    [string]$EditorPath='',
    [string]$EvidenceDirectory='',
    [switch]$Run
)
$ErrorActionPreference='Stop'
$repository=(Resolve-Path -LiteralPath $RepoRoot).Path
if(!$EditorPath){$EditorPath=Join-Path $repository 'bin/windows/v145/x64/MD/Debug/Editor.exe'}
$editorExecutable=(Resolve-Path -LiteralPath $EditorPath).Path
$binaryDirectory=Split-Path -Parent $editorExecutable
$nonce=[guid]::NewGuid().ToString('N')
if(!$EvidenceDirectory){$EvidenceDirectory=Join-Path $repository 'project_x64/test-evidence/ProjectSettingsRestart'}
New-Item -ItemType Directory -Path $EvidenceDirectory -Force | Out-Null
$runRoot=Join-Path (Resolve-Path -LiteralPath $EvidenceDirectory).Path ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+$nonce)
$working=Join-Path (Split-Path -Parent $binaryDirectory) ('SettingsRestart-'+$nonce)
New-Item -ItemType Directory -Path $runRoot,$working | Out-Null
@{fixture='project-settings-process-restart-v1';nonce=$nonce;workingDirectory=$working} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runRoot 'restart-fixture.json')
$bootstrap=Get-Content -LiteralPath (Join-Path $binaryDirectory 'wp_plugins_editor.cfg') -Raw
$bootstrap=[regex]::Replace($bootstrap,'(?m)^PluginFolder=.*$',('PluginFolder='+$binaryDirectory.Replace('\','/')))
[IO.File]::WriteAllText((Join-Path $working 'wp_plugins_editor.cfg'),$bootstrap,[Text.UTF8Encoding]::new($false))
Write-Output ('Evidence directory: '+$runRoot)
Write-Output ('Private startup directory: '+$working)
if(!$Run){Write-Output 'Prepared only; use -Run after coordinating Editor/DLL ownership.';exit 0}

function ReadUserPreferenceFingerprints {
    foreach($configuration in @('Debug','RelWithDebInfo')){
        foreach($filename in @('LioncatEditor.settings','LioncatEditor.ini')){
            $path=Join-Path (Split-Path -Parent $binaryDirectory) ($configuration+'/'+$filename)
            [pscustomobject]@{path=$path;sha256=$(if(Test-Path -LiteralPath $path){(Get-FileHash -LiteralPath $path).Hash}else{'absent'})}
        }
    }
}
function ReadSavedFileFingerprints {
    foreach($filename in @('project.fbproject','ProjectRuntime.settings','GameGraphics.settings')){
        $path=Join-Path $runRoot ('Project/'+$filename)
        [pscustomobject]@{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash;bytes=(Get-Item -LiteralPath $path).Length}
    }
}
function ShowStageFailure([string]$Stage){
    # Boost assertion details are on stdout; CRT heap reports can dominate stderr.
    Select-String -Path (Join-Path $runRoot ($Stage+'.stdout.log')),(Join-Path $runRoot ($Stage+'.stderr.log')) `
        -Pattern 'fatal error:|error: in|WP_ASSERT failed|last checkpoint' |
        Select-Object -First 12 | ForEach-Object { Write-Output $_.ToString() }
}
function InvokeRestartStage([string]$Stage){
    $info=[Diagnostics.ProcessStartInfo]::new()
    $info.FileName=$editorExecutable
    $info.WorkingDirectory=$working
    $info.UseShellExecute=$false
    $info.CreateNoWindow=$true
    $info.RedirectStandardOutput=$true
    $info.RedirectStandardError=$true
    foreach($argument in @('--run_test=editor_project_settings_live_bridge_test','--log_level=test_suite','--report_level=detailed','--catch_system_errors=yes')){$info.ArgumentList.Add($argument)}
    $info.Environment['WP_EDITOR_SETTINGS_RESTART_STAGE']=$Stage
    $info.Environment['WP_EDITOR_SETTINGS_RESTART_DIRECTORY']=$runRoot
    $info.Environment.Remove('WP_LUA_DEBUGGER') | Out-Null
    $process=[Diagnostics.Process]::new();$process.StartInfo=$info
    $started=[DateTime]::UtcNow
    if(!$process.Start()){throw 'Cannot start Editor test process'}
    $childPid=$process.Id
    $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
    $timedOut=$false
    while(!$process.WaitForExit(1000)){
        if(([DateTime]::UtcNow-$started).TotalSeconds -gt 90){$timedOut=$true;$process.Kill($true);$process.WaitForExit();break}
    }
    [IO.File]::WriteAllText((Join-Path $runRoot ($Stage+'.stdout.log')),$stdout.GetAwaiter().GetResult())
    [IO.File]::WriteAllText((Join-Path $runRoot ($Stage+'.stderr.log')),$stderr.GetAwaiter().GetResult())
    $record=[pscustomobject]@{stage=$Stage;pid=$childPid;startedUtc=$started;finishedUtc=[DateTime]::UtcNow;exitCode=$process.ExitCode;timedOut=$timedOut}
    $record | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $runRoot ($Stage+'-process.json'))
    Write-Output $record
    $process.Dispose()
}
$before=@(ReadUserPreferenceFingerprints)
$manifest=@{
    head=(& git -C $repository rev-parse HEAD);nonce=$nonce;privateWorkingDirectory=$working
    editorSha256=(Get-FileHash -LiteralPath $editorExecutable).Hash
    coreSha256=(Get-FileHash -LiteralPath (Join-Path $binaryDirectory 'Workphone.dll')).Hash
    graphicsSha256=(Get-FileHash -LiteralPath (Join-Path $binaryDirectory 'WPGraphics.dll')).Hash
    audioSha256=(Get-FileHash -LiteralPath (Join-Path $binaryDirectory 'WPAudio.dll')).Hash
    luaSha256=(Get-FileHash -LiteralPath (Join-Path $repository 'bin/Media/Scripts/Lua/Editor/ProjectSettings.lua')).Hash
    soundEditorLuaSha256=(Get-FileHash -LiteralPath (Join-Path $repository 'bin/Media/Scripts/Lua/Editor/SoundEditor.lua')).Hash
    userPreferencesBefore=$before
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runRoot 'manifest.json')
$writer=InvokeRestartStage 'write'
$writer | Format-List
if($writer.exitCode -ne 0 -or $writer.timedOut){ShowStageFailure 'write';throw 'Writer failed; reader was not started'}
if(!(Test-Path -LiteralPath (Join-Path $runRoot 'native-write.json'))){throw 'Writer native evidence is missing'}
$savedAfterWrite=@(ReadSavedFileFingerprints)
$privatePreferences=Get-Content -LiteralPath (Join-Path $working 'LioncatEditor.settings') -Raw
if(!$privatePreferences.Contains('project.fbproject')){throw 'Normal shutdown did not remember the private project'}
$reader=InvokeRestartStage 'read'
$reader | Format-List
$savedAfterRead=@(ReadSavedFileFingerprints)
$after=@(ReadUserPreferenceFingerprints)
$filesUnchanged=($savedAfterWrite | ConvertTo-Json -Compress) -eq ($savedAfterRead | ConvertTo-Json -Compress)
$preferencesUnchanged=($before | ConvertTo-Json -Compress) -eq ($after | ConvertTo-Json -Compress)
$result=@{
    runRoot=$runRoot;writer=$writer;reader=$reader;distinctPids=($writer.pid -ne $reader.pid)
    savedFilesUnchangedByReader=$filesUnchanged;userPreferencesUnchanged=$preferencesUnchanged
    savedAfterWrite=$savedAfterWrite;savedAfterRead=$savedAfterRead;userPreferencesAfter=$after
}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runRoot 'result.json')
if($reader.exitCode -ne 0 -or $reader.timedOut){ShowStageFailure 'read';throw 'Reader failed'}
if(!$filesUnchanged -or !$preferencesUnchanged -or $writer.pid -eq $reader.pid){throw 'Restart isolation/persistence checks failed'}
if(!(Test-Path -LiteralPath (Join-Path $runRoot 'native-read.json'))){throw 'Reader native evidence is missing'}
Write-Output 'PASS: two distinct Editor processes; Apply live-only, Save persisted, startup restored controls/runtime, originals unchanged.'
