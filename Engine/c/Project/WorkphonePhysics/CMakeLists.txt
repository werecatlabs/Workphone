project(WorkphonePhysics)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneCollision/ )
include_directories( ../../Include/WorkphonePhysics/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphonePhysics/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphonePhysics/*.c")

add_library(WorkphonePhysics STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(WorkphonePhysics PUBLIC WorkphoneCollision)

set_target_properties (WorkphonePhysics PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphonePhysics PUBLIC WorkphonePhysics_EXPORTS)
endif()
