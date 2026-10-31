cmake_minimum_required(VERSION 3.24)
set(CMAKE_CONFIGURATION_TYPES "RelWithDebInfo;Release" CACHE STRING "SDK-compatible configurations")
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL CACHE STRING "SDK DLL CRT")
project(WorkphoneCppSDKHeaders LANGUAGES CXX)

# These are the only link surfaces supplied by this development SDK. Runtime
# plugins present in the Editor package do not imply an imported SDK target.
if(CPP_SDK_TEST_REQUIRED_COMPONENT)
    find_package(WorkphoneCppSDK 0.1 CONFIG REQUIRED COMPONENTS "${CPP_SDK_TEST_REQUIRED_COMPONENT}")
elseif(CPP_SDK_TEST_OPTIONAL_COMPONENT)
    find_package(WorkphoneCppSDK 0.1 CONFIG REQUIRED COMPONENTS Core OPTIONAL_COMPONENTS "${CPP_SDK_TEST_OPTIONAL_COMPONENT}")
    if(WorkphoneCppSDK_${CPP_SDK_TEST_OPTIONAL_COMPONENT}_FOUND)
        message(FATAL_ERROR "Unavailable optional SDK component reported as found")
    endif()
else()
    find_package(WorkphoneCppSDK 0.1 CONFIG REQUIRED COMPONENTS Core Graphics SQLite)
endif()

add_executable(cpp-sdk-animator-contracts animator_contracts.cpp "${WorkphoneCppSDK_WINDOWS_MANIFEST}")
target_link_libraries(cpp-sdk-animator-contracts PRIVATE WorkphoneCpp::Core)
enable_testing()
add_test(NAME cpp-sdk-animator-contracts COMMAND cpp-sdk-animator-contracts)
get_target_property(_sdk_core_dll WorkphoneCpp::Core IMPORTED_LOCATION_RELWITHDEBINFO)
get_filename_component(_sdk_bin "${_sdk_core_dll}" DIRECTORY)
set_tests_properties(cpp-sdk-animator-contracts PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_sdk_bin}" TIMEOUT 60)

# Generate the effective installed target interface. The external audit invokes
# this exact compiler with /Zs, with one header per translation unit and no PCH.
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/header-check-$<CONFIG>.rsp"
    CONTENT "/nologo\n/std:c++17\n/MD\n/EHsc\n/GR\n/Zs\n/DWIN32\n/D_WINDOWS\n/DNDEBUG\n/D$<JOIN:$<TARGET_PROPERTY:WorkphoneCpp::Core,INTERFACE_COMPILE_DEFINITIONS>,\n/D>\n/I\"$<JOIN:$<TARGET_PROPERTY:WorkphoneCpp::Core,INTERFACE_INCLUDE_DIRECTORIES>,\"\n/I\">\"\n")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/header-check-compiler.txt" "${CMAKE_CXX_COMPILER}\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/header-check-includes-$<CONFIG>.txt"
    CONTENT "$<JOIN:$<TARGET_PROPERTY:WorkphoneCpp::Core,INTERFACE_INCLUDE_DIRECTORIES>,\n>\n")

