#requires -Version 7.0
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$PackageRoot,
    [Parameter(Mandatory)][string]$OutputPath,
    [string]$RepositoryRoot = (Split-Path $PSScriptRoot -Parent),
    [string]$BuildDirectory,
    [ValidateSet('RelWithDebInfo','Release')][string]$Configuration = 'RelWithDebInfo',
    [switch]$AllowIncompleteNotices
)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$package = (Resolve-Path -LiteralPath $PackageRoot).Path.TrimEnd('\','/')
$output = [IO.Path]::GetFullPath($OutputPath)
if ($output.StartsWith($package + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Write the audit outside the immutable package.'
}
if (Test-Path -LiteralPath $output) { throw 'Choose a new audit output to preserve earlier evidence.' }
function Hash-File([string]$Path) { (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
function Get-FontMetadata([string]$Path) {
    $bytes = [IO.File]::ReadAllBytes($Path)
    function U16([int]$Offset) {
        if ($Offset -lt 0 -or $Offset + 2 -gt $bytes.Length) { throw 'Truncated font table.' }
        [int]$bytes[$Offset] * 256 + [int]$bytes[$Offset+1]
    }
    function U32([int]$Offset) { [long](U16 $Offset) * 65536 + (U16 ($Offset+2)) }
    $count = U16 4
    for ($index=0; $index -lt $count; ++$index) {
        $table = 12 + $index * 16
        if ($table + 16 -gt $bytes.Length) { throw 'Truncated font directory.' }
        if ([Text.Encoding]::ASCII.GetString($bytes,$table,4) -ne 'name') { continue }
        $offset = U32 ($table+8)
        $recordCount = U16 ($offset+2)
        $strings = $offset + (U16 ($offset+4))
        $names = foreach ($record in 0..($recordCount-1)) {
            $entry = $offset + 6 + $record * 12
            $id = U16 ($entry+6)
            if ($id -notin @(0,1,2,5,13,14)) { continue }
            $platform = U16 $entry
            $length = U16 ($entry+8)
            $start = $strings + (U16 ($entry+10))
            if ($start + $length -gt $bytes.Length) { throw 'Truncated font name.' }
            $encoding = if ($platform -in @(0,3)) { [Text.Encoding]::BigEndianUnicode } else { [Text.Encoding]::Latin1 }
            [ordered]@{nameId=$id;platformId=$platform;text=$encoding.GetString($bytes,$start,$length)}
        }
        return @($names | Sort-Object { $_.nameId },{ $_.text } -Unique)
    }
    @()
}
function Get-PEImports([string]$Path) {
    $bytes=[IO.File]::ReadAllBytes($Path)
    if($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a){throw 'Invalid PE DOS header.'}
    $pe=[BitConverter]::ToInt32($bytes,0x3c)
    if($pe -lt 0 -or $pe+24 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550){throw 'Invalid PE header.'}
    $sections=[BitConverter]::ToUInt16($bytes,$pe+6)
    $optionalSize=[BitConverter]::ToUInt16($bytes,$pe+20)
    $optional=$pe+24
    $magic=[BitConverter]::ToUInt16($bytes,$optional)
    $directory=if($magic -eq 0x20b){$optional+112}elseif($magic -eq 0x10b){$optional+96}else{throw 'Unsupported PE format.'}
    $importRva=[BitConverter]::ToUInt32($bytes,$directory+8)
    if(-not $importRva){return @()}
    $sectionStart=$optional+$optionalSize
    function Rva-Offset([long]$Rva) {
        for($index=0;$index -lt $sections;++$index){
            $s=$sectionStart+$index*40
            if($s+40 -gt $bytes.Length){throw 'Truncated PE section.'}
            $virtualSize=[BitConverter]::ToUInt32($bytes,$s+8)
            $address=[BitConverter]::ToUInt32($bytes,$s+12)
            $rawSize=[BitConverter]::ToUInt32($bytes,$s+16)
            $raw=[BitConverter]::ToUInt32($bytes,$s+20)
            if($Rva -ge $address -and $Rva -lt $address+[Math]::Max($virtualSize,$rawSize)){return [int]($raw+$Rva-$address)}
        }
        throw 'PE import RVA is outside its sections.'
    }
    $descriptor=Rva-Offset $importRva
    $imports=for($count=0;$count -lt 4096;++$count){
        if($descriptor+20 -gt $bytes.Length){throw 'Truncated PE import descriptor.'}
        $nameRva=[BitConverter]::ToUInt32($bytes,$descriptor+12)
        if(-not $nameRva){break}
        $start=Rva-Offset $nameRva
        $end=$start
        while($end -lt $bytes.Length -and $end-$start -lt 1024 -and $bytes[$end]){++$end}
        if($end -eq $bytes.Length -or $end-$start -ge 1024){throw 'Invalid PE import name.'}
        [Text.Encoding]::ASCII.GetString($bytes,$start,$end-$start)
        $descriptor+=20
    }
    @($imports | Sort-Object -Unique)
}
$report=[ordered]@{schema='workphone.editor-package-audit.v1';recordedAtUtc=[DateTime]::UtcNow.ToString('o')
    packageRoot=$package;configuration=$Configuration;inventoryComplete=$false;noticeBytesVerified=$false
    licenseAuditComplete=$false;legalApprovalRecorded=$false;cleanMachineExecuted=$false;releaseCertified=$false}
try {
    $files=@(Get-ChildItem -LiteralPath $package -Recurse -Force)
    if(@($files | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}).Count){throw 'Package contains reparse points.'}
    $payload=@($files | Where-Object {-not $_.PSIsContainer} | Sort-Object FullName | ForEach-Object {
        [ordered]@{path=[IO.Path]::GetRelativePath($package,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=Hash-File $_.FullName}
    })
    $report['payload']=$payload
    $expected=[Collections.Generic.List[object]]::new()
    $expected.Add([ordered]@{source='LICENSE';installed='licenses/Workphone-LICENSE';required=$true})
    $installer=Get-Content -LiteralPath (Join-Path $repo 'CMake/InstallWorkphoneEditor.cmake') -Raw
    $block=[regex]::Match($installer,'foreach\(_notice\s+([\s\S]*?)\)\s*if\(EXISTS').Groups[1].Value
    foreach($match in [regex]::Matches($block,'"([^"]+)"')){
        $relative=$match.Groups[1].Value
        $expected.Add([ordered]@{source=$relative;installed='licenses/'+$relative.Replace('/','-');required=(Test-Path -LiteralPath (Join-Path $repo $relative) -PathType Leaf)})
    }
    $extra=Get-Content -LiteralPath (Join-Path $repo 'CMake/InstallWorkphoneEditorNotices.cmake') -Raw
    foreach($match in [regex]::Matches($extra,'"([^"|]+)\|([^"]+)"')){
        $expected.Add([ordered]@{source=$match.Groups[1].Value;installed='licenses/'+$match.Groups[2].Value;required=$true})
    }
    $notices=foreach($entry in $expected){
        $source=Join-Path $repo $entry.source
        $installed=Join-Path $package $entry.installed
        $sourceExists=Test-Path -LiteralPath $source -PathType Leaf
        $present=Test-Path -LiteralPath $installed -PathType Leaf
        $sourceHash=if($sourceExists){Hash-File $source}else{$null}
        $installedHash=if($present){Hash-File $installed}else{$null}
        [ordered]@{source=$entry.source;installed=$entry.installed;required=$entry.required;sourcePresent=$sourceExists
            installedPresent=$present;sourceSha256=$sourceHash;installedSha256=$installedHash
            originalBytesMatch=($sourceExists -and $present -and $sourceHash -eq $installedHash)}
    }
    $report['notices']=@($notices)
    $missing=@($notices | Where-Object {$_.required -and -not $_.originalBytesMatch})
    $report['missingOrChangedRequiredNotices']=$missing
    $report['noticeBytesVerified']=($missing.Count -eq 0)
    $binFiles=@($files | Where-Object {-not $_.PSIsContainer -and $_.Extension -in @('.dll','.exe')})
    $report['binaries']=@($binFiles | ForEach-Object {
        $binary=$_
        $imports=@(Get-PEImports $binary.FullName | ForEach-Object {
            $packageImport=Join-Path (Split-Path $binary.FullName -Parent) $_
            $windowsImport=Join-Path "$env:SystemRoot/System32" $_
            [ordered]@{name=$_;packagedBesideBinary=(Test-Path -LiteralPath $packageImport -PathType Leaf)
                presentOnAuditHost=(Test-Path -LiteralPath $windowsImport -PathType Leaf)
                apiSetContract=($_ -match '^(api-ms-|ext-ms-)');cleanGuestAvailabilityVerified=$false}
        })
        [ordered]@{path=[IO.Path]::GetRelativePath($package,$binary.FullName).Replace('\','/');sha256=Hash-File $binary.FullName;imports=$imports}
    })
    if($BuildDirectory){
        $build=(Resolve-Path -LiteralPath $BuildDirectory).Path
        $projects=@(Get-ChildItem -LiteralPath $build -Recurse -Filter '*.vcxproj' -File)
        $report['releaseLinkInputs']=@($binFiles | ForEach-Object {
            $binary=$_
            $project=$projects | Where-Object BaseName -eq $binary.BaseName | Select-Object -First 1
            if($project){
                [xml]$xml=Get-Content -LiteralPath $project.FullName -Raw
                $group=$xml.Project.ItemDefinitionGroup | Where-Object {$_.Condition -like "*$Configuration*"} | Select-Object -First 1
                [ordered]@{binary=$binary.Name;project=$project.FullName;projectSha256=Hash-File $project.FullName
                    libraries=@(([string]$group.Link.AdditionalDependencies -split ';') | Where-Object {$_ -and $_ -notlike '%(*)'})
                    inheritedLinkInputsFullyResolved=$false}
            }
        })
    }
    $report['media']=@($payload | Where-Object {$_.path -like 'Media/*'} | ForEach-Object {
        $entry=$_
        $source=Join-Path $repo ('bin/'+$entry.path)
        $matches=(Test-Path -LiteralPath $source -PathType Leaf) -and (Hash-File $source) -eq $entry.sha256
        $font=$null;$fontError=$null
        if([IO.Path]::GetExtension($entry.path) -ieq '.ttf'){
            try{$font=Get-FontMetadata (Join-Path $package $entry.path)}catch{$fontError=$_.Exception.Message}
        }
        [ordered]@{path=$entry.path;bytes=$entry.bytes;sha256=$entry.sha256;checkedOutSource=('bin/'+$entry.path)
            matchesCheckedOutBytes=$matches;fontNameRecords=$font;fontMetadataError=$fontError
            permissionReviewComplete=$false;note='Byte provenance and embedded metadata are inventory evidence, not permission approval.'}
    })
    $report['prerequisites']=[ordered]@{platform='Windows x64';runtime='Microsoft Visual C++ x64 release runtime matching the produced binaries'
        graphics='DX11 for normal Editor presentation; software renderer for package-smoke'
        sdkDevelopment='Matching producing MSVC compiler ABI, CMake, Windows SDK and release DLL CRT'
        cleanMachineVerified=$false;hostInstalledSoftwareIsNotCleanMachineEvidence=$true}
    $report['openReviewItems']=@('Per-media permission review, including shipped fonts and images.'
        'Native File Dialog: all seven bundled source/header files match release_116 after CRLF normalization; pinned original license retained. Prebuilt library version/provenance remains unestablished.'
        'ZZipLib declares 0.13.62; nine of twelve C source files match that upstream tag after CRLF normalization. Pinned Library GPL v2/MPL1.1 original terms are retained; modified-source/prebuilt provenance and selected distribution obligations remain unresolved.'
        'Static/transitive dependency selection and source/provenance obligations require review; no legal approval is inferred.'
        'Actual fresh guest execution and prerequisite reconciliation remain separate from host inventory.')
    $report['inventoryComplete']=$true
    if($missing.Count -and -not $AllowIncompleteNotices){throw 'Required dependency notices are missing or differ from their local originals.'}
} catch {
    $report['error']=$_.Exception.Message
    throw
} finally {
    $parent=Split-Path $output -Parent
    if(-not(Test-Path -LiteralPath $parent)){New-Item -ItemType Directory -Path $parent | Out-Null}
    $report | ConvertTo-Json -Depth 14 | Set-Content -LiteralPath $output -Encoding UTF8
}
[pscustomobject]$report
