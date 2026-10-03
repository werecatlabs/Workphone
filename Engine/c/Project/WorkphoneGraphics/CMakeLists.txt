project(WorkphoneGraphics)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePlatformWin32/ )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneGraphics/*.h" )
file(GLOB_RECURSE SOURCE_FILES "../../Source/WorkphoneGraphics/*.c")

add_library(WorkphoneGraphics STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(WorkphoneGraphics PUBLIC WorkphoneCore)

set_target_properties(
    WorkphoneGraphics
    PROPERTIES FOLDER Engine/C
               C_STANDARD 90
               C_STANDARD_REQUIRED ON
               C_EXTENSIONS OFF)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneGraphics PUBLIC WorkphoneGraphics_EXPORTS)
endif()
