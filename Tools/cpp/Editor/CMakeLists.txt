if (APPLE)
    set(DEVELOPMENT_PROJECT_NAME "project")                     # <== Set to your project name, e.g. project.xcodeproj
    set(DEVELOPMENT_TEAM_ID "AAAAAAAAAA")                       # <== Set to your team ID from Apple
    set(APP_NAME "YOURAPP")                                     # <== Set To your app's name
    set(APP_BUNDLE_IDENTIFIER "com.company.app")                # <== Set to your app's bundle identifier
    set(FRAMEWORK_NAME "FooBar")                                # <== Set to your framework's name
    set(FRAMEWORK_BUNDLE_IDENTIFIER "com.company.framework")    # <== Set to your framework's bundle identifier (cannot be the same as app bundle identifier)
    set(TEST_NAME "Tests")                                      # <== Set to your test's name
    set(TEST_BUNDLE_IDENTIFIER "com.company.tests")             # <== Set to your tests's bundle ID
    set(CODE_SIGN_IDENTITY "iPhone Developer")                  # <== Set to your preferred code sign identity, to see list:
    # /usr/bin/env xcrun security find-identity -v -p codesigning
    set(DEPLOYMENT_TARGET 8.0)                                  # <== Set your deployment target version of iOS
    set(DEVICE_FAMILY "1")                                      # <== Set to "1" to target iPhone, set to "2" to target iPad, set to "1,2" to target both
    set(LOGIC_ONLY_TESTS 0)                                     # <== Set to 1 if you do not want tests to be hosted by the application, speeds up pure logic tests but you can not run them on real devices
    project(Editor)

    message(STATUS XCTestFound:${XCTest_FOUND})

    set(PRODUCT_NAME ${APP_NAME})
    set(EXECUTABLE_NAME ${APP_NAME})
    set(MACOSX_BUNDLE_EXECUTABLE_NAME ${APP_NAME})
    set(MACOSX_BUNDLE_INFO_STRING ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_BUNDLE_NAME ${APP_BUNDLE_IDENTIFIER})
    set(MACOSX_BUNDLE_ICON_FILE "")
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "1.0")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "1.0")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright YOU")
    set(MACOSX_DEPLOYMENT_TARGET ${DEPLOYMENT_TARGET})
else ()
    project(Editor)
endif()

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

if (WP_EDITOR_TESTS)
    add_compile_definitions(
        $<$<CONFIG:Debug>:WP_EDITOR_TESTS=1>
        $<$<CONFIG:Release>:WP_EDITOR_TESTS=0>
        $<$<CONFIG:MinSizeRel>:WP_EDITOR_TESTS=0>
        $<$<CONFIG:RelWithDebInfo>:WP_EDITOR_TESTS=0>
        )
else ()
    add_compile_definitions (WP_EDITOR_TESTS=0)
endif()

add_compile_definitions( BOOST_ALL_NO_LIB )
add_compile_definitions( __TBB_NO_IMPLICIT_LINKAGE=1 )
add_compile_definitions( __TBBMALLOC_NO_IMPLICIT_LINKAGE=1 )
add_compile_definitions (_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
add_compile_definitions (_UNICODE)

if (WP_BUILD_WXWIDGETS)
    if (APPLE)
        add_compile_definitions(__WXMAC__)
        add_compile_definitions(__WXOSX__)
        add_compile_definitions(__WXOSX_COCOA__)
    elseif (UNIX)
        add_compile_definitions(__WXUNIX__)
        add_compile_definitions(__WXGTK__)
    endif()
endif()

# define header and source files for the library
file(GLOB HEADER_FILES "src/*.hpp" "src/*.hpp")
file(GLOB SOURCE_FILES "src/*.cpp")
set(WP_HEADER_REL_PATH "${WP_ROOT}/Tools/cpp/Editor/src")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Tools/cpp/Editor/src")

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/ai/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/ai/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/commands/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/commands/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/core/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/core/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/editor/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/editor/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/jobs/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/jobs/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/messages/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/messages/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/script/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/script/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/tests/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/tests/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

file(GLOB_RECURSE HEADER_FILES_EDITOR "src/ui/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_EDITOR "src/ui/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_EDITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_EDITOR})

