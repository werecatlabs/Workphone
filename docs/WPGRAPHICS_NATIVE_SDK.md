# Native graphics SDK (Windows/MSVC, static libraries)

This is the first native C SDK distribution slice of SHIP-03, version 0.1.0.
It reuses WorkphoneCore, WorkphoneGraphics and WorkphonePlatformWin32. It does
not contain the C++ editor, cooked-content sample or a physics library. Physics
headers are included to close Core header dependencies. Exported Core feature
definitions must remain enabled in consumers; STB implementation definitions are
private to the archives. Public graphics entry points support C90 and C++ linkage.

Build with `cmake -S . -B project_graphics_sdk -A x64 -DWP_BUILD_GRAPHICS_SDK_ONLY=ON`,
then build and install Debug and RelWithDebInfo with `cmake --build` and
`cmake --install` using `--config` and `--prefix`. Archives and compiler PDBs are
configuration-specific under `lib`. `Tools/ValidateGraphicsSDK.ps1` automates a
fresh build, relocation, and external sample build/run for both configurations.
Its evidence report hashes the installed files, input snapshot and distributable
ZIP; validation fails if inputs change during the run or required PDBs are absent.

Consumers use `find_package(WorkphoneGraphicsSDK 0.1 CONFIG REQUIRED)` and link
`WorkphoneGraphics::SDK`. Use the exported `WorkphoneGraphicsSDK_MSVC_RUNTIME_LIBRARY`
on consumer targets (Microsoft DLL CRT, Debug CRT in Debug). Match package architecture
and compiler ABI. Windows SDK import libraries are dependencies, not redistributed
binaries: d3d11, d3d12, dxgi, d3dcompiler, dxguid, user32 and gdi32. Core/Graphics
targets expose the libraries separately; the SDK aggregate supplies backend closure.
Install both configurations. Release and MinSizeRel consumers map to the
RelWithDebInfo archives; Debug maps only to Debug, preventing silent CRT mixing.

The Workphone license is installed alongside this document. Bundled STB headers
retain their full upstream MIT/public-domain dual-license notices, including
stb_rect_pack.h and stb_truetype.h in include/WorkphoneCore. Windows SDK and
Microsoft runtime redistribution terms are supplied by Microsoft. This package
does not redistribute Windows DLLs. No hardware or cross-platform certification
is implied by a software renderer consumer pass.
