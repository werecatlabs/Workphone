# Native graphics SDK (Windows/MSVC, static libraries)

This is the first native C SDK distribution slice of SHIP-03, version 0.1.0.
It reuses WorkphoneCore, WorkphoneGraphics and WorkphonePlatformWin32. It
contains a cooked native WPM1 triangle sample; it does not contain the C++ editor,
catalog-backed content sample or a physics library. Physics
headers are included to close Core header dependencies. Exported Core feature
definitions must remain enabled in consumers; STB implementation definitions are
private to the archives. Public graphics entry points support C90 and C++ linkage.

Build with `cmake -S . -B project_graphics_sdk -A x64 -DWP_BUILD_GRAPHICS_SDK_ONLY=ON`,
then build and install Debug and RelWithDebInfo with `cmake --build` and
`cmake --install` using `--config` and `--prefix`. Archives and compiler PDBs are
configuration-specific under `lib`. `Tools/ValidateGraphicsSDK.ps1` automates a
fresh build, relocation, ZIP creation/extraction and external sample build/run in
Debug, RelWithDebInfo and Release (using RelWithDebInfo archives).
Its evidence report hashes the installed files, input snapshot and distributable
ZIP; validation fails if inputs change during the run or required PDBs are absent.
Every extracted file must match the installed file hash. The consumer links only
the extracted package. The C90 runtime loads `sample/content/triangle.wpm`, created
at build time by the existing mesh writer; it does not regenerate geometry at
runtime. This exercises native mesh serialization, not the C++ runtime catalog.
The Debug and RelWithDebInfo cooks must produce byte-identical content; both
hashes are recorded and a mismatch fails validation.
The installed sample's CTest command supplies the content path; when running the
executable directly, pass the absolute path to `triangle.wpm` as its sole argument.

The runner also installs the RelWithDebInfo C90 consumer as
`WorkphoneGraphicsSample.exe` into a separate runtime-only ZIP. It includes the
cooked mesh, linker PDB, notices and a hash manifest. The extracted runtime is
launched from a fresh temporary directory, with inherited developer environment
variables removed and PATH limited to Windows directories. Its actual loaded
modules must reside under Windows or be the sample executable. Valid, missing
and truncated content runs must return their exact expected exit codes; payload
hashes must remain unchanged. See [runtime instructions](WPGRAPHICS_RUNTIME_SAMPLE.md).
This is a local source-free launch check, not a clean-machine or GPU acceptance run.

The native CI definition is `.github/workflows/native-graphics-release.yml`. It
builds/runs isolated native contracts in Debug/RelWithDebInfo and validates the
SDK ZIP on Windows 2022/MSVC, retaining JUnit, logs, hashes and package artifacts.
Pass `-Generator 'Visual Studio 17 2022'` to select that toolchain locally. This
workflow does not certify the full Claw/Editor, imported media or hardware gates;
remote execution must be observed before claiming a clean CI pass.

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
