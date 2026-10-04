set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SampleHelloWorld)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)


if (UNIX)
    if (NOT APPLE)
        include_directories(${X11_INCLUDE_DIR})
    endif()
endif()

if (WIN32)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)    
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/Dependencies/lib/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (UNIX)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(SampleHelloWorld ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(SampleHelloWorld ${HEADER_FILES} ${SOURCE_FILES})

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(SampleHelloWorld Workphone)

        if (WP_USE_ONETBB)
            target_link_libraries(SampleHelloWorld debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(SampleHelloWorld debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(SampleHelloWorld debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorld tbb_static.lib)
                target_link_libraries(SampleHelloWorld tbbmalloc_static.lib)
            else()
                target_link_libraries(SampleHelloWorld tbb.lib)
			    target_link_libraries(SampleHelloWorld tbbmalloc.lib)
            endif()
        endif()
    elseif (APPLE)
        # fireblade libraries
        target_link_libraries(SampleHelloWorld libWPCore.a)
        target_link_libraries(SampleHelloWorld libWPGraphics.a)
        target_link_libraries(SampleHelloWorld libWPMesh.a)
        target_link_libraries(SampleHelloWorld libWPPhysx.a)
        target_link_libraries(SampleHelloWorld libWPVehicle.a)
        target_link_libraries(SampleHelloWorld WPRenderUI.a)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorld FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorld freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorld WPGraphicsOgreNext.a)

            target_link_libraries(SampleHelloWorld harWPuzz.a)
            target_link_libraries(SampleHelloWorld harWPuzz-subset.a)
            target_link_libraries(SampleHelloWorld icucommon.a)
            target_link_libraries(SampleHelloWorld sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorld WPGraphicsOgre.a)
        endif()

        target_link_libraries(SampleHelloWorld libWPSQLite.a)

        target_link_libraries(SampleHelloWorld libboost_thread.a)
        target_link_libraries(SampleHelloWorld libboost_date_time.a)
        target_link_libraries(SampleHelloWorld libboost_chrono.a)
        target_link_libraries(SampleHelloWorld libboost_context.a)
        target_link_libraries(SampleHelloWorld libboost_timer.a)
        target_link_libraries(SampleHelloWorld libboost_filesystem.a)
        target_link_libraries(SampleHelloWorld libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorld WPPhysx.a)

            target_link_libraries(SampleHelloWorld PhysX3.a)
            target_link_libraries(SampleHelloWorld PhysX3Extensions.a)
            target_link_libraries(SampleHelloWorld PhysX3Common.a)
            target_link_libraries(SampleHelloWorld PhysX3Vehicle.a)
            target_link_libraries(SampleHelloWorld PhysX3Cooking.a)
            target_link_libraries(SampleHelloWorld PhysXProfileSDK.a)
            target_link_libraries(SampleHelloWorld PxTask.a)
            target_link_libraries(SampleHelloWorld PhysXVisualDebuggerSDK.a)
            target_link_libraries(SampleHelloWorld PhysX3MetaData.a)
            target_link_libraries(SampleHelloWorld PhysX3Foundation.a)
            target_link_libraries(SampleHelloWorld PxSceneQuery.a)
            target_link_libraries(SampleHelloWorld PxSimulationController.a)
            target_link_libraries(SampleHelloWorld PhysX3LowLevel.a)
            target_link_libraries(SampleHelloWorld PhysX3LowLevelCloth.a)
        endif()

        target_link_libraries(SampleHelloWorld minizip.a)
        target_link_libraries(SampleHelloWorld nativefiledialog.a)
        target_link_libraries(SampleHelloWorld tinyxml.a)
        target_link_libraries(SampleHelloWorld zlib.a)
        target_link_libraries(SampleHelloWorld zzip.a)

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorld tbb_static.a)
                target_link_libraries(SampleHelloWorld tbbmalloc_static.a)
            else()
                target_link_libraries(SampleHelloWorld tbb.a)
                target_link_libraries(SampleHelloWorld tbbmalloc.a)
            endif()
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SampleHelloWorld libWm5Core.a)
            target_link_libraries(SampleHelloWorld libWm5Mathematics.a)
        endif ()

        target_link_libraries(SampleHelloWorld stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SampleHelloWorld PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else ()
        target_link_libraries(SampleHelloWorld -Wl,--start-group WPCore.a WPMesh.a WPGraphics.a
            WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
            png Xaw Xrandr Xt xcb
            libboost_thread-gcc5-mt-s-x64-1_70.a
            libboost_date_time-gcc5-mt-s-x64-1_70.a
            libboost_chrono-gcc5-mt-s-x64-1_70.a
            libboost_timer-gcc5-mt-s-x64-1_70.a
            libboost_filesystem-gcc5-mt-s-x64-1_70.a
            debug libtbb_debug.a optimized libtbb.a
            debug libtbbmalloc_debug.a optimized libtbbmalloc.a
            -Wl,--end-group )

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            #target_link_libraries(SampleHelloWorld WPGraphicsOgreNext.a)
            target_link_libraries(SampleHelloWorld freetype.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorld WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SampleHelloWorld libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(SampleHelloWorld png)
        target_link_libraries(SampleHelloWorld Xaw)
        target_link_libraries(SampleHelloWorld Xrandr)
        target_link_libraries(SampleHelloWorld Xt)
        target_link_libraries(SampleHelloWorld xcb)
        target_link_libraries(SampleHelloWorld GL)
        target_link_libraries(SampleHelloWorld ${X11_LIBRARIES})
        target_link_libraries(SampleHelloWorld ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SampleHelloWorld ${CMAKE_DL_LIBS})
    endif ()
else()
    if (WIN32)
        target_link_libraries(SampleHelloWorld Workphone)
        target_link_libraries(SampleHelloWorld WPMesh.lib)
        target_link_libraries(SampleHelloWorld WPPhysx.lib)
        target_link_libraries(SampleHelloWorld WPSQLite)
        target_link_libraries(SampleHelloWorld WPGraphics)
        target_link_libraries(SampleHelloWorld WPRenderUI.lib)

        if (WP_BUILD_OISINPUT)

            #target_link_libraries(SampleHelloWorld debug OIS_d.lib optimized OIS.lib)
            target_link_libraries(SampleHelloWorld dinput8.lib)
            target_link_libraries(SampleHelloWorld dxguid.lib)
        endif()

        if (WP_USE_WPXSDK)
            #target_link_libraries(SampleHelloWorld libWPxsdk.lib)
            target_link_libraries(SampleHelloWorld libWPxsdk-mt.lib)
            target_link_libraries(SampleHelloWorld libxml2-mt.lib)
            target_link_libraries(SampleHelloWorld tinyxml2.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorld FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorld freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorld WPGraphicsOgreNext.lib)

            target_link_libraries(SampleHelloWorld harWPuzz.lib)
            target_link_libraries(SampleHelloWorld harWPuzz-subset.lib)
            target_link_libraries(SampleHelloWorld icucommon.lib)
            target_link_libraries(SampleHelloWorld sds_library.lib)

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SampleHelloWorld d3d11.lib)
                target_link_libraries(SampleHelloWorld dxgi.lib)
                target_link_libraries(SampleHelloWorld dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SampleHelloWorld opengl32.lib)
                target_link_libraries(SampleHelloWorld glu32.lib)
            endif ()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorld WPGraphicsOgre.lib)

            if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
                target_link_libraries(SampleHelloWorld debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
                target_link_libraries(SampleHelloWorld debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

                target_link_libraries(SampleHelloWorld debug RenderSystem_Direct3D9Static_d.lib
                                    optimized RenderSystem_Direct3D9Static.lib)

                target_link_libraries(SampleHelloWorld debug RenderSystem_Direct3D11Static_d.lib
                                    optimized RenderSystem_Direct3D11Static.lib)

                target_link_libraries(SampleHelloWorld debug RenderSystem_GL3PlusStatic_d.lib 
                                    optimized RenderSystem_GL3PlusStatic.lib)

                target_link_libraries(SampleHelloWorld debug RenderSystem_GLStatic_d.lib
                                    optimized RenderSystem_GLStatic.lib)

                target_link_libraries(SampleHelloWorld debug OgreGLSupportStatic_d.lib
                                    optimized OgreGLSupportStatic.lib)

                target_link_libraries(SampleHelloWorld debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

                target_link_libraries(SampleHelloWorld debug OgreRTShaderSystemStatic_d.lib
                                           optimized OgreRTShaderSystemStatic.lib)

                target_link_libraries(SampleHelloWorld debug Codec_STBIStatic_d.lib
                                           optimized Codec_STBIStatic.lib)

                target_link_libraries(SampleHelloWorld debug OgreBitesStatic_d.lib
                                           optimized OgreBitesStatic.lib)
            endif()

            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(SampleHelloWorld d3d9.lib)
                target_link_libraries(SampleHelloWorld d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SampleHelloWorld d3d11.lib)
                target_link_libraries(SampleHelloWorld dxgi.lib)
                target_link_libraries(SampleHelloWorld dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(SampleHelloWorld opengl32.lib)
                target_link_libraries(SampleHelloWorld glu32.lib)
            
                if (WP_PLATFORM_X64)
                    target_link_libraries(SampleHelloWorld amd_ags_x64.lib)
                endif ()
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SampleHelloWorld opengl32.lib)
                target_link_libraries(SampleHelloWorld glu32.lib)
            endif ()

            if (WP_USE_FREETYPE)
                target_link_libraries(SampleHelloWorld freetype.lib)
            endif ()
        elseif(WP_BUILD_DX11)
            target_link_libraries(SampleHelloWorld WPGraphicsD3D11.lib)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorld WPPhysx.lib)

            target_link_libraries(SampleHelloWorld PhysX3.lib)
            target_link_libraries(SampleHelloWorld PhysX3Extensions.lib)
            target_link_libraries(SampleHelloWorld PhysX3Common.lib)
            target_link_libraries(SampleHelloWorld PhysX3Vehicle.lib)
            target_link_libraries(SampleHelloWorld PhysX3Cooking.lib)
            target_link_libraries(SampleHelloWorld PhysXProfileSDK.lib)
            target_link_libraries(SampleHelloWorld PxTask.lib)
            target_link_libraries(SampleHelloWorld PhysXVisualDebuggerSDK.lib)
            target_link_libraries(SampleHelloWorld PhysX3MetaData.lib)
            target_link_libraries(SampleHelloWorld PhysX3Foundation.lib)
            target_link_libraries(SampleHelloWorld PxSceneQuery.lib)
            target_link_libraries(SampleHelloWorld PxSimulationController.lib)
            target_link_libraries(SampleHelloWorld PhysX3LowLevel.lib)
            target_link_libraries(SampleHelloWorld PhysX3LowLevelCloth.lib)
        endif()

        target_link_libraries(SampleHelloWorld bcrypt.lib)
        target_link_libraries(SampleHelloWorld libboost_thread.lib)
        target_link_libraries(SampleHelloWorld libboost_date_time.lib)
        target_link_libraries(SampleHelloWorld libboost_chrono.lib)
        target_link_libraries(SampleHelloWorld libboost_timer.lib)
        target_link_libraries(SampleHelloWorld libboost_filesystem.lib)
        target_link_libraries(SampleHelloWorld libboost_json.lib)
        target_link_libraries(SampleHelloWorld nativefiledialog.lib)
        target_link_libraries(SampleHelloWorld tinyxml.lib)    
        target_link_libraries(SampleHelloWorld minizip.lib)
        target_link_libraries(SampleHelloWorld zlib.lib)
        target_link_libraries(SampleHelloWorld zzip.lib)

        if (WP_USE_TBB)
            target_link_libraries(SampleHelloWorld tbb_static.lib)
            target_link_libraries(SampleHelloWorld tbbmalloc_static.lib)
        endif()
    elseif (APPLE)
        # fireblade libraries
        target_link_libraries(SampleHelloWorld libWPCore.a)
        target_link_libraries(SampleHelloWorld libWPGraphics.a)
        target_link_libraries(SampleHelloWorld libWPMesh.a)
        target_link_libraries(SampleHelloWorld libWPPhysx.a)
        target_link_libraries(SampleHelloWorld libWPVehicle.a)
        target_link_libraries(SampleHelloWorld WPRenderUI.a)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorld FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorld freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorld WPGraphicsOgreNext.a)

            target_link_libraries(SampleHelloWorld harWPuzz.a)
            target_link_libraries(SampleHelloWorld harWPuzz-subset.a)
            target_link_libraries(SampleHelloWorld icucommon.a)
            target_link_libraries(SampleHelloWorld sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorld WPGraphicsOgre.a)
        endif()

        target_link_libraries(SampleHelloWorld libWPSQLite.a)

        target_link_libraries(SampleHelloWorld libboost_thread.a)
        target_link_libraries(SampleHelloWorld libboost_date_time.a)
        target_link_libraries(SampleHelloWorld libboost_chrono.a)
        target_link_libraries(SampleHelloWorld libboost_context.a)
        target_link_libraries(SampleHelloWorld libboost_timer.a)
        target_link_libraries(SampleHelloWorld libboost_filesystem.a)
        target_link_libraries(SampleHelloWorld libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorld WPPhysx.a)

            target_link_libraries(SampleHelloWorld PhysX3.a)
            target_link_libraries(SampleHelloWorld PhysX3Extensions.a)
            target_link_libraries(SampleHelloWorld PhysX3Common.a)
            target_link_libraries(SampleHelloWorld PhysX3Vehicle.a)
            target_link_libraries(SampleHelloWorld PhysX3Cooking.a)
            target_link_libraries(SampleHelloWorld PhysXProfileSDK.a)
            target_link_libraries(SampleHelloWorld PxTask.a)
            target_link_libraries(SampleHelloWorld PhysXVisualDebuggerSDK.a)
            target_link_libraries(SampleHelloWorld PhysX3MetaData.a)
            target_link_libraries(SampleHelloWorld PhysX3Foundation.a)
            target_link_libraries(SampleHelloWorld PxSceneQuery.a)
            target_link_libraries(SampleHelloWorld PxSimulationController.a)
            target_link_libraries(SampleHelloWorld PhysX3LowLevel.a)
            target_link_libraries(SampleHelloWorld PhysX3LowLevelCloth.a)
        endif()

        target_link_libraries(SampleHelloWorld minizip.a)
        target_link_libraries(SampleHelloWorld nativefiledialog.a)
        target_link_libraries(SampleHelloWorld tinyxml.a)
        target_link_libraries(SampleHelloWorld zlib.a)
        target_link_libraries(SampleHelloWorld zzip.a)

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorld tbb_static.a)
                target_link_libraries(SampleHelloWorld tbbmalloc_static.a)
            else()
                target_link_libraries(SampleHelloWorld tbb.a)
                target_link_libraries(SampleHelloWorld tbbmalloc.a)
            endif()
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SampleHelloWorld libWm5Core.a)
            target_link_libraries(SampleHelloWorld libWm5Mathematics.a)
        endif ()

        target_link_libraries(SampleHelloWorld stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SampleHelloWorld PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else ()
        target_link_libraries(SampleHelloWorld -Wl,--start-group WPCore.a WPMesh.a WPGraphics.a
            WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
            png Xaw Xrandr Xt xcb
            libboost_thread-gcc5-mt-s-x64-1_70.a
            libboost_date_time-gcc5-mt-s-x64-1_70.a
            libboost_chrono-gcc5-mt-s-x64-1_70.a
            libboost_timer-gcc5-mt-s-x64-1_70.a
            libboost_filesystem-gcc5-mt-s-x64-1_70.a
            debug libtbb_debug.a optimized libtbb.a
            debug libtbbmalloc_debug.a optimized libtbbmalloc.a
            -Wl,--end-group )

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            #target_link_libraries(SampleHelloWorld WPGraphicsOgreNext.a)
            target_link_libraries(SampleHelloWorld freetype.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorld WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SampleHelloWorld libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(SampleHelloWorld png)
        target_link_libraries(SampleHelloWorld Xaw)
        target_link_libraries(SampleHelloWorld Xrandr)
        target_link_libraries(SampleHelloWorld Xt)
        target_link_libraries(SampleHelloWorld xcb)
        target_link_libraries(SampleHelloWorld GL)
        target_link_libraries(SampleHelloWorld ${X11_LIBRARIES})
        target_link_libraries(SampleHelloWorld ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SampleHelloWorld ${CMAKE_DL_LIBS})
    endif ()
endif ()

if (WIN32)
    set_property(TARGET SampleHelloWorld PROPERTY VS_DEBUGGER_WORKING_DIRECTORY 
        "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    add_dependencies(SampleHelloWorld WPGraphicsOgreNext)
endif()

if (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(SampleHelloWorld WPGraphicsOgre)
endif()

if (WP_GRAPHICS_SYSTEM_CLAW)
    add_dependencies(SampleHelloWorld WPGraphics)
endif()

if (WP_BUILD_ODE)
    add_dependencies(SampleHelloWorld WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(SampleHelloWorld WPPhysx)
endif()

set_target_properties (SampleHelloWorld PROPERTIES FOLDER Samples)
