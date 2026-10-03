project(WPPyPyBind)

add_compile_definitions (_WP_STATIC_LIB_)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPyPyBind/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPyPyBind/*.cpp")

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
include_directories( ${WP_ROOT}/AdditionalLibraries/wildmagic/SDK/Include )
include_directories( ../../Include )

#if (WP_USE_PRECOMPILED_HEADERS)
#   wp_use_precompiled_header(WPPyPyBind "WPPyPyBind/WPPyPyBindPCH.hpp" "${CMAKE_SOURCE_DIR}/FBEngine/cpp/Source/WPPyPyBind/WPPyPyBindPCH.cpp")
#endif()

add_library(WPPyPyBind STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPPyPyBind PROPERTIES FOLDER Engine/Cpp/Scripting)

if (NOT _WP_STATIC_LIB_)
	target_compile_definitions(WPPyPyBind PUBLIC WP_EXPORTS)
endif()