if (WIN32)
    file(GLOB HEADER_FILES_INCLUDE_WIN32 "${WP_ROOT}/Tools/cpp/Editor/src/application/win32/*.hpp")
    file(GLOB SOURCE_FILES_INCLUDE_WIN32 "${WP_ROOT}/Tools/cpp/Editor/src/application/win32/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_INCLUDE_WIN32})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_INCLUDE_WIN32})
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_path(COCOA_INCLUDE_DIR Cocoa/Cocoa.h)
    find_library(CARBON_LIBRARY Carbon)

    #add_definitions(-x objective-c++)

    file(GLOB HEADER_FILES_INCLUDE_OSX "${WP_ROOT}/Tools/cpp/Editor/src/application/apple/*.hpp")
    file(GLOB SOURCE_FILES_INCLUDE_OSX "${WP_ROOT}/Tools/cpp/Editor/src/application/apple/*.cpp" "${WP_ROOT}/Tools/cpp/Editor/src/application/apple/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_INCLUDE_OSX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_INCLUDE_OSX})
elseif(UNIX)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_UNIX "../../Include/WPGraphicsOgreNext/Window/OIS/*.hpp")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_UNIX "../../Source/WPGraphicsOgreNext/Window/OIS/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_UNIX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_UNIX})
endif()

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

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)

if (WP_BUILD_WXWIDGETS)
    #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets  )
    #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include )

    if (WIN32)
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc )
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc/wx )
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
    elseif (APPLE)
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1  )
    elseif (UNIX)
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1 )
        #include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1/wx )
    endif ()
endif()

include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories( ${WP_ROOT}/Tools/cpp/Editor/src )
include_directories( ${WP_ROOT}/AdditionalLibraries/entt/src )
include_directories( ${WP_ROOT}/AdditionalLibraries/imgui )
include_directories( ${WP_ROOT}/AdditionalLibraries/imgui/backends )

if (WIN32)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})

    if (WP_BUILD_PHYSX)
        if (WP_BUILD_PHYSICS_DEBUG)
            link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/physx/externals/nvToolsExt/1/lib/x64)
        endif()
    endif()

    if (WP_USE_ONETBB)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/oneTBB/)
    elseif (WP_USE_TBB)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    endif()

    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/python/)


    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)

    if (WP_BUILD_WILDMAGIC5)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})   
    endif()

    if (WP_USE_WPXSDK)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    endif()

    if (WP_BUILD_SDL2)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl2/${CMAKE_BUILD_TYPE})
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl_image/${CMAKE_BUILD_TYPE})
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/${CMAKE_BUILD_TYPE}/ )
        
        if (WP_PLATFORM_X64)
            link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()

    if (WP_BUILD_WXWIDGETS)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)
    endif()

    if (WP_USE_ASSET_IMPORT)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    endif()
