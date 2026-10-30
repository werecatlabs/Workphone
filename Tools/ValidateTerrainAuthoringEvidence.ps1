#requires -Version 5.1
[CmdletBinding()]
param(
    [string]$EvidenceDirectory,
    [string]$BaselinePath,
    [string]$PaintedPath,
    [string]$UndoPath,
    [string]$RedoPath,
    [string]$CancelledPath,
    [string]$ScenePath,
    [string]$RestartedPath,
    [string]$TerrainComponentUuid,
    [string]$LayerPaintedPath,
    [string]$LayerRestartedPath,
    [string]$HolePaintedPath,
    [string]$HoleRestartedPath,
    [string]$ScreenshotManifestPath,
    [string]$OperatorAttestationPath,
    [string]$OutputPath,
    [ValidateRange(0.001, 0.5)][double]$MaximumChangedFraction = 0.25,
    [ValidateRange(0.001, 1000)][double]$MaximumPhysicalHeightDelta = 16,
    [string]$SelfTestScenePath,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$invariant = [Globalization.CultureInfo]::InvariantCulture

function Assert-Authoring([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}
function Test-JsonNumber($Value) {
    return $Value -is [byte] -or $Value -is [sbyte] -or $Value -is [int16] -or $Value -is [uint16] -or
        $Value -is [int32] -or $Value -is [uint32] -or $Value -is [int64] -or $Value -is [uint64] -or
        $Value -is [single] -or $Value -is [double] -or $Value -is [decimal] -or
        ($null -ne $Value -and $Value.GetType().FullName -eq 'System.Numerics.BigInteger')
}
function Test-JsonObject($Value) {
    return $null -ne $Value -and $Value -isnot [string] -and $Value -isnot [ValueType] -and
        ($Value -is [Collections.IDictionary] -or $Value -isnot [Collections.IEnumerable])
}
function Test-JsonArray($Value) {
    return $null -ne $Value -and $Value -is [Collections.IEnumerable] -and
        $Value -isnot [string] -and $Value -isnot [Collections.IDictionary]
}
function Assert-NativeNumber($Value, [string]$Name) {
    Assert-Authoring (Test-JsonNumber $Value) "$Name must be a JSON number, not a string or Boolean."
    $number = [double]$Value
    Assert-Authoring (-not [double]::IsNaN($number) -and -not [double]::IsInfinity($number) -and
        [math]::Abs($number) -le [double][single]::MaxValue) "$Name exceeds the native finite f32 input range."
}
function Assert-NativeRange($Value, [double]$Minimum, [double]$Maximum, [string]$Name) {
    Assert-NativeNumber $Value $Name
    $number = [double][single]$Value
    Assert-Authoring ($number -ge [double][single]$Minimum -and $number -le [double][single]$Maximum) "$Name exceeds the native supported range."
}
function Assert-TextureIdentifier($Value, [string]$Name) {
    Assert-Authoring ($Value -is [string] -and [Text.Encoding]::UTF8.GetByteCount($Value) -le 4096 -and $Value.IndexOf([char]0) -lt 0) "$Name must be a bounded UTF-8 string without NUL."
}
function Assert-UnambiguousJson([string]$Text, [string]$Name) {
    Assert-Authoring ($Text.IndexOf('\u0000',[StringComparison]::Ordinal) -lt 0) "$Name contains an escaped NUL rejected by the native codec."
    # Native cJSON selects the first duplicate key; ConvertFrom-Json selects the
    # last. Reject ambiguity rather than comparing a payload the engine won't see.
    $frames=[Collections.Generic.Stack[object]]::new()
    foreach($token in [regex]::Matches($Text,'"(?:\\.|[^"\\])*"|[{}\[\]]')) {
        switch($token.Value) {
            '{' {$frames.Push([pscustomobject]@{kind='object';names=[Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)});continue}
            '[' {$frames.Push([pscustomobject]@{kind='array';names=$null});continue}
            '}' {Assert-Authoring ($frames.Count -gt 0 -and $frames.Peek().kind -eq 'object') "$Name has unbalanced JSON objects.";[void]$frames.Pop();continue}
            ']' {Assert-Authoring ($frames.Count -gt 0 -and $frames.Peek().kind -eq 'array') "$Name has unbalanced JSON arrays.";[void]$frames.Pop();continue}
        }
        if($token.Value[0] -eq '"') {
            $next=$token.Index+$token.Length
            while($next -lt $Text.Length -and [char]::IsWhiteSpace($Text[$next])){++$next}
            if($next -lt $Text.Length -and $Text[$next] -eq ':') {
                Assert-Authoring ($frames.Count -gt 0 -and $frames.Peek().kind -eq 'object') "$Name has a JSON key outside an object."
                $key=[string]($token.Value | ConvertFrom-Json)
                Assert-Authoring ($frames.Peek().names.Add($key)) "$Name has duplicate JSON key $key."
            }
        }
    }
    Assert-Authoring ($frames.Count -eq 0) "$Name has unterminated JSON containers."
}
function Get-Sha256Text([string]$Text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text)))).Replace('-', '') }
    finally { $sha.Dispose() }
}
function Get-Properties($Value) {
    if ($Value -is [Collections.IDictionary]) {
        foreach ($key in $Value.Keys) { [pscustomobject]@{ Name = [string]$key; Value = $Value[$key] } }
    } else {
        $Value.PSObject.Properties | Where-Object { $_.MemberType -eq 'NoteProperty' -or $_.MemberType -eq 'Property' }
    }
}
function ConvertTo-Canonical($Value) {
    if ($null -eq $Value) { return 'null' }
    if ($Value -is [string]) { return (ConvertTo-Json -InputObject $Value -Compress) }
    if ($Value -is [bool]) { if ($Value) { return 'true' } else { return 'false' } }
    if ($Value -is [ValueType]) {
        $number = [Convert]::ToDouble($Value, $invariant)
        Assert-Authoring (-not [double]::IsNaN($number) -and -not [double]::IsInfinity($number)) 'Nonfinite terrain value.'
        if ($number -eq 0) { return '0' }
        return $number.ToString('R', $invariant)
    }
    $parts = [Collections.Generic.List[string]]::new()
    if ($Value -is [Collections.IEnumerable] -and $Value -isnot [Collections.IDictionary]) {
        foreach ($item in $Value) { $parts.Add((ConvertTo-Canonical $item)) }
        return '[' + ($parts -join ',') + ']'
    }
    foreach ($property in @(Get-Properties $Value | Sort-Object Name -CaseSensitive)) {
        $parts.Add((ConvertTo-Canonical ([string]$property.Name)) + ':' + (ConvertTo-Canonical $property.Value))
    }
    return '{' + ($parts -join ',') + '}'
}
function Get-TerrainCanonical($Terrain, [string[]]$Except = @()) {
    $copy = [ordered]@{}
    foreach ($property in @(Get-Properties $Terrain)) {
        if ($property.Name -ne 'revision' -and $property.Name -notin $Except) { $copy[$property.Name] = $property.Value }
    }
    return ConvertTo-Canonical $copy
}
function Assert-FiniteArray($Values, [int]$Count, [string]$Name) {
    Assert-Authoring (Test-JsonArray $Values) "$Name must be a JSON array."
    $array = @($Values)
    Assert-Authoring ($array.Count -eq $Count) "$Name has $($array.Count) values; expected $Count."
    foreach ($value in $array) {
        Assert-NativeNumber $value $Name
    }
}
function Assert-TerrainSnapshot($Terrain, [string]$Name) {
    Assert-Authoring (Test-JsonObject $Terrain) "$Name must be a JSON object."
    $properties = @(Get-Properties $Terrain | ForEach-Object { $_.Name })
    foreach ($required in @('format', 'version', 'width', 'depth', 'heightScale', 'spacing', 'origin', 'heights', 'weights', 'holes', 'surfaceMasks', 'appearance', 'layers')) {
        Assert-Authoring ($required -cin $properties) "$Name lacks native v3 field $required."
    }
    Assert-NativeNumber $Terrain.version "$Name version"
    Assert-NativeNumber $Terrain.width "$Name width"
    Assert-NativeNumber $Terrain.depth "$Name depth"
    Assert-Authoring ($Terrain.format -is [string] -and $Terrain.format -ceq 'workphone.terrain' -and $Terrain.version -eq 3) "$Name must use the native version-three terrain schema."
    $width = [double]$Terrain.width; $depth = [double]$Terrain.depth
    Assert-Authoring ($width -eq [math]::Floor($width) -and $depth -eq [math]::Floor($depth) -and $width -ge 2 -and $depth -ge 2 -and $width -le 8193 -and $depth -le 8193 -and $width * $depth -le 4194304) "$Name dimensions exceed the native bounded format."
    $count = [int]($width * $depth)
    Assert-FiniteArray @($Terrain.heightScale) 1 "$Name heightScale"
    Assert-Authoring ([single]$Terrain.heightScale -ne 0) "$Name needs nonzero native height scale for sculpt validation."
    Assert-FiniteArray $Terrain.spacing 2 "$Name spacing"
    Assert-Authoring ([single]$Terrain.spacing[0] -gt 0 -and [single]$Terrain.spacing[1] -gt 0) "$Name native spacing must be positive."
    Assert-FiniteArray $Terrain.origin 2 "$Name origin"
    Assert-FiniteArray $Terrain.heights $count "$Name heights"
    $maximumCoordinate = 0.0; $axisMagnitudes = @(0.0,0.0)
    foreach ($axis in 0..1) {
        $origin = [double][single]$Terrain.origin[$axis]; $spacing = [double][single]$Terrain.spacing[$axis]
        $size = if ($axis -eq 0) { [int]$width } else { [int]$depth }
        $span = [single](($size - 1) * $spacing); $endpoint = [single]($origin + [double]$span)
        Assert-Authoring (-not [single]::IsInfinity($span) -and -not [single]::IsInfinity($endpoint)) "$Name coordinate extent overflows native f32."
        $maximumCoordinate = [math]::Max($maximumCoordinate, [math]::Max([math]::Abs($origin), [math]::Abs($origin + ($size - 1) * $spacing)))
        $shaderEndpoint=[single]($origin + ($size - 1) * $spacing)
        $axisMagnitudes[$axis]=[math]::Max([math]::Abs($origin),[math]::Abs([double]$shaderEndpoint))
        $previous = [single]$origin
        for ($i = 1; $i -lt $size; ++$i) {
            $coordinate = [single]($origin + $i * $spacing)
            Assert-Authoring ($coordinate -gt $previous) "$Name spacing collapses adjacent native renderer coordinates."
            $previous = $coordinate
        }
    }
    $maximumScaledHeight = 0.0
    foreach ($height in $Terrain.heights) {
        $scaled = [double][single]$height * [double][single]$Terrain.heightScale
        Assert-Authoring ([math]::Abs($scaled) -le [double][single]::MaxValue) "$Name scaled height overflows native f32."
        $maximumScaledHeight = [math]::Max($maximumScaledHeight, [math]::Abs($scaled))
    }
    $weights = @($Terrain.weights); $holes = @($Terrain.holes); $masks = @($Terrain.surfaceMasks)
    Assert-Authoring ($weights.Count -eq 0 -or $weights.Count -eq 4 * $count) "$Name weights have invalid length."
    Assert-FiniteArray $weights $weights.Count "$Name weights"
    for ($sample = 0; $sample -lt $weights.Count; $sample += 4) {
        $sum = 0.0
        foreach ($channel in 0..3) {
            $value = [double][single]$weights[$sample + $channel]
            Assert-Authoring ($value -ge 0 -and $value -le 1) "$Name weight is outside 0..1."
            $sum += $value
        }
        Assert-Authoring ($sum -eq 0 -or [math]::Abs($sum - 1) -le 0.00001) "$Name weights are not normalized (zero uses native layer-zero fallback)."
    }
    Assert-Authoring ($holes.Count -eq 0 -or $holes.Count -eq ($width - 1) * ($depth - 1)) "$Name holes have invalid length."
    Assert-FiniteArray $Terrain.holes $holes.Count "$Name holes"
    foreach ($hole in $holes) { Assert-Authoring ($hole -eq 0 -or $hole -eq 1) "$Name has a nonbinary numeric hole. $hole" }
    Assert-Authoring ($masks.Count -eq 0 -or $masks.Count -eq 2 * $count) "$Name surface masks have invalid length."
    Assert-FiniteArray $masks $masks.Count "$Name masks"
    foreach ($mask in $masks) { Assert-NativeRange $mask 0 1 "$Name surface mask" }
    Assert-Authoring ((Test-JsonArray $Terrain.layers) -and @($Terrain.layers).Count -eq 4) "$Name must retain four terrain layers."
    foreach ($layer in @($Terrain.layers)) {
        Assert-Authoring (Test-JsonObject $layer) "$Name layer must be a JSON object."
        foreach ($map in @('albedo', 'normal', 'orm')) { Assert-TextureIdentifier $layer.$map "$Name layer $map" }
        foreach ($scalar in @('roughness', 'metallic', 'normalStrength', 'heightScale', 'heightBias')) { Assert-FiniteArray @($layer.$scalar) 1 "$Name layer $scalar" }
        foreach ($scalar in @('roughness','metallic')) { Assert-NativeRange $layer.$scalar 0 1 "$Name layer $scalar" }
        Assert-NativeRange $layer.normalStrength 0 16 "$Name layer normalStrength"
        Assert-NativeRange $layer.heightScale 0 4 "$Name layer heightScale"
        Assert-NativeRange $layer.heightBias -1 1 "$Name layer heightBias"
        Assert-FiniteArray $layer.uvScale 2 "$Name layer UV scale"
        Assert-Authoring ([single]$layer.uvScale[0] -gt 0 -and [single]$layer.uvScale[1] -gt 0) "$Name layer UV scale must be native positive."
        Assert-FiniteArray $layer.tint 4 "$Name layer tint"
        foreach ($tint in $layer.tint) { Assert-NativeRange $tint 0 1 "$Name layer tint" }
    }
    $appearance = $Terrain.appearance
    Assert-Authoring (Test-JsonObject $appearance) "$Name appearance must be a JSON object."
    Assert-NativeNumber $appearance.projection "$Name projection"
    Assert-Authoring ($appearance.projection -in @(0,1) -and $appearance.heightBlend -is [bool] -and $appearance.detailNormal -is [string]) "$Name appearance mode, flag or texture ID is invalid."
    Assert-FiniteArray @($appearance.macroSeed) 1 "$Name macro seed"
    Assert-Authoring ($appearance.macroSeed -ge 0 -and $appearance.macroSeed -le 4294967295 -and $appearance.macroSeed -eq [math]::Floor($appearance.macroSeed)) "$Name macro seed is outside uint32."
    foreach ($field in @('triplanarSharpness','heightTransition','detailStrength','macroScale','macroStrength','wetness','wetDarkening','wetRoughness','snow','snowRoughness')) { Assert-FiniteArray @($appearance.$field) 1 "$Name appearance $field" }
    Assert-NativeRange $appearance.triplanarSharpness 1 16 "$Name triplanar sharpness"
    Assert-NativeRange $appearance.heightTransition 0.001 1 "$Name height transition"
    Assert-NativeRange $appearance.detailStrength 0 16 "$Name detail strength"
    foreach ($field in @('macroStrength','wetness','wetDarkening','snow')) { Assert-NativeRange $appearance.$field 0 1 "$Name appearance $field" }
    foreach ($field in @('wetRoughness','snowRoughness')) { Assert-NativeRange $appearance.$field 0.045 1 "$Name appearance $field" }
    Assert-TextureIdentifier $appearance.detailNormal "$Name detail normal"
    Assert-FiniteArray $appearance.detailUvScale 2 "$Name detail UV scale"
    Assert-Authoring ([single]$appearance.detailUvScale[0] -gt 0 -and [single]$appearance.detailUvScale[1] -gt 0 -and [single]$appearance.macroScale -gt 0) "$Name appearance UV scales must be native positive."
    foreach ($field in @('macroTint','snowTint')) { Assert-FiniteArray $appearance.$field 4 "$Name appearance $field"; foreach($tint in $appearance.$field){Assert-NativeRange $tint 0 1 "$Name appearance $field tint"} }
    if ([single]$appearance.macroStrength -gt 0) { Assert-Authoring ($maximumCoordinate * [double][single]$appearance.macroScale -le [double][single]::MaxValue) "$Name macro coordinates overflow native shader range." }
    if ($appearance.projection -eq 1) { $maximumCoordinate=[math]::Max($maximumCoordinate,$maximumScaledHeight) }
    if ($appearance.detailNormal.Length) { Assert-Authoring ($maximumCoordinate * [math]::Max([double][single]$appearance.detailUvScale[0],[double][single]$appearance.detailUvScale[1]) -le [double][single]::MaxValue) "$Name detail coordinates overflow native shader range." }
    foreach ($layer in $Terrain.layers) {
        $uvMaximum=[math]::Max([double][single]$layer.uvScale[0],[double][single]$layer.uvScale[1])
        if($appearance.projection -eq 1){Assert-Authoring ($maximumCoordinate * $uvMaximum -le [double][single]::MaxValue) "$Name triplanar material coordinates overflow native shader range."}
        foreach($axis in 0..1){Assert-Authoring ($axisMagnitudes[$axis] * [double][single]$layer.uvScale[$axis] -le [double][single]::MaxValue) "$Name planar material coordinates overflow native shader range."}
    }
}
function Assert-SameTerrain($Actual, $Expected, [string]$Name) {
    Assert-Authoring ((Get-TerrainCanonical $Actual) -ceq (Get-TerrainCanonical $Expected)) "$Name differs from its required canonical terrain payload."
}
function Measure-SculptChange($Baseline, $Painted, [double]$FractionLimit, [double]$DeltaLimit) {
    Assert-Authoring ((Get-TerrainCanonical $Baseline @('heights')) -ceq (Get-TerrainCanonical $Painted @('heights'))) 'The sculpt stroke changed metadata, materials, weights, holes or masks; record those as separate optional edits.'
    $changed = 0; $maximum = 0.0; $minimumX = [int]$Baseline.width; $maximumX = -1; $minimumZ = [int]$Baseline.depth; $maximumZ = -1
    for ($index = 0; $index -lt @($Baseline.heights).Count; ++$index) {
        $delta = ([double][single]$Painted.heights[$index] - [double][single]$Baseline.heights[$index]) * [double][single]$Baseline.heightScale
        if ($delta -ne 0) {
            ++$changed; $maximum = [math]::Max($maximum, [math]::Abs($delta))
            $x = $index % [int]$Baseline.width; $z = [int][math]::Floor($index / [double]$Baseline.width)
            $minimumX = [math]::Min($minimumX, $x); $maximumX = [math]::Max($maximumX, $x)
            $minimumZ = [math]::Min($minimumZ, $z); $maximumZ = [math]::Max($maximumZ, $z)
        }
    }
    Assert-Authoring ($changed -gt 0) 'Painted terrain is unchanged; a screenshot or success label cannot substitute for an actual sculpt edit.'
    Assert-Authoring ($changed -le [math]::Floor(@($Baseline.heights).Count * $FractionLimit)) 'Sculpt edit exceeds the configured bounded sample fraction.'
    Assert-Authoring ($maximum -le $DeltaLimit) 'Sculpt edit exceeds the configured physical height delta.'
    return [ordered]@{ changedSamples = $changed; totalSamples = @($Baseline.heights).Count; maximumPhysicalHeightDelta = $maximum; heightDeltaSpace = 'Terrain-local metres: native f32 samples multiplied by native heightScale; actor world scale is outside this comparison.'; changedSampleBounds = [ordered]@{ minX = $minimumX; maxX = $maximumX; minZ = $minimumZ; maxZ = $maximumZ }; maximumChangedFraction = $FractionLimit; maximumAllowedPhysicalHeightDelta = $DeltaLimit }
}
function Assert-AuthoringSequence($Snapshots, [double]$FractionLimit, [double]$DeltaLimit) {
    $metrics = Measure-SculptChange $Snapshots.baseline $Snapshots.painted $FractionLimit $DeltaLimit
    Assert-SameTerrain $Snapshots.undo $Snapshots.baseline 'Undo snapshot'
    foreach ($name in @('redo', 'cancelled', 'scene', 'restarted')) { Assert-SameTerrain $Snapshots[$name] $Snapshots.painted "$name snapshot" }
    return $metrics
}
function Measure-EffectiveWeightChanges($Before, $After) {
    $beforeWeights=@($Before.weights); $afterWeights=@($After.weights); $changed=0
    for($sample=0;$sample -lt @($Before.heights).Count;++$sample){
        $beforeValues=@(0.0,0.0,0.0,0.0); $afterValues=@(0.0,0.0,0.0,0.0); $beforeSum=0.0; $afterSum=0.0
        foreach($channel in 0..3){
            if($beforeWeights.Count){$beforeValues[$channel]=[double][single]$beforeWeights[$sample*4+$channel]}
            if($afterWeights.Count){$afterValues[$channel]=[double][single]$afterWeights[$sample*4+$channel]}
            $beforeSum+=$beforeValues[$channel];$afterSum+=$afterValues[$channel]
        }
        if($beforeSum -eq 0){$beforeValues[0]=1.0}
        if($afterSum -eq 0){$afterValues[0]=1.0}
        $different=$false
        foreach($channel in 0..3){if($beforeValues[$channel] -ne $afterValues[$channel]){$different=$true}}
        if($different){++$changed}
    }
    return $changed
}
function Get-InputFile([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    Assert-Authoring (Test-Path -LiteralPath $full -PathType Leaf) "Required evidence is missing: $full"
    $file = Get-Item -LiteralPath $full -Force
    Assert-Authoring (-not ($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) "Evidence cannot be a reparse point: $full"
    Assert-Authoring ($file.Length -gt 0 -and $file.Length -le 134217728) "Evidence is empty or exceeds 128 MiB: $full"
    return [ordered]@{ path = $full; bytes = $file.Length; sha256 = (Get-FileHash -LiteralPath $full -Algorithm SHA256).Hash }
}
function Read-TerrainFile([string]$Path, [string]$Name, $Inventory) {
    $identity = Get-InputFile $Path
    $text=Get-Content -LiteralPath $identity.path -Raw -Encoding UTF8
    Assert-UnambiguousJson $text $Name
    $value = $text | ConvertFrom-Json
    Assert-TerrainSnapshot $value $Name
    $identity.canonicalPayloadSha256 = Get-Sha256Text (Get-TerrainCanonical $value)
    $Inventory[$Name] = $identity
    return $value
}
function Find-ActorTerrain($Actor, $Matches, [string]$ActorPath) {
    Assert-Authoring (Test-JsonObject $Actor) 'Native scene actor must be an object.'
    $names=@(Get-Properties $Actor | ForEach-Object {$_.Name})
    foreach($required in @('uuid','label','children')){Assert-Authoring ($required -cin $names) "Native actor lacks $required."}
    Assert-Authoring ($Actor.uuid -is [string] -and -not [string]::IsNullOrWhiteSpace($Actor.uuid) -and $Actor.label -is [string] -and (Test-JsonArray $Actor.children)) 'Native actor identity/children are invalid.'
    $index=0
    foreach($child in $Actor.children){
        Assert-Authoring (Test-JsonObject $child) 'Native actor child must be a properties object.'
        $childNames=@(Get-Properties $child | ForEach-Object {$_.Name})
        Assert-Authoring ('handle_name' -cin $childNames -and $child.handle_name -is [string]) 'Native actor child needs a properties handle_name.'
        $path=$ActorPath+'/children['+$index+']'
        if($child.handle_name -ceq 'child'){Find-ActorTerrain $child $Matches $path}
        elseif($child.handle_name -ceq 'component' -and 'componentType' -cin $childNames -and $child.componentType -ceq 'workphone::scene::TerrainSystem'){
            Assert-Authoring ('terrainDataV1' -cin $childNames -and 'uuid' -cin $childNames -and $child.uuid -is [string] -and -not [string]::IsNullOrWhiteSpace($child.uuid)) 'Native terrain component needs source and a nonempty UUID.'
            if(-not $TerrainComponentUuid -or $child.uuid -ceq $TerrainComponentUuid){$Matches.Add([pscustomobject]@{component=$child;actorUuid=$Actor.uuid;actorLabel=$Actor.label;actorPath=$ActorPath;componentPath=$path})}
        }
        ++$index
    }
}
function Find-SceneTerrain($Scene, $Matches) {
    Assert-Authoring (Test-JsonObject $Scene) 'Native scene root must be an object.'
    $names=@(Get-Properties $Scene | ForEach-Object {$_.Name})
    Assert-Authoring ('children' -cin $names -and (Test-JsonArray $Scene.children)) 'Native scene root needs its properties children array.'
    $index=0
    foreach($child in $Scene.children){
        Assert-Authoring (Test-JsonObject $child) 'Native scene child must be a properties object.'
        $childNames=@(Get-Properties $child | ForEach-Object {$_.Name})
        Assert-Authoring ('handle_name' -cin $childNames -and $child.handle_name -is [string]) 'Native scene child needs a properties handle_name.'
        if($child.handle_name -ceq 'actors'){Find-ActorTerrain $child $Matches ('/children['+$index+']')}
        ++$index
    }
}
function Test-ExpectedRejection([scriptblock]$Action, [string]$Name) {
    $rejected = $false
    try { & $Action | Out-Null } catch { $rejected = $true }
    Assert-Authoring $rejected "Self-test failed to reject $Name."
    $script:selfTestRejections.Add($Name)
}
if ($SelfTest) {
    $script:selfTestRejections=[Collections.Generic.List[string]]::new()
    $base = [pscustomobject]@{format='workphone.terrain';version=3;width=3;depth=3;heightScale=1;spacing=@(1,1);origin=@(-1,-1);heights=@(0,0,0,0,0,0,0,0,0);weights=@();holes=@();surfaceMasks=@();revision=1
        layers=@(foreach($i in 1..4){[pscustomobject]@{albedo='';normal='';orm='';roughness=1;metallic=0;normalStrength=1;heightScale=0;heightBias=0;uvScale=@(1,1);tint=@(1,1,1,1)}})
        appearance=[pscustomobject]@{projection=0;heightBlend=$false;detailNormal='';triplanarSharpness=4;heightTransition=0.1;detailStrength=0;macroSeed=0;macroScale=1;macroStrength=0;wetness=0;wetDarkening=0;wetRoughness=0.1;snow=0;snowRoughness=0.5;detailUvScale=@(1,1);macroTint=@(1,1,1,1);snowTint=@(1,1,1,1)}}
    function Copy-SelfTestSnapshot($Value){return ($Value | ConvertTo-Json -Depth 16 | ConvertFrom-Json)}
    $paint=Copy-SelfTestSnapshot $base; $paint.heights[4]=0.5; $paint.revision=8
    Assert-TerrainSnapshot $base 'synthetic baseline'; Assert-TerrainSnapshot $paint 'synthetic painted'
    $sequence = @{ baseline = $base; painted = $paint; undo = $base; redo = $paint; cancelled = $paint; scene = $paint; restarted = $paint }
    $metrics = Assert-AuthoringSequence $sequence 0.25 16
    Assert-Authoring ($metrics.changedSamples -eq 1) 'Self-test failed to measure a bounded sculpt.'
    Test-ExpectedRejection { Measure-SculptChange $base $base 0.25 16 } 'unchanged sculpt'
    $sequence.redo = $base
    Test-ExpectedRejection { Assert-AuthoringSequence $sequence 0.25 16 } 'mismatched redo'
    $sequence.redo = $paint; $sequence.cancelled = $base
    Test-ExpectedRejection { Assert-AuthoringSequence $sequence 0.25 16 } 'failed cancellation'
    Test-ExpectedRejection { Measure-SculptChange $base $paint 0.01 16 } 'unbounded sample coverage'
    Test-ExpectedRejection { Measure-SculptChange $base $paint 0.25 0.1 } 'unbounded physical delta'
    $tiny=Copy-SelfTestSnapshot $base; $tiny.heights=@(1,1,1,1,1.00000001,1,1,1,1)
    $flat=Copy-SelfTestSnapshot $base; $flat.heights=@(1,1,1,1,1,1,1,1,1)
    Test-ExpectedRejection {Measure-SculptChange $flat $tiny 0.25 16} 'encoded change that vanishes in native f32'
    $bad=Copy-SelfTestSnapshot $base; $bad.version='3'; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'string version'
    $bad=Copy-SelfTestSnapshot $base; $bad.width='3'; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'string width'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.projection=$false; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'Boolean projection'
    $bad=Copy-SelfTestSnapshot $base; $bad.holes=@($false,0,0,0); Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'Boolean hole'
    $bad=Copy-SelfTestSnapshot $base; $bad.holes=@('0',0,0,0); Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'string hole'
    $bad=Copy-SelfTestSnapshot $base; $bad.heights[4]=3.5e38; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'height outside native f32'
    $bad=Copy-SelfTestSnapshot $base; $bad.heights[4]=3e38; $bad.heightScale=2; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'scaled height overflow'
    $bad=Copy-SelfTestSnapshot $base; $bad.spacing[0]=1e-100; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'spacing underflow'
    $bad=Copy-SelfTestSnapshot $base; $bad.origin[0]=1e10; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'collapsed f32 adjacent coordinates'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].uvScale[0]=0; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'zero layer UV scale'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].roughness=-0.1; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'negative roughness'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].heightScale=5; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'layer height scale'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].heightBias=1.1; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'layer height bias'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].normalStrength=17; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'normal strength'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].tint[3]=1.1; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'layer tint'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].albedo='bad'+[char]0; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'NUL resource identifier'
    $bad=Copy-SelfTestSnapshot $base; $bad.layers[0].albedo=(([char]0x00E9).ToString()*2200); Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'UTF-8 resource identifier byte limit'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.triplanarSharpness=0; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'triplanar sharpness'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.heightTransition=0; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'height transition'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.wetRoughness=0; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'wet roughness'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.macroStrength=1.1; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'macro strength'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.macroSeed='1'; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'string macro seed'
    $bad=Copy-SelfTestSnapshot $base; $bad.appearance.snowTint[0]=-1; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'snow tint'
    $bad=Copy-SelfTestSnapshot $base; $bad.origin[0]=0; $bad.layers[0].uvScale[0]=3e38; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'shader UV overflow'
    $zero=Copy-SelfTestSnapshot $base; $zero.weights=@(foreach($i in 1..36){0})
    Assert-TerrainSnapshot $zero 'native zero fallback'
    $explicit=Copy-SelfTestSnapshot $base; $explicit.weights=@(foreach($i in 1..9){1;0;0;0})
    Assert-TerrainSnapshot $explicit 'explicit layer zero'
    Assert-Authoring ((Measure-EffectiveWeightChanges $base $explicit) -eq 0 -and (Measure-EffectiveWeightChanges $zero $explicit) -eq 0) 'Implicit/zero fallback was counted as a layer edit.'
    $bad=Copy-SelfTestSnapshot $explicit; $bad.weights[0]=0.5; $bad.weights[1]=0.50005; Test-ExpectedRejection {Assert-TerrainSnapshot $bad 'bad'} 'weights outside native normalization tolerance'
    Test-ExpectedRejection {Assert-UnambiguousJson '{"width":3,"\u0077idth":4}' 'bad'} 'escaped duplicate JSON key'
    Test-ExpectedRejection {Assert-UnambiguousJson '{"children":[],"\u0063hildren":[]}' 'bad scene'} 'duplicate scene children key'
    $component=[pscustomobject]@{handle_name='component';uuid='terrain-component';componentType='workphone::scene::TerrainSystem';terrainDataV1=($paint | ConvertTo-Json -Depth 16 -Compress)}
    $actor=[pscustomobject]@{handle_name='actors';uuid='terrain-owner';label='Terrain';children=@($component)}
    $native=[pscustomobject]@{handle_name='';children=@($actor)}
    $matches=[Collections.Generic.List[object]]::new(); Find-SceneTerrain $native $matches
    Assert-Authoring ($matches.Count -eq 1 -and $matches[0].actorUuid -eq 'terrain-owner' -and $matches[0].component.uuid -eq 'terrain-component') 'Native actor/component ownership was not found.'
    $orphan=[pscustomobject]@{handle_name='';children=@();metadata=$component}
    Test-ExpectedRejection {$m=[Collections.Generic.List[object]]::new();Find-SceneTerrain $orphan $m;Assert-Authoring ($m.Count -eq 1) 'No loadable terrain'} 'terrain hidden in ignored scene metadata'
    $wrong=Copy-SelfTestSnapshot $native; $wrong.children[0].children[0].handle_name='metadata'
    Test-ExpectedRejection {$m=[Collections.Generic.List[object]]::new();Find-SceneTerrain $wrong $m;Assert-Authoring ($m.Count -eq 1) 'No loadable terrain'} 'terrain outside native component slot'
    $wrong=Copy-SelfTestSnapshot $native; $wrong.children[0].children+=Copy-SelfTestSnapshot $component
    Test-ExpectedRejection {$m=[Collections.Generic.List[object]]::new();Find-SceneTerrain $wrong $m;Assert-Authoring ($m.Count -eq 1) 'Ambiguous terrain'} 'duplicate matching native components'
    $nativeExample=$null
    if($SelfTestScenePath){
        $identity=Get-InputFile $SelfTestScenePath
        $sceneText=Get-Content -LiteralPath $identity.path -Raw -Encoding UTF8
        Assert-UnambiguousJson $sceneText 'native scene example'
        $native=$sceneText | ConvertFrom-Json
        $matches=[Collections.Generic.List[object]]::new(); Find-SceneTerrain $native $matches
        Assert-Authoring ($matches.Count -eq 1) 'Read-only native scene example did not contain one owned terrain.'
        $source=$matches[0].component.terrainDataV1; Assert-UnambiguousJson $source 'native example'
        Assert-TerrainSnapshot ($source | ConvertFrom-Json) 'native example'
        Assert-Authoring ((Get-FileHash -LiteralPath $identity.path).Hash -eq $identity.sha256) 'Read-only native example changed.'
        $nativeExample=[ordered]@{file=$identity;actorUuid=$matches[0].actorUuid;componentUuid=$matches[0].component.uuid;componentPath=$matches[0].componentPath;payloadValidated=$true}
    }
    Write-Output 'Terrain authoring evidence helper checks passed (no Editor/UI execution).'
    [ordered]@{schema='workphone.terrain-authoring-validator-self-test.v1';passed=$true;rejections=@($script:selfTestRejections);nativeExample=$nativeExample;nativeCodecExecutionVerified=$false;uiMouseInputVerified=$false;processRestartIndependentlyVerified=$false;interactiveEditorAuthoringAccepted=$false;hardwareAcceptanceVerified=$false;independentVisualAcceptanceVerified=$false} | ConvertTo-Json -Depth 8 -Compress | Write-Output
    return
}

