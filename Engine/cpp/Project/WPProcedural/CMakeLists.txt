project(WPProcedural)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPProcedural/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPProcedural/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPProcedural/../../Include/WPProcedural")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPProcedural/../../Source/WPProcedural")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    String(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    String(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Header Files\\${_group_file_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    String(REPLACE "${WP_SOURCE_REL_PATH}" "" _group_file_path "${_source_file_path}")
    String(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
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
include_directories( ${WP_ROOT}/AdditionalLibraries/wildmagic/SDK/Include )
include_directories( ../../Include )

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPProcedural "WPProcedural/WPProceduralPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPProcedural/WPProceduralPCH.cpp")
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPProcedural ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPProcedural STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPProcedural SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPProcedural Workphone)

        if (WP_USE_ONETBB)
            target_link_libraries(WPProcedural debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPProcedural debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPProcedural debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPProcedural tbb_static.lib)
                target_link_libraries(WPProcedural tbbmalloc_static.lib)
            else()
                target_link_libraries(WPProcedural tbb.lib)
			    target_link_libraries(WPProcedural tbbmalloc.lib)
            endif()
        endif()
    elseif (APPLE)
        target_link_libraries(WPProcedural libWorkphone.a)
        target_link_libraries(WPProcedural libFBMesh.a)
        target_link_libraries(WPProcedural libWPProcedural.a)
        target_link_libraries(WPProcedural libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(WPProcedural WPProcedural.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(WPProcedural WPLua.a)
            target_link_libraries(WPProcedural WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(WPProcedural WPPython.a)
            target_link_libraries(WPProcedural WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(WPProcedural SDL2main.a)
            target_link_libraries(WPProcedural SDL2_image.a)
            target_link_libraries(WPProcedural debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(WPProcedural libWPImGui.a)
            target_link_libraries(WPProcedural imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPProcedural libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPProcedural libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(WPProcedural libWPProcedural.a)
            #target_link_libraries(WPProcedural debug libOIS_d.a optimized libOIS.a)
        endif()

        target_link_libraries(WPProcedural libWPVehiclePhysics.a)

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(WPProcedural libWPWxWidgets.a)

            target_link_libraries(WPProcedural libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(WPProcedural libwx_baseu-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(WPProcedural libwxexpat-3.1.a)
            target_link_libraries(WPProcedural libwx_baseu_xml-3.1.a)
            target_link_libraries(WPProcedural libwx_baseu_net-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(WPProcedural libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(WPProcedural libwxtiff-3.1.a)
            target_link_libraries(WPProcedural libwxjpeg-3.1.a)
            target_link_libraries(WPProcedural libwxpng-3.1.a)

            target_link_libraries(WPProcedural libiconv.a)
        endif()

        target_link_libraries(WPProcedural libFBRenderUI.a)
        target_link_libraries(WPProcedural libWPSQLite.a)
        target_link_libraries(WPProcedural libFBCPlusPlus.a)
        target_link_libraries(WPProcedural libFBGraphics.a)

        target_link_libraries(WPProcedural libboost_thread.a)
        target_link_libraries(WPProcedural libboost_date_time.a)
        target_link_libraries(WPProcedural libboost_chrono.a)
        target_link_libraries(WPProcedural libboost_timer.a)
        target_link_libraries(WPProcedural libboost_filesystem.a)
        target_link_libraries(WPProcedural libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPProcedural WPPhysx.a)

            target_link_libraries(WPProcedural PhysX3.a)
            target_link_libraries(WPProcedural PhysX3Extensions.a)
            target_link_libraries(WPProcedural PhysX3Common.a)
            target_link_libraries(WPProcedural PhysX3Vehicle.a)
            target_link_libraries(WPProcedural PhysX3Cooking.a)
            target_link_libraries(WPProcedural PhysXProfileSDK.a)
            target_link_libraries(WPProcedural PxTask.a)
            target_link_libraries(WPProcedural PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPProcedural PhysX3MetaData.a)
            target_link_libraries(WPProcedural PhysX3Foundation.a)
            target_link_libraries(WPProcedural PxSceneQuery.a)
            target_link_libraries(WPProcedural PxSimulationController.a)
            target_link_libraries(WPProcedural PhysX3LowLevel.a)
            target_link_libraries(WPProcedural PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPProcedural libtbb.a)
            target_link_libraries(WPProcedural libtbbmalloc.a)
        endif()

        target_link_libraries(WPProcedural minizip.a)
        target_link_libraries(WPProcedural nativefiledialog.a)
        target_link_libraries(WPProcedural tinyxml.a)
        target_link_libraries(WPProcedural zlib.a)
        target_link_libraries(WPProcedural zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPProcedural libWm5Core.a)
            target_link_libraries(WPProcedural libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPProcedural debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPProcedural FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPProcedural freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(WPProcedural WPProcedural.a)
            target_link_libraries(WPProcedural harfbuzz.a)
            target_link_libraries(WPProcedural harfbuzz-subset.a)
            target_link_libraries(WPProcedural icucommon.a)
            target_link_libraries(WPProcedural sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(WPProcedural WPGraphicsOgre.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPProcedural stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPProcedural PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPProcedural -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPProcedural.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPProcedural.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPProcedural WPProcedural.a)
            target_link_libraries(WPProcedural freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPProcedural libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPProcedural png)
        target_link_libraries(WPProcedural Xaw)
        target_link_libraries(WPProcedural Xrandr)
        target_link_libraries(WPProcedural Xt)
        target_link_libraries(WPProcedural xcb)
        target_link_libraries(WPProcedural GL)
        target_link_libraries(WPProcedural ${X11_LIBRARIES})
        target_link_libraries(WPProcedural ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPProcedural ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPProcedural PROPERTIES FOLDER Engine/Cpp/Procedural)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPProcedural PRIVATE WPProcedural_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPProcedural Workphone)
endif()

# Focused integration checks that do not need a graphics device or the full test suite.
option(WP_BUILD_PROCEDURAL_SERVICE_TESTS "Build procedural service contract tests" OFF)
if(WP_BUILD_PROCEDURAL_SERVICE_TESTS)
    add_executable(ProceduralServiceTests
        ${WP_ROOT}/Tests/cpp/ProceduralServices/ServiceTests.cpp
        ${WP_ROOT}/Tests/cpp/ProceduralServices/ClientContracts.cpp)
    target_compile_features(ProceduralServiceTests PRIVATE cxx_std_17)
    if(MSVC)
        set_source_files_properties(
            ${WP_ROOT}/Tests/cpp/ProceduralServices/ServiceTests.cpp
            ${WP_ROOT}/Tests/cpp/ProceduralServices/ClientContracts.cpp
            PROPERTIES COMPILE_OPTIONS /Y- SKIP_PRECOMPILE_HEADERS ON)
    endif()
    target_link_libraries(ProceduralServiceTests PRIVATE Workphone WPProcedural)
    if(WP_GRAPHICS_SYSTEM_CLAW)
        target_sources(ProceduralServiceTests PRIVATE ${WP_ROOT}/Tests/cpp/ProceduralServices/ClawContracts.cpp)
        target_compile_definitions(ProceduralServiceTests PRIVATE WP_PROCEDURAL_TEST_CLAW=1)
        target_include_directories(ProceduralServiceTests PRIVATE
            ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
        target_link_libraries(ProceduralServiceTests PRIVATE WPGraphics WorkphoneGraphics)
        if(MSVC)
            set_source_files_properties(${WP_ROOT}/Tests/cpp/ProceduralServices/ClawContracts.cpp
                PROPERTIES COMPILE_OPTIONS /Y- SKIP_PRECOMPILE_HEADERS ON)
        endif()
    endif()
    set_target_properties(ProceduralServiceTests PROPERTIES FOLDER Tests/Cpp)
endif()
