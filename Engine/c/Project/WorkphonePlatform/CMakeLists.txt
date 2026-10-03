project(WorkphonePlatform)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneCollision/ )
include_directories( ../../Include/WorkphonePhysics/ )
include_directories( ../../Include/WorkphonePlatform/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphonePlatform/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphonePlatform/*.c")

add_library(WorkphonePlatform STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(
    WorkphonePlatform
    PROPERTIES FOLDER Engine/C
               C_STANDARD 90
               C_STANDARD_REQUIRED ON
               C_EXTENSIONS OFF)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphonePlatform PUBLIC WorkphonePlatform_EXPORTS)
endif()
