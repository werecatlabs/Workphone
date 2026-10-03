set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SamplePhysics)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

if (WIN32)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(SamplePhysics ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(SamplePhysics ${HEADER_FILES} ${SOURCE_FILES})

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(SamplePhysics Workphone.lib)  
        target_link_libraries(SamplePhysics WPSQLite.lib)

        if (WP_BUILD_OISINPUT)
            target_link_libraries(SamplePhysics WPOISInput.lib)
            #target_link_libraries(SamplePhysics debug OIS_d.lib optimized OIS.lib)
            target_link_libraries(SamplePhysics dinput8.lib)
            target_link_libraries(SamplePhysics dxguid.lib)
        endif ()

        if (WP_USE_WPXSDK)
            #target_link_libraries(SamplePhysics libWPxsdk.lib)
            target_link_libraries(SamplePhysics libWPxsdk-mt.lib)
            target_link_libraries(SamplePhysics libxml2-mt.lib)
            target_link_libraries(SamplePhysics tinyxml2.lib)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SamplePhysics WPPhysx.lib)

            target_link_libraries(SamplePhysics PhysX3.lib)
            target_link_libraries(SamplePhysics PhysX3Extensions.lib)
            target_link_libraries(SamplePhysics PhysX3Common.lib)
            target_link_libraries(SamplePhysics PhysX3Vehicle.lib)
            target_link_libraries(SamplePhysics PhysX3Cooking.lib)
            target_link_libraries(SamplePhysics PhysXProfileSDK.lib)
            target_link_libraries(SamplePhysics PxTask.lib)
            target_link_libraries(SamplePhysics PhysXVisualDebuggerSDK.lib)
            target_link_libraries(SamplePhysics PhysX3MetaData.lib)
            target_link_libraries(SamplePhysics PhysX3Foundation.lib)
            target_link_libraries(SamplePhysics PxSceneQuery.lib)
            target_link_libraries(SamplePhysics PxSimulationController.lib)
            target_link_libraries(SamplePhysics PhysX3LowLevel.lib)
            target_link_libraries(SamplePhysics PhysX3LowLevelCloth.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SamplePhysics FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SamplePhysics freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SamplePhysics WPGraphicsOgreNext.lib)

            target_link_libraries(SamplePhysics harfbuzz.lib)
            target_link_libraries(SamplePhysics harfbuzz-subset.lib)
            target_link_libraries(SamplePhysics icucommon.lib)
            target_link_libraries(SamplePhysics sds_library.lib)
    
            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SamplePhysics d3d11.lib)
                target_link_libraries(SamplePhysics dxgi.lib)
                target_link_libraries(SamplePhysics dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SamplePhysics WPGraphicsOgre.lib)

            if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
                target_link_libraries(SamplePhysics debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
                target_link_libraries(SamplePhysics debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_Direct3D9Static_d.lib
                                    optimized RenderSystem_Direct3D9Static.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_Direct3D11Static_d.lib
                                    optimized RenderSystem_Direct3D11Static.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_GL3PlusStatic_d.lib 
                                    optimized RenderSystem_GL3PlusStatic.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_GLStatic_d.lib 
                                    optimized RenderSystem_GLStatic.lib)

                            
                target_link_libraries(SamplePhysics debug OgreGLSupportStatic_d.lib
                                    optimized OgreGLSupportStatic.lib)

                target_link_libraries(SamplePhysics debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

                target_link_libraries(SamplePhysics debug OgreRTShaderSystemStatic_d.lib
                                           optimized OgreRTShaderSystemStatic.lib)

                target_link_libraries(SamplePhysics debug Codec_STBIStatic_d.lib
                                           optimized Codec_STBIStatic.lib)

                target_link_libraries(SamplePhysics debug OgreBitesStatic_d.lib
                                           optimized OgreBitesStatic.lib)
            endif()

            #target_link_libraries(SamplePhysics freetype.lib)   
        
            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(SamplePhysics d3d9.lib)
                target_link_libraries(SamplePhysics d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SamplePhysics d3d11.lib)
                target_link_libraries(SamplePhysics dxgi.lib)
                target_link_libraries(SamplePhysics dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()
        elseif(WP_BUILD_DX11)
            target_link_libraries(SamplePhysics WPGraphicsD3D11.lib)
        endif()

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SamplePhysics tbb_static.lib)
                target_link_libraries(SamplePhysics tbbmalloc_static.lib)
            else()
                target_link_libraries(SamplePhysics tbb.lib)
                target_link_libraries(SamplePhysics tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(SamplePhysics bcrypt.lib)
        target_link_libraries(SamplePhysics libboost_thread.lib)
        target_link_libraries(SamplePhysics libboost_date_time.lib)
        target_link_libraries(SamplePhysics libboost_chrono.lib)
        target_link_libraries(SamplePhysics libboost_timer.lib)
        target_link_libraries(SamplePhysics libboost_filesystem.lib)
        target_link_libraries(SamplePhysics libboost_json.lib)
        target_link_libraries(SamplePhysics nativefiledialog.lib)
        target_link_libraries(SamplePhysics rttr.lib)
        target_link_libraries(SamplePhysics tinyxml.lib)    
        target_link_libraries(SamplePhysics yaml-cpp.lib)
        target_link_libraries(SamplePhysics minizip.lib)
        target_link_libraries(SamplePhysics zlib.lib)
        target_link_libraries(SamplePhysics zzip.lib)

        target_link_libraries(SamplePhysics minizip.lib)
        target_link_libraries(SamplePhysics nativefiledialog.lib)
        target_link_libraries(SamplePhysics rttr.lib)
        target_link_libraries(SamplePhysics tinyxml.lib)
        target_link_libraries(SamplePhysics yaml-cpp.lib)
        target_link_libraries(SamplePhysics zlib.lib)
        target_link_libraries(SamplePhysics zzip.lib)
    elseif (APPLE)
        target_link_libraries(SamplePhysics libWPCore.a)
        target_link_libraries(SamplePhysics libWPGraphics.a)
        target_link_libraries(SamplePhysics libWPMesh.a)
        target_link_libraries(SamplePhysics libWPPhysx.a)
        target_link_libraries(SamplePhysics libWPAudio.a)
        target_link_libraries(SamplePhysics libWPRenderUI.a)

        if (WP_BUILD_OISINPUT)
            target_link_libraries(SamplePhysics WPOISInput.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SamplePhysics FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SamplePhysics freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SamplePhysics WPGraphicsOgreNext.a)

            target_link_libraries(SamplePhysics harWPuzz.a)
            target_link_libraries(SamplePhysics harWPuzz-subset.a)
            target_link_libraries(SamplePhysics icucommon.a)
            target_link_libraries(SamplePhysics sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SamplePhysics WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SamplePhysics libWPGraphicsSoftware.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SamplePhysics WPPhysx.a)

            target_link_libraries(SamplePhysics PhysX3.a)
            target_link_libraries(SamplePhysics PhysX3Extensions.a)
            target_link_libraries(SamplePhysics PhysX3Common.a)
            target_link_libraries(SamplePhysics PhysX3Vehicle.a)
            target_link_libraries(SamplePhysics PhysX3Cooking.a)
            target_link_libraries(SamplePhysics PhysXProfileSDK.a)
            target_link_libraries(SamplePhysics PxTask.a)
            target_link_libraries(SamplePhysics PhysXVisualDebuggerSDK.a)
            target_link_libraries(SamplePhysics PhysX3MetaData.a)
            target_link_libraries(SamplePhysics PhysX3Foundation.a)
            target_link_libraries(SamplePhysics PxSceneQuery.a)
            target_link_libraries(SamplePhysics PxSimulationController.a)
            target_link_libraries(SamplePhysics PhysX3LowLevel.a)
            target_link_libraries(SamplePhysics PhysX3LowLevelCloth.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(SamplePhysics libWPODE3.a)
        endif()

        target_link_libraries(SamplePhysics libWPSQLite.a)

        target_link_libraries(SamplePhysics libboost_thread.a)
        target_link_libraries(SamplePhysics libboost_date_time.a)
        target_link_libraries(SamplePhysics libboost_chrono.a)
        target_link_libraries(SamplePhysics libboost_context.a)
        target_link_libraries(SamplePhysics libboost_timer.a)
        target_link_libraries(SamplePhysics libboost_filesystem.a)
        target_link_libraries(SamplePhysics libboost_unit_test_framework.a)
        target_link_libraries(SamplePhysics libboost_json.a)

        target_link_libraries(SamplePhysics minizip.a)
        target_link_libraries(SamplePhysics nativefiledialog.a)
        target_link_libraries(SamplePhysics rttr.a)
        target_link_libraries(SamplePhysics tinyxml.a)
        target_link_libraries(SamplePhysics yaml-cpp.a)
        target_link_libraries(SamplePhysics zlib.a)
        target_link_libraries(SamplePhysics zzip.a)

        if (WP_USE_TBB)
            target_link_libraries(SamplePhysics libtbb_static.a)
            target_link_libraries(SamplePhysics libtbbmalloc_static.a)
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SamplePhysics libWm5Core.a)
            target_link_libraries(SamplePhysics libWm5Mathematics.a)
        endif()

        target_link_libraries(SamplePhysics debug libassimpd.a optimized libassimp.a)

        # physx libraries
        target_link_libraries(SamplePhysics libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )

        #target_link_libraries(OverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OverlayTest ${AUDIO_UNIT})
        target_link_libraries(SamplePhysics stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SamplePhysics PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else (UNIX)
        target_link_libraries(SamplePhysics -Wl,--start-group WPCore.a WPGraphics.a
            libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a  
            WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
            png Xaw Xrandr Xt xcb GL
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

        #target_link_libraries(SamplePhysics -Wl,--start-group WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -Wl,--end-group )

        target_link_libraries(SamplePhysics freetype.a)

        target_link_libraries(SamplePhysics png)
        target_link_libraries(SamplePhysics Xaw)
        target_link_libraries(SamplePhysics Xrandr)
        target_link_libraries(SamplePhysics Xt)
        target_link_libraries(SamplePhysics xcb)
        target_link_libraries(SamplePhysics ${X11_LIBRARIES})
        target_link_libraries(SamplePhysics ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SamplePhysics ${CMAKE_DL_LIBS})
    endif ()
else()
    if (WIN32)
        target_link_libraries(SamplePhysics Workphone.lib)    
        target_link_libraries(SamplePhysics WPMesh.lib)
        target_link_libraries(SamplePhysics WPSQLite.lib)
        target_link_libraries(SamplePhysics WPGraphics.lib)
        target_link_libraries(SamplePhysics WPRenderUI.lib)

        if (WP_BUILD_OISINPUT)
            target_link_libraries(SamplePhysics WPOISInput.lib)
            #target_link_libraries(SamplePhysics debug OIS_d.lib optimized OIS.lib)
            target_link_libraries(SamplePhysics dinput8.lib)
            target_link_libraries(SamplePhysics dxguid.lib)
        endif ()

        if (WP_USE_WPXSDK)
            #target_link_libraries(SamplePhysics libWPxsdk.lib)
            target_link_libraries(SamplePhysics libWPxsdk-mt.lib)
            target_link_libraries(SamplePhysics libxml2-mt.lib)
            target_link_libraries(SamplePhysics tinyxml2.lib)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SamplePhysics WPPhysx.lib)

            target_link_libraries(SamplePhysics PhysX3.lib)
            target_link_libraries(SamplePhysics PhysX3Extensions.lib)
            target_link_libraries(SamplePhysics PhysX3Common.lib)
            target_link_libraries(SamplePhysics PhysX3Vehicle.lib)
            target_link_libraries(SamplePhysics PhysX3Cooking.lib)
            target_link_libraries(SamplePhysics PhysXProfileSDK.lib)
            target_link_libraries(SamplePhysics PxTask.lib)
            target_link_libraries(SamplePhysics PhysXVisualDebuggerSDK.lib)
            target_link_libraries(SamplePhysics PhysX3MetaData.lib)
            target_link_libraries(SamplePhysics PhysX3Foundation.lib)
            target_link_libraries(SamplePhysics PxSceneQuery.lib)
            target_link_libraries(SamplePhysics PxSimulationController.lib)
            target_link_libraries(SamplePhysics PhysX3LowLevel.lib)
            target_link_libraries(SamplePhysics PhysX3LowLevelCloth.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SamplePhysics FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SamplePhysics freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SamplePhysics WPGraphicsOgreNext.lib)

            target_link_libraries(SamplePhysics harWPuzz.lib)
            target_link_libraries(SamplePhysics harWPuzz-subset.lib)
            target_link_libraries(SamplePhysics icucommon.lib)
            target_link_libraries(SamplePhysics sds_library.lib)
    
            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SamplePhysics d3d11.lib)
                target_link_libraries(SamplePhysics dxgi.lib)
                target_link_libraries(SamplePhysics dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SamplePhysics WPGraphicsOgre.lib)

            if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
                target_link_libraries(SamplePhysics debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
                target_link_libraries(SamplePhysics debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_Direct3D9Static_d.lib
                                    optimized RenderSystem_Direct3D9Static.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_Direct3D11Static_d.lib
                                    optimized RenderSystem_Direct3D11Static.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_GL3PlusStatic_d.lib 
                                    optimized RenderSystem_GL3PlusStatic.lib)

                target_link_libraries(SamplePhysics debug RenderSystem_GLStatic_d.lib 
                                    optimized RenderSystem_GLStatic.lib)

                            
                target_link_libraries(SamplePhysics debug OgreGLSupportStatic_d.lib
                                    optimized OgreGLSupportStatic.lib)

                target_link_libraries(SamplePhysics debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

                target_link_libraries(SamplePhysics debug OgreRTShaderSystemStatic_d.lib
                                           optimized OgreRTShaderSystemStatic.lib)

                target_link_libraries(SamplePhysics debug Codec_STBIStatic_d.lib
                                           optimized Codec_STBIStatic.lib)

                target_link_libraries(SamplePhysics debug OgreBitesStatic_d.lib
                                           optimized OgreBitesStatic.lib)
            endif()

            #target_link_libraries(SamplePhysics freetype.lib)   
        
            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(SamplePhysics d3d9.lib)
                target_link_libraries(SamplePhysics d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SamplePhysics d3d11.lib)
                target_link_libraries(SamplePhysics dxgi.lib)
                target_link_libraries(SamplePhysics dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SamplePhysics opengl32.lib)
                target_link_libraries(SamplePhysics glu32.lib)
            endif ()
        elseif(WP_BUILD_DX11)
            target_link_libraries(SamplePhysics WPGraphicsD3D11.lib)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(SamplePhysics tbb.lib)
            target_link_libraries(SamplePhysics tbbmalloc.lib)
        endif()

        target_link_libraries(SamplePhysics bcrypt.lib)
        target_link_libraries(SamplePhysics libboost_thread.lib)
        target_link_libraries(SamplePhysics libboost_date_time.lib)
        target_link_libraries(SamplePhysics libboost_chrono.lib)
        target_link_libraries(SamplePhysics libboost_timer.lib)
        target_link_libraries(SamplePhysics libboost_filesystem.lib)
        target_link_libraries(SamplePhysics libboost_json.lib)
        target_link_libraries(SamplePhysics nativefiledialog.lib)
        target_link_libraries(SamplePhysics rttr.lib)
        target_link_libraries(SamplePhysics tinyxml.lib)    
        target_link_libraries(SamplePhysics yaml-cpp.lib)
        target_link_libraries(SamplePhysics minizip.lib)
        target_link_libraries(SamplePhysics zlib.lib)
        target_link_libraries(SamplePhysics zzip.lib)

        target_link_libraries(SamplePhysics minizip.lib)
        target_link_libraries(SamplePhysics nativefiledialog.lib)
        target_link_libraries(SamplePhysics rttr.lib)
        target_link_libraries(SamplePhysics tinyxml.lib)
        target_link_libraries(SamplePhysics yaml-cpp.lib)
        target_link_libraries(SamplePhysics zlib.lib)
        target_link_libraries(SamplePhysics zzip.lib)
    elseif (APPLE)
        target_link_libraries(SamplePhysics libWPCore.a)
        target_link_libraries(SamplePhysics libWPGraphics.a)
        target_link_libraries(SamplePhysics libWPMesh.a)
        target_link_libraries(SamplePhysics libWPPhysx.a)
        target_link_libraries(SamplePhysics libWPAudio.a)
        target_link_libraries(SamplePhysics libWPRenderUI.a)

        if (WP_BUILD_OISINPUT)
            target_link_libraries(SamplePhysics WPOISInput.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SamplePhysics FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SamplePhysics freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SamplePhysics WPGraphicsOgreNext.a)

            target_link_libraries(SamplePhysics harWPuzz.a)
            target_link_libraries(SamplePhysics harWPuzz-subset.a)
            target_link_libraries(SamplePhysics icucommon.a)
            target_link_libraries(SamplePhysics sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SamplePhysics WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SamplePhysics libWPGraphicsSoftware.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SamplePhysics WPPhysx.a)

            target_link_libraries(SamplePhysics PhysX3.a)
            target_link_libraries(SamplePhysics PhysX3Extensions.a)
            target_link_libraries(SamplePhysics PhysX3Common.a)
            target_link_libraries(SamplePhysics PhysX3Vehicle.a)
            target_link_libraries(SamplePhysics PhysX3Cooking.a)
            target_link_libraries(SamplePhysics PhysXProfileSDK.a)
            target_link_libraries(SamplePhysics PxTask.a)
            target_link_libraries(SamplePhysics PhysXVisualDebuggerSDK.a)
            target_link_libraries(SamplePhysics PhysX3MetaData.a)
            target_link_libraries(SamplePhysics PhysX3Foundation.a)
            target_link_libraries(SamplePhysics PxSceneQuery.a)
            target_link_libraries(SamplePhysics PxSimulationController.a)
            target_link_libraries(SamplePhysics PhysX3LowLevel.a)
            target_link_libraries(SamplePhysics PhysX3LowLevelCloth.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(SamplePhysics libWPODE3.a)
        endif()

        target_link_libraries(SamplePhysics libWPSQLite.a)

        target_link_libraries(SamplePhysics libboost_thread.a)
        target_link_libraries(SamplePhysics libboost_date_time.a)
        target_link_libraries(SamplePhysics libboost_chrono.a)
        target_link_libraries(SamplePhysics libboost_context.a)
        target_link_libraries(SamplePhysics libboost_timer.a)
        target_link_libraries(SamplePhysics libboost_filesystem.a)
        target_link_libraries(SamplePhysics libboost_unit_test_framework.a)
        target_link_libraries(SamplePhysics libboost_json.a)

        target_link_libraries(SamplePhysics minizip.a)
        target_link_libraries(SamplePhysics nativefiledialog.a)
        target_link_libraries(SamplePhysics rttr.a)
        target_link_libraries(SamplePhysics tinyxml.a)
        target_link_libraries(SamplePhysics yaml-cpp.a)
        target_link_libraries(SamplePhysics zlib.a)
        target_link_libraries(SamplePhysics zzip.a)

        if (WP_USE_TBB)
            target_link_libraries(SamplePhysics libtbb_static.a)
            target_link_libraries(SamplePhysics libtbbmalloc_static.a)
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SamplePhysics libWm5Core.a)
            target_link_libraries(SamplePhysics libWm5Mathematics.a)
        endif()

        target_link_libraries(SamplePhysics debug libassimpd.a optimized libassimp.a)

        # physx libraries
        target_link_libraries(SamplePhysics libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )

        #target_link_libraries(OverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OverlayTest ${AUDIO_UNIT})
        target_link_libraries(SamplePhysics stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SamplePhysics PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else (UNIX)
        target_link_libraries(SamplePhysics -Wl,--start-group WPCore.a WPGraphics.a
            libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a  
            WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
            png Xaw Xrandr Xt xcb GL
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

        #target_link_libraries(SamplePhysics -Wl,--start-group WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -Wl,--end-group )

        target_link_libraries(SamplePhysics freetype.a)

        target_link_libraries(SamplePhysics png)
        target_link_libraries(SamplePhysics Xaw)
        target_link_libraries(SamplePhysics Xrandr)
        target_link_libraries(SamplePhysics Xt)
        target_link_libraries(SamplePhysics xcb)
        target_link_libraries(SamplePhysics ${X11_LIBRARIES})
        target_link_libraries(SamplePhysics ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SamplePhysics ${CMAKE_DL_LIBS})
    endif ()
endif ()

if (WIN32)
    set_property(TARGET SamplePhysics PROPERTY VS_DEBUGGER_WORKING_DIRECTORY 
        "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
    add_dependencies(SamplePhysics WPGraphicsOgreNext)
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(SamplePhysics WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(SamplePhysics WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(SamplePhysics WPPhysx)
endif()

set_target_properties (SamplePhysics PROPERTIES FOLDER Samples)
