set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(cpp_defines)
include(cpp_compiler_options)

if (WIN32)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
elseif (ANDROID)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE})
elseif (APPLE_IOS)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/iOS/${WP_TARGET_ARCH}/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/iOS/${WP_TARGET_ARCH}/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/iOS/${WP_TARGET_ARCH}/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/macOS/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/macOS/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/macOS/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${WP_ROOT}/libs/linux/${CMAKE_BUILD_TYPE})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${WP_ROOT}/libs/linux/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${WP_ROOT}/bin/linux/${CMAKE_BUILD_TYPE})
endif (WIN32)

if (APPLE)
	#add_compile_definitions(__MATH_H__)
endif ()

# Set the BOOST_INSTALL_LAYOUT
set(BOOST_INSTALL_LAYOUT "SYSTEM" CACHE STRING "Boost install layout")
	
# Only compile the Boost libraries used by the engine, tools and tests.
# Boost's CMake build adds their transitive dependencies automatically.
option(WP_BUILD_ALL_BOOST_LIBRARIES "Build every bundled Boost library" OFF)
if(NOT WP_BUILD_ALL_BOOST_LIBRARIES AND NOT BOOST_INCLUDE_LIBRARIES)
    set(BOOST_INCLUDE_LIBRARIES
        chrono context date_time filesystem json system thread timer)
    if(WP_BUILD_TESTS OR WP_EDITOR_TESTS)
        list(APPEND BOOST_INCLUDE_LIBRARIES test)
    endif()
endif()
add_subdirectory(boost)
add_subdirectory(cppsqlite)
add_subdirectory(FreeImage)
add_subdirectory(freetype)
add_subdirectory(lua)
add_subdirectory(lua_cjson)
add_subdirectory(luabind)
add_subdirectory(imgui)
add_subdirectory(minizip)
add_subdirectory(nativefiledialog)
add_subdirectory(sqlite)
add_subdirectory(tinyxml)
add_subdirectory(tinyusdz)
add_subdirectory(zlib)
add_subdirectory(zzip)

foreach(_wp_boost_target IN ITEMS
        boost_assert
        boost_atomic
        boost_chrono
        boost_container
        boost_context
        boost_contract
        boost_core
        boost_coroutine
        boost_date_time
        boost_exception
        boost_fiber
        boost_fiber_numa
        boost_filesystem
        boost_graph
        boost_iostreams
        boost_json
        boost_locale
        boost_log
        boost_nowide
        boost_prg_exec_monitor
        boost_program_options
        boost_log_setup
        boost_random
        boost_serialization
        boost_stacktrace_basic
        boost_stacktrace_noop
        boost_stacktrace_windbg
        boost_stacktrace_windbg_cached
        boost_system
        boost_test_exec_monitor
        boost_thread
        boost_timer
        boost_type_erasure
        boost_unit_test_framework
        boost_url
        boost_variant2
        boost_wave
        boost_wserialization)
    if(TARGET ${_wp_boost_target})
        set_target_properties(${_wp_boost_target} PROPERTIES FOLDER Dependencies/boost)
    endif()
endforeach()

set_target_properties (FreeImage PROPERTIES FOLDER Dependencies/FreeImage)
set_target_properties (freetype PROPERTIES FOLDER Dependencies/freetype)

set_target_properties (imgui PROPERTIES FOLDER Dependencies/imgui)

set_target_properties (lua PROPERTIES FOLDER Dependencies/lua)
set_target_properties (luavm PROPERTIES FOLDER Dependencies/lua)
set_target_properties (lua_cjson PROPERTIES FOLDER Dependencies/lua)
set_target_properties (luabind PROPERTIES FOLDER Dependencies/lua)

set_target_properties (minizip PROPERTIES FOLDER Dependencies/minizip)
set_target_properties (nativefiledialog PROPERTIES FOLDER Dependencies/nativefiledialog)
set_target_properties (tinyxml PROPERTIES FOLDER Dependencies/tinyxml)
set_target_properties (tinyusdz_static PROPERTIES FOLDER Dependencies/tinyusdz)

set_target_properties (zlib PROPERTIES FOLDER Dependencies/zlib)
set_target_properties (zzip PROPERTIES FOLDER Dependencies/zzip)

set_target_properties (cppsqlite PROPERTIES FOLDER Dependencies/cppsqlite)
set_target_properties (sqlite PROPERTIES FOLDER Dependencies/sqlite)
