if (APPLE)
    project(WPGraphics C CXX OBJC)
else()
    project(WPGraphics C CXX)
endif()

if (WP_BUILD_DEPENDENCIES)
# Build the bundled codec so fixes to its plugin lifetime are used even when
# the remaining third-party dependencies are supplied as prebuilt libraries.
if (NOT TARGET FreeImage)
    add_subdirectory("${WP_ROOT}/Dependencies/FreeImage"
                     "${CMAKE_BINARY_DIR}/Dependencies/FreeImage")
    set_target_properties(FreeImage PROPERTIES FOLDER Dependencies/FreeImage)
endif()
endif()

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPGraphics/*.h" "../../Include/WPGraphics/*.hpp" "../../Include/WPGraphics/*.inl")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPGraphics/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPGraphics/../../Include/WPGraphics")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPGraphics/../../Source/WPGraphics")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Header Files\\${_group_file_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_SOURCE_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Source Files\\${_group_file_path}" FILES "${_source}")
endforeach()

include_directories( ${WP_ROOT}/Dependencies/boost )

include_directories( ${WP_ROOT}/Dependencies/bgfx/bx/include/compat/msvc )
include_directories( ${WP_ROOT}/Dependencies/bgfx )
include_directories( ${WP_ROOT}/Dependencies/bgfx/bx/include )
include_directories( ${WP_ROOT}/Dependencies/bgfx/bx/3rdparty )
include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/include )
include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/examples/common )
include_directories( ${WP_ROOT}/Dependencies/bgfx/bimg/include )
include_directories( ${WP_ROOT}/Dependencies/imgui )
include_directories( ${WP_ROOT}/Dependencies/FreeImage/Source )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinybind/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )

include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneCore )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32 )

include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphics )
include_directories( ${WP_ROOT}/Engine/cpp/Include )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPGraphics ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPGraphics "WPGraphics/WPClawHammerPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPGraphics/WPClawHammerPCH.cpp")
endif()

if (WP_STATIC_LIB)
    add_library(WPGraphics STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPGraphics SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPGraphics Workphone)
        target_link_libraries(WPGraphics WorkphoneCore.lib)
        target_link_libraries(WPGraphics WorkphonePlatformWin32)
        target_link_libraries(WPGraphics d3d11.lib)
        target_link_libraries(WPGraphics dxgi.lib)
        target_link_libraries(WPGraphics d3dcompiler.lib)
        target_link_libraries(WPGraphics dxguid.lib)
        target_link_libraries(WPGraphics FreeImage)
    endif()
endif()

# Use the CMake target rather than a platform-specific library filename.  This
# also preserves the dependency for static and non-Windows builds.
target_link_libraries(WPGraphics WorkphoneGraphics)

target_compile_definitions(WPGraphics PRIVATE FREEIMAGE_LIB)
if (TARGET FreeImage)
    add_dependencies(WPGraphics FreeImage)
endif()

if (WP_BUILD_IMGUI)
    # ClawImguiManager drives Dear ImGui through the C89 renderer, so it needs
    # the Dear ImGui core (imgui) and the WPImGui wrapper plugin, mirroring
    # WPGraphicsOgreNext.
    if (WIN32 AND NOT WP_STATIC_LIB)
        target_link_libraries(WPGraphics WPImGui)
        target_link_libraries(WPGraphics imgui)
    endif()

    if (TARGET WPImGui)
        add_dependencies(WPGraphics WPImGui)
    endif()
endif()

set_target_properties (WPGraphics PROPERTIES FOLDER Engine/Cpp/Render)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPGraphics PUBLIC WPGraphics_EXPORTS)
endif()

