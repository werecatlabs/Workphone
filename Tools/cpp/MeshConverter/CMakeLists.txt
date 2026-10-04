set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(MeshConverter)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (_UNICODE)

find_library(CORW_GRAPHICS CoreGraphics)

if (APPLE)
    add_compile_definitions(__WXMAC__)
    add_compile_definitions(__WXOSX__)
    add_compile_definitions(__WXOSX_COCOA__)
endif()

include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/boost)
include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include)
include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tinyxml/include )
include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/cppsqlite/include)
include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include)
include_directories("./")

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

if (WP_BUILD_WXWIDGETS)
    include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets  )
    include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/include )

    if (WIN32)
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/include/msvc )
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/include/msvc/wx )
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
    elseif (APPLE)
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1  )
    elseif (UNIX)
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1 )
        include_directories( ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1/wx )
    endif ()
endif()

if (WIN32)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl_image/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        
        if (WP_PLATFORM_X64)
            link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h" "./*.hpp")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(MeshConverter "MeshConverterPCH.hpp" "${CMAKE_SOURCE_DIR}/Tools/cpp/MeshConverter/MeshConverterPCH.cpp")
endif()

if (WIN32)
    add_executable(MeshConverter WIN32 ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(MeshConverter ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(MeshConverter Workphone)
    target_link_libraries(MeshConverter WPProcedural)
    target_link_libraries(MeshConverter WPSQLite)
    target_link_libraries(MeshConverter WPAssimp)

    if (WP_BUILD_SDL)
        target_link_libraries(MeshConverter SDL2main.lib)
        target_link_libraries(MeshConverter SDL2_image.lib)
        target_link_libraries(MeshConverter debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_OISINPUT)

        #target_link_libraries(MeshConverter debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(MeshConverter dinput8.lib)
        target_link_libraries(MeshConverter dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(MeshConverter libWPxsdk.lib)
        target_link_libraries(MeshConverter libWPxsdk-mt.lib)
        target_link_libraries(MeshConverter libxml2-mt.lib)
        target_link_libraries(MeshConverter tinyxml2.lib)
    endif()

    target_link_libraries(MeshConverter bcrypt.lib)
    target_link_libraries(MeshConverter libboost_thread.lib)
    target_link_libraries(MeshConverter libboost_date_time.lib)
    target_link_libraries(MeshConverter libboost_chrono.lib)
    target_link_libraries(MeshConverter libboost_timer.lib)
    target_link_libraries(MeshConverter libboost_filesystem.lib)
    target_link_libraries(MeshConverter libboost_json.lib)
    target_link_libraries(MeshConverter minizip.lib)
    target_link_libraries(MeshConverter nativefiledialog.lib)
    target_link_libraries(MeshConverter tinyxml.lib)   
    target_link_libraries(MeshConverter zlib.lib)
    target_link_libraries(MeshConverter zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(MeshConverter tbb_static.lib)
			target_link_libraries(MeshConverter tbbmalloc_static.lib)
		else()
			target_link_libraries(MeshConverter tbb.lib)
			target_link_libraries(MeshConverter tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(MeshConverter libWPCore.a)
    target_link_libraries(MeshConverter libWPGraphics.a)
    target_link_libraries(MeshConverter libWPMesh.a)
    target_link_libraries(MeshConverter libWPProcedural.a)
    target_link_libraries(MeshConverter libWPPhysx.a)
    target_link_libraries(MeshConverter libWPAero.a)
    target_link_libraries(MeshConverter libWPVehicle.a)
    target_link_libraries(MeshConverter libWPAudio.a)

    target_link_libraries(MeshConverter libWPAssimp.a)

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(MeshConverter libWPWxWidgets.a)

        target_link_libraries(MeshConverter libwx_osx_cocoau_core-3.1.a)
        target_link_libraries(MeshConverter libwx_baseu-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_xrc-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_propgrid-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_richtext-3.1.a)
        target_link_libraries(MeshConverter libwx_baseu_xml-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_adv-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_aui-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_html-3.1.a)
        target_link_libraries(MeshConverter libwxtiff-3.1.a)
        target_link_libraries(MeshConverter libwxjpeg-3.1.a)
        target_link_libraries(MeshConverter libwxpng-3.1.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshConverter FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshConverter freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshConverter WPGraphicsOgreNext.a)

        target_link_libraries(MeshConverter harWPuzz.a)
        target_link_libraries(MeshConverter harWPuzz-subset.a)
        target_link_libraries(MeshConverter icucommon.a)
        target_link_libraries(MeshConverter sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshConverter WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(MeshConverter libWPGraphicsSoftware.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshConverter WPImGui.a)
        target_link_libraries(MeshConverter imgui.a)
    endif()

    if (WP_BUILD_OISINPUT)
        #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
    endif()

    target_link_libraries(MeshConverter libWPSQLite.a)

    target_link_libraries(MeshConverter libboost_thread.a)
    target_link_libraries(MeshConverter libboost_date_time.a)
    target_link_libraries(MeshConverter libboost_chrono.a)
    target_link_libraries(MeshConverter libboost_context.a)
    target_link_libraries(MeshConverter libboost_timer.a)
    target_link_libraries(MeshConverter libboost_filesystem.a)
    target_link_libraries(MeshConverter libboost_json.a)

    target_link_libraries(MeshConverter libPxTask.a)
    target_link_libraries(MeshConverter libPhysXVisualDebuggerSDK.a)
    target_link_libraries(MeshConverter libPvdRuntime.a)

    target_link_libraries(MeshConverter minizip.a)
    target_link_libraries(MeshConverter nativefiledialog.a)
    target_link_libraries(MeshConverter tinyxml.a)
    target_link_libraries(MeshConverter zlib.a)
    target_link_libraries(MeshConverter zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(MeshConverter libtbb.a)
        target_link_libraries(MeshConverter libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(MeshConverter libWm5Core.a)
        target_link_libraries(MeshConverter libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(MeshConverter libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(MeshConverter -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(MeshConverter libiconv.a)

    #target_link_libraries(MeshConverter stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(MeshConverter ${AUDIO_UNIT})
    target_link_libraries(MeshConverter stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(MeshConverter PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(MeshConverter -Wl,--start-group WPCore.a WPVehiclePhysics.a 
        WPApplication.a WPMath.a WPInterface.a WPSystem.a WPVehicle.a WPData.a WPMesh.a 
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
        target_link_libraries(MeshConverter libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(MeshConverter ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(MeshConverter ${CMAKE_DL_LIBS})
endif ()
else ()
if (WIN32)
    target_link_libraries(MeshConverter Workphone)
    target_link_libraries(MeshConverter WPProcedural)
    target_link_libraries(MeshConverter WPSQLite)
    target_link_libraries(MeshConverter WPVehicle.lib)
    target_link_libraries(MeshConverter WPWxWidgets.lib)
    target_link_libraries(MeshConverter WPGraphics)
    target_link_libraries(MeshConverter WPRenderUI.lib)
    target_link_libraries(MeshConverter WPAssimp)

    if (WP_BUILD_SDL)
        target_link_libraries(MeshConverter SDL2main.lib)
        target_link_libraries(MeshConverter SDL2_image.lib)
        target_link_libraries(MeshConverter debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshConverter WPImGui)
        target_link_libraries(MeshConverter imgui.lib)
    endif()

    if (WP_BUILD_OISINPUT)

        #target_link_libraries(MeshConverter debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(MeshConverter dinput8.lib)
        target_link_libraries(MeshConverter dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(MeshConverter libWPxsdk.lib)
        target_link_libraries(MeshConverter libWPxsdk-mt.lib)
        target_link_libraries(MeshConverter libxml2-mt.lib)
        target_link_libraries(MeshConverter tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshConverter FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshConverter freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshConverter WPGraphicsOgreNext.lib)

        target_link_libraries(MeshConverter harWPuzz.lib)
        target_link_libraries(MeshConverter harWPuzz-subset.lib)
        target_link_libraries(MeshConverter icucommon.lib)
        target_link_libraries(MeshConverter sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(MeshConverter d3d11.lib)
            target_link_libraries(MeshConverter dxgi.lib)
            target_link_libraries(MeshConverter dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(MeshConverter opengl32.lib)
            target_link_libraries(MeshConverter glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(MeshConverter amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshConverter WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(MeshConverter debug OgreMainStatic_d.lib
                                optimized OgreMainStatic.lib)

            target_link_libraries(MeshConverter debug OgreOverlayStatic_d.lib
                                optimized OgreOverlayStatic.lib)
        
            target_link_libraries(MeshConverter debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(MeshConverter debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(MeshConverter debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(MeshConverter debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(MeshConverter debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            #target_link_libraries(MeshConverter OgreHLMSStatic_d.lib)

            target_link_libraries(MeshConverter debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(MeshConverter debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(MeshConverter debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            #target_link_libraries(MeshConverter debug OgreTerrainStatic_d.lib
            #                            optimized OgreTerrainStatic.lib)
    
            #target_link_libraries(MeshConverter Plugin_CgProgramManagerStatic_d.lib)
            #target_link_libraries(MeshConverter cg.lib)
        endif()

        #target_link_libraries(MeshConverter freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(MeshConverter d3d9.lib)
            target_link_libraries(MeshConverter d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(MeshConverter d3d11.lib)
            target_link_libraries(MeshConverter dxgi.lib)
            target_link_libraries(MeshConverter dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(MeshConverter opengl32.lib)
            target_link_libraries(MeshConverter glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(MeshConverter amd_ags_x64.lib)
            endif()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(MeshConverter opengl32.lib)
            target_link_libraries(MeshConverter glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(MeshConverter amd_ags_x64.lib)
            endif()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(MeshConverter WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(MeshConverter bcrypt.lib)
    target_link_libraries(MeshConverter libboost_thread.lib)
    target_link_libraries(MeshConverter libboost_date_time.lib)
    target_link_libraries(MeshConverter libboost_chrono.lib)
    target_link_libraries(MeshConverter libboost_timer.lib)
    target_link_libraries(MeshConverter libboost_filesystem.lib)
    target_link_libraries(MeshConverter libboost_json.lib)
    target_link_libraries(MeshConverter minizip.lib)
    target_link_libraries(MeshConverter nativefiledialog.lib)
    target_link_libraries(MeshConverter tinyxml.lib)   
    target_link_libraries(MeshConverter zlib.lib)
    target_link_libraries(MeshConverter zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(MeshConverter tbb_static.lib)
			target_link_libraries(MeshConverter tbbmalloc_static.lib)
		else()
			target_link_libraries(MeshConverter tbb.lib)
			target_link_libraries(MeshConverter tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(MeshConverter libWPCore.a)
    target_link_libraries(MeshConverter libWPGraphics.a)
    target_link_libraries(MeshConverter libWPMesh.a)
    target_link_libraries(MeshConverter libWPProcedural.a)
    target_link_libraries(MeshConverter libWPPhysx.a)
    target_link_libraries(MeshConverter libWPAero.a)
    target_link_libraries(MeshConverter libWPVehicle.a)
    target_link_libraries(MeshConverter libWPAudio.a)

    target_link_libraries(MeshConverter libWPAssimp.a)

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(MeshConverter libWPWxWidgets.a)

        target_link_libraries(MeshConverter libwx_osx_cocoau_core-3.1.a)
        target_link_libraries(MeshConverter libwx_baseu-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_xrc-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_propgrid-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_richtext-3.1.a)
        target_link_libraries(MeshConverter libwx_baseu_xml-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_adv-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_aui-3.1.a)
        target_link_libraries(MeshConverter libwx_osx_cocoau_html-3.1.a)
        target_link_libraries(MeshConverter libwxtiff-3.1.a)
        target_link_libraries(MeshConverter libwxjpeg-3.1.a)
        target_link_libraries(MeshConverter libwxpng-3.1.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshConverter FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshConverter freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshConverter WPGraphicsOgreNext.a)

        target_link_libraries(MeshConverter harWPuzz.a)
        target_link_libraries(MeshConverter harWPuzz-subset.a)
        target_link_libraries(MeshConverter icucommon.a)
        target_link_libraries(MeshConverter sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshConverter WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(MeshConverter libWPGraphicsSoftware.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshConverter WPImGui.a)
        target_link_libraries(MeshConverter imgui.a)
    endif()

    if (WP_BUILD_OISINPUT)
        #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
    endif()

    target_link_libraries(MeshConverter libWPSQLite.a)

    target_link_libraries(MeshConverter libboost_thread.a)
    target_link_libraries(MeshConverter libboost_date_time.a)
    target_link_libraries(MeshConverter libboost_chrono.a)
    target_link_libraries(MeshConverter libboost_context.a)
    target_link_libraries(MeshConverter libboost_timer.a)
    target_link_libraries(MeshConverter libboost_filesystem.a)
    target_link_libraries(MeshConverter libboost_json.a)

    target_link_libraries(MeshConverter libPxTask.a)
    target_link_libraries(MeshConverter libPhysXVisualDebuggerSDK.a)
    target_link_libraries(MeshConverter libPvdRuntime.a)

    target_link_libraries(MeshConverter minizip.a)
    target_link_libraries(MeshConverter nativefiledialog.a)
    target_link_libraries(MeshConverter tinyxml.a)
    target_link_libraries(MeshConverter zlib.a)
    target_link_libraries(MeshConverter zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(MeshConverter libtbb.a)
        target_link_libraries(MeshConverter libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(MeshConverter libWm5Core.a)
        target_link_libraries(MeshConverter libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(MeshConverter libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(MeshConverter -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(MeshConverter libiconv.a)

    #target_link_libraries(MeshConverter stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(MeshConverter ${AUDIO_UNIT})
    target_link_libraries(MeshConverter stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(MeshConverter PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(MeshConverter -Wl,--start-group WPCore.a WPVehiclePhysics.a 
        WPApplication.a WPMath.a WPInterface.a WPSystem.a WPVehicle.a WPData.a WPMesh.a 
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
        target_link_libraries(MeshConverter libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(MeshConverter ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(MeshConverter ${CMAKE_DL_LIBS})
endif()
endif()

if (WIN32)
    set_property(TARGET MeshConverter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(MeshConverter Workphone)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        add_dependencies(MeshConverter WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(MeshConverter WPGraphicsOgre)
    endif()

    if (WP_BUILD_ODE)
        add_dependencies(MeshConverter WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(MeshConverter WPPhysx)
    endif()
endif()

set_target_properties (MeshConverter PROPERTIES FOLDER Tools/Cpp)