elseif (APPLE)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})

    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})

    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ois/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tinybind)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(Editor ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(Editor "EditorPCH.hpp" "${WP_ROOT}/Tools/cpp/Editor/src/EditorPCH.cpp")
endif()

set(RESOURCES
        "${WP_ROOT}/Tools/cpp/Editor/src/application/apple/macOS/MainMenu.storyboard")


set(STORYBOARD
        "${WP_ROOT}/Tools/cpp/Editor/src/application/apple/macOS/MainMenu.storyboard")

# Add Lua files to the project
file(GLOB_RECURSE LUA_AI_SOURCES "${WP_ROOT}/Bin/Media/Scripts/Lua/Ai/*.lua") 
source_group("Lua/Ai" FILES ${LUA_AI_SOURCES})

file(GLOB_RECURSE LUA_EDITOR_SOURCES "${WP_ROOT}/Bin/Media/Scripts/Lua/Editor/*.lua") 
source_group("Lua/Editor" FILES ${LUA_EDITOR_SOURCES})

file(GLOB_RECURSE LUA_GAME_SOURCES "${WP_ROOT}/Bin/Media/Scripts/Lua/Game/*.lua") 
source_group("Lua/Game" FILES ${LUA_GAME_SOURCES})

if (WIN32)
    add_executable(Editor ${HEADER_FILES} ${SOURCE_FILES} ${LUA_AI_SOURCES} ${LUA_EDITOR_SOURCES} ${LUA_GAME_SOURCES})
    #add_executable(Editor WIN32 ${HEADER_FILES} ${SOURCE_FILES})
elseif(APPLE)
    #add_executable(Editor ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCES})
    add_executable(Editor MACOSX_BUNDLE ${HEADER_FILES} ${SOURCE_FILES} ${RESOURCES})
else()
    add_executable(Editor ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(Editor Workphone.lib)
        target_link_libraries(Editor WPAssimp.lib)
        target_link_libraries(Editor WPSQLite.lib)

        if (WP_BUILD_AUDIO)
            #target_link_libraries(Editor WPAudio.lib)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(Editor WPLua.lib)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(Editor WPPython.lib)
        endif()

        if (WP_USE_ONETBB)
            target_link_libraries(Editor debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(Editor debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(Editor debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(Editor tbb_static.lib)
                target_link_libraries(Editor tbbmalloc_static.lib)
            else()
                target_link_libraries(Editor tbb.lib)
			    target_link_libraries(Editor tbbmalloc.lib)
            endif()
        endif()

        if (WP_USE_WPXSDK)
            #target_link_libraries(Editor libWPxsdk.lib)
            target_link_libraries(Editor libWPxsdk-mt.lib)
            target_link_libraries(Editor libxml2-mt.lib)
            target_link_libraries(Editor tinyxml2.lib)
        endif()
    elseif (APPLE)
        target_link_libraries(Editor libWPCore.a)
        target_link_libraries(Editor libWPMesh.a)
        target_link_libraries(Editor libWPProcedural.a)
        target_link_libraries(Editor libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(Editor WPAudio.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(Editor WPLua.a)
            target_link_libraries(Editor WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(Editor WPPython.a)
            target_link_libraries(Editor WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(Editor SDL2main.a)
            target_link_libraries(Editor SDL2_image.a)
            target_link_libraries(Editor debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(Editor libWPImGui.a)
            target_link_libraries(Editor imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(Editor libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(Editor libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(Editor libWPOISInput.a)
            #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
        endif()

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(Editor libWPWxWidgets.a)

            target_link_libraries(Editor libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(Editor libwx_baseu-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(Editor libwxexpat-3.1.a)
            target_link_libraries(Editor libwx_baseu_xml-3.1.a)
            target_link_libraries(Editor libwx_baseu_net-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(Editor libwxtiff-3.1.a)
            target_link_libraries(Editor libwxjpeg-3.1.a)
            target_link_libraries(Editor libwxpng-3.1.a)

            target_link_libraries(Editor libiconv.a)
        endif()

        target_link_libraries(Editor libWPRenderUI.a)
        target_link_libraries(Editor libWPSQLite.a)
        target_link_libraries(Editor libWPGraphics.a)

        target_link_libraries(Editor libboost_thread.a)
        target_link_libraries(Editor libboost_date_time.a)
        target_link_libraries(Editor libboost_chrono.a)
        target_link_libraries(Editor libboost_context.a)
        target_link_libraries(Editor libboost_timer.a)
        target_link_libraries(Editor libboost_filesystem.a)
        target_link_libraries(Editor libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(Editor WPPhysx.a)

            target_link_libraries(Editor PhysX3.a)
            target_link_libraries(Editor PhysX3Extensions.a)
            target_link_libraries(Editor PhysX3Common.a)
            target_link_libraries(Editor PhysX3Vehicle.a)
            target_link_libraries(Editor PhysX3Cooking.a)
            target_link_libraries(Editor PhysXProfileSDK.a)
            target_link_libraries(Editor PxTask.a)
            target_link_libraries(Editor PhysXVisualDebuggerSDK.a)
            target_link_libraries(Editor PhysX3MetaData.a)
            target_link_libraries(Editor PhysX3Foundation.a)
            target_link_libraries(Editor PxSceneQuery.a)
            target_link_libraries(Editor PxSimulationController.a)
            target_link_libraries(Editor PhysX3LowLevel.a)
            target_link_libraries(Editor PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(Editor libtbb.a)
            target_link_libraries(Editor libtbbmalloc.a)
        endif()

        target_link_libraries(Editor minizip.a)
        target_link_libraries(Editor nativefiledialog.a)
        target_link_libraries(Editor pugixml.a)
        target_link_libraries(Editor rttr.a)
        target_link_libraries(Editor simplefilewatcher.a)
        target_link_libraries(Editor tinyusdz_static.a)
        target_link_libraries(Editor tinyxml.a)
        target_link_libraries(Editor yaml-cpp.a)
        target_link_libraries(Editor zlib.a)
        target_link_libraries(Editor zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(Editor libWm5Core.a)
            target_link_libraries(Editor libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(Editor debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(Editor FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(Editor freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(Editor WPGraphicsOgreNext.a)
            target_link_libraries(Editor harWPuzz.a)
            target_link_libraries(Editor harWPuzz-subset.a)
            target_link_libraries(Editor icucommon.a)
            target_link_libraries(Editor sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(Editor WPGraphicsOgre.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(Editor stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(Editor PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(Editor -Wl,--start-group WPCore.a WPVehiclePhysics.a WPImGui.a
            WPMath.a WPMesh.a WPOISInput.a WPData.a WPState.a libWPPhysx.a libWPSQLite.a
            WPGraphics.a WPGraphicsOgreNext.a WPAssimp.a WPRenderUI.a libWPCPlusPlus.a
            libboost_thread-gcc5-mt-s-x64-1_70.a
            libboost_date_time-gcc5-mt-s-x64-1_70.a
            libboost_chrono-gcc5-mt-s-x64-1_70.a
            libboost_timer-gcc5-mt-s-x64-1_70.a
            libboost_filesystem-gcc5-mt-s-x64-1_70.a
            debug libtbb_debug.a optimized libtbb.a
            debug libtbbmalloc_debug.a optimized libtbbmalloc.a
            libWm5Core.a libWm5Mathematics.a
            libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
            libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
            -Wl,--end-group )

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(Editor WPGraphicsOgreNext.a)
            target_link_libraries(Editor freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(Editor libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(Editor png)
        target_link_libraries(Editor Xaw)
        target_link_libraries(Editor Xrandr)
        target_link_libraries(Editor Xt)
        target_link_libraries(Editor xcb)
        target_link_libraries(Editor GL)
        target_link_libraries(Editor ${X11_LIBRARIES})
        target_link_libraries(Editor ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(Editor ${CMAKE_DL_LIBS})
    endif ()
else()
    if (WIN32)
        target_link_libraries(Editor WPAssimp.lib)
        target_link_libraries(Editor Workphone.lib)
        target_link_libraries(Editor WPGraphics.lib)
        #target_link_libraries(Editor WPMesh.lib)
        target_link_libraries(Editor WPRenderUI.lib)
        target_link_libraries(Editor WPSQLite.lib)

        if (WP_BUILD_AUDIO)
            target_link_libraries(Editor WPAudio.lib)
            target_link_libraries(Editor winmm.lib)
            target_link_libraries(Editor xaudio2.lib)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(Editor WPLua.lib)
            target_link_libraries(Editor WPLuaBind.lib)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(Editor WPPython.lib)
            target_link_libraries(Editor WPPythonBind.lib)
        endif()

        if (WP_USE_ONETBB)
            target_link_libraries(Editor debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(Editor debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(Editor debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(Editor tbb_static.lib)
                target_link_libraries(Editor tbbmalloc_static.lib)
            else()
                target_link_libraries(Editor tbb.lib)
			    target_link_libraries(Editor tbbmalloc.lib)
            endif()
        endif()

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(Editor WPWxWidgets.lib)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(Editor SDL2main.lib)
            target_link_libraries(Editor SDL2_image.lib)
            target_link_libraries(Editor debug SDL2d.lib optimized SDL2.lib)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(Editor WPImGui.lib)
            target_link_libraries(Editor imgui.lib)
        endif()

        if (WP_USE_WPXSDK)
            #target_link_libraries(Editor libWPxsdk.lib)
            target_link_libraries(Editor libWPxsdk-mt.lib)
            target_link_libraries(Editor libxml2-mt.lib)
            target_link_libraries(Editor tinyxml2.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(Editor FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(Editor freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(Editor WPGraphicsOgreNext.lib)
            target_link_libraries(Editor harWPuzz.lib)
            target_link_libraries(Editor harWPuzz-subset.lib)
            target_link_libraries(Editor icucommon.lib)
            target_link_libraries(Editor sds_library.lib)

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(Editor d3d11.lib)
                target_link_libraries(Editor dxgi.lib)
                target_link_libraries(Editor dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(Editor opengl32.lib)
                target_link_libraries(Editor glu32.lib)
            endif ()

            if (WP_PLATFORM_X64)
                #target_link_libraries(Editor amd_ags_x64.lib)
            endif()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(Editor WPGraphicsOgre.lib)

            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(Editor d3d9.lib)
                target_link_libraries(Editor d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(Editor d3d11.lib)
                target_link_libraries(Editor dxgi.lib)
                target_link_libraries(Editor dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(Editor opengl32.lib)
                target_link_libraries(Editor glu32.lib)
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(Editor opengl32.lib)
                target_link_libraries(Editor glu32.lib)
            endif ()
        elseif(WP_BUILD_DX11)
            target_link_libraries(Editor WPGraphicsD3D11.lib)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(Editor WPPhysx.lib)

            target_link_libraries(Editor PhysX3.lib)
            target_link_libraries(Editor PhysX3Extensions.lib)
            target_link_libraries(Editor PhysX3Common.lib)
            target_link_libraries(Editor PhysX3Vehicle.lib)
            target_link_libraries(Editor PhysX3Cooking.lib)
            target_link_libraries(Editor PxTask.lib)
            target_link_libraries(Editor PhysX3MetaData.lib)
            target_link_libraries(Editor PhysX3Foundation.lib)
            target_link_libraries(Editor PxSceneQuery.lib)
            target_link_libraries(Editor PxSimulationController.lib)
            target_link_libraries(Editor PhysX3LowLevel.lib)
            target_link_libraries(Editor PhysX3LowLevelCloth.lib)
            target_link_libraries(Editor PhysXProfileSDK.lib)

            if(WP_BUILD_PHYSICS_DEBUG)
                target_link_libraries(Editor PhysXVisualDebuggerSDK.lib)
                target_link_libraries(Editor PvdRuntime.lib)

                if (WP_PLATFORM_X64)
                    target_link_libraries(Editor nvToolsExt64_1.lib)
                endif()
            endif()
        elseif(WP_BUILD_ODE)
            target_link_libraries(Editor WPODE3.lib)
        elseif(WP_BUILD_PHYSICS3)
            target_link_libraries(Editor WPPhysics.lib)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(Editor debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
        endif()

        target_link_libraries(Editor bcrypt.lib)
        target_link_libraries(Editor libboost_thread.lib)
        target_link_libraries(Editor libboost_date_time.lib)
        target_link_libraries(Editor libboost_chrono.lib)
        target_link_libraries(Editor libboost_context.lib)
        target_link_libraries(Editor libboost_timer.lib)
        target_link_libraries(Editor libboost_filesystem.lib)
        target_link_libraries(Editor libboost_json.lib)
        target_link_libraries(Editor minizip.lib)
        target_link_libraries(Editor nativefiledialog.lib)
        target_link_libraries(Editor pugixml.lib)
        target_link_libraries(Editor rttr.lib)
        target_link_libraries(Editor tinyusdz_static.lib) 
        target_link_libraries(Editor tinyxml.lib)   
        target_link_libraries(Editor yaml-cpp.lib)
        target_link_libraries(Editor zlib.lib)
        target_link_libraries(Editor zzip.lib)
    elseif (APPLE)
        target_link_libraries(Editor libWPCore.a)
        target_link_libraries(Editor libWPMesh.a)
        target_link_libraries(Editor libWPProcedural.a)
        target_link_libraries(Editor libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(Editor WPAudio.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(Editor WPLua.a)
            target_link_libraries(Editor WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(Editor WPPython.a)
            target_link_libraries(Editor WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(Editor SDL2main.a)
            target_link_libraries(Editor SDL2_image.a)
            target_link_libraries(Editor debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(Editor libWPImGui.a)
            target_link_libraries(Editor imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(Editor libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(Editor libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(Editor libWPOISInput.a)
            #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
        endif()

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(Editor libWPWxWidgets.a)

            target_link_libraries(Editor libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(Editor libwx_baseu-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(Editor libwxexpat-3.1.a)
            target_link_libraries(Editor libwx_baseu_xml-3.1.a)
            target_link_libraries(Editor libwx_baseu_net-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(Editor libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(Editor libwxtiff-3.1.a)
            target_link_libraries(Editor libwxjpeg-3.1.a)
            target_link_libraries(Editor libwxpng-3.1.a)

            target_link_libraries(Editor libiconv.a)
        endif()

        target_link_libraries(Editor libWPRenderUI.a)
        target_link_libraries(Editor libWPSQLite.a)
        target_link_libraries(Editor libWPGraphics.a)

        target_link_libraries(Editor libboost_thread.a)
        target_link_libraries(Editor libboost_date_time.a)
        target_link_libraries(Editor libboost_chrono.a)
        target_link_libraries(Editor libboost_context.a)
        target_link_libraries(Editor libboost_timer.a)
        target_link_libraries(Editor libboost_filesystem.a)
        target_link_libraries(Editor libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(Editor WPPhysx.a)

            target_link_libraries(Editor PhysX3.a)
            target_link_libraries(Editor PhysX3Extensions.a)
            target_link_libraries(Editor PhysX3Common.a)
            target_link_libraries(Editor PhysX3Vehicle.a)
            target_link_libraries(Editor PhysX3Cooking.a)
            target_link_libraries(Editor PhysXProfileSDK.a)
            target_link_libraries(Editor PxTask.a)
            target_link_libraries(Editor PhysXVisualDebuggerSDK.a)
            target_link_libraries(Editor PhysX3MetaData.a)
            target_link_libraries(Editor PhysX3Foundation.a)
            target_link_libraries(Editor PxSceneQuery.a)
            target_link_libraries(Editor PxSimulationController.a)
            target_link_libraries(Editor PhysX3LowLevel.a)
            target_link_libraries(Editor PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(Editor tbb_static.a)
                target_link_libraries(Editor tbbmalloc_static.a)
            else()
                target_link_libraries(Editor libtbb.a)
                target_link_libraries(Editor libtbbmalloc.a)
            endif()
        endif()

        target_link_libraries(Editor minizip.a)
        target_link_libraries(Editor nativefiledialog.a)
        target_link_libraries(Editor rttr.a)
        target_link_libraries(Editor simplefilewatcher.a)
        target_link_libraries(Editor tinyusdz_static.a)
        target_link_libraries(Editor tinyxml.a)
        target_link_libraries(Editor yaml-cpp.a)
        target_link_libraries(Editor zlib.a)
        target_link_libraries(Editor zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(Editor libWm5Core.a)
            target_link_libraries(Editor libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(Editor debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(Editor FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(Editor freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(Editor WPGraphicsOgreNext.a)
            target_link_libraries(Editor harWPuzz.a)
            target_link_libraries(Editor harWPuzz-subset.a)
            target_link_libraries(Editor icucommon.a)
            target_link_libraries(Editor sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(Editor WPGraphicsOgre.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(Editor stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(Editor PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(Editor -Wl,--start-group WPCore.a WPVehiclePhysics.a WPImGui.a
            WPMath.a WPMesh.a WPOISInput.a WPData.a WPState.a libWPPhysx.a libWPSQLite.a
            WPGraphics.a WPGraphicsOgreNext.a WPAssimp.a WPRenderUI.a libWPCPlusPlus.a
            libboost_thread-gcc5-mt-s-x64-1_70.a
            libboost_date_time-gcc5-mt-s-x64-1_70.a
            libboost_chrono-gcc5-mt-s-x64-1_70.a
            libboost_timer-gcc5-mt-s-x64-1_70.a
            libboost_filesystem-gcc5-mt-s-x64-1_70.a
            debug libtbb_debug.a optimized libtbb.a
            debug libtbbmalloc_debug.a optimized libtbbmalloc.a
            libWm5Core.a libWm5Mathematics.a
            libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
            libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
            -Wl,--end-group )

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(Editor WPGraphicsOgreNext.a)
            target_link_libraries(Editor freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(Editor libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(Editor png)
        target_link_libraries(Editor Xaw)
        target_link_libraries(Editor Xrandr)
        target_link_libraries(Editor Xt)
        target_link_libraries(Editor xcb)
        target_link_libraries(Editor GL)
        target_link_libraries(Editor ${X11_LIBRARIES})
        target_link_libraries(Editor ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(Editor ${CMAKE_DL_LIBS})
    endif ()
endif ()

# Create the app target
set_target_properties(Editor PROPERTIES
        RESOURCE "${RESOURCES}"
        MACOSX_BUNDLE_INFO_PLIST ${WP_ROOT}/Tools/cpp/Editor/src/application/apple/macOS/Info-macOS.plist
        )

if (WP_BUILD_ENGINE)
    add_dependencies(Editor Workphone)
    add_dependencies(Editor WPAssimp)
    add_dependencies(Editor WPSQLite)

    if (WP_ENABLE_LUA)
        add_dependencies(Editor WPLua)
        add_dependencies(Editor WPLuaBind)
    elseif (WP_ENABLE_PYTHON)
        add_dependencies(Editor WPPython)
        add_dependencies(Editor WPPythonBind)
    endif()

    if (WP_BUILD_IMGUI)
        add_dependencies(Editor WPImGui)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        add_dependencies(Editor WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(Editor WPGraphicsOgre)
    elseif (WP_GRAPHICS_SYSTEM_CLAW OR WP_GRAPHICS_SYSTEM_SOFTWARE)
        add_dependencies(Editor WPGraphics)
    endif()

    if (WP_BUILD_PHYSX)
        add_dependencies(Editor WPPhysx)
    elseif (WP_BUILD_ODE)
        add_dependencies(Editor WPODE3)
    elseif (WP_BUILD_PHYSICS3)
        add_dependencies(Editor WPPhysics)
    endif()

    if (WP_STATIC_LIB)
        add_dependencies(Editor WPGraphics)
        add_dependencies(Editor WPRenderUI)

        if (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                add_dependencies(Editor tbb_static)
                add_dependencies(Editor tbbmalloc_static)
            endif()
        endif()

        if (WP_BUILD_IMGUI)
            add_dependencies(Editor WPImGui)
            add_dependencies(Editor imgui)
        endif()

        if (WP_BUILD_WXWIDGETS)
            #add_dependencies(Editor WPWxWidgets)
        endif()

        if (WP_BUILD_PHYSX)
            add_dependencies(Editor WPPhysx)

            add_dependencies(Editor PhysX3)
            add_dependencies(Editor PhysX3Extensions)
            add_dependencies(Editor PhysX3Common)
            add_dependencies(Editor PhysX3Vehicle)
            add_dependencies(Editor PhysX3Cooking)
            add_dependencies(Editor PxTask)    
            add_dependencies(Editor PhysX3MetaData)
            add_dependencies(Editor PhysX3Foundation)
            add_dependencies(Editor PxSceneQuery)
            add_dependencies(Editor PxSimulationController)
            add_dependencies(Editor PhysX3LowLevel)
            add_dependencies(Editor PhysX3LowLevelCloth)
            add_dependencies(Editor PhysXProfileSDK)

            if(WP_BUILD_PHYSICS_DEBUG)
                add_dependencies(Editor PhysXVisualDebuggerSDK)        
                add_dependencies(Editor PvdRuntime)
            endif()
        elseif(WP_BUILD_ODE)
            add_dependencies(Editor WPODE3)
        elseif(WP_BUILD_PHYSICS3)
            add_dependencies(Editor WPPhysics)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
            add_dependencies(Editor WPGraphicsOgreNext)
            add_dependencies(Editor harWPuzz)
            add_dependencies(Editor harWPuzz-subset)
            add_dependencies(Editor icucommon)
            add_dependencies(Editor sds_library)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            add_dependencies(Editor WPGraphicsOgre)
        endif()

        add_dependencies(Editor WPProcedural)
        add_dependencies(Editor WPAudio)

        add_dependencies(Editor boost_thread)
        add_dependencies(Editor boost_date_time)
        add_dependencies(Editor boost_chrono)
        add_dependencies(Editor boost_timer)
        add_dependencies(Editor boost_filesystem)
        add_dependencies(Editor boost_json)
        add_dependencies(Editor minizip)
        add_dependencies(Editor nativefiledialog)
        add_dependencies(Editor rttr)
        add_dependencies(Editor simplefilewatcher)
        add_dependencies(Editor tinyxml) 
        #add_dependencies(Editor tinyxmlstatic) 
        add_dependencies(Editor yaml-cpp)
        add_dependencies(Editor zlib)
        add_dependencies(Editor zzip)

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                add_dependencies(Editor tbb_static)
                add_dependencies(Editor tbbmalloc_static)
            else()
                add_dependencies(Editor tbb)
                add_dependencies(Editor tbbmalloc)
            endif()
        endif()
    endif()
endif()

if (WIN32)
    #set_property(TARGET Editor PROPERTY VS_DEBUGGER_COMMAND ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug/Editor.exe)
    #set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug)

    set_target_properties(Editor PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")

    set_target_properties (Editor PROPERTIES FOLDER Tools/Cpp)
endif()

if (WP_EDITOR_TESTS)
    set(WP_EDITOR_TEST_CASES
        play_mode
        play_stop_mode
        play_mode_repeated_cycles
        play_mode_duplicate_requests
        play_mode_rapid_requests
        play_mode_pause_and_restart
        play_mode_camera_eligibility
        play_mode_no_game_camera
        play_mode_editor_camera_override
        play_mode_saved_scene_preserves_edits
        play_mode_unsaved_scene_cache_paths
        play_mode_runtime_scene_switch
        play_mode_failed_load_does_not_enter
        play_mode_without_scene
        play_mode_scene_data_preserves_editor_camera
        play_mode_legacy_editor_camera_migration
        play_mode_file_camera_migration
        play_mode_worker_transitions
        play_mode_worker_latest_scene_request
        play_mode_editor_camera_data_precedence
        physics_mesh_test
        actor_component_test
        actor_test
        actor_dragdrop_test
        actor_prefab_test
        actor_addremove_test
        terrain_test
        ui_test
        project_metadata_test
        project_paths_and_build_settings_test
        project_default_data_test
        project_serialization_roundtrip_test
        project_legacy_defaults_and_dirty_test
        project_file_lifecycle_test
        project_owner_and_unload_test
        script_window_class_name_test
        editor_script_manager_lua_support_test
        editor_script_window_owner_binding_test
        editor_input_window_live_values_test
        editor_window_callbacks_disconnect_test
        editor_material_selection_visibility_test
        procedural_components_road_properties
        procedural_components_texture_resolution
        procedural_components_texture_properties
        procedural_components_sky_bounds
        procedural_components_sky_nonfinite
        procedural_components_sky_service_recovery
        procedural_components_vehicle_properties
        procedural_components_detached_meshes
        expired_pooled_weak_references
        editor_repeated_lifecycle
        editor_start
        editor_run
        editor_run_single_threaded)

    if (WP_USE_WP_STRING AND WP_USE_STANDARD_STRING_ALLOCATOR)
        list(APPEND WP_EDITOR_TEST_CASES editor_cross_module_string_pool)
    endif()

    if (WIN32)
        list(APPEND WP_EDITOR_TEST_CASES project_compile_integration_test)
    endif()

    foreach(WP_EDITOR_TEST_CASE IN LISTS WP_EDITOR_TEST_CASES)
        set(WP_EDITOR_TEST_SELECTOR "${WP_EDITOR_TEST_CASE}")
        if (WP_EDITOR_TEST_CASE STREQUAL "physics_mesh_test" OR
            WP_EDITOR_TEST_CASE STREQUAL "terrain_test")
            set(WP_EDITOR_TEST_SELECTOR "play_mode,${WP_EDITOR_TEST_CASE}")
        endif()

        add_test(
            NAME "Editor.${WP_EDITOR_TEST_CASE}"
            COMMAND Editor
                "--run_test=${WP_EDITOR_TEST_SELECTOR}"
                "--log_level=error"
                "--report_level=detailed"
                "--catch_system_errors=yes"
            CONFIGURATIONS Debug)
        set_tests_properties(
            "Editor.${WP_EDITOR_TEST_CASE}"
            PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:Editor>"
                RUN_SERIAL TRUE)
        set_tests_properties("Editor.${WP_EDITOR_TEST_CASE}" PROPERTIES TIMEOUT 90)
        if (WP_EDITOR_TEST_CASE MATCHES "^play_")
            set_tests_properties("Editor.${WP_EDITOR_TEST_CASE}" PROPERTIES
                LABELS "editor;playmode"
                TIMEOUT 60)
        endif()
    endforeach()

    add_test(NAME Editor.all COMMAND Editor
        "--log_level=error" "--report_level=detailed" "--catch_system_errors=yes"
        CONFIGURATIONS Debug)
    set_tests_properties(Editor.all PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:Editor>" RUN_SERIAL TRUE TIMEOUT 600
        LABELS "editor;lifecycle")
endif()