Assert-Authoring (-not [string]::IsNullOrWhiteSpace($EvidenceDirectory)) 'Supply -EvidenceDirectory for the operator snapshots and report; individual input paths may override the default names.'
$EvidenceDirectory = [IO.Path]::GetFullPath($EvidenceDirectory)
Assert-Authoring (Test-Path -LiteralPath $EvidenceDirectory -PathType Container) 'Evidence directory must already exist.'
$defaults = @{ BaselinePath = '01-baseline.terrain.json'; PaintedPath = '02-painted.terrain.json'; UndoPath = '03-undo.terrain.json'; RedoPath = '04-redo.terrain.json'; CancelledPath = '05-cancelled.terrain.json'; ScenePath = '06-authored.fbscene'; RestartedPath = '07-restarted.terrain.json' }
foreach ($parameter in $defaults.Keys) {
    if (-not (Get-Variable -Name $parameter -ValueOnly)) { Set-Variable -Name $parameter -Value (Join-Path $EvidenceDirectory $defaults[$parameter]) }
}
if (-not $OutputPath) { $OutputPath = Join-Path $EvidenceDirectory 'terrain-authoring-evidence.json' }
$OutputPath = [IO.Path]::GetFullPath($OutputPath)
Assert-Authoring (-not (Test-Path -LiteralPath $OutputPath)) 'Choose a fresh output report; earlier evidence is preserved.'
Assert-Authoring (Test-Path -LiteralPath (Split-Path $OutputPath -Parent) -PathType Container) 'Output parent must already exist.'
$inventory = [ordered]@{}; $snapshots = @{}; $optional = [ordered]@{}; $screenshots = @(); $attestation = $null
$passed = $false; $failure = $null; $metrics = $null; $componentUuid = $null; $ownerIdentity=$null
$validatorIdentity = Get-InputFile $PSCommandPath
try {
    foreach ($entry in @(@('baseline',$BaselinePath), @('painted',$PaintedPath), @('undo',$UndoPath), @('redo',$RedoPath), @('cancelled',$CancelledPath), @('restarted',$RestartedPath))) {
        $snapshots[$entry[0]] = Read-TerrainFile $entry[1] $entry[0] $inventory
    }
    $sceneIdentity = Get-InputFile $ScenePath
    $sceneText=Get-Content -LiteralPath $sceneIdentity.path -Raw -Encoding UTF8
    Assert-UnambiguousJson $sceneText 'scene'
    $scene = $sceneText | ConvertFrom-Json
    $matches = [Collections.Generic.List[object]]::new()
    Find-SceneTerrain $scene $matches
    Assert-Authoring ($matches.Count -eq 1) 'Scene must contain exactly one matching TerrainSystem with embedded terrainDataV1; use -TerrainComponentUuid to select among multiple terrains.'
    $component=$matches[0].component
    Assert-Authoring ($component.terrainDataV1 -is [string]) 'Scene terrainDataV1 must be the native serialized source string.'
    Assert-UnambiguousJson $component.terrainDataV1 'scene source'
    $snapshots.scene = $component.terrainDataV1 | ConvertFrom-Json
    Assert-TerrainSnapshot $snapshots.scene 'scene'
    $componentUuid = $component.uuid
    $ownerIdentity=[ordered]@{actorUuid=$matches[0].actorUuid;actorLabel=$matches[0].actorLabel;actorPath=$matches[0].actorPath;componentPath=$matches[0].componentPath}
    $sceneIdentity.canonicalPayloadSha256 = Get-Sha256Text (Get-TerrainCanonical $snapshots.scene)
    $inventory.scene = $sceneIdentity
    $metrics = Assert-AuthoringSequence $snapshots $MaximumChangedFraction $MaximumPhysicalHeightDelta
    $optionalBase = $snapshots.painted
    if ($LayerPaintedPath) {
        $layer = Read-TerrainFile $LayerPaintedPath 'layerPainted' $inventory
        Assert-Authoring ((Get-TerrainCanonical $optionalBase @('weights')) -ceq (Get-TerrainCanonical $layer @('weights'))) 'Layer-only edit changed other terrain payload.'
        $changedWeightSamples = Measure-EffectiveWeightChanges $optionalBase $layer
        Assert-Authoring ($changedWeightSamples -gt 0) 'Optional layer paint did not change effective weights; materializing the implicit layer-zero fallback is insufficient.'
        $persisted = $false
        if ($LayerRestartedPath) { $restored = Read-TerrainFile $LayerRestartedPath 'layerRestarted' $inventory; Assert-SameTerrain $restored $layer 'Layer restart'; $persisted = $true }
        $optional.layerPaint = [ordered]@{ contentEditPassed = $true; changedWeightSamples = $changedWeightSamples; contentRestartSnapshotMatches = $persisted }
        $optionalBase = $layer
    } else { Assert-Authoring (-not $LayerRestartedPath) 'Layer restart evidence requires layer painted evidence.' }
    if ($HolePaintedPath) {
        $holes = Read-TerrainFile $HolePaintedPath 'holePainted' $inventory
        Assert-Authoring ((Get-TerrainCanonical $optionalBase @('holes')) -ceq (Get-TerrainCanonical $holes @('holes'))) 'Hole-only edit changed other terrain payload.'
        Assert-Authoring ((ConvertTo-Canonical $optionalBase.holes) -cne (ConvertTo-Canonical $holes.holes)) 'Optional hole paint did not change holes.'
        Assert-Authoring (@($holes.holes | Where-Object { $_ -eq 1 }).Count -gt @($optionalBase.holes | Where-Object { $_ -eq 1 }).Count) 'Optional hole paint must add at least one hole cell.'
        $persisted = $false
        if ($HoleRestartedPath) { $restored = Read-TerrainFile $HoleRestartedPath 'holeRestarted' $inventory; Assert-SameTerrain $restored $holes 'Hole restart'; $persisted = $true }
        $optional.holePaint = [ordered]@{ contentEditPassed = $true; contentRestartSnapshotMatches = $persisted }
    } else { Assert-Authoring (-not $HoleRestartedPath) 'Hole restart evidence requires hole painted evidence.' }
    if ($ScreenshotManifestPath) {
        $manifestIdentity = Get-InputFile $ScreenshotManifestPath
        $manifest = Get-Content -LiteralPath $manifestIdentity.path -Raw -Encoding UTF8 | ConvertFrom-Json
        Assert-Authoring ($manifest.schema -ceq 'workphone.terrain-authoring-screenshots.v1') 'Unsupported screenshot manifest schema.'
        foreach ($capture in @($manifest.screenshots)) {
            Assert-Authoring (-not [string]::IsNullOrWhiteSpace($capture.step)) 'Screenshot needs a step label.'
            $capturePath = [string]$capture.path
            if (-not [IO.Path]::IsPathRooted($capturePath)) { $capturePath = Join-Path (Split-Path $manifestIdentity.path -Parent) $capturePath }
            $identity = Get-InputFile $capturePath
            Assert-Authoring ([IO.Path]::GetExtension($identity.path).ToLowerInvariant() -in @('.png','.jpg','.jpeg','.bmp')) 'Screenshot must be PNG/JPEG/BMP; this script records identity without reviewing pixels.'
            $screenshots += [ordered]@{ step = $capture.step; file = $identity; pixelsReviewed = $false }
        }
        $inventory.screenshotManifest = $manifestIdentity
    }
    if ($OperatorAttestationPath) {
        $identity = Get-InputFile $OperatorAttestationPath
        $declaration = Get-Content -LiteralPath $identity.path -Raw -Encoding UTF8 | ConvertFrom-Json
        Assert-Authoring ($declaration.schema -ceq 'workphone.terrain-authoring-operator.v1') 'Unsupported operator declaration schema.'
        $attestation = [ordered]@{ file = $identity; declaration = $declaration; independentlyVerified = $false }
        $inventory.operatorAttestation = $identity
    }
    foreach ($item in $inventory.Values) { Assert-Authoring ((Get-FileHash -LiteralPath $item.path -Algorithm SHA256).Hash -eq $item.sha256) "Evidence changed while validating: $($item.path)" }
    foreach ($capture in $screenshots) { Assert-Authoring ((Get-FileHash -LiteralPath $capture.file.path -Algorithm SHA256).Hash -eq $capture.file.sha256) 'Screenshot changed while validating.' }
    Assert-Authoring ((Get-FileHash -LiteralPath $validatorIdentity.path -Algorithm SHA256).Hash -eq $validatorIdentity.sha256) 'Validator source changed while collecting evidence.'
    $passed = $true
} catch { $failure = $_.Exception.Message; throw } finally {
    [ordered]@{
        schema = 'workphone.terrain-authoring-content-evidence.v1'; recordedAtUtc = [DateTime]::UtcNow.ToString('o')
        contentRoundtripPassed = $passed; failure = $failure; terrainComponentUuid = $componentUuid; sceneOwner = $ownerIdentity
        canonicalization = 'Object keys sorted; numeric values compared by invariant double roundtrip; only root revision is excluded; array order and all other payload are preserved.'
        sculpt = $metrics; inputFiles = $inventory; optionalEdits = $optional; screenshots = $screenshots; operatorAttestation = $attestation
        validator = $validatorIdentity; nativeCodecExecutionVerified = $false
        sourceFormatValidation = 'Used v3 JSON object/number types, native f32 bounds/coordinates, material/appearance domains and unambiguous keys; no native codec was executed.'
        sceneStructureValidation = 'Terrain payload matched only in root actors / nested child / component properties, with recorded actor ownership; not a complete native scene load.'
        uiMouseInputVerified = $false; interactiveEditorAuthoringAccepted = $false; processRestartIndependentlyVerified = $false
        hardwareAcceptanceVerified = $false; independentVisualAcceptanceVerified = $false; gt0Certified = $false; releaseCertified = $false
        limitations = @('Matching payloads prove content equivalence, not which input device or UI path produced them.', 'The restarted snapshot is an input artifact; a distinct process restart requires observed session/process evidence.', 'Screenshot identity and operator declarations are recorded separately and are not automatic visual or interactive acceptance.', 'Hardware, DPI/focus/input behavior and independent visual quality require their own observed validation.')
    } | ConvertTo-Json -Depth 32 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
}
Write-Output "Terrain content roundtrip evidence passed: $OutputPath"
