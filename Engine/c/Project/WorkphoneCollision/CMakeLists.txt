project(WorkphoneCollision C)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneCollision/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePhysics/ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneCollision/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneCollision/*.c")

add_library(WorkphoneCollision STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WorkphoneCollision PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneCollision PUBLIC WorkphoneCore_EXPORTS)
endif()