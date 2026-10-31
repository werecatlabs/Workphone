# C++ Editor package candidate

Build the shared Windows x64 Claw/Lua Editor with `Tools/BuildEditorPackage.ps1`.
The `WorkphoneEditor` CMake install component installs the RelWithDebInfo Editor,
its configured dynamic plugins, resolved PE runtime dependencies, media
and available dependency notices. Debug Editor is a Boost test executable and is
deliberately rejected by this component. Other install components are unrelated.
Matching linker symbols are installed by `WorkphoneEditorSymbols` and archived
separately as `WorkphoneEditorSymbols-Windows-x64.zip`.
The separate `WorkphoneCppSDK` component adds development headers, import
libraries and CMake targets over this runtime; see [the C++ SDK guide](WPGRAPHICS_CPP_SDK.md).

Extract the ZIP and launch `bin/Editor.exe`. The adjacent `editor-package.cfg`
selects packaged startup automatically. Media is located relative to the package;
settings, logs and caches use `%LOCALAPPDATA%/Workphone/Editor`. Use
`--workspace PATH` to select another writable directory outside the package.
Existing checkout settings, asset databases and personal layouts are excluded.
Windows x64 and the Microsoft Visual C++ x64 release Redistributable are required.

`--package-smoke` exercises the release executable's existing headless Editor
lifecycle with the software renderer, mounted Editor scripts, resource and
graphics managers, actual Lua object binding/execution, and orderly teardown.
The validator extracts outside the checkout, restricts the process environment,
records loaded modules while plugins are alive, and checks immutable payload
hashes. This is startup/dependency evidence; interactive DX11 presentation,
clean-machine prerequisites, hardware performance, project authoring workflows,
complete media/dependency license auditing and shipping certification remain open.
This package is a local validation candidate until those gates pass.

`Tools/AuditEditorPackage.ps1` inventories the exact candidate's binaries, PE
imports, link inputs, media and embedded font metadata. It requires the installed
notice files to match the checked-in originals. Both package validators run it
before archiving and preserve `dependency-media-audit.json` outside the payload.
The inventory is evidence of bytes and provenance; its `openReviewItems` identify
permission and redistribution questions that still require review.

Prepare a portable clean-Windows validation kit with stock Windows PowerShell:

```powershell
Tools/ValidateEditorCleanMachine.ps1 -Mode Prepare -KitDirectory <new-kit> `
  -ArchivePath <Editor.zip> -PackageEvidencePath <editor-package-evidence.json> `
  -ExpectedArchiveSha256 <sha256>
```

The generated README describes Launch/Collect on a host where Windows Sandbox is
already available. The kit pins the archive, scripts and payload inventory,
requires a distinct fresh guest identity, probes loader prerequisites, and checks
real Editor/Lua/plugin startup, module provenance, immutable files and missing
media rejection. Preparation does not establish a guest pass. An explicitly
provided Microsoft-signed redistributable can be provisioned inside the guest;
the tools do not enable host features or install host prerequisites.

`Tools/ValidateGraphicsReleaseEvidence.ps1` consolidates the SDK, Editor and
graphics reports, rechecks their current inputs and archive identities, and
requires matching candidate runtime bytes and exact dependency-audit inventories.
Supply `-RequireComplete` to reject any remaining release gate. A successful
consistency check keeps `releaseReady` false while any required acceptance is open.
