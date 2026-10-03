project(WorkphoneCore C)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)
add_compile_definitions (WORKPHONE_STRTOD_NEEDED)
add_compile_definitions (STB_RECT_PACK_IMPLEMENTATION)
add_compile_definitions (STB_TRUETYPE_IMPLEMENTATION)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePhysics/ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneCore/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneCore/*.c")

add_library(WorkphoneCore STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WorkphoneCore PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneCore PUBLIC WorkphoneCore_EXPORTS)
endif()