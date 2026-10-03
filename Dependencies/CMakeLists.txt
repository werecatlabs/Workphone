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

set_target_properties (boost_assert PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_atomic PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_chrono PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_container PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_context PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_contract PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_core PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_coroutine PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_date_time PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_exception PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_fiber PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_fiber_numa PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_filesystem PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_graph PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_iostreams PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_json PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_locale PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_log PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_nowide PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_prg_exec_monitor PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_program_options PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_log_setup PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_random PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_serialization PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_stacktrace_basic PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_stacktrace_noop PROPERTIES FOLDER Dependencies/boost)

if (WIN32)
	set_target_properties (boost_stacktrace_windbg PROPERTIES FOLDER Dependencies/boost)
	set_target_properties (boost_stacktrace_windbg_cached PROPERTIES FOLDER Dependencies/boost)
endif()

set_target_properties (boost_system PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_test_exec_monitor PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_thread PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_timer PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_type_erasure PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_unit_test_framework PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_url PROPERTIES FOLDER Dependencies/boost)

set_target_properties (boost_variant2 PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_wave PROPERTIES FOLDER Dependencies/boost)
set_target_properties (boost_wserialization PROPERTIES FOLDER Dependencies/boost)

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
