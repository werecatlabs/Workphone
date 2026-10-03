# ---------------------------------------------------------------------------
#  AssetDatabaseEditor
#
#  C++17 port of the C# `Tools/csharp/AssetDatabaseTool` and the in-progress
#  `Tools/cpp/Editor/src/ui/AssetDatabaseWindow.cpp` work-in-progress.  The
#  editor re-uses the engine's renderer, ImGui based UI system, and database
#  interfaces to deliver a stand-alone tool for browsing the asset database.
# ---------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)
project(AssetDatabaseEditor C CXX)

# The C++ tool CMake file is included as a subdirectory of Tools/cpp/CMakeLists.txt
# which is itself included from the top-level workphone project.  All of the
# engine-wide options (WP_STATIC_LIB, WP_BUILD_ENGINE, etc.) are therefore
# already defined.

if(NOT WP_BUILD_CPP)
    message(STATUS "AssetDatabaseEditor: WP_BUILD_CPP is OFF; skipping target.")
    return()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
#  Project paths
# ---------------------------------------------------------------------------
set(ADBEDITOR_ROOT       "${CMAKE_CURRENT_SOURCE_DIR}")
set(ADBEDITOR_SRC_DIR    "${ADBEDITOR_ROOT}/src")
set(ADBEDITOR_INC_DIR    "${ADBEDITOR_ROOT}/src")

# Resolve the location of the engine so we can re-use its libraries and
# include paths without copying anything.
if(NOT DEFINED WP_ROOT)
    message(FATAL_ERROR
        "AssetDatabaseEditor must be added through the top-level workphone "
        "CMakeLists.txt so that WP_ROOT is defined. Did you add it directly?")
endif()

set(ADBEDITOR_HEADER_REL_PATH "${ADBEDITOR_INC_DIR}")
set(ADBEDITOR_SOURCE_REL_PATH "${ADBEDITOR_SRC_DIR}")

# ---------------------------------------------------------------------------
#  Source / header discovery
# ---------------------------------------------------------------------------
file(GLOB HEADER_FILES CONFIGURE_DEPENDS
    "${ADBEDITOR_INC_DIR}/*.hpp"
    "${ADBEDITOR_INC_DIR}/*.h")

file(GLOB_RECURSE HEADER_FILES_UI "${ADBEDITOR_SRC_DIR}/ui/*.hpp")
list(APPEND HEADER_FILES ${HEADER_FILES_UI})

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS "${ADBEDITOR_SRC_DIR}/*.cpp")
file(GLOB_RECURSE SOURCE_FILES_UI        "${ADBEDITOR_SRC_DIR}/ui/*.cpp")
list(APPEND SOURCE_FILES ${SOURCE_FILES_UI})

