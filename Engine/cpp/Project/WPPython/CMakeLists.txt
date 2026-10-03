project(WPPython)

add_compile_definitions (_WP_STATIC_LIB_)
add_compile_definitions (BOOST_PYTHON_STATIC_LIB)
add_compile_definitions (__TBB_STATISTICS=0)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPython/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPython/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPython/../../Include/WPPython")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPython/../../Source/WPPython")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "\\" "/" _group_file_path "${_group_file_path}")
    source_group("Header Files\\${_group_file_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_SOURCE_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Source Files\\${_group_file_path}" FILES "${_source}")
endforeach()

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
include_directories( ${WP_ROOT}/AdditionalLibraries/python/PC )
include_directories( ${WP_ROOT}/AdditionalLibraries/python/Include )
#include_directories( ${WP_ROOT}/AdditionalLibraries/pypy/pypy39/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/wildmagic/SDK/Include )
include_directories( ../../Include )

#if (WP_USE_PRECOMPILED_HEADERS)
#    wp_use_precompiled_header(WPPython "WPPython/WPPythonPCH.hpp" "${CMAKE_SOURCE_DIR}/FBEngine/cpp/Source/WPPython/WPPythonPCH.cpp")
#endif()

add_library(WPPython STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPPython PROPERTIES FOLDER Engine/Cpp/Scripting)

if (NOT _WP_STATIC_LIB_)
	target_compile_definitions(WPPython PUBLIC WP_EXPORTS)
endif()
