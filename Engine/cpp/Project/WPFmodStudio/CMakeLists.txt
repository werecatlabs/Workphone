project(WPFmodStudio)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPFmodStudio/*.h" "../../Include/WPFmodStudio/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPFmodStudio/*.cpp")

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/tbb/include )
include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinybind/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/Dependencies/fmod/studio/api/studio/inc )
include_directories( ${WP_ROOT}/Dependencies/fmod/studio/api/lowlevel/inc )
include_directories( ../../Include )
include_directories( ../../Include/WPFmodStudio )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPFmodStudio ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
        link_directories(WPFmodStudio ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/fmod/)
        link_directories(WPFmodStudio ${CMAKE_SOURCE_DIR}/Dependencies/fmod/studio/api/lowlevel/lib)
        link_directories(WPFmodStudio ${CMAKE_SOURCE_DIR}/Dependencies/fmod/studio/api/studio/lib)
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPFmodStudio STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPFmodStudio SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPFmodStudio Workphone.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(WPFmodStudio debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPFmodStudio debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPFmodStudio debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPFmodStudio tbb_static.lib)
                target_link_libraries(WPFmodStudio tbbmalloc_static.lib)
            else()
                target_link_libraries(WPFmodStudio tbb.lib)
			    target_link_libraries(WPFmodStudio tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPFmodStudio fmod64_vc.lib )
        target_link_libraries(WPFmodStudio fmodstudio64_vc.lib )
    elseif (APPLE)
        target_link_libraries(WPFmodStudio libWorkphone.a)

        if (WP_USE_TBB)
            target_link_libraries(WPFmodStudio libtbb.a)
            target_link_libraries(WPFmodStudio libtbbmalloc.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPFmodStudio stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPFmodStudio PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPFmodStudio -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPFmodStudio.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPFmodStudio WPFmodStudio.a)
            target_link_libraries(WPFmodStudio freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPFmodStudio libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPFmodStudio png)
        target_link_libraries(WPFmodStudio Xaw)
        target_link_libraries(WPFmodStudio Xrandr)
        target_link_libraries(WPFmodStudio Xt)
        target_link_libraries(WPFmodStudio xcb)
        target_link_libraries(WPFmodStudio GL)
        target_link_libraries(WPFmodStudio ${X11_LIBRARIES})
        target_link_libraries(WPFmodStudio ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPFmodStudio ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPFmodStudio PROPERTIES FOLDER Engine/Cpp/Sound)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPFmodStudio PUBLIC WPFMODStudio_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPFmodStudio Workphone)
endif()