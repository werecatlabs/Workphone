project(WPAi)

add_compile_definitions (_WP_STATIC_LIB_)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPAi/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPAi/*.cpp")

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
include_directories( ../../Include )
include_directories( ../../Include/WPAi/Extern )

add_library(WPAi STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPAi PROPERTIES FOLDER Engine/Cpp/Core)
