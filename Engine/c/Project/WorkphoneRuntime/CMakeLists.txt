project(WorkphoneRuntime)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)
add_compile_definitions (WORKPHONE_D3D11_IMPLEMENTATION)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePlatformWin32/ )
include_directories( ../../Include/WorkphoneRuntime/ )
include_directories( ../../Include/WorkphoneScript/ )

if (WIN32)
    link_directories(WorkphoneRuntime ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
endif()

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneRuntime/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneRuntime/*.c")

add_executable(WorkphoneRuntime ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(WorkphoneRuntime WorkphoneCore.lib)
    target_link_libraries(WorkphoneRuntime WorkphoneGraphics.lib)
    target_link_libraries(WorkphoneRuntime WorkphonePlatformWin32.lib)
    target_link_libraries(WorkphoneRuntime WorkphoneScript.lib)
    target_link_libraries(WorkphoneRuntime lua.lib)
    target_link_libraries(WorkphoneRuntime luabind.lib)
    target_link_libraries(WorkphoneRuntime user32.lib)
    target_link_libraries(WorkphoneRuntime dxguid.lib)
    target_link_libraries(WorkphoneRuntime d3d11.lib)
endif()

set_target_properties (WorkphoneRuntime PROPERTIES
    FOLDER Engine/C
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneRuntime PUBLIC WorkphoneRuntime_EXPORTS)
endif()

add_dependencies(WorkphoneRuntime WorkphoneCore)
add_dependencies(WorkphoneRuntime WorkphoneGraphics)
add_dependencies(WorkphoneRuntime WorkphonePlatformWin32)
add_dependencies(WorkphoneRuntime WorkphoneScript)
