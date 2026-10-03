project(WorkphoneNetwork)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_D3D11_IMPLEMENTATION)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphoneNetwork/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneNetwork/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneNetwork/*.c")

add_library(WorkphoneNetwork STATIC ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(WorkphoneNetwork PUBLIC ws2_32)
endif()

set_target_properties (WorkphoneNetwork PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneNetwork PUBLIC WorkphoneNetwork_EXPORTS)
endif()
