# Development files overlay the existing Editor runtime component. Keep the
# installed interface independent of the build tree's private link dependencies.
include(CMakePackageConfigHelpers)
set(_sdk_build_artifacts "")
foreach(_runtime_target IN LISTS _editor_runtime_targets)
    string(APPEND _sdk_build_artifacts "bin/$<TARGET_FILE_NAME:${_runtime_target}>|$<TARGET_FILE:${_runtime_target}>\n")
endforeach()
install(CODE "if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL \"RelWithDebInfo\")\n message(FATAL_ERROR \"C++ SDK requires RelWithDebInfo DLLs and release CRT\")\nendif()"
    COMPONENT WorkphoneCppSDK)
foreach(_sdk_target Workphone WPGraphics WPSQLite)
    string(APPEND _sdk_build_artifacts "lib/$<TARGET_LINKER_FILE_NAME:${_sdk_target}>|$<TARGET_LINKER_FILE:${_sdk_target}>\n")
    install(FILES "$<TARGET_LINKER_FILE:${_sdk_target}>" DESTINATION lib
        COMPONENT WorkphoneCppSDK CONFIGURATIONS RelWithDebInfo)
endforeach()
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cpp-sdk/$<CONFIG>/build-artifacts.txt"
    CONTENT "${_sdk_build_artifacts}")
install(DIRECTORY "${WP_ROOT}/Engine/cpp/Include/Workphone" "${WP_ROOT}/Engine/cpp/Include/WPGraphics" "${WP_ROOT}/Engine/cpp/Include/WPSQLite"
    DESTINATION include COMPONENT WorkphoneCppSDK
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl"
    PATTERN "iOS" EXCLUDE PATTERN "OSX" EXCLUDE)
foreach(_native_headers WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32 WorkphonePhysics)
    install(DIRECTORY "${WP_ROOT}/Engine/c/Include/${_native_headers}" DESTINATION include
        COMPONENT WorkphoneCppSDK FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")
endforeach()
# Boost is part of the public header surface, even though the DLL owns its
# compiled dependency libraries. Do not ship dependency build trees or binaries.
install(DIRECTORY "${WP_ROOT}/Dependencies/boost/boost" DESTINATION include
    COMPONENT WorkphoneCppSDK FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.ipp" PATTERN "*.inc")
install(DIRECTORY "${WP_ROOT}/Dependencies/rapidjson/include/rapidjson" DESTINATION include
    COMPONENT WorkphoneCppSDK FILES_MATCHING PATTERN "*.h")

# Capture the actual DLL's configuration, including string/atomic/pointer layouts.
# Reconstructing these options from defaults can silently break the C++ ABI.
get_target_property(_sdk_definitions Workphone COMPILE_DEFINITIONS)
get_target_property(_sdk_source_directory Workphone SOURCE_DIR)
get_directory_property(_sdk_directory_definitions DIRECTORY "${_sdk_source_directory}" COMPILE_DEFINITIONS)
list(APPEND _sdk_definitions ${_sdk_directory_definitions})
set(WORKPHONE_CPP_SDK_DEFINITIONS "")
foreach(_definition IN LISTS _sdk_definitions)
    if(_definition MATCHES "(WP_|^ENABLE_PTR_RTTI=|^BOOST_ALL_NO_LIB|^_ENABLE_ATOMIC_ALIGNMENT_FIX)"
       AND NOT _definition MATCHES "_EXPORTS($|=)")
        list(APPEND WORKPHONE_CPP_SDK_DEFINITIONS "${_definition}")
    endif()
endforeach()
list(REMOVE_DUPLICATES WORKPHONE_CPP_SDK_DEFINITIONS)
# These switches alter native public structures embedded by Claw UI classes.
# Require the C library and its C++ adapter to agree, and publish the actual
# shared view. STB implementation and WORKPHONE_STRTOD_NEEDED are implementation
# switches and must never leak into a consuming translation unit.
set(_sdk_native_definition_evidence "")
set(WORKPHONE_CPP_SDK_NATIVE_DEFINITIONS "")
foreach(_native_target WorkphoneCore WPGraphics)
    get_target_property(_native_definitions ${_native_target} COMPILE_DEFINITIONS)
    get_target_property(_native_source_directory ${_native_target} SOURCE_DIR)
    get_directory_property(_native_directory_definitions DIRECTORY "${_native_source_directory}" COMPILE_DEFINITIONS)
    list(APPEND _native_definitions ${_native_directory_definitions})
    set(_native_public_definitions "")
    foreach(_definition IN LISTS _native_definitions)
        if(_definition MATCHES "^WORKPHONE_INCLUDE_")
            list(APPEND _native_public_definitions "${_definition}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _native_public_definitions)
    list(SORT _native_public_definitions)
    if(_native_target STREQUAL "WorkphoneCore")
        set(WORKPHONE_CPP_SDK_NATIVE_DEFINITIONS ${_native_public_definitions})
    elseif(NOT "${WORKPHONE_CPP_SDK_NATIVE_DEFINITIONS}" STREQUAL "${_native_public_definitions}")
        message(FATAL_ERROR "C++ SDK native public structure switches differ between WorkphoneCore and WPGraphics")
    endif()
    string(APPEND _sdk_native_definition_evidence "${_native_target}|${_native_public_definitions}\n")
endforeach()
list(APPEND WORKPHONE_CPP_SDK_DEFINITIONS ${WORKPHONE_CPP_SDK_NATIVE_DEFINITIONS})
string(APPEND _sdk_native_definition_evidence "SDK|${WORKPHONE_CPP_SDK_NATIVE_DEFINITIONS}\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cpp-sdk/$<CONFIG>/native-public-definitions.txt"
    CONTENT "${_sdk_native_definition_evidence}")
configure_package_config_file("${CMAKE_CURRENT_LIST_DIR}/WorkphoneCppSDKConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/WorkphoneCppSDKConfig.configured.cmake"
    INSTALL_DESTINATION lib/cmake/WorkphoneCppSDK)
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cpp-sdk/$<CONFIG>/WorkphoneCppSDKConfig.cmake"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/WorkphoneCppSDKConfig.configured.cmake" TARGET Workphone)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/WorkphoneCppSDKConfigVersion.cmake"
    VERSION 0.1.0 COMPATIBILITY SameMinorVersion)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cpp-sdk/RelWithDebInfo/WorkphoneCppSDKConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/WorkphoneCppSDKConfigVersion.cmake"
    DESTINATION lib/cmake/WorkphoneCppSDK COMPONENT WorkphoneCppSDK)
install(DIRECTORY "${WP_ROOT}/Tests/CppSDKConsumer/" DESTINATION share/WorkphoneCppSDK/sample
    COMPONENT WorkphoneCppSDK)
install(DIRECTORY "${WP_ROOT}/Tests/CppSDKHeaders/" DESTINATION share/WorkphoneCppSDK/header-check
    COMPONENT WorkphoneCppSDK)
install(FILES "${WP_ROOT}/Tools/ValidateCppSDKHeaders.ps1" DESTINATION share/WorkphoneCppSDK/header-check
    COMPONENT WorkphoneCppSDK)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/WorkphoneWindows.manifest" DESTINATION share/WorkphoneCppSDK
    COMPONENT WorkphoneCppSDK)
install(FILES "${WP_ROOT}/docs/WPGRAPHICS_CPP_SDK.md" DESTINATION .
    COMPONENT WorkphoneCppSDK)
