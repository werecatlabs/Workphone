set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(Terrain)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (OGRE_STATIC_LIB)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

if (WIN32)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(Terrain ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h" "./*.hpp")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(Terrain ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(Terrain PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(Terrain PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(Terrain Workphone.lib)

    target_link_libraries(Terrain bcrypt.lib)
    target_link_libraries(Terrain libboost_thread.lib)
    target_link_libraries(Terrain libboost_date_time.lib)
    target_link_libraries(Terrain libboost_chrono.lib)
    target_link_libraries(Terrain libboost_timer.lib)
    target_link_libraries(Terrain libboost_filesystem.lib)
    target_link_libraries(Terrain libboost_json.lib)
    target_link_libraries(Terrain minizip.lib)
    target_link_libraries(Terrain nativefiledialog.lib)
    target_link_libraries(Terrain rttr.lib)
    target_link_libraries(Terrain tinyxml.lib)   
    target_link_libraries(Terrain yaml-cpp.lib)
    target_link_libraries(Terrain zlib.lib)
    target_link_libraries(Terrain zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(Terrain tbb_static.lib)
			target_link_libraries(Terrain tbbmalloc_static.lib)
		else()
			target_link_libraries(Terrain tbb.lib)
			target_link_libraries(Terrain tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(Terrain libWPCore.a)
    target_link_libraries(Terrain libWPMesh.a)
    target_link_libraries(Terrain libWPProcedural.a)
    target_link_libraries(Terrain libWPPhysx.a)
    target_link_libraries(Terrain libWPAero.a)
    target_link_libraries(Terrain libWPVehicle.a)
    target_link_libraries(Terrain libWPAudio.a)
    target_link_libraries(Terrain libWPGraphics.a)
    target_link_libraries(Terrain libWPAssimp.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(Terrain WPGraphicsOgreNext.a)

        target_link_libraries(Terrain harWPuzz.a)
        target_link_libraries(Terrain harWPuzz-subset.a)
        target_link_libraries(Terrain icucommon.a)
        target_link_libraries(Terrain sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(Terrain WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(Terrain libWPGraphicsSoftware.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(Terrain FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(Terrain freetype.a)
    endif()

    target_link_libraries(Terrain libWPSQLite.a)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(Terrain WPOISInput.a)
    endif()

    target_link_libraries(Terrain libboost_thread.a)
    target_link_libraries(Terrain libboost_date_time.a)
    target_link_libraries(Terrain libboost_chrono.a)
    target_link_libraries(Terrain libboost_context.a)
    target_link_libraries(Terrain libboost_json.a)
    target_link_libraries(Terrain libboost_timer.a)
    target_link_libraries(Terrain libboost_filesystem.a)
    target_link_libraries(Terrain libboost_unit_test_framework.a)

    target_link_libraries(Terrain libPxTask.a)
    target_link_libraries(Terrain libPhysXVisualDebuggerSDK.a)
    target_link_libraries(Terrain libPvdRuntime.a)

    if (WP_USE_TBB)
        target_link_libraries(Terrain libtbb.a)
        target_link_libraries(Terrain libtbbmalloc.a)
    endif()

    target_link_libraries(Terrain minizip.a)
    target_link_libraries(Terrain nativefiledialog.a)
    target_link_libraries(Terrain rttr.a)
    target_link_libraries(Terrain tinyxml.a)
    target_link_libraries(Terrain yaml-cpp.a)
    target_link_libraries(Terrain zlib.a)
    target_link_libraries(Terrain zzip.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(Terrain libWm5Core.a)
        target_link_libraries(Terrain libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(Terrain libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(Terrain -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(Terrain stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(Terrain PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(Terrain -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(Terrain libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(Terrain ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(Terrain ${CMAKE_DL_LIBS})
endif ()
else()
if (WIN32)
    target_link_libraries(Terrain Workphone.lib)
    target_link_libraries(Terrain WPMesh.lib)
    target_link_libraries(Terrain WPPhysx.lib)
    target_link_libraries(Terrain WPSQLite.lib)
    target_link_libraries(Terrain WPVehicle.lib)
    target_link_libraries(Terrain WPOISInput.lib)
    target_link_libraries(Terrain WPGraphics.lib)
    target_link_libraries(Terrain WPRenderUI.lib)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(Terrain WPOISInput.lib)

        #target_link_libraries(Terrain debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(Terrain dinput8.lib)
        target_link_libraries(Terrain dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(Terrain libWPxsdk.lib)
        target_link_libraries(Terrain libWPxsdk-mt.lib)
        target_link_libraries(Terrain libxml2-mt.lib)
        target_link_libraries(Terrain tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(Terrain FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(Terrain freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(Terrain WPGraphicsOgreNext.lib)

        target_link_libraries(Terrain harWPuzz.lib)
        target_link_libraries(Terrain harWPuzz-subset.lib)
        target_link_libraries(Terrain icucommon.lib)
        target_link_libraries(Terrain sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(Terrain d3d11.lib)
            target_link_libraries(Terrain dxgi.lib)
            target_link_libraries(Terrain dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(Terrain opengl32.lib)
            target_link_libraries(Terrain glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(Terrain amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(Terrain WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(Terrain debug OgreMainStatic_d.lib
                                optimized OgreMainStatic.lib)

            target_link_libraries(Terrain debug OgreOverlayStatic_d.lib
                                optimized OgreOverlayStatic.lib)
        
            target_link_libraries(Terrain debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(Terrain debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(Terrain debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(Terrain debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(Terrain debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            #target_link_libraries(Terrain OgreHLMSStatic_d.lib)

            target_link_libraries(Terrain debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(Terrain debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(Terrain debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            #target_link_libraries(Terrain debug OgreTerrainStatic_d.lib
            #                            optimized OgreTerrainStatic.lib)
    
            #target_link_libraries(Terrain Plugin_CgProgramManagerStatic_d.lib)
            #target_link_libraries(Terrain cg.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(Terrain debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(Terrain freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(Terrain d3d9.lib)
            target_link_libraries(Terrain d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(Terrain d3d11.lib)
            target_link_libraries(Terrain dxgi.lib)
            target_link_libraries(Terrain dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(Terrain opengl32.lib)
            target_link_libraries(Terrain glu32.lib)
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(Terrain opengl32.lib)
            target_link_libraries(Terrain glu32.lib)
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(Terrain WPGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(Terrain WPPhysx.lib)

        target_link_libraries(Terrain PhysX3.lib)
        target_link_libraries(Terrain PhysX3Extensions.lib)
        target_link_libraries(Terrain PhysX3Common.lib)
        target_link_libraries(Terrain PhysX3Vehicle.lib)
        target_link_libraries(Terrain PhysX3Cooking.lib)
        target_link_libraries(Terrain PhysXProfileSDK.lib)
        target_link_libraries(Terrain PxTask.lib)
        target_link_libraries(Terrain PhysXVisualDebuggerSDK.lib)
        target_link_libraries(Terrain PhysX3MetaData.lib)
        target_link_libraries(Terrain PhysX3Foundation.lib)
        target_link_libraries(Terrain PxSceneQuery.lib)
        target_link_libraries(Terrain PxSimulationController.lib)
        target_link_libraries(Terrain PhysX3LowLevel.lib)
        target_link_libraries(Terrain PhysX3LowLevelCloth.lib)
    endif()

    target_link_libraries(Terrain bcrypt.lib)
    target_link_libraries(Terrain libboost_thread.lib)
    target_link_libraries(Terrain libboost_date_time.lib)
    target_link_libraries(Terrain libboost_chrono.lib)
    target_link_libraries(Terrain libboost_timer.lib)
    target_link_libraries(Terrain libboost_filesystem.lib)
    target_link_libraries(Terrain libboost_json.lib)
    target_link_libraries(Terrain minizip.lib)
    target_link_libraries(Terrain nativefiledialog.lib)
    target_link_libraries(Terrain rttr.lib)
    target_link_libraries(Terrain tinyxml.lib)   
    target_link_libraries(Terrain yaml-cpp.lib)
    target_link_libraries(Terrain zlib.lib)
    target_link_libraries(Terrain zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(Terrain tbb_static.lib)
			target_link_libraries(Terrain tbbmalloc_static.lib)
		else()
			target_link_libraries(Terrain tbb.lib)
			target_link_libraries(Terrain tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(Terrain libWPCore.a)
    target_link_libraries(Terrain libWPMesh.a)
    target_link_libraries(Terrain libWPProcedural.a)
    target_link_libraries(Terrain libWPPhysx.a)
    target_link_libraries(Terrain libWPAero.a)
    target_link_libraries(Terrain libWPVehicle.a)
    target_link_libraries(Terrain libWPAudio.a)
    target_link_libraries(Terrain libWPGraphics.a)
    target_link_libraries(Terrain libWPAssimp.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(Terrain WPGraphicsOgreNext.a)

        target_link_libraries(Terrain harWPuzz.a)
        target_link_libraries(Terrain harWPuzz-subset.a)
        target_link_libraries(Terrain icucommon.a)
        target_link_libraries(Terrain sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(Terrain WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(Terrain libWPGraphicsSoftware.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(Terrain FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(Terrain freetype.a)
    endif()

    target_link_libraries(Terrain libWPSQLite.a)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(Terrain WPOISInput.a)
    endif()

    target_link_libraries(Terrain libboost_thread.a)
    target_link_libraries(Terrain libboost_date_time.a)
    target_link_libraries(Terrain libboost_chrono.a)
    target_link_libraries(Terrain libboost_context.a)
    target_link_libraries(Terrain libboost_json.a)
    target_link_libraries(Terrain libboost_timer.a)
    target_link_libraries(Terrain libboost_filesystem.a)
    target_link_libraries(Terrain libboost_unit_test_framework.a)

    target_link_libraries(Terrain libPxTask.a)
    target_link_libraries(Terrain libPhysXVisualDebuggerSDK.a)
    target_link_libraries(Terrain libPvdRuntime.a)

    if (WP_USE_TBB)
        target_link_libraries(Terrain libtbb.a)
        target_link_libraries(Terrain libtbbmalloc.a)
    endif()

    target_link_libraries(Terrain minizip.a)
    target_link_libraries(Terrain nativefiledialog.a)
    target_link_libraries(Terrain rttr.a)
    target_link_libraries(Terrain tinyxml.a)
    target_link_libraries(Terrain yaml-cpp.a)
    target_link_libraries(Terrain zlib.a)
    target_link_libraries(Terrain zzip.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(Terrain libWm5Core.a)
        target_link_libraries(Terrain libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(Terrain libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(Terrain -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(Terrain stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(Terrain PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(Terrain -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(Terrain libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(Terrain ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(Terrain ${CMAKE_DL_LIBS})
endif ()
endif ()

if (WIN32)
    set_property(TARGET Terrain PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(Terrain Workphone)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
        add_dependencies(Terrain WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(Terrain WPGraphicsOgre)
    endif()

    if (WP_BUILD_ODE)
        add_dependencies(Terrain WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(Terrain WPPhysx)
    endif()
endif()

set_target_properties (Terrain PROPERTIES FOLDER Tests/Cpp )
