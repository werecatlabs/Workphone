cmake_minimum_required(VERSION 3.13)
project(WPFFMpeg)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPFFMpeg/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPFFMpeg/*.cpp")

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
include_directories( ../../Include/WPFFMpeg )


add_library(WPFFMpeg STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPFFMpeg PROPERTIES FOLDER Engine/Cpp/Render)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPFFMpeg PUBLIC WP_EXPORTS)
endif()

