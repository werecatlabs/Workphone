project(WPRakNet C CXX)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPRakNet/*.h" "../../Include/WPRakNet/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPRakNet/*.cpp")

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
include_directories( ${WP_ROOT}/Dependencies/raknet/Source )
include_directories( ../../Include )
include_directories( ../../Include/WPRakNet/Extern )

add_library(WPRakNet STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPRakNet PROPERTIES FOLDER Engine/Cpp/Network)
