set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(RendererOgre)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

if (APPLE)
    find_library(CORE_FOUNDATION CoreFoundation)
endif()

add_compile_definitions (RAPIDJSON_HAS_STDSTRING=1)
add_compile_definitions (OGRE_STATIC_LIB)
#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    if(WIN32)
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/include/linux )
    endif()

    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/OgreMain/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Deprecated )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Overlay/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Paging/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Bites/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Common/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Pbs/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Unlit/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/PlugIns/CgProgramManager/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/GLES2/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include )
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/Ogre/RenderSystems/NULL/include )
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )

    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Overlay/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Terrain/include )
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

    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )

    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/linux )
    endif()

    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
    endif()
endif()

if (WIN32)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/Dependencies/lib/${CMAKE_BUILD_TYPE})
    endif ()
elseif (UNIX)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(RendererOgre ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(RendererOgre ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(RendererOgre PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(RendererOgre PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(RendererOgre Workphone)
    target_link_libraries(RendererOgre WPMesh.lib)
    target_link_libraries(RendererOgre WPProcedural)
    target_link_libraries(RendererOgre WPPhysx.lib)
    target_link_libraries(RendererOgre WPSQLite)
    target_link_libraries(RendererOgre WPVehicle.lib)
    target_link_libraries(RendererOgre WPGraphics)

    #target_link_libraries(RendererOgre debug OIS_d.lib optimized OIS.lib)
    target_link_libraries(RendererOgre dinput8.lib)
    target_link_libraries(RendererOgre dxguid.lib)

    if (WP_USE_WPXSDK)
        #target_link_libraries(RendererOgre libWPxsdk.lib)
        target_link_libraries(RendererOgre libWPxsdk-mt.lib)
        target_link_libraries(RendererOgre libxml2-mt.lib)
        target_link_libraries(RendererOgre tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(RendererOgre debug FreeImage_d.lib optimized FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(RendererOgre debug freetype_d.lib optimized freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererOgre WPGraphicsOgreNext.lib)

        #target_link_libraries(RendererOgre debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
        #target_link_libraries(RendererOgre debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)
        #target_link_libraries(RendererOgre debug RenderSystem_Direct3D11Static_d.lib optimized RenderSystem_Direct3D11Static.lib)
        #target_link_libraries(RendererOgre debug RenderSystem_GL3PlusStatic_d.lib optimized RenderSystem_GL3PlusStatic.lib)

       # target_link_libraries(RendererOgre debug OgreHlmsPbsStatic_d.lib optimized OgreHlmsPbsStatic.lib)
        #target_link_libraries(RendererOgre debug OgreHlmsUnlitStatic_d.lib optimized OgreHlmsUnlitStatic.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(RendererOgre d3d11.lib)
            target_link_libraries(RendererOgre dxgi.lib)
            target_link_libraries(RendererOgre dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(RendererOgre opengl32.lib)
            target_link_libraries(RendererOgre glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererOgre amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(RendererOgre WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(RendererOgre debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(RendererOgre debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(RendererOgre debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(RendererOgre debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(RendererOgre debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(RendererOgre debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(RendererOgre debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(RendererOgre debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(RendererOgre debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(RendererOgre debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(RendererOgre debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            target_link_libraries(RendererOgre d3d9.lib)
            target_link_libraries(RendererOgre d3dx9.lib)
    
            target_link_libraries(RendererOgre freetype.lib)
            target_link_libraries(RendererOgre debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
        endif()

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(RendererOgre d3d9.lib)
            target_link_libraries(RendererOgre d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(RendererOgre d3d11.lib)
            target_link_libraries(RendererOgre dxgi.lib)
            target_link_libraries(RendererOgre dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(RendererOgre opengl32.lib)
            target_link_libraries(RendererOgre glu32.lib)
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(RendererOgre opengl32.lib)
            target_link_libraries(RendererOgre glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererOgre amd_ags_x64.lib)
        endif()
    elseif(WP_BUILD_DX11)
        target_link_libraries(RendererOgre WPGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(RendererOgre WPPhysx.lib)

        if (WP_PLATFORM_X64)
            target_link_libraries(RendererOgre 
                        debug PhysX3DEBUG_x64.lib
                        optimized PhysX3_x64.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3CommonDEBUG_x64.lib 
                        optimized PhysX3Common_x64.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3CookingDEBUG_x64.lib
                        optimized PhysX3Cooking_x64.lib)
            target_link_libraries(RendererOgre 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(RendererOgre 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(RendererOgre 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        else ()
            target_link_libraries(RendererOgre 
                        debug PhysX3DEBUG_x86.lib
                        optimized PhysX3_x86.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3CommonDEBUG_x86.lib 
                        optimized PhysX3Common_x86.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(RendererOgre 
                        debug PhysX3CookingDEBUG_x86.lib
                        optimized PhysX3Cooking_x86.lib)
            target_link_libraries(RendererOgre 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(RendererOgre 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(RendererOgre 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        endif()
    endif()

    target_link_libraries(RendererOgre bcrypt.lib)
    target_link_libraries(RendererOgre libboost_thread.lib)
    target_link_libraries(RendererOgre libboost_date_time.lib)
    target_link_libraries(RendererOgre libboost_chrono.lib)
    target_link_libraries(RendererOgre libboost_timer.lib)
    target_link_libraries(RendererOgre libboost_filesystem.lib)
    target_link_libraries(RendererOgre libboost_json.lib)
    target_link_libraries(RendererOgre minizip.lib)
    target_link_libraries(RendererOgre nativefiledialog.lib)
    target_link_libraries(RendererOgre tinyxml.lib)   
    target_link_libraries(RendererOgre zlib.lib)
    target_link_libraries(RendererOgre zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(RendererOgre tbb_static.lib)
			target_link_libraries(RendererOgre tbbmalloc_static.lib)
		else()
			target_link_libraries(RendererOgre tbb.lib)
			target_link_libraries(RendererOgre tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(RendererOgre libWPCore.a)
    target_link_libraries(RendererOgre libWPMath.a)
    target_link_libraries(RendererOgre libWPMesh.a)
    target_link_libraries(RendererOgre libWPProcedural.a)
    target_link_libraries(RendererOgre libWPPhysx.a)
    target_link_libraries(RendererOgre libWPData.a)
    target_link_libraries(RendererOgre libWPAero.a)
    target_link_libraries(RendererOgre libWPVehicle.a)
    target_link_libraries(RendererOgre libWPAudio.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(RendererOgre WPGraphicsOgreNext.a)

        target_link_libraries(RendererOgre FreeImage.a)
        target_link_libraries(RendererOgre freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(RendererOgre libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(RendererOgre libWPSQLite.a)

    target_link_libraries(RendererOgre libboost_thread.a)
    target_link_libraries(RendererOgre libboost_date_time.a)
    target_link_libraries(RendererOgre libboost_chrono.a)
    target_link_libraries(RendererOgre libboost_timer.a)
    target_link_libraries(RendererOgre libboost_filesystem.a)
    target_link_libraries(RendererOgre libboost_unit_test_framework.a)
    target_link_libraries(RendererOgre libboost_system.a)

    target_link_libraries(RendererOgre libtbb.a)
    target_link_libraries(RendererOgre libtbbmalloc.a)

    #target_link_libraries(RendererOgre stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(RendererOgre ${AUDIO_UNIT})
    target_link_libraries(RendererOgre stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(RendererOgre PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(RendererOgre -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPMath.a WPVehicle.a WPData.a WPMesh.a 
        libWPObjectTemplates.a libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPAudio.a      
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
        target_link_libraries(RendererOgre WPGraphicsOgreNext.a)
        target_link_libraries(RendererOgre freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(RendererOgre libWPGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(RendererOgre libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(RendererOgre png)
    target_link_libraries(RendererOgre Xaw)
    target_link_libraries(RendererOgre Xrandr)
    target_link_libraries(RendererOgre Xt)
    target_link_libraries(RendererOgre xcb)
    target_link_libraries(RendererOgre ${X11_LIBRARIES})
    target_link_libraries(RendererOgre ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(RendererOgre ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET RendererOgre PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(RendererOgre Workphone)

set_target_properties (RendererOgre PROPERTIES FOLDER Tests/Cpp )
