project(WorkphonePlatformApple)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_D3D11_IMPLEMENTATION)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePlatformApple/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphonePlatformApple/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphonePlatformApple/*.c")

add_library(WorkphonePlatformApple STATIC ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(WorkphonePlatformApple PUBLIC d3d11 dxgi d3dcompiler dxguid)
endif()

set_target_properties (WorkphonePlatformApple PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphonePlatformApple PUBLIC WorkphonePlatformApple_EXPORTS)
endif()
