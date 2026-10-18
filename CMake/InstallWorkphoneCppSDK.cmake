# Development files overlay the existing Editor runtime component. Keep the
# installed interface independent of the build tree's private link dependencies.
include(CMakePackageConfigHelpers)
install(CODE "if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL \"RelWithDebInfo\")\n message(FATAL_ERROR \"C++ SDK requires RelWithDebInfo DLLs and release CRT\")\nendif()"
    COMPONENT WorkphoneCppSDK)
foreach(_sdk_target Workphone WPGraphics)
    install(FILES "$<TARGET_LINKER_FILE:${_sdk_target}>" DESTINATION lib
        COMPONENT WorkphoneCppSDK CONFIGURATIONS RelWithDebInfo)
endforeach()
install(DIRECTORY "${WP_ROOT}/Engine/cpp/Include/Workphone" "${WP_ROOT}/Engine/cpp/Include/WPGraphics"
    DESTINATION include COMPONENT WorkphoneCppSDK
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl")
foreach(_native_headers WorkphoneCore WorkphoneGraphics WorkphonePlatformWin32 WorkphonePhysics)
    install(DIRECTORY "${WP_ROOT}/Engine/c/Include/${_native_headers}" DESTINATION include
        COMPONENT WorkphoneCppSDK FILES_MATCHING PATTERN "*.h" PATTERN "*.inl")
endforeach()
# Boost is part of the public header surface, even though the DLL owns its
# compiled dependency libraries. Do not ship dependency build trees or binaries.
install(DIRECTORY "${WP_ROOT}/Dependencies/boost/boost" DESTINATION include
    COMPONENT WorkphoneCppSDK FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.ipp" PATTERN "*.inc")

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
install(FILES "${WP_ROOT}/docs/WPGRAPHICS_CPP_SDK.md" DESTINATION .
    COMPONENT WorkphoneCppSDK)
