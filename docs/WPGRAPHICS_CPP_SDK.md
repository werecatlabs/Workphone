# Windows C++ development SDK candidate

The `WorkphoneCppSDK` install component overlays the `WorkphoneEditor` runtime
with Workphone/Claw C++ headers, native interface headers, Boost headers, import
libraries and a relocatable CMake package. The SQLite headers/import library are
also installed for the authoring example. Use the same prefix for both components.
This candidate supports Windows x64, the producing MSVC compiler version, C++17
and the release DLL CRT. Debug consumers are rejected because this distribution
does not contain matching Debug DLLs. The package records the DLL's compile
definitions, including its string, pointer, atomic and precision configuration.
`Tools/ValidateCppSDK.ps1` builds and installs both components, creates a ZIP,
extracts it outside the checkout, copies the installed sample, and builds/runs
RelWithDebInfo and Release consumers against that extraction. It verifies hashes
before/after read-only execution, restricts the consumer process environment,
records loaded modules, and checks rejection of Debug installs/consumers and
static CRT consumers. Logs, JUnit and a JSON report stay beside each candidate.

The validator builds the installed authoring example with `CPP_SDK_BUILD_COOKER=ON`.
It generates an original 4x4 TGA and material/texture descriptors in private scratch,
assigns durable catalog UUIDs and Runtime shipping policies, and uses the existing
GraphicsResourceCompiler and catalog-checked package exporter. Only `runtime.wprm`
and its two pinned WPRS containers enter `share/WorkphoneCppSDK/content`; the private
sources, catalog and compilation index are deleted before archiving. The generated
sample uses the repository's Workphone license and no imported media.

The runtime example constructs `ResourceSystem(nullptr, nullptr)`, resolves both
UUIDs from the relocated manifest, decodes the typed material and all three exact
texture mips, checks shared dependency identity, and retains resources across
shutdown. It needs no application manager, compiler registry or SQLite plugin.
`Tools/ValidateCookedCppContent.ps1` checks each build in fresh restricted processes,
including missing/corrupt manifest, wrong target and missing/corrupt root/dependency
containers. Negative tests use private copies; the good package stays read-only.

```powershell
cmake --install project_x64 --config RelWithDebInfo --component WorkphoneEditor --prefix <sdk>
cmake --install project_x64 --config RelWithDebInfo --component WorkphoneCppSDK --prefix <sdk>
cmake -S <sdk>/share/WorkphoneCppSDK/sample -B <external-build> -A x64 -DCMAKE_PREFIX_PATH=<sdk>
cmake --build <external-build> --config RelWithDebInfo
ctest --test-dir <external-build> -C RelWithDebInfo --output-on-failure
```

Consumers use `find_package(WorkphoneCppSDK 0.1 CONFIG REQUIRED)` and link
`WorkphoneCpp::Core` or `WorkphoneCpp::Graphics`; authoring code can additionally
link `WorkphoneCpp::SQLite`. Select only Release,
RelWithDebInfo or MinSizeRel and set `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`.
Keep the runtime's complete `bin` dependency closure beside the executable or
on its DLL search path. Raw component installation does not run an authoring tool:
to generate content manually, configure the sample with `CPP_SDK_BUILD_COOKER=ON`,
build it, and run `cpp-sdk-cook-sample <new-content-directory> <private-work-directory>`
with the SDK's bin directory on PATH. Both directories must be separate, and the
content destination must not exist. Point `CPP_SDK_CONTENT_DIRECTORY` at that output
when configuring a copied sample. Validator-produced ZIPs already contain it.
The installed examples test real exported APIs without initializing an Editor or
graphics device. They do not certify all public headers,
optional plugin SDKs, interactive rendering, licensing or clean-machine support.
