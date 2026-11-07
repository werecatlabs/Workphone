project(WPNetwork C CXX)
option(WP_NETWORK_GNS "Build the GNS engine adapter (requires the pinned transport package)" OFF)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPNetwork/*.h" "../../Include/WPNetwork/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPNetwork/*.cpp")

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ../../Include )
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneNetwork )

if (WP_STATIC_LIB)
    add_library(WPNetwork STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPNetwork SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

target_compile_definitions(WPNetwork PRIVATE WPNetwork_EXPORTS)
target_include_directories(WPNetwork PRIVATE "${WP_ROOT}/Dependencies/rapidjson/include")

target_link_libraries(WPNetwork PUBLIC Workphone WorkphoneNetwork)
if(WP_NETWORK_GNS)
    set(Protobuf_USE_STATIC_LIBS ON)
    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
    if(WP_NETWORK_GNS_VALIDATION_OUTPUT)
        # A trust fixture builds one configuration against a matching single-config
        # SDK. Let CMake generate the other configurations without changing the
        # engine generator's configuration list or rebuilding shared dependencies.
        list(APPEND CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Debug)
        list(APPEND CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Debug)
    endif()
    # The validation script installs a native Windows x64 DLL-CRT bundle. Make it
    # discoverable on subsequent CMake runs, including from the IDE.
    if(WIN32 AND MSVC AND CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64" AND
       NOT CMAKE_CROSSCOMPILING AND NOT WP_STATIC_LINK_CRT)
        set(_wp_network_install "${WP_ROOT}/.codex/network/installed")
        set(_wp_network_prefix "${_wp_network_install}/x64-windows-static-md")
        set(_wp_network_protobuf "${_wp_network_prefix}/share/protobuf")
        set(_wp_network_protoc "${_wp_network_install}/x64-windows/tools/protobuf/protoc.exe")
        if(EXISTS "${_wp_network_protobuf}/protobuf-config.cmake" AND
           EXISTS "${_wp_network_prefix}/share/gamenetworkingsockets/GameNetworkingSocketsConfig.cmake")
            list(APPEND CMAKE_PREFIX_PATH "${_wp_network_prefix}")
            if(NOT CMAKE_TOOLCHAIN_FILE AND NOT Protobuf_PROTOC_EXECUTABLE AND
               (NOT Protobuf_DIR OR Protobuf_DIR STREQUAL _wp_network_protobuf) AND
               NOT DEFINED ENV{Protobuf_DIR} AND NOT Protobuf_ROOT AND NOT DEFINED ENV{Protobuf_ROOT} AND
               EXISTS "${_wp_network_protoc}")
                set(Protobuf_PROTOC_EXECUTABLE "${_wp_network_protoc}")
            endif()
        endif()
    endif()

    find_package(Protobuf CONFIG QUIET)
    if(NOT Protobuf_FOUND)
        message(FATAL_ERROR
            "WP_NETWORK_GNS requires a Protobuf CMake config and matching protoc. "
            "On Windows x64 run Scripts/Validate-WPNetworkTransport.ps1 to install the pinned bundle, "
            "or specify your package with CMAKE_PREFIX_PATH or Protobuf_DIR. "
            "Configure with -DWP_NETWORK_GNS=OFF for the native UDP backend.")
    endif()
    # GNS calls find_dependency(Protobuf) without CONFIG. Keep it on the same
    # imported package instead of reopening FindProtobuf's module cache.
    set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
    if(GameNetworkingSockets_DIR)
        # A missing explicitly selected operator package must fail instead
        # of falling back to a stock SDK with a different trust root.
        set(_wp_network_selected_gns "${GameNetworkingSockets_DIR}")
        find_package(GameNetworkingSockets CONFIG QUIET
            PATHS "${_wp_network_selected_gns}" NO_DEFAULT_PATH)
        if(NOT GameNetworkingSockets_FOUND)
            # CMake can replace an invalid path with -NOTFOUND. Keep the explicit
            # selection so the next configure still fails until it is corrected.
            set(GameNetworkingSockets_DIR "${_wp_network_selected_gns}" CACHE PATH
                "Directory containing the selected GameNetworkingSockets package config" FORCE)
            message(FATAL_ERROR
                "Could not load the selected GameNetworkingSockets package at '${_wp_network_selected_gns}'. "
                "Correct GameNetworkingSockets_DIR or clear it to allow automatic discovery. "
                "${GameNetworkingSockets_NOT_FOUND_MESSAGE}")
        endif()
    else()
        find_package(GameNetworkingSockets CONFIG REQUIRED)
    endif()
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    target_link_libraries(WPNetwork PRIVATE GameNetworkingSockets::static OpenSSL::Crypto)
    target_compile_definitions(WPNetwork PUBLIC WP_NETWORK_GNS=1)
    if(WP_NETWORK_GNS_VALIDATION_OUTPUT)
        set_target_properties(WPNetwork PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${WP_NETWORK_GNS_VALIDATION_OUTPUT}"
            LIBRARY_OUTPUT_DIRECTORY "${WP_NETWORK_GNS_VALIDATION_OUTPUT}"
            ARCHIVE_OUTPUT_DIRECTORY "${WP_NETWORK_GNS_VALIDATION_OUTPUT}")
    endif()
else()
    target_compile_definitions(WPNetwork PUBLIC WP_NETWORK_GNS=0)
endif()
# Workphone still exports some third-party dependencies as bare library names.
# Publish their configuration-specific search path to network consumers too.
if(WIN32)
    target_link_directories(WPNetwork PUBLIC
        "${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>")
else()
    target_link_directories(WPNetwork PUBLIC $<TARGET_PROPERTY:Workphone,LINK_DIRECTORIES>)
endif()

set_target_properties (WPNetwork PROPERTIES FOLDER Engine/Cpp/Network)
