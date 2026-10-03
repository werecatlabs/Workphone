project(WPPyPy)

add_compile_definitions (_WP_STATIC_LIB_)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPyPy/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPyPy/*.cpp")

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
#    wp_use_precompiled_header(WPProcedural "WPPyPy/WPPyPyPCH.hpp" "${CMAKE_SOURCE_DIR}/FBEngine/cpp/Source/WPPyPy/WPPyPyPCH.cpp")
#endif()

add_library(WPPyPy STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPPyPy PROPERTIES FOLDER Engine/Cpp/Scripting)

if (NOT _WP_STATIC_LIB_)
	target_compile_definitions(WPPyPy PUBLIC WP_EXPORTS)
endif()
