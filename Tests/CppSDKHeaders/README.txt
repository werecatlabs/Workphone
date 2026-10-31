Installed C++ SDK header audit

Requires PowerShell 7, CMake, and the MSVC version matching this SDK.
Run against the extracted SDK, with a NEW output directory outside the SDK:

  pwsh -File <sdk>/share/WorkphoneCppSDK/header-check/ValidateCppSDKHeaders.ps1 -SdkRoot <sdk> -EvidenceDirectory <new-evidence-directory>

Use -Exhaustive to syntax-check all installed engine .h/.hpp files (except PCH
aggregations), with one translation unit per file and no precompiled header.
The default checks the maintained consumer/authoring contract headers. Add
-GeneratorInstance <Visual-Studio-installation> when several MSVC installations
exist. -Generator and -Toolset can also pin the producing build's generator.

sdk-header-evidence.json inventories every installed engine header. It records
each checked file's hash, result and failure diagnostic. Implementation .inl and
PCH files require their owning translation unit; they are not standalone APIs.
An exhaustive run with ANY failed header keeps fullPublicSurfacePassed=false.
A required-only pass also keeps that field false. Passing syntax does not prove
that every declared symbol is exported or that its runtime behavior is correct.
The same command builds and runs the installed Position/ScaleAnimator and
AtomicString behavior contracts, including explicit template instantiation.

windows-header-exclusions.json records the two Apple-only headers retained in
engine source and omitted from this Windows SDK. Their original audit failures
remain in the release evidence; the audit verifies they are absent here.

Supported CMake link components are Core, Graphics (Claw), and SQLite. An Editor
runtime plugin in bin/ does not imply its development headers/library are shipped.
Requesting an unavailable required component fails; an unavailable optional
component has WorkphoneCppSDK_<component>_FOUND=FALSE.
