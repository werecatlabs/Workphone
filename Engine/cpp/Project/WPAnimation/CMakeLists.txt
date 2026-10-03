cmake_minimum_required(VERSION 3.13)
project(WPAnimation)

add_compile_definitions (_WP_STATIC_LIB_)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPAnimation/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPAnimation/*.cpp")

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib )
include_directories( ${WP_ROOT}/AdditionalLibraries/zziplib )
include_directories( ../../Include )

add_library(WPAnimation STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPAnimation PROPERTIES FOLDER Engine/Cpp/Render)
