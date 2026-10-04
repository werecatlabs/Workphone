project(WPSQLite)

option(WP_BUILD_SQLITECODEC "Build sqlite codec" ON)

if (WP_BUILD_SQLITECODEC)
	add_compile_definitions (SQLITE_HAS_CODEC)
endif()

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPSQLite/*.h" "../../Include/WPSQLite/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPSQLite/extern/*.c" "../../Source/WPSQLite/*.c" "../../Source/WPSQLite/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPSQLite/../../Include/WPSQLite")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPSQLite/../../Source/WPSQLite")

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

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib )
include_directories( ${WP_ROOT}/AdditionalLibraries/zziplib )
#include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include )
include_directories( ../../Include )
include_directories(../../Include/WPSQLite/extern)

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPSQLite ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPSQLite STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPSQLite SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPSQLite Workphone)

        if (WP_USE_ONETBB)
            target_link_libraries(WPSQLite debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPSQLite debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPSQLite debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(WPSQLite tbb_static.lib)
                target_link_libraries(WPSQLite tbbmalloc_static.lib)
            else()
                target_link_libraries(WPSQLite tbb.lib)
			    target_link_libraries(WPSQLite tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPSQLite tinyxml.lib)
    elseif (APPLE)
        target_link_libraries(WPSQLite libWorkphone.a)
    elseif (UNIX)
        target_link_libraries(WPSQLite -Wl,--start-group Workphone.a 
                -Wl,--end-group )

        target_link_libraries(WPSQLite ${X11_LIBRARIES})
        target_link_libraries(WPSQLite ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPSQLite ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPSQLite PROPERTIES FOLDER Engine/Cpp/Database)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPSQLite PUBLIC WPSQLite_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPSQLite Workphone)
endif()
