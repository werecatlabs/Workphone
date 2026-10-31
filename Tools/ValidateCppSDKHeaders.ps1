#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$SdkRoot,
    [Parameter(Mandatory)][string]$EvidenceDirectory,
    [ValidateSet('RelWithDebInfo','Release')][string]$Configuration='RelWithDebInfo',
    [ValidateRange(1,8)][int]$Parallel=4,
    [switch]$Exhaustive,
    [string]$ProjectDirectory,
    [string]$Generator,
    [string]$GeneratorInstance,
    [string]$Toolset
)
$ErrorActionPreference='Stop'
$sdk=[IO.Path]::GetFullPath($SdkRoot)
$evidence=[IO.Path]::GetFullPath($EvidenceDirectory)
if($evidence.StartsWith($sdk.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or
   $sdk.StartsWith($evidence.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase) -or $evidence -ieq $sdk) {
    throw 'Header evidence and installed SDK directories must not overlap'
}
if(Test-Path -LiteralPath $evidence) {
    if(@(Get-ChildItem -LiteralPath $evidence -Force).Count) { throw 'Header evidence directory must be new or empty' }
} else { New-Item -ItemType Directory -Path $evidence | Out-Null }
$project=if($ProjectDirectory) { [IO.Path]::GetFullPath($ProjectDirectory) } else { Join-Path $sdk 'share/WorkphoneCppSDK/header-check' }
$report=[ordered]@{schemaVersion=1;validationPassed=$false;fullPublicSurfacePassed=$false;
    scope='Installed C++ SDK standalone header syntax, without PCH or source-tree include paths';
    configuration=$Configuration;exhaustive=[bool]$Exhaustive;sdkRoot=$sdk;supportedComponents=@('Core','Graphics','SQLite')}
$processes=[Collections.Generic.List[object]]::new()
function Invoke-HeaderConfigure([string]$Label,[string[]]$Extra,[bool]$ExpectSuccess=$true) {
    # MSBuild compiler-probe tlog paths still have a MAX_PATH limit. Keep its
    # scratch names short even when the descriptive evidence root is long.
    $scratch=switch($Label) {
        'configure' { 'c' }
        'reject-required-audio' { 'r' }
        'accept-unavailable-optional-audio' { 'o' }
        default { throw "Unknown header configuration label: $Label" }
    }
    $directory=Join-Path $evidence $scratch
    $arguments=@('-S',$project,'-B',$directory,'-A','x64',"-DCMAKE_PREFIX_PATH=$sdk")
    if($Generator) { $arguments+=@('-G',$Generator) }
    if($GeneratorInstance) { $arguments+="-DCMAKE_GENERATOR_INSTANCE=$GeneratorInstance" }
    if($Toolset) { $arguments+=@('-T',$Toolset) }
    & cmake @arguments @Extra *> (Join-Path $evidence "$Label.log")
    $code=$LASTEXITCODE
    if($ExpectSuccess -and $code -ne 0) { throw "Header configuration failed: $Label ($code); see $Label.log" }
    if(-not $ExpectSuccess -and $code -eq 0) { throw "Unavailable required SDK component was accepted: $Label" }
    $directory
}
try {
    $platformManifest=Get-Content -LiteralPath (Join-Path $project 'windows-header-exclusions.json') -Raw | ConvertFrom-Json
    foreach($excluded in $platformManifest.headers) {
        if(Test-Path -LiteralPath (Join-Path $sdk "include/$($excluded.header)")) {
            throw "Foreign platform header unexpectedly installed in Windows SDK: $($excluded.header)"
        }
    }
    $report['platformExclusions']=$platformManifest.headers
    $required=@(Get-Content -LiteralPath (Join-Path $project 'required-headers.txt') | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })
    $requiredSet=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach($header in $required) {
        if(-not $requiredSet.Add($header)) { throw "Duplicate required header: $header" }
        if(-not (Test-Path -LiteralPath (Join-Path $sdk "include/$header") -PathType Leaf)) { throw "Missing required installed header: $header" }
    }
    $build=Invoke-HeaderConfigure 'configure' @()
    [void](Invoke-HeaderConfigure 'reject-required-audio' @('-DCPP_SDK_TEST_REQUIRED_COMPONENT=Audio') $false)
    if((Get-Content (Join-Path $evidence 'reject-required-audio.log') -Raw) -notlike '*Unavailable C++ SDK component(s): Audio*') {
        throw 'Required component failed for an unexpected reason'
    }
    [void](Invoke-HeaderConfigure 'accept-unavailable-optional-audio' @('-DCPP_SDK_TEST_OPTIONAL_COMPONENT=Audio'))
    $report['unsupportedRequiredComponentRejected']=$true
    $report['unsupportedOptionalComponentReportedUnavailable']=$true
    $compiler=(Get-Content -LiteralPath (Join-Path $build 'header-check-compiler.txt') -Raw).Trim()
    if(-not (Test-Path -LiteralPath $compiler -PathType Leaf)) { throw 'CMake did not find the SDK compiler' }
    $vsMarker=$compiler.IndexOf('/VC/Tools/MSVC/',[StringComparison]::OrdinalIgnoreCase)
    if($vsMarker -lt 0) { $vsMarker=$compiler.IndexOf('\VC\Tools\MSVC\',[StringComparison]::OrdinalIgnoreCase) }
    if($vsMarker -lt 0) { throw "Cannot locate developer environment for compiler $compiler" }
    $devCmd=Join-Path $compiler.Substring(0,$vsMarker) 'Common7/Tools/VsDevCmd.bat'
    if(-not (Test-Path -LiteralPath $devCmd -PathType Leaf) -or $devCmd.Contains('"')) { throw 'Missing or invalid Visual Studio developer command' }
    # Read only INCLUDE from the developer environment. Do not dump the calling
    # process environment or depend on a caller having launched a developer shell.
    $developerStart=[Diagnostics.ProcessStartInfo]::new($env:ComSpec)
    $developerStart.UseShellExecute=$false
    $developerStart.CreateNoWindow=$true
    $developerStart.RedirectStandardOutput=$true
    $developerStart.RedirectStandardError=$true
    $developerStart.Environment['INCLUDE']=''
    $developerStart.Arguments="/d /s /c `"`"$devCmd`" -no_logo -arch=x64 -host_arch=x64 >nul && set INCLUDE`""
    $developerProcess=[Diagnostics.Process]::Start($developerStart)
    $developerOutput=$developerProcess.StandardOutput.ReadToEndAsync()
    $developerError=$developerProcess.StandardError.ReadToEndAsync()
    if(-not $developerProcess.WaitForExit(60000)) { $developerProcess.Kill($true);throw 'MSVC developer environment timed out' }
    if($developerProcess.ExitCode -ne 0) { throw 'Could not initialize MSVC include directories' }
    $includeOutput=$developerOutput.GetAwaiter().GetResult() -split '\r?\n'
    [void]$developerError.GetAwaiter().GetResult()
    $developerProcess.Dispose()
    $includeLine=@($includeOutput | Where-Object { $_.StartsWith('INCLUDE=',[StringComparison]::OrdinalIgnoreCase) })
    if($includeLine.Count -ne 1) { throw 'Developer environment did not report INCLUDE' }
    $includeValue=$includeLine[0].Substring(8)
    $generatedResponse=Join-Path $build "header-check-$Configuration.rsp"
    if(-not (Test-Path -LiteralPath $generatedResponse -PathType Leaf)) { throw 'Installed target compiler response file missing' }
    # cl reads an unmarked response file using the active Windows code page.
    # Preserve Unicode relocation paths by giving its equivalent UTF-16LE BOM.
    $response=Join-Path $evidence 'compiler-arguments.rsp'
    [IO.File]::WriteAllText($response,[IO.File]::ReadAllText($generatedResponse),[Text.UnicodeEncoding]::new($false,$true))
    $report['generatedCompilerResponseSha256']=(Get-FileHash -LiteralPath $generatedResponse).Hash
    $report['compilerResponseEncoding']='UTF-16LE with BOM'
    $report['compiler']=$compiler
    $report['compilerResponseSha256']=(Get-FileHash -LiteralPath $response).Hash
    $report['compilerResponse']=Get-Content -LiteralPath $response
    $targetIncludes=@(Get-Content -LiteralPath (Join-Path $build "header-check-includes-$Configuration.txt") | Where-Object { $_ })
    foreach($directory in $targetIncludes) {
        if(-not [IO.Path]::GetFullPath($directory).StartsWith($sdk.TrimEnd('\')+'\',[StringComparison]::OrdinalIgnoreCase)) {
            throw "Installed target leaks an include directory outside the SDK: $directory"
        }
    }
    $report['installedIncludeDirectories']=$targetIncludes
    $families=@('Workphone','WPGraphics','WPSQLite','WorkphoneCore','WorkphoneGraphics','WorkphonePlatformWin32','WorkphonePhysics')
    $inventory=@(foreach($family in $families) {
        $root=Join-Path $sdk "include/$family"
        if(Test-Path -LiteralPath $root) {
            Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object { $_.Extension -in @('.hpp','.h','.inl') } | ForEach-Object {
                $relative=[IO.Path]::GetRelativePath((Join-Path $sdk 'include'),$_.FullName).Replace('\','/')
                $fragment=$_.Extension -eq '.inl' -or $_.BaseName -match 'PCH$'
                [ordered]@{header=$relative;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash;
                    required=$requiredSet.Contains($relative);kind=$(if($fragment) { 'implementationFragmentOrPCH' } else { 'standaloneCandidate' });
                    status=$(if($fragment) { 'requiresOwnerTranslationUnit' } else { 'notChecked' })}
            }
        }
    }) | Sort-Object { $_.header }
    $selected=@($inventory | Where-Object { $_.kind -eq 'standaloneCandidate' -and ($Exhaustive -or $_.required) })
    $sources=Join-Path $evidence 'translation-units'
    $logs=Join-Path $evidence 'diagnostics'
    New-Item -ItemType Directory -Path $sources,$logs | Out-Null
    $next=0
    $completed=0
    $watch=[Diagnostics.Stopwatch]::StartNew()
    $lastProgress=0
    while($next -lt $selected.Count -or $processes.Count) {
        while($next -lt $selected.Count -and $processes.Count -lt $Parallel) {
            $entry=$selected[$next]
            $id=$next.ToString('D5')
            $source=Join-Path $sources "$id.cpp"
            [IO.File]::WriteAllText($source,"#include <$($entry.header)>`r`n",[Text.UTF8Encoding]::new($false))
            $start=[Diagnostics.ProcessStartInfo]::new($compiler)
            $start.UseShellExecute=$false
            $start.CreateNoWindow=$true
            $start.RedirectStandardOutput=$true
            $start.RedirectStandardError=$true
            $start.WorkingDirectory=$sources
            $start.Environment['INCLUDE']=$includeValue
            [void]$start.Environment.Remove('CL')
            [void]$start.Environment.Remove('_CL_')
            $start.ArgumentList.Add('@'+$response)
            $start.ArgumentList.Add($source)
            $process=[Diagnostics.Process]::Start($start)
            $processes.Add(@{process=$process;entry=$entry;id=$id;started=[DateTime]::UtcNow;
                stdout=$process.StandardOutput.ReadToEndAsync();stderr=$process.StandardError.ReadToEndAsync()})
            $next++
        }
        for($index=$processes.Count-1;$index -ge 0;$index--) {
            $job=$processes[$index]
            if(-not $job.process.HasExited -and ([DateTime]::UtcNow-$job.started).TotalSeconds -gt 120) {
                $job.process.Kill($true)
                $job.entry['timedOut']=$true
            }
            if($job.process.WaitForExit(5)) {
                $out=$job.stdout.GetAwaiter().GetResult()+$job.stderr.GetAwaiter().GetResult()
                $job.entry['exitCode']=$job.process.ExitCode
                $job.entry['status']=if($job.process.ExitCode -eq 0) { 'standaloneCompilationPassed' } else { 'unavailableStandalone' }
                if($job.process.ExitCode -ne 0) {
                    $log=Join-Path $logs "$($job.id).log"
                    $out | Set-Content -LiteralPath $log -Encoding UTF8
                    $job.entry['diagnostic']=[IO.Path]::GetRelativePath($evidence,$log).Replace('\','/')
                }
                $job.process.Dispose()
                $processes.RemoveAt($index)
                $completed++
            }
        }
        if($watch.Elapsed.TotalSeconds-$lastProgress -ge 10) {
            Write-Host "Installed SDK headers: $completed/$($selected.Count) checked"
            $lastProgress=$watch.Elapsed.TotalSeconds
        }
        if($processes.Count) { Start-Sleep -Milliseconds 10 }
    }
    $report['durationSeconds']=[Math]::Round($watch.Elapsed.TotalSeconds,2)
    foreach($entry in $inventory) {
        if((Get-FileHash -LiteralPath (Join-Path $sdk "include/$($entry.header)" )).Hash -ne $entry.sha256) { throw "Installed header changed: $($entry.header)" }
    }
    $failed=@($inventory | Where-Object { $_.status -eq 'unavailableStandalone' })
    $requiredFailures=@($failed | Where-Object { $_.required })
    $report['inventory']=$inventory
    $report['installedHeaderFiles']=$inventory.Count
    $report['checkedHeaders']=$selected.Count
    $report['passedHeaders']=@($selected | Where-Object { $_.status -eq 'standaloneCompilationPassed' }).Count
    $report['failedHeaders']=$failed.Count
    $report['requiredHeaders']=$required.Count
    $report['requiredFailures']=@($requiredFailures | ForEach-Object { $_.header })
    $report['fragmentOrPchFiles']=@($inventory | Where-Object { $_.kind -eq 'implementationFragmentOrPCH' }).Count
    $report['fullPublicSurfacePassed']=[bool]($Exhaustive -and $failed.Count -eq 0)
    $report['installedHeadersUnchanged']=$true
    if($requiredFailures.Count) { throw "Required standalone SDK headers failed: $($requiredFailures.header -join ', ')" }
    & cmake --build $build --config $Configuration --target cpp-sdk-animator-contracts --parallel 2 *> (Join-Path $evidence 'animator-build.log')
    if($LASTEXITCODE -ne 0) { throw 'Installed animator/AtomicString contract build failed; see animator-build.log' }
    & ctest --test-dir $build -C $Configuration --output-on-failure --output-junit (Join-Path $evidence 'animator-contracts.xml') *> (Join-Path $evidence 'animator-contracts.log')
    if($LASTEXITCODE -ne 0) { throw 'Installed animator/AtomicString runtime contracts failed; see animator-contracts.log' }
    $report['animatorContracts']=[ordered]@{passed=$true;tests=1;
        executableSha256=(Get-FileHash -LiteralPath (Join-Path $build "$Configuration/cpp-sdk-animator-contracts.exe")).Hash}
    $report['validationPassed']=$true
    $report
} catch {
    $report['error']=$_.Exception.Message
    throw
} finally {
    foreach($job in $processes) { if(-not $job.process.HasExited) { $job.process.Kill($true) }; $job.process.Dispose() }
    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $evidence 'sdk-header-evidence.json') -Encoding UTF8
    Write-Host "SDK header evidence: $evidence/sdk-header-evidence.json"
}
