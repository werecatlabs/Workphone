set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SampleHelloWorldC89)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)

include_directories( ${WP_ROOT}/Dependencies/boost)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneCore )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32 )
include_directories(Source)


if (UNIX)
    if (NOT APPLE)
        include_directories(${X11_INCLUDE_DIR})
    endif()
endif()

if (WIN32)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)    
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/Dependencies/lib/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (UNIX)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(SampleHelloWorldC89 ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(SampleHelloWorldC89 ${HEADER_FILES} ${SOURCE_FILES})

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(SampleHelloWorldC89 Workphone.lib)
        
        target_link_libraries(SampleHelloWorldC89 WorkphoneCore.lib)
        target_link_libraries(SampleHelloWorldC89 WorkphoneGraphics.lib)
        target_link_libraries(SampleHelloWorldC89 WorkphonePlatformWin32.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(SampleHelloWorldC89 debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(SampleHelloWorldC89 debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(SampleHelloWorldC89 debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorldC89 tbb_static.lib)
                target_link_libraries(SampleHelloWorldC89 tbbmalloc_static.lib)
            else()
                target_link_libraries(SampleHelloWorldC89 tbb.lib)
			    target_link_libraries(SampleHelloWorldC89 tbbmalloc.lib)
            endif()
        endif()
    elseif (APPLE)
        # fireblade libraries
        target_link_libraries(SampleHelloWorldC89 libWPCore.a)
        target_link_libraries(SampleHelloWorldC89 libWPGraphics.a)
        target_link_libraries(SampleHelloWorldC89 libWPMesh.a)
        target_link_libraries(SampleHelloWorldC89 libWPPhysx.a)
        target_link_libraries(SampleHelloWorldC89 libWPVehicle.a)
        target_link_libraries(SampleHelloWorldC89 WPRenderUI.a)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorldC89 FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorldC89 freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgreNext.a)

            target_link_libraries(SampleHelloWorldC89 harWPuzz.a)
            target_link_libraries(SampleHelloWorldC89 harWPuzz-subset.a)
            target_link_libraries(SampleHelloWorldC89 icucommon.a)
            target_link_libraries(SampleHelloWorldC89 sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgre.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 libWPSQLite.a)

        target_link_libraries(SampleHelloWorldC89 libboost_thread.a)
        target_link_libraries(SampleHelloWorldC89 libboost_date_time.a)
        target_link_libraries(SampleHelloWorldC89 libboost_chrono.a)
        target_link_libraries(SampleHelloWorldC89 libboost_context.a)
        target_link_libraries(SampleHelloWorldC89 libboost_timer.a)
        target_link_libraries(SampleHelloWorldC89 libboost_filesystem.a)
        target_link_libraries(SampleHelloWorldC89 libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorldC89 WPPhysx.a)

            target_link_libraries(SampleHelloWorldC89 PhysX3.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Extensions.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Common.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Vehicle.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Cooking.a)
            target_link_libraries(SampleHelloWorldC89 PhysXProfileSDK.a)
            target_link_libraries(SampleHelloWorldC89 PxTask.a)
            target_link_libraries(SampleHelloWorldC89 PhysXVisualDebuggerSDK.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3MetaData.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Foundation.a)
            target_link_libraries(SampleHelloWorldC89 PxSceneQuery.a)
            target_link_libraries(SampleHelloWorldC89 PxSimulationController.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevel.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevelCloth.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 minizip.a)
        target_link_libraries(SampleHelloWorldC89 nativefiledialog.a)
        target_link_libraries(SampleHelloWorldC89 rttr.a)
        target_link_libraries(SampleHelloWorldC89 tinyxml.a)
        target_link_libraries(SampleHelloWorldC89 yaml-cpp.a)
        target_link_libraries(SampleHelloWorldC89 zlib.a)
        target_link_libraries(SampleHelloWorldC89 zzip.a)

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorldC89 tbb_static.a)
                target_link_libraries(SampleHelloWorldC89 tbbmalloc_static.a)
            else()
                target_link_libraries(SampleHelloWorldC89 tbb.a)
                target_link_libraries(SampleHelloWorldC89 tbbmalloc.a)
            endif()
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SampleHelloWorldC89 libWm5Core.a)
            target_link_libraries(SampleHelloWorldC89 libWm5Mathematics.a)
        endif ()

        target_link_libraries(SampleHelloWorldC89 stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SampleHelloWorldC89 PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else ()
        target_link_libraries(SampleHelloWorldC89 -Wl,--start-group WPCore.a WPMesh.a WPGraphics.a
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
            #target_link_libraries(SampleHelloWorldC89 WPGraphicsOgreNext.a)
            target_link_libraries(SampleHelloWorldC89 freetype.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SampleHelloWorldC89 libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 png)
        target_link_libraries(SampleHelloWorldC89 Xaw)
        target_link_libraries(SampleHelloWorldC89 Xrandr)
        target_link_libraries(SampleHelloWorldC89 Xt)
        target_link_libraries(SampleHelloWorldC89 xcb)
        target_link_libraries(SampleHelloWorldC89 GL)
        target_link_libraries(SampleHelloWorldC89 ${X11_LIBRARIES})
        target_link_libraries(SampleHelloWorldC89 ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SampleHelloWorldC89 ${CMAKE_DL_LIBS})
    endif ()
else()
    if (WIN32)
        target_link_libraries(SampleHelloWorldC89 Workphone.lib)
        target_link_libraries(SampleHelloWorldC89 WPMesh.lib)
        target_link_libraries(SampleHelloWorldC89 WPPhysx.lib)
        target_link_libraries(SampleHelloWorldC89 WPSQLite.lib)
        target_link_libraries(SampleHelloWorldC89 WPOISInput.lib)
        target_link_libraries(SampleHelloWorldC89 WPGraphics.lib)
        target_link_libraries(SampleHelloWorldC89 WPRenderUI.lib)

        if (WP_BUILD_OISINPUT)
            target_link_libraries(SampleHelloWorldC89 WPOISInput.lib)

            #target_link_libraries(SampleHelloWorldC89 debug OIS_d.lib optimized OIS.lib)
            target_link_libraries(SampleHelloWorldC89 dinput8.lib)
            target_link_libraries(SampleHelloWorldC89 dxguid.lib)
        endif()

        if (WP_USE_WPXSDK)
            #target_link_libraries(SampleHelloWorldC89 libWPxsdk.lib)
            target_link_libraries(SampleHelloWorldC89 libWPxsdk-mt.lib)
            target_link_libraries(SampleHelloWorldC89 libxml2-mt.lib)
            target_link_libraries(SampleHelloWorldC89 tinyxml2.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorldC89 FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorldC89 freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgreNext.lib)

            target_link_libraries(SampleHelloWorldC89 harWPuzz.lib)
            target_link_libraries(SampleHelloWorldC89 harWPuzz-subset.lib)
            target_link_libraries(SampleHelloWorldC89 icucommon.lib)
            target_link_libraries(SampleHelloWorldC89 sds_library.lib)

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SampleHelloWorldC89 d3d11.lib)
                target_link_libraries(SampleHelloWorldC89 dxgi.lib)
                target_link_libraries(SampleHelloWorldC89 dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SampleHelloWorldC89 opengl32.lib)
                target_link_libraries(SampleHelloWorldC89 glu32.lib)
            endif ()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgre.lib)

            if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
                target_link_libraries(SampleHelloWorldC89 debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
                target_link_libraries(SampleHelloWorldC89 debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug RenderSystem_Direct3D9Static_d.lib
                                    optimized RenderSystem_Direct3D9Static.lib)

                target_link_libraries(SampleHelloWorldC89 debug RenderSystem_Direct3D11Static_d.lib
                                    optimized RenderSystem_Direct3D11Static.lib)

                target_link_libraries(SampleHelloWorldC89 debug RenderSystem_GL3PlusStatic_d.lib 
                                    optimized RenderSystem_GL3PlusStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug RenderSystem_GLStatic_d.lib
                                    optimized RenderSystem_GLStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug OgreGLSupportStatic_d.lib
                                    optimized OgreGLSupportStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug OgreRTShaderSystemStatic_d.lib
                                           optimized OgreRTShaderSystemStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug Codec_STBIStatic_d.lib
                                           optimized Codec_STBIStatic.lib)

                target_link_libraries(SampleHelloWorldC89 debug OgreBitesStatic_d.lib
                                           optimized OgreBitesStatic.lib)
            endif()

            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(SampleHelloWorldC89 d3d9.lib)
                target_link_libraries(SampleHelloWorldC89 d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(SampleHelloWorldC89 d3d11.lib)
                target_link_libraries(SampleHelloWorldC89 dxgi.lib)
                target_link_libraries(SampleHelloWorldC89 dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(SampleHelloWorldC89 opengl32.lib)
                target_link_libraries(SampleHelloWorldC89 glu32.lib)
            
                if (WP_PLATFORM_X64)
                    target_link_libraries(SampleHelloWorldC89 amd_ags_x64.lib)
                endif ()
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(SampleHelloWorldC89 opengl32.lib)
                target_link_libraries(SampleHelloWorldC89 glu32.lib)
            endif ()

            if (WP_USE_FREETYPE)
                target_link_libraries(SampleHelloWorldC89 freetype.lib)
            endif ()
        elseif(WP_BUILD_DX11)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsD3D11.lib)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorldC89 WPPhysx.lib)

            target_link_libraries(SampleHelloWorldC89 PhysX3.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3Extensions.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3Common.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3Vehicle.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3Cooking.lib)
            target_link_libraries(SampleHelloWorldC89 PhysXProfileSDK.lib)
            target_link_libraries(SampleHelloWorldC89 PxTask.lib)
            target_link_libraries(SampleHelloWorldC89 PhysXVisualDebuggerSDK.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3MetaData.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3Foundation.lib)
            target_link_libraries(SampleHelloWorldC89 PxSceneQuery.lib)
            target_link_libraries(SampleHelloWorldC89 PxSimulationController.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevel.lib)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevelCloth.lib)
        endif()

        target_link_libraries(SampleHelloWorldC89 bcrypt.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_thread.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_date_time.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_chrono.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_timer.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_filesystem.lib)
        target_link_libraries(SampleHelloWorldC89 libboost_json.lib)
        target_link_libraries(SampleHelloWorldC89 nativefiledialog.lib)
        target_link_libraries(SampleHelloWorldC89 rttr.lib)
        target_link_libraries(SampleHelloWorldC89 tinyxml.lib)    
        target_link_libraries(SampleHelloWorldC89 yaml-cpp.lib)
        target_link_libraries(SampleHelloWorldC89 minizip.lib)
        target_link_libraries(SampleHelloWorldC89 zlib.lib)
        target_link_libraries(SampleHelloWorldC89 zzip.lib)

        if (WP_USE_TBB)
            target_link_libraries(SampleHelloWorldC89 tbb_static.lib)
            target_link_libraries(SampleHelloWorldC89 tbbmalloc_static.lib)
        endif()
    elseif (APPLE)
        # fireblade libraries
        target_link_libraries(SampleHelloWorldC89 libWPCore.a)
        target_link_libraries(SampleHelloWorldC89 libWPGraphics.a)
        target_link_libraries(SampleHelloWorldC89 libWPMesh.a)
        target_link_libraries(SampleHelloWorldC89 libWPPhysx.a)
        target_link_libraries(SampleHelloWorldC89 libWPVehicle.a)
        target_link_libraries(SampleHelloWorldC89 WPRenderUI.a)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(SampleHelloWorldC89 FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(SampleHelloWorldC89 freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgreNext.a)

            target_link_libraries(SampleHelloWorldC89 harWPuzz.a)
            target_link_libraries(SampleHelloWorldC89 harWPuzz-subset.a)
            target_link_libraries(SampleHelloWorldC89 icucommon.a)
            target_link_libraries(SampleHelloWorldC89 sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgre.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 libWPSQLite.a)

        target_link_libraries(SampleHelloWorldC89 libboost_thread.a)
        target_link_libraries(SampleHelloWorldC89 libboost_date_time.a)
        target_link_libraries(SampleHelloWorldC89 libboost_chrono.a)
        target_link_libraries(SampleHelloWorldC89 libboost_context.a)
        target_link_libraries(SampleHelloWorldC89 libboost_timer.a)
        target_link_libraries(SampleHelloWorldC89 libboost_filesystem.a)
        target_link_libraries(SampleHelloWorldC89 libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(SampleHelloWorldC89 WPPhysx.a)

            target_link_libraries(SampleHelloWorldC89 PhysX3.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Extensions.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Common.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Vehicle.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Cooking.a)
            target_link_libraries(SampleHelloWorldC89 PhysXProfileSDK.a)
            target_link_libraries(SampleHelloWorldC89 PxTask.a)
            target_link_libraries(SampleHelloWorldC89 PhysXVisualDebuggerSDK.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3MetaData.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3Foundation.a)
            target_link_libraries(SampleHelloWorldC89 PxSceneQuery.a)
            target_link_libraries(SampleHelloWorldC89 PxSimulationController.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevel.a)
            target_link_libraries(SampleHelloWorldC89 PhysX3LowLevelCloth.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 minizip.a)
        target_link_libraries(SampleHelloWorldC89 nativefiledialog.a)
        target_link_libraries(SampleHelloWorldC89 rttr.a)
        target_link_libraries(SampleHelloWorldC89 tinyxml.a)
        target_link_libraries(SampleHelloWorldC89 yaml-cpp.a)
        target_link_libraries(SampleHelloWorldC89 zlib.a)
        target_link_libraries(SampleHelloWorldC89 zzip.a)

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(SampleHelloWorldC89 tbb_static.a)
                target_link_libraries(SampleHelloWorldC89 tbbmalloc_static.a)
            else()
                target_link_libraries(SampleHelloWorldC89 tbb.a)
                target_link_libraries(SampleHelloWorldC89 tbbmalloc.a)
            endif()
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(SampleHelloWorldC89 libWm5Core.a)
            target_link_libraries(SampleHelloWorldC89 libWm5Mathematics.a)
        endif ()

        target_link_libraries(SampleHelloWorldC89 stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(SampleHelloWorldC89 PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    else ()
        target_link_libraries(SampleHelloWorldC89 -Wl,--start-group WPCore.a WPMesh.a WPGraphics.a
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
            #target_link_libraries(SampleHelloWorldC89 WPGraphicsOgreNext.a)
            target_link_libraries(SampleHelloWorldC89 freetype.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(SampleHelloWorldC89 WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(SampleHelloWorldC89 libWPGraphicsSoftware.a)
        endif()

        target_link_libraries(SampleHelloWorldC89 png)
        target_link_libraries(SampleHelloWorldC89 Xaw)
        target_link_libraries(SampleHelloWorldC89 Xrandr)
        target_link_libraries(SampleHelloWorldC89 Xt)
        target_link_libraries(SampleHelloWorldC89 xcb)
        target_link_libraries(SampleHelloWorldC89 GL)
        target_link_libraries(SampleHelloWorldC89 ${X11_LIBRARIES})
        target_link_libraries(SampleHelloWorldC89 ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(SampleHelloWorldC89 ${CMAKE_DL_LIBS})
    endif ()
endif ()

if (WIN32)
    set_property(TARGET SampleHelloWorldC89 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY 
        "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    add_dependencies(SampleHelloWorldC89 WPGraphicsOgreNext)
endif()

if (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(SampleHelloWorldC89 WPGraphicsOgre)
endif()

if (WP_GRAPHICS_SYSTEM_CLAW)
    add_dependencies(SampleHelloWorldC89 WPGraphics)
endif()

if (WP_BUILD_ODE)
    add_dependencies(SampleHelloWorldC89 WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(SampleHelloWorldC89 WPPhysx)
endif()

set_target_properties (SampleHelloWorldC89 PROPERTIES FOLDER Samples)
