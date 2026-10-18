# C++ Editor package candidate

Build the shared Windows x64 Claw/Lua Editor with `Tools/BuildEditorPackage.ps1`.
The `WorkphoneEditor` CMake install component installs the RelWithDebInfo Editor,
its configured dynamic plugins, resolved PE runtime dependencies, media
and available dependency notices. Debug Editor is a Boost test executable and is
deliberately rejected by this component. Other install components are unrelated.
Matching linker symbols are installed by `WorkphoneEditorSymbols` and archived
separately as `WorkphoneEditorSymbols-Windows-x64.zip`.

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
