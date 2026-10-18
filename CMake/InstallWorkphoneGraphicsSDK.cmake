include(CMakePackageConfigHelpers)

# Reuse the engine targets; the standalone profile only changes distribution.
set(_sdk_targets WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32)
foreach(_target IN LISTS _sdk_targets)
    set_target_properties(${_target} PROPERTIES
        C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF
        COMPILE_PDB_NAME ${_target}
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/symbols/$<CONFIG>")
    target_include_directories(${_target} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Engine/c/Include>"
        "$<INSTALL_INTERFACE:include>")
    foreach(_headers WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32 WorkphonePhysics)
        target_include_directories(${_target} PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/Engine/c/Include/${_headers}>"
            "$<INSTALL_INTERFACE:include/${_headers}>")
    endforeach()
    install(FILES "${CMAKE_BINARY_DIR}/symbols/$<CONFIG>/${_target}.pdb"
        DESTINATION "lib/$<CONFIG>" CONFIGURATIONS Debug RelWithDebInfo OPTIONAL)
endforeach()
set_target_properties(WorkphoneCore PROPERTIES EXPORT_NAME Core)
set_target_properties(WorkphoneGraphics PROPERTIES EXPORT_NAME Graphics)
set_target_properties(WorkphonePlatformWin32 PROPERTIES EXPORT_NAME PlatformWin32)
target_compile_definitions(WorkphoneCore PUBLIC
    WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR WORKPHONE_INCLUDE_FONT_BAKING
    WORKPHONE_INCLUDE_DEFAULT_FONT WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT
    WORKPHONE_INCLUDE_STANDARD_IO WORKPHONE_STRTOD_NEEDED)
target_link_libraries(WorkphonePlatformWin32 PUBLIC d3d12 user32 gdi32)
add_library(WorkphoneGraphicsSDK INTERFACE)
set_target_properties(WorkphoneGraphicsSDK PROPERTIES EXPORT_NAME SDK)
target_link_libraries(WorkphoneGraphicsSDK INTERFACE WorkphonePlatformWin32)
install(TARGETS ${_sdk_targets} WorkphoneGraphicsSDK
    EXPORT WorkphoneGraphicsSDKTargets ARCHIVE DESTINATION "lib/$<CONFIG>")
# Physics headers close Core's public header dependencies, not a physics library.
foreach(_headers WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32 WorkphonePhysics)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/Engine/c/Include/${_headers}"
        DESTINATION include FILES_MATCHING PATTERN "*.h")
endforeach()
install(EXPORT WorkphoneGraphicsSDKTargets NAMESPACE WorkphoneGraphics::
    DESTINATION lib/cmake/WorkphoneGraphicsSDK)
configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/CMake/WorkphoneGraphicsSDKConfig.cmake.in"
    "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDKConfig.cmake"
    INSTALL_DESTINATION lib/cmake/WorkphoneGraphicsSDK)
write_basic_package_version_file(
    "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDKConfigVersion.cmake"
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
file(WRITE "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDK-dependencies.json"
    "{\n  \"schemaVersion\": 1,\n  \"packageVersion\": \"${PROJECT_VERSION}\",\n  \"platform\": \"Windows\",\n  \"pointerBytes\": ${CMAKE_SIZEOF_VOID_P},\n  \"compiler\": \"${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}\",\n  \"linkage\": \"static\",\n  \"crt\": \"Microsoft DLL CRT; Debug CRT for Debug\",\n  \"windowsSdkImportLibraries\": [\"d3d11\", \"d3d12\", \"dxgi\", \"d3dcompiler\", \"dxguid\", \"user32\", \"gdi32\"],\n  \"bundledThirdParty\": [\"STB: embedded MIT/public-domain license notices in installed headers\"],\n  \"scope\": \"Native graphics SDK only; no C++ engine, editor, cooked sample or physics library\",\n  \"releaseCertified\": false\n}\n")
install(FILES "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDKConfig.cmake"
    "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDKConfigVersion.cmake"
    DESTINATION lib/cmake/WorkphoneGraphicsSDK)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_BINARY_DIR}/WorkphoneGraphicsSDK-dependencies.json"
    "${CMAKE_SOURCE_DIR}/docs/WPGRAPHICS_NATIVE_SDK.md"
    DESTINATION share/WorkphoneGraphicsSDK)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Tests/GraphicsSDKConsumer/"
    DESTINATION share/WorkphoneGraphicsSDK/sample)
