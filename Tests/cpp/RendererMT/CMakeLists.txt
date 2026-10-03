set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(RendererMT)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

if (WIN32)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(RendererMT ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(RendererMT ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(RendererMT PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(RendererMT PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(RendererMT Workphone.lib)

    target_link_libraries(RendererMT bcrypt.lib)
    target_link_libraries(RendererMT libboost_thread.lib)
    target_link_libraries(RendererMT libboost_date_time.lib)
    target_link_libraries(RendererMT libboost_chrono.lib)
    target_link_libraries(RendererMT libboost_timer.lib)
    target_link_libraries(RendererMT libboost_filesystem.lib)
    target_link_libraries(RendererMT libboost_json.lib)
    target_link_libraries(RendererMT minizip.lib)
    target_link_libraries(RendererMT nativefiledialog.lib)
    target_link_libraries(RendererMT rttr.lib)
    target_link_libraries(RendererMT tinyxml.lib)   
    target_link_libraries(RendererMT yaml-cpp.lib)
    target_link_libraries(RendererMT zlib.lib)
    target_link_libraries(RendererMT zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(RendererMT tbb_static.lib)
			target_link_libraries(RendererMT tbbmalloc_static.lib)
		else()
			target_link_libraries(RendererMT tbb.lib)
			target_link_libraries(RendererMT tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(RendererMT libWPCore.a)
    target_link_libraries(RendererMT libWPMesh.a)
    target_link_libraries(RendererMT libWPPhysx.a)
    target_link_libraries(RendererMT libWPAero.a)
    target_link_libraries(RendererMT libWPAudio.a)
    target_link_libraries(RendererMT libWPGraphics.a)
    target_link_libraries(RendererMT libWPAssimp.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(RendererMT FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(RendererMT freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererMT WPGraphicsOgreNext.a)
        target_link_libraries(RendererMT harWPuzz.a)
        target_link_libraries(RendererMT harWPuzz-subset.a)
        target_link_libraries(RendererMT icucommon.a)
        target_link_libraries(RendererMT sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(RendererMT WPGraphicsOgre.a)
    endif()

    target_link_libraries(RendererMT libWPSQLite.a)

    target_link_libraries(RendererMT libboost_thread.a)
    target_link_libraries(RendererMT libboost_date_time.a)
    target_link_libraries(RendererMT libboost_chrono.a)
    target_link_libraries(RendererMT libboost_timer.a)
    target_link_libraries(RendererMT libboost_filesystem.a)
    target_link_libraries(RendererMT libboost_json.a)

    target_link_libraries(RendererMT minizip.a)
    target_link_libraries(RendererMT nativefiledialog.a)
    target_link_libraries(RendererMT rttr.a)
    target_link_libraries(RendererMT tinyxml.a)
    target_link_libraries(RendererMT yaml-cpp.a)
    target_link_libraries(RendererMT zlib.a)
    target_link_libraries(RendererMT zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(RendererMT tbb_static.a)
        target_link_libraries(RendererMT tbbmalloc_static.a)
    endif ()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(RendererMT libWm5Core.a)
        target_link_libraries(RendererMT libWm5Mathematics.a)
    endif()

    target_link_libraries(RendererMT stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(RendererMT PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(RendererMT -Wl,--start-group WPCore.a WPVehiclePhysics.a 
        WPApplication.a WPMath.a WPVehicle.a WPData.a WPMesh.a 
        libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPAudio.a
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        libboost_unit_test_framework-gcc5-mt-s-x64-1_70.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
        libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
        -Wl,--end-group )

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererMT WPGraphicsOgreNext.a)
        target_link_libraries(RendererMT freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(RendererMT libWPGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(RendererMT libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(RendererMT png)
    target_link_libraries(RendererMT Xaw)
    target_link_libraries(RendererMT Xrandr)
    target_link_libraries(RendererMT Xt)
    target_link_libraries(RendererMT xcb)
    target_link_libraries(RendererMT ${X11_LIBRARIES})
    target_link_libraries(RendererMT ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(RendererMT ${CMAKE_DL_LIBS})
endif ()
else()
if (WIN32)
    target_link_libraries(RendererMT Workphone.lib)
    target_link_libraries(RendererMT WPMesh.lib)
    target_link_libraries(RendererMT WPProcedural.lib)
    target_link_libraries(RendererMT WPPhysx.lib)
    target_link_libraries(RendererMT WPSQLite.lib)
    target_link_libraries(RendererMT WPVehicle.lib)
    target_link_libraries(RendererMT WPOISInput.lib)
    target_link_libraries(RendererMT WPGraphics.lib)
    target_link_libraries(RendererMT WPRenderUI.lib)

    #target_link_libraries(RendererMT debug OIS_d.lib optimized OIS.lib)
    target_link_libraries(RendererMT dinput8.lib)
    target_link_libraries(RendererMT dxguid.lib)

    if (WP_USE_WPXSDK)
        #target_link_libraries(RendererMT libWPxsdk.lib)
        target_link_libraries(RendererMT libWPxsdk-mt.lib)
        target_link_libraries(RendererMT libxml2-mt.lib)
        target_link_libraries(RendererMT tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(RendererMT debug FreeImage_d.lib optimized FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(RendererMT debug freetype_d.lib optimized freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererMT WPGraphicsOgreNext.lib)

        target_link_libraries(RendererMT harWPuzz.lib)
        target_link_libraries(RendererMT harWPuzz-subset.lib)
        target_link_libraries(RendererMT icucommon.lib)
        target_link_libraries(RendererMT sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(RendererMT d3d11.lib)
            target_link_libraries(RendererMT dxgi.lib)
            target_link_libraries(RendererMT dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(RendererMT opengl32.lib)
            target_link_libraries(RendererMT glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererMT amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(RendererMT WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(RendererMT debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(RendererMT debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(RendererMT debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(RendererMT debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(RendererMT debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(RendererMT debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(RendererMT debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(RendererMT debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(RendererMT debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(RendererMT debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(RendererMT debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            target_link_libraries(RendererMT d3d9.lib)
            target_link_libraries(RendererMT d3dx9.lib)
    
            target_link_libraries(RendererMT freetype.lib)
            target_link_libraries(RendererMT debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
        endif()

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(RendererMT d3d9.lib)
            target_link_libraries(RendererMT d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(RendererMT d3d11.lib)
            target_link_libraries(RendererMT dxgi.lib)
            target_link_libraries(RendererMT dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(RendererMT opengl32.lib)
            target_link_libraries(RendererMT glu32.lib)
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(RendererMT opengl32.lib)
            target_link_libraries(RendererMT glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererMT amd_ags_x64.lib)
        endif()
    elseif(WP_BUILD_DX11)
        target_link_libraries(RendererMT WPGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(RendererMT WPPhysx.lib)

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererMT 
                        debug PhysX3DEBUG_x64.lib
                        optimized PhysX3_x64.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3CommonDEBUG_x64.lib 
                        optimized PhysX3Common_x64.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3CookingDEBUG_x64.lib
                        optimized PhysX3Cooking_x64.lib)
            target_link_libraries(RendererMT 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(RendererMT 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(RendererMT 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        else ()
            target_link_libraries(RendererMT 
                        debug PhysX3DEBUG_x86.lib
                        optimized PhysX3_x86.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3CommonDEBUG_x86.lib 
                        optimized PhysX3Common_x86.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(RendererMT 
                        debug PhysX3CookingDEBUG_x86.lib
                        optimized PhysX3Cooking_x86.lib)
            target_link_libraries(RendererMT 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(RendererMT 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(RendererMT 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        endif()
    endif()

    target_link_libraries(RendererMT bcrypt.lib)
    target_link_libraries(RendererMT libboost_thread.lib)
    target_link_libraries(RendererMT libboost_date_time.lib)
    target_link_libraries(RendererMT libboost_chrono.lib)
    target_link_libraries(RendererMT libboost_timer.lib)
    target_link_libraries(RendererMT libboost_filesystem.lib)
    target_link_libraries(RendererMT libboost_json.lib)
    target_link_libraries(RendererMT minizip.lib)
    target_link_libraries(RendererMT nativefiledialog.lib)
    target_link_libraries(RendererMT rttr.lib)
    target_link_libraries(RendererMT tinyxml.lib)   
    target_link_libraries(RendererMT yaml-cpp.lib)
    target_link_libraries(RendererMT zlib.lib)
    target_link_libraries(RendererMT zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(RendererMT tbb_static.lib)
			target_link_libraries(RendererMT tbbmalloc_static.lib)
		else()
			target_link_libraries(RendererMT tbb.lib)
			target_link_libraries(RendererMT tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(RendererMT libWPCore.a)
    target_link_libraries(RendererMT libWPMesh.a)
    target_link_libraries(RendererMT libWPPhysx.a)
    target_link_libraries(RendererMT libWPAero.a)
    target_link_libraries(RendererMT libWPAudio.a)
    target_link_libraries(RendererMT libWPGraphics.a)
    target_link_libraries(RendererMT libWPAssimp.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(RendererMT FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(RendererMT freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererMT WPGraphicsOgreNext.a)
        target_link_libraries(RendererMT harWPuzz.a)
        target_link_libraries(RendererMT harWPuzz-subset.a)
        target_link_libraries(RendererMT icucommon.a)
        target_link_libraries(RendererMT sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(RendererMT WPGraphicsOgre.a)
    endif()

    target_link_libraries(RendererMT libWPSQLite.a)

    target_link_libraries(RendererMT libboost_thread.a)
    target_link_libraries(RendererMT libboost_date_time.a)
    target_link_libraries(RendererMT libboost_chrono.a)
    target_link_libraries(RendererMT libboost_timer.a)
    target_link_libraries(RendererMT libboost_filesystem.a)
    target_link_libraries(RendererMT libboost_json.a)

    target_link_libraries(RendererMT minizip.a)
    target_link_libraries(RendererMT nativefiledialog.a)
    target_link_libraries(RendererMT rttr.a)
    target_link_libraries(RendererMT tinyxml.a)
    target_link_libraries(RendererMT yaml-cpp.a)
    target_link_libraries(RendererMT zlib.a)
    target_link_libraries(RendererMT zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(RendererMT tbb_static.a)
        target_link_libraries(RendererMT tbbmalloc_static.a)
    endif ()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(RendererMT libWm5Core.a)
        target_link_libraries(RendererMT libWm5Mathematics.a)
    endif()

    target_link_libraries(RendererMT stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(RendererMT PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(RendererMT -Wl,--start-group WPCore.a WPVehiclePhysics.a 
        WPApplication.a WPMath.a WPVehicle.a WPData.a WPMesh.a 
        libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPAudio.a
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        libboost_unit_test_framework-gcc5-mt-s-x64-1_70.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
        libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
        -Wl,--end-group )

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererMT WPGraphicsOgreNext.a)
        target_link_libraries(RendererMT freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(RendererMT libWPGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(RendererMT libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(RendererMT png)
    target_link_libraries(RendererMT Xaw)
    target_link_libraries(RendererMT Xrandr)
    target_link_libraries(RendererMT Xt)
    target_link_libraries(RendererMT xcb)
    target_link_libraries(RendererMT ${X11_LIBRARIES})
    target_link_libraries(RendererMT ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(RendererMT ${CMAKE_DL_LIBS})
endif ()
endif ()


if (WIN32)
elseif (APPLE)
    target_link_directories(RendererMT PUBLIC ../Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(RendererMT -L../dependencies/lib/linux/boost/)
elseif (UNIX)
    target_link_libraries(RendererMT -L../dependencies/lib/linux/boost/)
    target_link_libraries(RendererMT -L../dependencies/lib/linux/physx/)
    target_link_libraries(RendererMT -L../dependencies/lib/linux/)
endif (WIN32)

if (WIN32)
    set_property(TARGET RendererMT PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(RendererMT Workphone)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
        add_dependencies(RendererMT WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(RendererMT WPGraphicsOgre)
    endif()


    if (WP_BUILD_ODE)
        add_dependencies(RendererMT WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(RendererMT WPPhysx)
    endif()
endif()

set_target_properties (RendererMT PROPERTIES FOLDER Tests/Cpp )