if(WIN32)
    file(GLOB_RECURSE HEADER_FILES_WIN32 "${ADBEDITOR_SRC_DIR}/application/win32/*.hpp")
    file(GLOB_RECURSE SOURCE_FILES_WIN32 "${ADBEDITOR_SRC_DIR}/application/win32/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_WIN32})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_WIN32})
elseif(APPLE)
    file(GLOB_RECURSE HEADER_FILES_MACOS "${ADBEDITOR_SRC_DIR}/application/macOS/*.hpp")
    file(GLOB_RECURSE SOURCE_FILES_MACOS
        "${ADBEDITOR_SRC_DIR}/application/macOS/*.cpp"
        "${ADBEDITOR_SRC_DIR}/application/macOS/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_MACOS})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_MACOS})
elseif(UNIX)
    file(GLOB_RECURSE HEADER_FILES_LINUX "${ADBEDITOR_SRC_DIR}/application/linux/*.hpp")
    file(GLOB_RECURSE SOURCE_FILES_LINUX "${ADBEDITOR_SRC_DIR}/application/linux/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_LINUX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_LINUX})
endif()

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_dir "${_source}" DIRECTORY)
    string(REPLACE "${ADBEDITOR_HEADER_REL_PATH}" "" _group "${_source_dir}")
    string(REPLACE "\\" "/" _group "${_group}")
    source_group("Header Files\\${_group}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_dir "${_source}" DIRECTORY)
    string(REPLACE "${ADBEDITOR_SOURCE_REL_PATH}" "" _group "${_source_dir}")
    string(REPLACE "/" "\\" _group "${_group}")
    source_group("Source Files\\${_group}" FILES "${_source}")
endforeach()

# ---------------------------------------------------------------------------
#  Compile definitions shared with the rest of the Workphone tools
# ---------------------------------------------------------------------------
add_compile_definitions(BOOST_ALL_NO_LIB)
add_compile_definitions(__TBB_NO_IMPLICIT_LINKAGE=1)
add_compile_definitions(__TBBMALLOC_NO_IMPLICIT_LINKAGE=1)
add_compile_definitions(_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
add_compile_definitions(_UNICODE)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_compile_definitions(WIN32_LEAN_AND_MEAN)

if(WP_BUILD_WXWIDGETS)
    if(APPLE)
        add_compile_definitions(__WXMAC__ __WXOSX__ __WXOSX_COCOA__)
    elseif(UNIX)
        add_compile_definitions(__WXUNIX__ __WXGTK__)
    endif()
endif()

# ---------------------------------------------------------------------------
#  Include directories
# ---------------------------------------------------------------------------
# Workphone engine public headers
include_directories(${WP_ROOT}/Engine/cpp/Include)
include_directories(${WP_ROOT}/Tools/cpp/AssetDatabaseEditor/src)

# Public, dependency-free, headers exposed by the in-progress port
include_directories(${ADBEDITOR_INC_DIR})

# Third-party headers that ship with the engine
include_directories(${WP_ROOT}/Dependencies/boost)
include_directories(${WP_ROOT}/Dependencies/tinyxml/include)

if(WP_USE_ONETBB)
    include_directories(${WP_ROOT}/Dependencies/onetbb/include)
elseif(WP_USE_TBB)
    include_directories(${WP_ROOT}/Dependencies/tbb/include)
endif()

if(WP_BUILD_IMGUI)
    include_directories(${WP_ROOT}/AdditionalLibraries/imgui)
    include_directories(${WP_ROOT}/AdditionalLibraries/imgui/backends)
endif()

# ---------------------------------------------------------------------------
#  Create the executable target
# ---------------------------------------------------------------------------
if(APPLE)
    add_executable(AssetDatabaseEditor MACOSX_BUNDLE ${HEADER_FILES} ${SOURCE_FILES})
elseif(WIN32)
    add_executable(AssetDatabaseEditor ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(AssetDatabaseEditor ${HEADER_FILES} ${SOURCE_FILES})
endif()

# Several legacy engine targets still expose third-party dependencies as bare
# library names (for example libboost_thread.lib).  Make the engine's normal
# per-configuration output directory visible to this standalone executable.
if(WIN32)
    target_link_directories(AssetDatabaseEditor PRIVATE
        "${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>")
endif()

# ---------------------------------------------------------------------------
#  Library dependencies
#
#  The editor re-uses the engine's renderer (WPGraphics), ImGui-based UI
#  system (WPImGui) and the SQLite-backed database interfaces (WPSQLite).
# ---------------------------------------------------------------------------
if(WP_BUILD_ENGINE)
    # The engine's umbrella target
    # Link by target file rather than inheriting the legacy targets' PUBLIC
    # *_EXPORTS definitions, which are only valid while compiling the DLLs.
    target_link_libraries(AssetDatabaseEditor "$<TARGET_LINKER_FILE:Workphone>")

    # Database interfaces backed by SQLite
    target_link_libraries(AssetDatabaseEditor "$<TARGET_LINKER_FILE:WPSQLite>")

    # ImGui based UI (IUITreeCtrl / IUITabBar / IUIDataGrid / IUIToolbar / etc.)
    if(WP_BUILD_IMGUI)
        target_link_libraries(AssetDatabaseEditor "$<TARGET_LINKER_FILE:WPImGui>")
    endif()

    # Renderer interfaces and concrete implementations
    if(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(AssetDatabaseEditor
            "$<TARGET_LINKER_FILE:WPGraphics>" "$<TARGET_LINKER_FILE:WPGraphicsOgre>")
    elseif(WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(AssetDatabaseEditor
            "$<TARGET_LINKER_FILE:WPGraphics>" "$<TARGET_LINKER_FILE:WPGraphicsOgreNext>")
    elseif(WP_GRAPHICS_SYSTEM_CLAW)
        target_link_libraries(AssetDatabaseEditor "$<TARGET_LINKER_FILE:WPGraphics>")
    endif()

    # Imported assets
    if(WP_USE_ASSET_IMPORT)
        target_link_libraries(AssetDatabaseEditor "$<TARGET_LINKER_FILE:WPAssimp>")
    endif()

    # Build-order dependencies so the editor waits for the engine libraries
    add_dependencies(AssetDatabaseEditor Workphone)
    add_dependencies(AssetDatabaseEditor WPSQLite)
    if(WP_BUILD_IMGUI)
        add_dependencies(AssetDatabaseEditor WPImGui)
    endif()
    if(WP_USE_ASSET_IMPORT)
        add_dependencies(AssetDatabaseEditor WPAssimp)
    endif()
    if(WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(AssetDatabaseEditor WPGraphics WPGraphicsOgre)
    elseif(WP_GRAPHICS_SYSTEM_OGRENEXT)
        add_dependencies(AssetDatabaseEditor WPGraphics WPGraphicsOgreNext)
    elseif(WP_GRAPHICS_SYSTEM_CLAW)
        add_dependencies(AssetDatabaseEditor WPGraphics)
    endif()
endif()

# ---------------------------------------------------------------------------
#  Per-platform link libraries
# ---------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(AssetDatabaseEditor bcrypt)
    target_link_libraries(AssetDatabaseEditor opengl32)
    target_link_libraries(AssetDatabaseEditor winmm)

    if(WP_USE_TBB)
        if(WP_TBB_BUILD_STATIC)
            target_link_libraries(AssetDatabaseEditor tbb_static tbbmalloc_static)
        else()
            target_link_libraries(AssetDatabaseEditor tbb tbbmalloc)
        endif()
    endif()

    target_link_libraries(AssetDatabaseEditor ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(AssetDatabaseEditor ${CMAKE_DL_LIBS})
elseif(APPLE)
    target_link_libraries(AssetDatabaseEditor
        stdc++
        "-framework AppKit"
        "-framework Metal"
        "-framework MetalKit"
        "-framework OpenGL"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework CoreGraphics"
        "-framework CoreServices"
        "-framework Cocoa"
        "-framework Carbon"
        "-framework IOKit"
        "-framework GameController"
        objc)
    set_target_properties(AssetDatabaseEditor PROPERTIES
        LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif(UNIX)
    target_link_libraries(AssetDatabaseEditor png GL)
    target_link_libraries(AssetDatabaseEditor ${X11_LIBRARIES})
    target_link_libraries(AssetDatabaseEditor ${CMAKE_THREAD_LIBS_INIT})
    target_link_libraries(AssetDatabaseEditor ${CMAKE_DL_LIBS})
endif()

# ---------------------------------------------------------------------------
#  Visual Studio / Xcode organisation
# ---------------------------------------------------------------------------
set_target_properties(AssetDatabaseEditor PROPERTIES
    FOLDER "Tools/Cpp"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")

if(APPLE)
    set_target_properties(AssetDatabaseEditor PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${ADBEDITOR_ROOT}/src/application/macOS/Info-macOS.plist"
        MACOSX_BUNDLE TRUE)
endif()
