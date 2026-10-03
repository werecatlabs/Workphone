cmake_minimum_required(VERSION 3.13)
project(WPFMODSound)

set(CMAKE_CXX_STANDARD 17)
add_compile_definitions (WP_OGRE_USE_PARTICLE_UNIVERSE=0)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPFMODSound/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPFMODSound/*.cpp")

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/tbb/include )
include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinybind/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/fmod/studio/api/studio/inc )
include_directories( ${WP_ROOT}/AdditionalLibraries/fmod/studio/api/lowlevel/inc )
include_directories( ../../Include )
include_directories( ../../Include/WPFMODSound )


add_library(WPFMODSound STATIC ${HEADER_FILES} ${SOURCE_FILES})
