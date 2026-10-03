project(WorkphoneVehicle)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneVehicle/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneVehicle/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneVehicle/*.c")

add_library(WorkphoneVehicle STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WorkphoneVehicle PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneVehicle PUBLIC WorkphoneVehicle_EXPORTS)
endif()