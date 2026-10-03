cmake_minimum_required(VERSION 3.13)
project(WPHoudini)




# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPHoudini/*.hpp" "../../Include/WPHoudini/*.hpppp" "../../Include/WPHoudini/*.inl")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPHoudini/*.cpp" "../../Source/WPHoudini/*.c")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPHoudini/../../Include/WPHoudini")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPHoudini/../../Source/WPHoudini")

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
include_directories( ../../Include/WPHoudini )
include_directories( ../../Source/WPHoudini )

if(WIN32)
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
elseif(APPLE)
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
else()
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Overlay/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Paging/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Bites/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/particle_universe/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/PlugIns/CgProgramManager/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/RenderSystems/Direct3D9/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/RenderSystems/Direct3D11/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/RenderSystems/GL/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/directx/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/PlugIns/STBICodec/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/RTShaderSystem/include )

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPHoudini "WPHoudini/WPHoudiniPCH.hpp" "${CMAKE_SOURCE_DIR}/FBEngine/cpp/Source/WPHoudini/WPHoudiniPCH.cpp")
endif()

add_library(WPHoudini STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPHoudini PROPERTIES FOLDER Engine/Cpp/Procedural)


if (NOT _WP_STATIC_LIB_)
	target_compile_definitions(WPHoudini PUBLIC WP_EXPORTS)
endif()


# Link against the Houdini libraries, and add required include directories and compile definitions.
#target_link_libraries( WPHoudini Houdini )
# Configure several common target properties, such as its output directory.
#houdini_configure_target( WPHoudini )