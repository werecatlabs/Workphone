set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(TextureViewer)

add_compile_definitions (_WP_STATIC_LIB_)
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
include_directories( ${CMAKE_SOURCE_DIR}/WPEngine/Include)
include_directories("./")

if (WP_GRAPHICS_SYSTEM_OGRE)
    include_directories( ../../AdditionalLibraries/ogre/include )
    include_directories( ../../AdditionalLibraries/ogre/OgreMain/include )

    include_directories( ../../AdditionalLibraries/ogre/Components/Overlay/include )
    include_directories( ../../AdditionalLibraries/ogre/Components/Terrain/include )
    include_directories( ../../AdditionalLibraries/ogre/Components/Paging/include )
    include_directories( ../../AdditionalLibraries/ogre/Components/Bites/include )
    include_directories( ../../AdditionalLibraries/particle_universe/include )
    include_directories( ../../AdditionalLibraries/ogre/PlugIns/CgProgramManager/include )
    include_directories( ../../AdditionalLibraries/ogre/RenderSystems/Direct3D9/include )
    include_directories( ../../AdditionalLibraries/ogre/RenderSystems/Direct3D11/include )
    include_directories( ../../AdditionalLibraries/ogre/RenderSystems/GL/include )
    include_directories( ../../AdditionalLibraries/directx/include )
    include_directories( ../../AdditionalLibraries/ogre/PlugIns/STBICodec/include )
    include_directories( ../../AdditionalLibraries/ogre/Components/RTShaderSystem/include )

    if(WIN32)
        include_directories( ../../AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ../../AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ../../AdditionalLibraries/ogre/include/linux )
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
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl_image/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        
        if (WP_PLATFORM_X64)
            link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(TextureViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(TextureViewer "TextureViewerPCH.h" "${CMAKE_SOURCE_DIR}/Tools/TextureViewer/TextureViewerPCH.cpp")
endif()

if (WIN32)
    add_executable(TextureViewer WIN32 ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(TextureViewer ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (WIN32)
target_link_options(TextureViewer PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(TextureViewer PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(TextureViewer WPCore.lib)
    target_link_libraries(TextureViewer WPFileSystem.lib)
    target_link_libraries(TextureViewer WPMath.lib)
    target_link_libraries(TextureViewer WPMesh.lib)
    target_link_libraries(TextureViewer WPProcedural)
    target_link_libraries(TextureViewer WPPhysx.lib)
    target_link_libraries(TextureViewer WPData.lib)
    target_link_libraries(TextureViewer WPSQLite)
    target_link_libraries(TextureViewer WPObjectTemplates.lib)
    target_link_libraries(TextureViewer WPVehicle.lib)
    target_link_libraries(TextureViewer WPWxWidgets.lib)
    target_link_libraries(TextureViewer WPObjectTemplates.lib)
    target_link_libraries(TextureViewer.lib)
    target_link_libraries(TextureViewer WPState.lib)
    target_link_libraries(TextureViewer WPGraphics)
    target_link_libraries(TextureViewer WPRenderUI.lib)
    target_link_libraries(TextureViewer WPAssimp)

    if (WP_BUILD_SDL)
        target_link_libraries(TextureViewer SDL2main.lib)
        target_link_libraries(TextureViewer SDL2_image.lib)
        target_link_libraries(TextureViewer debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(TextureViewer WPImGui)
    endif()

    if (WP_BUILD_OISINPUT)

        #target_link_libraries(WPGameEditor debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(WPGameEditor dinput8.lib)
        target_link_libraries(WPGameEditor dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(TextureViewer libWPxsdk.lib)
        target_link_libraries(TextureViewer libWPxsdk-mt.lib)
        target_link_libraries(TextureViewer libxml2-mt.lib)
        target_link_libraries(TextureViewer tinyxml2.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(TextureViewer WPGraphicsOgreNext.lib)

        #target_link_libraries(TextureViewer debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
        #target_link_libraries(TextureViewer debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)
        #target_link_libraries(TextureViewer debug RenderSystem_Direct3D11Static_d.lib optimized RenderSystem_Direct3D11Static.lib)
        #target_link_libraries(TextureViewer debug RenderSystem_GL3PlusStatic_d.lib optimized RenderSystem_GL3PlusStatic.lib)
        #target_link_libraries(TextureViewer debug OgreHlmsPbsStatic_d.lib optimized OgreHlmsPbsStatic.lib)
        #target_link_libraries(TextureViewer debug OgreHlmsUnlitStatic_d.lib optimized OgreHlmsUnlitStatic.lib)

        target_link_libraries(TextureViewer debug FreeImage_d.lib optimized FreeImage.lib)
        target_link_libraries(TextureViewer debug freetype_d.lib optimized freetype.lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(TextureViewer WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(TextureViewer debug OgreMainStatic_d.lib
                                optimized OgreMainStatic.lib)

            target_link_libraries(TextureViewer debug OgreOverlayStatic_d.lib
                                optimized OgreOverlayStatic.lib)
        
            target_link_libraries(TextureViewer debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(TextureViewer debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(TextureViewer debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(TextureViewer debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(TextureViewer debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            #target_link_libraries(TextureViewer OgreHLMSStatic_d.lib)

            target_link_libraries(TextureViewer debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(TextureViewer debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(TextureViewer debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            #target_link_libraries(TextureViewer debug OgreTerrainStatic_d.lib
            #                            optimized OgreTerrainStatic.lib)
    
            #target_link_libraries(TextureViewer Plugin_CgProgramManagerStatic_d.lib)
            #target_link_libraries(TextureViewer cg.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(TextureViewer debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(TextureViewer freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(TextureViewer d3d9.lib)
            target_link_libraries(TextureViewer d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(TextureViewer d3d11.lib)
            target_link_libraries(TextureViewer dxgi.lib)
            target_link_libraries(TextureViewer dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(TextureViewer opengl32.lib)
            target_link_libraries(TextureViewer glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(TextureViewer amd_ags_x64.lib)
            endif()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(TextureViewer opengl32.lib)
            target_link_libraries(TextureViewer glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(TextureViewer amd_ags_x64.lib)
            endif()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(TextureViewer WPGraphicsD3D11.lib)
    endif()
elseif (APPLE)
    target_link_libraries(TextureViewer libWPCore.a)

    target_link_libraries(TextureViewer libWPGraphics.a)
    target_link_libraries(TextureViewer libWPFileSystem.a)
    target_link_libraries(TextureViewer libWPMath.a)
    target_link_libraries(TextureViewer libWPMesh.a)
    target_link_libraries(TextureViewer libWPProcedural.a)
    target_link_libraries(TextureViewer libWPPhysx.a)
    target_link_libraries(TextureViewer libWPData.a)
    target_link_libraries(TextureViewer libWPObjectTemplates.a)
    target_link_libraries(TextureViewer libWPAero.a)
    target_link_libraries(TextureViewer libWPVehicle.a)
    target_link_libraries(TextureViewer libWPAudio.a)
    target_link_libraries(TextureViewer libWPApplication.a)
    target_link_libraries(TextureViewer libWPState.a)

    target_link_libraries(TextureViewer libWPAssimp.a)

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(TextureViewer libWPWxWidgets.a)

        target_link_libraries(TextureViewer libwx_osx_cocoau_core-3.1.a)
        target_link_libraries(TextureViewer libwx_baseu-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_xrc-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_propgrid-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_richtext-3.1.a)
        target_link_libraries(TextureViewer libwx_baseu_xml-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_adv-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_aui-3.1.a)
        target_link_libraries(TextureViewer libwx_osx_cocoau_html-3.1.a)
        target_link_libraries(TextureViewer libwxtiff-3.1.a)
        target_link_libraries(TextureViewer libwxjpeg-3.1.a)
        target_link_libraries(TextureViewer libwxpng-3.1.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(TextureViewer WPGraphicsOgreNext.a)
        target_link_libraries(TextureViewer freetype.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(TextureViewer WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(TextureViewer libWPGraphicsSoftware.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(TextureViewer WPImGui.a)
    endif()

    target_link_libraries(TextureViewer libWPSQLite.a)

    target_link_libraries(TextureViewer libboost_thread.a)
    target_link_libraries(TextureViewer libboost_date_time.a)
    target_link_libraries(TextureViewer libboost_chrono.a)
    target_link_libraries(TextureViewer libboost_timer.a)
    target_link_libraries(TextureViewer libboost_filesystem.a)
    target_link_libraries(TextureViewer libboost_system.a)

    target_link_libraries(TextureViewer libPxTask.a)
    target_link_libraries(TextureViewer libPhysXVisualDebuggerSDK.a)
    target_link_libraries(TextureViewer libPvdRuntime.a)

    target_link_libraries(TextureViewer debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(TextureViewer debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(TextureViewer libWm5Core.a)
        target_link_libraries(TextureViewer libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(TextureViewer libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(TextureViewer -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(TextureViewer libiconv.a)

    #target_link_libraries(TextureViewer stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(TextureViewer ${AUDIO_UNIT})
    target_link_libraries(TextureViewer stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(TextureViewer PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(TextureViewer -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPFileSystem.a WPMath.a WPInterface.a WPSystem.a WPVehicle.a WPData.a WPMesh.a 
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
        target_link_libraries(TextureViewer libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(TextureViewer ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(TextureViewer ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET TextureViewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${WP_ROOT}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/Debug)
endif()


add_dependencies(TextureViewerWPRenderUI WPAero WPAi WPAudio WPCore WPData WPFileSystem WPMath WPMesh WPObjectTemplates WPProcedural WPSQLite WPObjectTemplates WPVehicle)

if (WP_BUILD_ODE)
    add_dependencies(TextureViewer WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(TextureViewer WPPhysx)
endif()

set_target_properties (TextureViewer PROPERTIES FOLDER Tools)
