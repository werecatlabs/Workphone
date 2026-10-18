# Windows C++ development SDK candidate

The `WorkphoneCppSDK` install component overlays the `WorkphoneEditor` runtime
with Workphone/Claw C++ headers, native interface headers, Boost headers, import
libraries and a relocatable CMake package. Use the same prefix for both components.
This candidate supports Windows x64, the producing MSVC compiler version, C++17
and the release DLL CRT. Debug consumers are rejected because this distribution
does not contain matching Debug DLLs. The package records the DLL's compile
definitions, including its string, pointer, atomic and precision configuration.

```powershell
cmake --install project_x64 --config RelWithDebInfo --component WorkphoneEditor --prefix <sdk>
cmake --install project_x64 --config RelWithDebInfo --component WorkphoneCppSDK --prefix <sdk>
cmake -S <sdk>/share/WorkphoneCppSDK/sample -B <external-build> -A x64 -DCMAKE_PREFIX_PATH=<sdk>
cmake --build <external-build> --config RelWithDebInfo
ctest --test-dir <external-build> -C RelWithDebInfo --output-on-failure
```

Consumers use `find_package(WorkphoneCppSDK 0.1 CONFIG REQUIRED)` and link
`WorkphoneCpp::Core` or `WorkphoneCpp::Graphics`. Select only Release,
RelWithDebInfo or MinSizeRel and set `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`.
Keep the runtime's complete `bin` dependency closure beside the executable or
on its DLL search path. The installed sample tests real exported APIs without
initializing an Editor or graphics device. It does not certify all public headers,
optional plugin SDKs, interactive rendering, licensing or clean-machine support.
