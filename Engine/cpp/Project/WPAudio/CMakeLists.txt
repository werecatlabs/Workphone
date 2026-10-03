project(WPAudio)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPAudio/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPAudio/*.cpp")

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
include_directories( ${WP_ROOT}/AdditionalLibraries/zlib )
include_directories( ${WP_ROOT}/AdditionalLibraries/zziplib )
include_directories( ../../Include )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPAudio ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPAudio STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPAudio SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (WIN32)
    target_link_libraries(WPAudio xaudio2 winmm ole32)
elseif (APPLE)
    find_library(WP_AUDIO_TOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    find_library(WP_AUDIO_UNIT_FRAMEWORK AudioUnit REQUIRED)
    target_link_libraries(WPAudio
        ${WP_AUDIO_TOOLBOX_FRAMEWORK}
        ${WP_AUDIO_UNIT_FRAMEWORK})
elseif (ANDROID)
    target_link_libraries(WPAudio OpenSLES)
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPAudio Workphone.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(WPAudio debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPAudio debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPAudio debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPAudio tbb_static.lib)
                target_link_libraries(WPAudio tbbmalloc_static.lib)
            else()
                target_link_libraries(WPAudio tbb.lib)
			    target_link_libraries(WPAudio tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPAudio bcrypt.lib)
        target_link_libraries(WPAudio libboost_thread.lib)
        target_link_libraries(WPAudio libboost_date_time.lib)
        target_link_libraries(WPAudio libboost_chrono.lib)
        target_link_libraries(WPAudio libboost_timer.lib)
        target_link_libraries(WPAudio libboost_filesystem.lib)
        target_link_libraries(WPAudio libboost_json.lib)
        target_link_libraries(WPAudio minizip.lib)
        target_link_libraries(WPAudio tinyxml.lib)   
        target_link_libraries(WPAudio zlib.lib)
        target_link_libraries(WPAudio zzip.lib)
    elseif (APPLE)
        target_link_libraries(WPAudio libWorkphone.a)
        target_link_libraries(WPAudio libFBMesh.a)
        target_link_libraries(WPAudio libWPProcedural.a)
        target_link_libraries(WPAudio libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(WPAudio WPAudio.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(WPAudio WPLua.a)
            target_link_libraries(WPAudio WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(WPAudio WPPython.a)
            target_link_libraries(WPAudio WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(WPAudio SDL2main.a)
            target_link_libraries(WPAudio SDL2_image.a)
            target_link_libraries(WPAudio debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(WPAudio libWPImGui.a)
            target_link_libraries(WPAudio imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPAudio libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPAudio libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(WPAudio libWPOISInput.a)
            #target_link_libraries(WPAudio debug libOIS_d.a optimized libOIS.a)
        endif()

        target_link_libraries(WPAudio libWPVehiclePhysics.a)

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(WPAudio libWPWxWidgets.a)

            target_link_libraries(WPAudio libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(WPAudio libwx_baseu-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(WPAudio libwxexpat-3.1.a)
            target_link_libraries(WPAudio libwx_baseu_xml-3.1.a)
            target_link_libraries(WPAudio libwx_baseu_net-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(WPAudio libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(WPAudio libwxtiff-3.1.a)
            target_link_libraries(WPAudio libwxjpeg-3.1.a)
            target_link_libraries(WPAudio libwxpng-3.1.a)

            target_link_libraries(WPAudio libiconv.a)
        endif()

        target_link_libraries(WPAudio libFBRenderUI.a)
        target_link_libraries(WPAudio libWPSQLite.a)
        target_link_libraries(WPAudio libFBCPlusPlus.a)
        target_link_libraries(WPAudio libFBGraphics.a)

        target_link_libraries(WPAudio libboost_thread.a)
        target_link_libraries(WPAudio libboost_date_time.a)
        target_link_libraries(WPAudio libboost_chrono.a)
        target_link_libraries(WPAudio libboost_timer.a)
        target_link_libraries(WPAudio libboost_filesystem.a)
        target_link_libraries(WPAudio libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPAudio WPPhysx.a)

            target_link_libraries(WPAudio PhysX3.a)
            target_link_libraries(WPAudio PhysX3Extensions.a)
            target_link_libraries(WPAudio PhysX3Common.a)
            target_link_libraries(WPAudio PhysX3Vehicle.a)
            target_link_libraries(WPAudio PhysX3Cooking.a)
            target_link_libraries(WPAudio PhysXProfileSDK.a)
            target_link_libraries(WPAudio PxTask.a)
            target_link_libraries(WPAudio PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPAudio PhysX3MetaData.a)
            target_link_libraries(WPAudio PhysX3Foundation.a)
            target_link_libraries(WPAudio PxSceneQuery.a)
            target_link_libraries(WPAudio PxSimulationController.a)
            target_link_libraries(WPAudio PhysX3LowLevel.a)
            target_link_libraries(WPAudio PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPAudio libtbb.a)
            target_link_libraries(WPAudio libtbbmalloc.a)
        endif()

        target_link_libraries(WPAudio minizip.a)
        target_link_libraries(WPAudio nativefiledialog.a)
        target_link_libraries(WPAudio rttr.a)
        target_link_libraries(WPAudio tinyxml.a)
        target_link_libraries(WPAudio yaml-cpp.a)
        target_link_libraries(WPAudio zlib.a)
        target_link_libraries(WPAudio zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPAudio libWm5Core.a)
            target_link_libraries(WPAudio libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPAudio debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPAudio FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPAudio freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(WPAudio WPAudio.a)
            target_link_libraries(WPAudio harfbuzz.a)
            target_link_libraries(WPAudio harfbuzz-subset.a)
            target_link_libraries(WPAudio icucommon.a)
            target_link_libraries(WPAudio sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(WPAudio WPGraphicsOgre.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPAudio stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPAudio PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPAudio -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPAudio.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPAudio WPAudio.a)
            target_link_libraries(WPAudio freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPAudio libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPAudio png)
        target_link_libraries(WPAudio Xaw)
        target_link_libraries(WPAudio Xrandr)
        target_link_libraries(WPAudio Xt)
        target_link_libraries(WPAudio xcb)
        target_link_libraries(WPAudio GL)
        target_link_libraries(WPAudio ${X11_LIBRARIES})
        target_link_libraries(WPAudio ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPAudio ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPAudio PROPERTIES FOLDER Engine/Cpp/Sound)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPAudio PUBLIC WPAudio_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPAudio Workphone)
endif()
