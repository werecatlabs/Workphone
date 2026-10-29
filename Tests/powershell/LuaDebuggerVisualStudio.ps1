# Use the supported Visual Studio DTE debugger API against a LuaQA instance.
# This fixture does not use Windows UI Automation or capture screen pixels.
# Supply the Editor's opt-in activity JSON to distinguish a new pause from the
# old Break mode while an asynchronous DTE step is still being dispatched.
param(
    [Parameter(Mandatory=$true)][int]$VsProcessId,
    [ValidateSet('Status','OpenToolWindow','ShowLocals','Breakpoint','ClearBreakpoints','Attach','StepOver','StepInto','StepOut','Continue','Pause','Disconnect','Inspect')]
    [string]$Action='Status',
    [string]$Endpoint,[string]$Source,[int]$Line=203,
    [string]$Expression='state.port',[string]$Adapter,[string]$Report,[string]$ActivityReport,
    [switch]$WaitForBreak
)
$ErrorActionPreference='Stop'
function Read-LuaActivityReport([string]$Path) {
 $stream=$null
 try {
  $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
  $bytes=New-Object byte[] 65537
  $size=0
  while($size -lt $bytes.Length) {
   $count=$stream.Read($bytes,$size,$bytes.Length-$size)
   if($count -eq 0) {break}
   $size+=$count
  }
  if($size -gt 65536) {throw 'Lua debugger activity report exceeds 64 KiB'}
  $activity=[Text.Encoding]::UTF8.GetString($bytes,0,$size) | ConvertFrom-Json
  if($activity.paused -isnot [bool] -or
     ($activity.pauseSequence -isnot [int] -and $activity.pauseSequence -isnot [long]) -or
     [long]$activity.pauseSequence -lt 0) {
   throw 'Invalid Lua debugger activity report'
  }
  return $activity
 } catch [IO.FileNotFoundException] {return $null}
   catch [IO.DirectoryNotFoundException] {return $null}
 finally {if($stream) {$stream.Dispose()}}
}
$process=Get-CimInstance Win32_Process -Filter "ProcessId=$VsProcessId"
if (!$process -or $process.Name -ne 'devenv.exe' -or $process.CommandLine -notmatch '/RootSuffix\s+"?LuaQA"?(?:\s|$)') { throw 'Only the isolated LuaQA IDE is permitted' }
if (-not ('LuaVsRot' -as [type])) {
Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
public static class LuaVsRot {
 [DllImport("ole32.dll")] static extern int CreateBindCtx(uint reserved,out IBindCtx context);
 public static object Get(int processId) {
  IBindCtx context; Marshal.ThrowExceptionForHR(CreateBindCtx(0,out context));
  IRunningObjectTable table=null; IEnumMoniker enumerator=null;
  try { context.GetRunningObjectTable(out table); table.EnumRunning(out enumerator);
   var monikers=new IMoniker[1];
   while(enumerator.Next(1,monikers,IntPtr.Zero)==0) {
    string name=null; try {monikers[0].GetDisplayName(context,null,out name);} catch(UnauthorizedAccessException) {}
    if(name!=null && name.Contains("VisualStudio.DTE") && name.EndsWith(":"+processId)) {object value; table.GetObject(monikers[0],out value); return value;}
   }
   throw new InvalidOperationException("DTE is not registered yet");
  } finally {if(enumerator!=null) Marshal.ReleaseComObject(enumerator); if(table!=null) Marshal.ReleaseComObject(table); Marshal.ReleaseComObject(context);}
 }
}
"@
}
$dte=[LuaVsRot]::Get($VsProcessId)
$debugger=$dte.Debugger
if ($null -eq $debugger -or $null -eq $debugger.CurrentMode) { throw 'Visual Studio automation is unavailable. Dismiss any modal dialog in the LuaQA instance and retry.' }
$activityBefore=if($ActivityReport) {Read-LuaActivityReport $ActivityReport} else {$null}
$previousPauseSequence=if($activityBefore) {[long]$activityBefore.pauseSequence} else {0L}
switch($Action) {
 'OpenToolWindow' {$dte.Commands.Raise('{2EBFD2BD-111F-4E2D-A607-4E6D8474D40C}',256,'',0)}
 'ShowLocals' {$dte.ExecuteCommand('Debug.Locals','')}
 'Breakpoint' { $points=$debugger.Breakpoints.Add('',$Source,$Line,1,'',1,'','',1,'',0,1); Write-Output ('Breakpoints: '+$points.Count) }
 'ClearBreakpoints' { while($debugger.Breakpoints.Count -gt 0){$debugger.Breakpoints.Item(1).Delete()} }
 'Attach' {
  if([int]$debugger.CurrentMode -ne 1) {throw 'Test IDE already has a debug session'}
  if (!(Test-Path -LiteralPath $Endpoint) -or (Get-Item -LiteralPath $Endpoint).Length -gt 65536) { throw 'Invalid endpoint file' }
  $data=Get-Content -LiteralPath $Endpoint -Raw | ConvertFrom-Json
  if (!$Adapter) {
    $profile=($dte.RegistryRoot -split '\\')[-1]
    $extensions=Join-Path $env:LOCALAPPDATA ('Microsoft/VisualStudio/'+$profile+'/Extensions')
    $candidates=@(Get-ChildItem -LiteralPath $extensions -Recurse -File -Filter WorkphoneLuaDebugAdapter.exe)
    if ($candidates.Count -ne 1) { throw 'Specify the installed adapter executable explicitly with -Adapter' }
    $Adapter=$candidates[0].FullName
  }
  if (!(Test-Path -LiteralPath $Adapter)) { throw 'Adapter executable missing' }
  $launch=Join-Path (Split-Path $Endpoint -Parent) ('vs-attach-'+[guid]::NewGuid().ToString('N')+'.json')
  @{'$adapter'=$Adapter;'$adapterArgs'=[string]$data.port;type='workphone-lua';request='attach';name='Workphone Lua';token=$data.token;stopOnEntry=$false;'$localsScopeName'='Locals'} | ConvertTo-Json | Set-Content -LiteralPath $launch -Encoding UTF8
  $dte.Commands.Raise('0ddba113-7ac1-4c6e-a2ef-dcac3f9e731e',257,('/LaunchJson:"'+$launch+'" /EngineGuid:"8BC205D5-3836-4E05-A729-A6D15A6E3884"'),0)
 }
 'StepOver' {$debugger.StepOver($false)}
 'StepInto' {$debugger.StepInto($false)}
 'StepOut' {$debugger.StepOut($false)}
 'Continue' {$debugger.Go($false)}
 'Pause' {$debugger.Break($false)}
 'Disconnect' {$debugger.Stop($false)}
 'Inspect' {
  if ([int]$debugger.CurrentMode -ne 2) { throw 'Inspection requires a stopped debug session' }
  $value=$debugger.GetExpression($Expression,$false,5000)
  $inspection=[ordered]@{expression=$Expression;valid=$value.IsValidValue;value=$value.Value;type=$value.Type;members=@($value.DataMembers | Select-Object -First 16 | ForEach-Object { [ordered]@{name=$_.Name;value=$_.Value;type=$_.Type} })}
 }
}
if ($WaitForBreak) {
 $deadline=[DateTime]::UtcNow.AddSeconds(20)
 $requiresNewPause=$Action -in @('Attach','StepOver','StepInto','StepOut','Continue','Pause')
 while ($true) {
  $activity=if($ActivityReport) {Read-LuaActivityReport $ActivityReport} else {$null}
  $activityReady=(!$ActivityReport) -or ($activity -and $activity.paused -and
   (!$requiresNewPause -or [long]$activity.pauseSequence -gt $previousPauseSequence))
  if ([int]$debugger.CurrentMode -eq 2 -and $activityReady) {break}
  if ([DateTime]::UtcNow -gt $deadline) {
   if($ActivityReport) {throw 'Visual Studio did not reach the requested paused activity sequence within 20 seconds'}
   throw 'Visual Studio did not stop within 20 seconds'
  }
  Start-Sleep -Milliseconds 100
 }
}
$mode=[int]$debugger.CurrentMode
$result=[ordered]@{action=$Action;mode=$mode;breakpoints=$debugger.Breakpoints.Count}
if ($ActivityReport) {
 $activity=Read-LuaActivityReport $ActivityReport
 $result.activityPauseSequence=if($activity) {[long]$activity.pauseSequence} else {$null}
}
if ($inspection) { $result.inspection=$inspection }
if($mode -eq 2) {
 $frame=$debugger.CurrentStackFrame
 $result.frame=[ordered]@{function=$frame.FunctionName;language=$frame.Language;file=$dte.ActiveDocument.FullName;line=$dte.ActiveDocument.Selection.ActivePoint.Line}
 $result.locals=@($frame.Locals | ForEach-Object { [ordered]@{name=$_.Name;value=$_.Value;type=$_.Type} })
}
$json=$result | ConvertTo-Json -Depth 8
if ($Report) { [IO.File]::WriteAllText([IO.Path]::GetFullPath($Report),$json) }
$json
