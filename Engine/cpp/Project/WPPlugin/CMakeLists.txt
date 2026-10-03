project(WPPlugin)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPlugin/*.hpp" "../../Include/WPPlugin/*.hpppp" "../../Include/WPPlugin/*.inl")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPlugin/*.cpp" "../../Source/WPPlugin/*.c")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPlugin/../../Include/WPPlugin")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPlugin/../../Source/WPPlugin")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
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
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/ois/includes )
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib )
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib/src )
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib/src/contrib/minizip )
include_directories( ${WP_ROOT}/AdditionalLibraries/zziplib )
include_directories( ../../Include )
include_directories( ../../Include/extern )

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPPlugin "WPPlugin/WPPluginPCH.hpp" "${CMAKE_SOURCE_DIR}/FBEngine/cpp/Source/WPPlugin/WPPluginPCH.cpp")
endif()

add_library(WPPlugin STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPPlugin PROPERTIES FOLDER Engine/Cpp/Core)

if (NOT _WP_STATIC_LIB_)
	target_compile_definitions(WPPlugin PUBLIC WP_EXPORTS)
endif()
