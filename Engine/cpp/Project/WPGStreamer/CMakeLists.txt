cmake_minimum_required(VERSION 3.13)
project(WPGStreamer)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPGStreamer/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPGStreamer/*.cpp")

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/tbb/include )
include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinybind/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Overlay/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Terrain/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Paging/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Bites/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/particle_universe/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/PlugIns/CgProgramManager/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/RenderSystems/GL/include )
include_directories( ../../Include )
include_directories( ../../Include/WPGStreamer )

add_library(WPGStreamer STATIC ${HEADER_FILES} ${SOURCE_FILES})
