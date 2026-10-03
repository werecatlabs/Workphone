project(WPTheoraVideo)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPTheoraVideo/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPTheoraVideo/*.cpp")

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinybind/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/Dependencies/ffmpeg )
include_directories( ../../Include )
include_directories( ../../Include/WPTheoraVideo )


add_library(WPTheoraVideo STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPTheoraVideo PROPERTIES FOLDER Engine/Cpp/Render)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPTheoraVideo PUBLIC WP_EXPORTS)
endif()

