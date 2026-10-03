set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(HoudiniRender)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/WPEngine/cpp/Include )
include_directories(Source)

if (WP_GRAPHICS_SYSTEM_OGRE)
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

    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
    endif()
endif()

if (WIN32)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic)
elseif (UNIX)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniRender ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(HoudiniRender ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(HoudiniRender PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(HoudiniRender PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(HoudiniRender WPCore.lib)
    target_link_libraries(HoudiniRender WPMesh.lib)
    target_link_libraries(HoudiniRender WPProcedural.lib)
    target_link_libraries(HoudiniRender WPPhysx.lib)
    target_link_libraries(HoudiniRender WPSQLite.lib)
    target_link_libraries(HoudiniRender WPVehicle.lib)
    target_link_libraries(HoudiniRender WPWxWidgets.lib)
    target_link_libraries(HoudiniRender WPOISInput.lib)
    target_link_libraries(HoudiniRender WPGraphics.lib)
    target_link_libraries(HoudiniRender WPRenderUI.lib)
    target_link_libraries(HoudiniRender WPHoudini.lib)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(HoudiniRender WPGraphicsOgreNext.lib)
        target_link_libraries(HoudiniRender debug FreeImage_d.lib optimized FreeImage.lib)
        target_link_libraries(HoudiniRender debug freetype_d.lib optimized freetype.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(HoudiniRender d3d11.lib)
            target_link_libraries(HoudiniRender dxgi.lib)
            target_link_libraries(HoudiniRender dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(HoudiniRender opengl32.lib)
            target_link_libraries(HoudiniRender glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(HoudiniRender amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(HoudiniRender WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(HoudiniRender debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(HoudiniRender debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(HoudiniRender debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(HoudiniRender debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(HoudiniRender debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(HoudiniRender debug RenderSystem_GLStatic_d.lib 
                                optimized RenderSystem_GLStatic.lib)

                            
            target_link_libraries(HoudiniRender debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(HoudiniRender debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(HoudiniRender debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(HoudiniRender debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(HoudiniRender debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)
        endif()
    
        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(HoudiniRender debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(HoudiniRender freetype.lib)   
        
        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(HoudiniRender d3d9.lib)
            target_link_libraries(HoudiniRender d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(HoudiniRender d3d11.lib)
            target_link_libraries(HoudiniRender dxgi.lib)
            target_link_libraries(HoudiniRender dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(HoudiniRender opengl32.lib)
            target_link_libraries(HoudiniRender glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(HoudiniRender amd_ags_x64.lib)
            endif ()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(HoudiniRender opengl32.lib)
            target_link_libraries(HoudiniRender glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(HoudiniRender amd_ags_x64.lib)
            endif ()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(HoudiniRender WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(HoudiniRender bcrypt.lib)
    target_link_libraries(HoudiniRender libboost_thread.lib)
    target_link_libraries(HoudiniRender libboost_date_time.lib)
    target_link_libraries(HoudiniRender libboost_chrono.lib)
    target_link_libraries(HoudiniRender libboost_timer.lib)
    target_link_libraries(HoudiniRender libboost_filesystem.lib)
    target_link_libraries(HoudiniRender libboost_json.lib)
    target_link_libraries(HoudiniRender minizip.lib)
    target_link_libraries(HoudiniRender nativefiledialog.lib)
    target_link_libraries(HoudiniRender rttr.lib)
    target_link_libraries(HoudiniRender tinyxml.lib)   
    target_link_libraries(HoudiniRender yaml-cpp.lib)
    target_link_libraries(HoudiniRender zlib.lib)
    target_link_libraries(HoudiniRender zzip.lib)

    if (WP_USE_TBB)
        target_link_libraries(HoudiniRender tbb_static.lib)
        target_link_libraries(HoudiniRender tbbmalloc_static.lib)
    endif()
elseif (APPLE)
    target_link_libraries(HoudiniRender libWPCore.a)
    target_link_libraries(HoudiniRender libWPFileSystem.a)
    target_link_libraries(HoudiniRender libWPMath.a)
    target_link_libraries(HoudiniRender libWPMesh.a)
    target_link_libraries(HoudiniRender libWPInterface.a)
    target_link_libraries(HoudiniRender libWPProcedural.a)
    target_link_libraries(HoudiniRender libWPPhysx.a)
    target_link_libraries(HoudiniRender libWPData.a)
    target_link_libraries(HoudiniRender libWPObjectTemplates.a)
    target_link_libraries(HoudiniRender libWPAero.a)
    target_link_libraries(HoudiniRender libWPVehicle.a)
    target_link_libraries(HoudiniRender libWPAudio.a)
    target_link_libraries(HoudiniRender libWPApplication.a)
    

    if(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(HoudiniRender libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(HoudiniRender libWPSQLite.a)

    target_link_libraries(HoudiniRender libboost_thread.a)
    target_link_libraries(HoudiniRender libboost_date_time.a)
    target_link_libraries(HoudiniRender libboost_chrono.a)
    target_link_libraries(HoudiniRender libboost_timer.a)
    target_link_libraries(HoudiniRender libboost_filesystem.a)
    target_link_libraries(HoudiniRender libboost_unit_test_framework.a)
    target_link_libraries(HoudiniRender libboost_system.a)

    target_link_libraries(HoudiniRender libPxTask.a)
    target_link_libraries(HoudiniRender libPhysXVisualDebuggerSDK.a)
    target_link_libraries(HoudiniRender libPvdRuntime.a)

    target_link_libraries(HoudiniRender debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(HoudiniRender debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(HoudiniRender libWm5Core.a)
        target_link_libraries(HoudiniRender libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(HoudiniRender libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(HoudiniRender -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)
elseif (UNIX)
    target_link_libraries(HoudiniRender -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPMath.a WPInterface.a WPVehicle.a WPData.a WPMesh.a 
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

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(HoudiniRender libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(HoudiniRender ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(HoudiniRender ${CMAKE_DL_LIBS})
endif ()


if (WIN32)
elseif (APPLE)
    target_link_directories(HoudiniRender PUBLIC ../Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(HoudiniRender -L../dependencies/lib/linux/boost/)
elseif (UNIX)
    target_link_libraries(HoudiniRender -L../dependencies/lib/linux/boost/)
    target_link_libraries(HoudiniRender -L../dependencies/lib/linux/physx/)
    target_link_libraries(HoudiniRender -L../dependencies/lib/linux/)
endif (WIN32)

if (WIN32)
    set_property(TARGET HoudiniRender PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(HoudiniRender WPRenderUI WPCore WPMesh WPProcedural WPSQLite WPVehicle)

if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
    add_dependencies(HoudiniRender WPGraphicsOgreNext)
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(HoudiniRender WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(HoudiniRender WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(HoudiniRender WPPhysx)
endif()

set_target_properties (HoudiniRender PROPERTIES FOLDER Tests/Cpp )
