set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(OverlayTest)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (FREEIMAGE_LIB)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

if (WIN32)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        
        if (WP_PLATFORM_X64)
            link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/Dependencies/lib/${CMAKE_BUILD_TYPE})
    endif ()
elseif (UNIX)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(OverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(OverlayTest ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(OverlayTest PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(OverlayTest PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(OverlayTest Workphone.lib)

    target_link_libraries(OverlayTest bcrypt.lib)
    target_link_libraries(OverlayTest libboost_thread.lib)
    target_link_libraries(OverlayTest libboost_date_time.lib)
    target_link_libraries(OverlayTest libboost_chrono.lib)
    target_link_libraries(OverlayTest libboost_timer.lib)
    target_link_libraries(OverlayTest libboost_filesystem.lib)
    target_link_libraries(OverlayTest libboost_json.lib)
    target_link_libraries(OverlayTest minizip.lib)
    target_link_libraries(OverlayTest nativefiledialog.lib)
    target_link_libraries(OverlayTest rttr.lib)
    target_link_libraries(OverlayTest tinyxml.lib)   
    target_link_libraries(OverlayTest yaml-cpp.lib)
    target_link_libraries(OverlayTest zlib.lib)
    target_link_libraries(OverlayTest zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(OverlayTest tbb_static.lib)
			target_link_libraries(OverlayTest tbbmalloc_static.lib)
		else()
			target_link_libraries(OverlayTest tbb.lib)
			target_link_libraries(OverlayTest tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(OverlayTest libWPCore.a)
    target_link_libraries(OverlayTest libWPMesh.a)
    target_link_libraries(OverlayTest libWPProcedural.a)
    target_link_libraries(OverlayTest libWPPhysx.a)
    target_link_libraries(OverlayTest libWPAero.a)
    target_link_libraries(OverlayTest libWPVehicle.a)
    target_link_libraries(OverlayTest libWPAudio.a)
    target_link_libraries(OverlayTest WPGraphics.a)
    target_link_libraries(OverlayTest WPRenderUI.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(OverlayTest FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(OverlayTest freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(OverlayTest WPGraphicsOgreNext.a)
        target_link_libraries(OverlayTest harWPuzz.a)
        target_link_libraries(OverlayTest harWPuzz-subset.a)
        target_link_libraries(OverlayTest icucommon.a)
        target_link_libraries(OverlayTest sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(OverlayTest WPGraphicsOgre.a)
    endif()

    target_link_libraries(OverlayTest debug libassimpd.a optimized libassimp.a)

    if (WP_BUILD_PHYSX)
        target_link_libraries(OverlayTest libWPPhysx.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(OverlayTest libWPODE3.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(OverlayTest libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a)

        target_link_libraries(OverlayTest libPxTask.a)
        target_link_libraries(OverlayTest libPhysXVisualDebuggerSDK.a)
        target_link_libraries(OverlayTest libPvdRuntime.a)
    endif()

    target_link_libraries(OverlayTest libWPSQLite.a)

    target_link_libraries(OverlayTest libboost_thread.a)
    target_link_libraries(OverlayTest libboost_date_time.a)
    target_link_libraries(OverlayTest libboost_chrono.a)
    target_link_libraries(OverlayTest libboost_json.a)
    target_link_libraries(OverlayTest libboost_timer.a)
    target_link_libraries(OverlayTest libboost_filesystem.a)
    target_link_libraries(OverlayTest libboost_unit_test_framework.a)

    target_link_libraries(OverlayTest minizip.a)
    target_link_libraries(OverlayTest nativefiledialog.a)
    target_link_libraries(OverlayTest rttr.a)
    target_link_libraries(OverlayTest tinyxml.a)
    target_link_libraries(OverlayTest yaml-cpp.a)
    target_link_libraries(OverlayTest zlib.a)
    target_link_libraries(OverlayTest zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(OverlayTest tbb_static.a)
        target_link_libraries(OverlayTest tbbmalloc_static.a)
    endif ()

    #target_link_libraries(OverlayTest stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(OverlayTest ${AUDIO_UNIT})
    target_link_libraries(OverlayTest stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(OverlayTest PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(OverlayTest -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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
        target_link_libraries(OverlayTest libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(OverlayTest ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(OverlayTest ${CMAKE_DL_LIBS})
endif ()
else()
if (WIN32)
    target_link_libraries(OverlayTest Workphone.lib)
    target_link_libraries(OverlayTest WPMesh.lib)
    target_link_libraries(OverlayTest WPProcedural.lib)
    target_link_libraries(OverlayTest WPPhysx.lib)
    target_link_libraries(OverlayTest WPSQLite.lib)
    target_link_libraries(OverlayTest WPVehicle.lib)
    target_link_libraries(OverlayTest WPWxWidgets.lib)
    target_link_libraries(OverlayTest WPOISInput.lib)
    target_link_libraries(OverlayTest WPGraphics.lib)
    target_link_libraries(OverlayTest WPRenderUI.lib)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(OverlayTest debug FreeImage_d.lib optimized FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(OverlayTest debug freetype_d.lib optimized freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(OverlayTest WPGraphicsOgreNext.lib)

        target_link_libraries(OverlayTest harWPuzz.lib)
        target_link_libraries(OverlayTest harWPuzz-subset.lib)
        target_link_libraries(OverlayTest icucommon.lib)
        target_link_libraries(OverlayTest sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(OverlayTest d3d11.lib)
            target_link_libraries(OverlayTest dxgi.lib)
            target_link_libraries(OverlayTest dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(OverlayTest opengl32.lib)
            target_link_libraries(OverlayTest glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            #target_link_libraries(OverlayTest amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(OverlayTest WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(OverlayTest debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(OverlayTest debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(OverlayTest debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(OverlayTest debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(OverlayTest debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(OverlayTest debug RenderSystem_GLStatic_d.lib 
                                optimized RenderSystem_GLStatic.lib)

                            
            target_link_libraries(OverlayTest debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(OverlayTest debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(OverlayTest debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(OverlayTest debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(OverlayTest debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)
        endif()
    
        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(OverlayTest debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(OverlayTest freetype.lib)   
        
        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(OverlayTest d3d9.lib)
            target_link_libraries(OverlayTest d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(OverlayTest d3d11.lib)
            target_link_libraries(OverlayTest dxgi.lib)
            target_link_libraries(OverlayTest dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(OverlayTest opengl32.lib)
            target_link_libraries(OverlayTest glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(OverlayTest amd_ags_x64.lib)
            endif ()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(OverlayTest opengl32.lib)
            target_link_libraries(OverlayTest glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(OverlayTest amd_ags_x64.lib)
            endif ()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(OverlayTest WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(OverlayTest bcrypt.lib)
    target_link_libraries(OverlayTest libboost_thread.lib)
    target_link_libraries(OverlayTest libboost_date_time.lib)
    target_link_libraries(OverlayTest libboost_chrono.lib)
    target_link_libraries(OverlayTest libboost_timer.lib)
    target_link_libraries(OverlayTest libboost_filesystem.lib)
    target_link_libraries(OverlayTest libboost_json.lib)
    target_link_libraries(OverlayTest minizip.lib)
    target_link_libraries(OverlayTest nativefiledialog.lib)
    target_link_libraries(OverlayTest rttr.lib)
    target_link_libraries(OverlayTest tinyxml.lib)   
    target_link_libraries(OverlayTest yaml-cpp.lib)
    target_link_libraries(OverlayTest zlib.lib)
    target_link_libraries(OverlayTest zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(OverlayTest tbb_static.lib)
			target_link_libraries(OverlayTest tbbmalloc_static.lib)
		else()
			target_link_libraries(OverlayTest tbb.lib)
			target_link_libraries(OverlayTest tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(OverlayTest libWPCore.a)
    target_link_libraries(OverlayTest libWPMesh.a)
    target_link_libraries(OverlayTest libWPProcedural.a)
    target_link_libraries(OverlayTest libWPPhysx.a)
    target_link_libraries(OverlayTest libWPAero.a)
    target_link_libraries(OverlayTest libWPVehicle.a)
    target_link_libraries(OverlayTest libWPAudio.a)
    target_link_libraries(OverlayTest WPGraphics.a)
    target_link_libraries(OverlayTest WPRenderUI.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(OverlayTest FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(OverlayTest freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(OverlayTest WPGraphicsOgreNext.a)
        target_link_libraries(OverlayTest harWPuzz.a)
        target_link_libraries(OverlayTest harWPuzz-subset.a)
        target_link_libraries(OverlayTest icucommon.a)
        target_link_libraries(OverlayTest sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(OverlayTest WPGraphicsOgre.a)
    endif()

    target_link_libraries(OverlayTest debug libassimpd.a optimized libassimp.a)

    if (WP_BUILD_PHYSX)
        target_link_libraries(OverlayTest libWPPhysx.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(OverlayTest libWPODE3.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(OverlayTest libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a)

        target_link_libraries(OverlayTest libPxTask.a)
        target_link_libraries(OverlayTest libPhysXVisualDebuggerSDK.a)
        target_link_libraries(OverlayTest libPvdRuntime.a)
    endif()

    target_link_libraries(OverlayTest libWPSQLite.a)

    target_link_libraries(OverlayTest libboost_thread.a)
    target_link_libraries(OverlayTest libboost_date_time.a)
    target_link_libraries(OverlayTest libboost_chrono.a)
    target_link_libraries(OverlayTest libboost_json.a)
    target_link_libraries(OverlayTest libboost_timer.a)
    target_link_libraries(OverlayTest libboost_filesystem.a)
    target_link_libraries(OverlayTest libboost_unit_test_framework.a)

    target_link_libraries(OverlayTest minizip.a)
    target_link_libraries(OverlayTest nativefiledialog.a)
    target_link_libraries(OverlayTest rttr.a)
    target_link_libraries(OverlayTest tinyxml.a)
    target_link_libraries(OverlayTest yaml-cpp.a)
    target_link_libraries(OverlayTest zlib.a)
    target_link_libraries(OverlayTest zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(OverlayTest tbb_static.a)
        target_link_libraries(OverlayTest tbbmalloc_static.a)
    endif ()

    #target_link_libraries(OverlayTest stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(OverlayTest ${AUDIO_UNIT})
    target_link_libraries(OverlayTest stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(OverlayTest PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(OverlayTest -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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
        target_link_libraries(OverlayTest libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(OverlayTest ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(OverlayTest ${CMAKE_DL_LIBS})
endif ()
endif ()

if (WIN32)
    set_property(TARGET OverlayTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(OverlayTest Workphone)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
        add_dependencies(OverlayTest WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(OverlayTest WPGraphicsOgre)
    endif()

    if (WP_BUILD_ODE)
        add_dependencies(OverlayTest WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(OverlayTest WPPhysx)
    endif()
endif()

set_target_properties (OverlayTest PROPERTIES FOLDER Tests/Cpp)
