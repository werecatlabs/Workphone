project(WorkphoneProcedural)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneProcedural/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneProcedural/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneProcedural/*.c")

add_library(WorkphoneProcedural STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WorkphoneProcedural PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneProcedural PUBLIC WorkphoneProcedural_EXPORTS)
endif()