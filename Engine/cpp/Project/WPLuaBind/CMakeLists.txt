project(WPLuaBind)

add_compile_definitions (BOOST_BIND_GLOBAL_PLACEHOLDERS)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPLuaBind/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPLuaBind/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPLuaBind/../../Include/WPLuaBind")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPLuaBind/../../Source/WPLuaBind")

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
include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc/luabind )

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
include_directories( ../../Include/WPLua )
include_directories( ../../Include/WPLua/extern )
include_directories( ../../Include/WPLua/extern/lua/inc )
include_directories( ../../Include/WPLua/extern/luabind/inc )
include_directories( ../../Include/WPLua/extern/lua_cjson )

if (WP_USE_PRECOMPILED_HEADERS)
   wp_use_precompiled_header(WPLuaBind "WPLuaBind/WPLuaBindPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPLuaBind/WPLuaBindPCH.cpp")
endif()

add_library(WPLuaBind STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPLuaBind PROPERTIES FOLDER Engine/Cpp/Scripting)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPLuaBind PUBLIC WP_EXPORTS)
endif()

if (WP_BUILD_DEPENDENCIES)
	if (NOT WP_STATIC_LIB)
		add_dependencies(WPLua lua)
		add_dependencies(WPLua luabind)
	endif()
endif()