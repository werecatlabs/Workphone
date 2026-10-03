set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(OgreOverlayTest)

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

if (WP_GRAPHICS_SYSTEM_OGRE)
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
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/include/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/include/linux )
    endif()

    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/OgreMain/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/OgreMain/include/Deprecated )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Overlay/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Paging/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Bites/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Hlms/Common/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Hlms/Pbs/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/Components/Hlms/Unlit/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/PlugIns/CgProgramManager/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/Direct3D11/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/GL3Plus/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/GL3Plus/include/GLSL )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/GLES2/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/Metal/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogrenext/RenderSystems/NULL/include )
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

    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
    endif()
endif()

if (WIN32)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/Dependencies/lib/${CMAKE_BUILD_TYPE})
    endif ()
elseif (UNIX)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(OgreOverlayTest ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(OgreOverlayTest ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(OgreOverlayTest Workphone.lib)
    target_link_libraries(OgreOverlayTest WPProcedural.lib)
    target_link_libraries(OgreOverlayTest WPPhysx.lib)
    target_link_libraries(OgreOverlayTest WPSQLite.lib)
    target_link_libraries(OgreOverlayTest WPVehicle.lib)
    target_link_libraries(OgreOverlayTest WPOISInput.lib)
    target_link_libraries(OgreOverlayTest WPGraphics.lib)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        #target_link_libraries(OgreOverlayTest WPGraphicsOgreNext.lib)

        target_link_libraries(OgreOverlayTest debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
        target_link_libraries(OgreOverlayTest debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)
        target_link_libraries(OgreOverlayTest debug RenderSystem_Direct3D11Static_d.lib optimized RenderSystem_Direct3D11Static.lib)
        target_link_libraries(OgreOverlayTest debug RenderSystem_GL3PlusStatic_d.lib optimized RenderSystem_GL3PlusStatic.lib)

        target_link_libraries(OgreOverlayTest debug OgreHlmsPbsStatic_d.lib optimized OgreHlmsPbsStatic.lib)
        target_link_libraries(OgreOverlayTest debug OgreHlmsUnlitStatic_d.lib optimized OgreHlmsUnlitStatic.lib)

        target_link_libraries(OgreOverlayTest debug FreeImage_d.lib optimized FreeImage.lib)
        target_link_libraries(OgreOverlayTest debug freetype_d.lib optimized freetype.lib)


        target_link_libraries(OgreOverlayTest debug FreeImage_d.lib optimized FreeImage.lib)
        target_link_libraries(OgreOverlayTest debug freetype_d.lib optimized freetype.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(OgreOverlayTest d3d11.lib)
            target_link_libraries(OgreOverlayTest dxgi.lib)
            target_link_libraries(OgreOverlayTest dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(OgreOverlayTest opengl32.lib)
            target_link_libraries(OgreOverlayTest glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(OgreOverlayTest amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(OgreOverlayTest WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(OgreOverlayTest debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(OgreOverlayTest debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(OgreOverlayTest debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(OgreOverlayTest debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(OgreOverlayTest debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(OgreOverlayTest debug RenderSystem_GLStatic_d.lib 
                                optimized RenderSystem_GLStatic.lib)

                            
            target_link_libraries(OgreOverlayTest debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(OgreOverlayTest debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(OgreOverlayTest debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(OgreOverlayTest debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(OgreOverlayTest debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)
        endif()
    
        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(OgreOverlayTest debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(OgreOverlayTest freetype.lib)   
        
        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(OgreOverlayTest d3d9.lib)
            target_link_libraries(OgreOverlayTest d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(OgreOverlayTest d3d11.lib)
            target_link_libraries(OgreOverlayTest dxgi.lib)
            target_link_libraries(OgreOverlayTest dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(OgreOverlayTest opengl32.lib)
            target_link_libraries(OgreOverlayTest glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(OgreOverlayTest amd_ags_x64.lib)
            endif ()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(OgreOverlayTest opengl32.lib)
            target_link_libraries(OgreOverlayTest glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(OgreOverlayTest amd_ags_x64.lib)
            endif ()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(OgreOverlayTest WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(OgreOverlayTest bcrypt.lib)
    target_link_libraries(OgreOverlayTest libboost_thread.lib)
    target_link_libraries(OgreOverlayTest libboost_date_time.lib)
    target_link_libraries(OgreOverlayTest libboost_chrono.lib)
    target_link_libraries(OgreOverlayTest libboost_timer.lib)
    target_link_libraries(OgreOverlayTest libboost_filesystem.lib)
    target_link_libraries(OgreOverlayTest libboost_json.lib)
    target_link_libraries(OgreOverlayTest minizip.lib)
    target_link_libraries(OgreOverlayTest nativefiledialog.lib)
    target_link_libraries(OgreOverlayTest rttr.lib)
    target_link_libraries(OgreOverlayTest tinyxml.lib)   
    target_link_libraries(OgreOverlayTest yaml-cpp.lib)
    target_link_libraries(OgreOverlayTest zlib.lib)
    target_link_libraries(OgreOverlayTest zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(OgreOverlayTest tbb_static.lib)
			target_link_libraries(OgreOverlayTest tbbmalloc_static.lib)
		else()
			target_link_libraries(OgreOverlayTest tbb.lib)
			target_link_libraries(OgreOverlayTest tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(OgreOverlayTest libWPCore.a)
    target_link_libraries(OgreOverlayTest libWPFileSystem.a)
    target_link_libraries(OgreOverlayTest libWPMath.a)
    target_link_libraries(OgreOverlayTest libWPMesh.a)
    target_link_libraries(OgreOverlayTest libWPProcedural.a)
    target_link_libraries(OgreOverlayTest libWPPhysx.a)
    target_link_libraries(OgreOverlayTest libWPData.a)
    target_link_libraries(OgreOverlayTest libWPObjectTemplates.a)
    target_link_libraries(OgreOverlayTest libWPAero.a)
    target_link_libraries(OgreOverlayTest libWPVehicle.a)
    target_link_libraries(OgreOverlayTest libWPAudio.a)
    target_link_libraries(OgreOverlayTest libWPApplication.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(OgreOverlayTest WPGraphicsOgreNext.a)

        target_link_libraries(OgreOverlayTest FreeImage.a)
        target_link_libraries(OgreOverlayTest freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(OgreOverlayTest libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(OgreOverlayTest libWPSQLite.a)

    target_link_libraries(OgreOverlayTest libboost_thread.a)
    target_link_libraries(OgreOverlayTest libboost_date_time.a)
    target_link_libraries(OgreOverlayTest libboost_chrono.a)
    target_link_libraries(OgreOverlayTest libboost_timer.a)
    target_link_libraries(OgreOverlayTest libboost_filesystem.a)
    target_link_libraries(OgreOverlayTest libboost_unit_test_framework.a)
    target_link_libraries(OgreOverlayTest libboost_system.a)

    target_link_libraries(OgreOverlayTest debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(OgreOverlayTest debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
    target_link_libraries(OgreOverlayTest stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(OgreOverlayTest PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(OgreOverlayTest -Wl,--start-group WPCore.a WPAero.a 
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
        target_link_libraries(OgreOverlayTest WPGraphicsOgreNext.a)
        target_link_libraries(OgreOverlayTest freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(OgreOverlayTest libWPGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(OgreOverlayTest libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(OgreOverlayTest png)
    target_link_libraries(OgreOverlayTest Xaw)
    target_link_libraries(OgreOverlayTest Xrandr)
    target_link_libraries(OgreOverlayTest Xt)
    target_link_libraries(OgreOverlayTest xcb)
    target_link_libraries(OgreOverlayTest ${X11_LIBRARIES})
    target_link_libraries(OgreOverlayTest ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(OgreOverlayTest ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET OgreOverlayTest PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(OgreOverlayTest Workphone)

set_target_properties (OgreOverlayTest PROPERTIES FOLDER Tests/Cpp)
