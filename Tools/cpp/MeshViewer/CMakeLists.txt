set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(MeshViewer)

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
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl_image/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        
        if (WP_PLATFORM_X64)
            link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/sdl2/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(MeshViewer ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h" "./*.hpp")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(MeshViewer "MeshViewerPCH.hpp" "${CMAKE_SOURCE_DIR}/Tools/cpp/MeshViewer/MeshViewerPCH.cpp")
endif()

if (WIN32)
    add_executable(MeshViewer WIN32 ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(MeshViewer ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(MeshViewer Workphone.lib)
    target_link_libraries(MeshViewer WPProcedural.lib)
    target_link_libraries(MeshViewer WPSQLite.lib)
    target_link_libraries(MeshViewer WPAssimp.lib)

    if (WP_BUILD_SDL)
        target_link_libraries(MeshViewer SDL2main.lib)
        target_link_libraries(MeshViewer SDL2_image.lib)
        target_link_libraries(MeshViewer debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_OISINPUT)
        target_link_libraries(MeshViewer WPOISInput.lib)

        #target_link_libraries(MeshViewer debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(MeshViewer dinput8.lib)
        target_link_libraries(MeshViewer dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(MeshViewer libWPxsdk.lib)
        target_link_libraries(MeshViewer libWPxsdk-mt.lib)
        target_link_libraries(MeshViewer libxml2-mt.lib)
        target_link_libraries(MeshViewer tinyxml2.lib)
    endif()

    target_link_libraries(MeshViewer bcrypt.lib)
    target_link_libraries(MeshViewer libboost_thread.lib)
    target_link_libraries(MeshViewer libboost_date_time.lib)
    target_link_libraries(MeshViewer libboost_chrono.lib)
    target_link_libraries(MeshViewer libboost_timer.lib)
    target_link_libraries(MeshViewer libboost_filesystem.lib)
    target_link_libraries(MeshViewer libboost_json.lib)
    target_link_libraries(MeshViewer minizip.lib)
    target_link_libraries(MeshViewer nativefiledialog.lib)
    target_link_libraries(MeshViewer rttr.lib)
    target_link_libraries(MeshViewer tinyxml.lib)   
    target_link_libraries(MeshViewer yaml-cpp.lib)
    target_link_libraries(MeshViewer zlib.lib)
    target_link_libraries(MeshViewer zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(MeshViewer tbb_static.lib)
			target_link_libraries(MeshViewer tbbmalloc_static.lib)
		else()
			target_link_libraries(MeshViewer tbb.lib)
			target_link_libraries(MeshViewer tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(MeshViewer libWPCore.a)
    target_link_libraries(MeshViewer libWPGraphics.a)
    target_link_libraries(MeshViewer libWPMesh.a)
    target_link_libraries(MeshViewer libWPProcedural.a)
    target_link_libraries(MeshViewer libWPPhysx.a)
    target_link_libraries(MeshViewer libWPAero.a)
    target_link_libraries(MeshViewer libWPVehicle.a)
    target_link_libraries(MeshViewer libWPAudio.a)

    target_link_libraries(MeshViewer libWPAssimp.a)

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(MeshViewer libWPWxWidgets.a)

        target_link_libraries(MeshViewer libwx_osx_cocoau_core-3.1.a)
        target_link_libraries(MeshViewer libwx_baseu-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_xrc-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_propgrid-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_richtext-3.1.a)
        target_link_libraries(MeshViewer libwx_baseu_xml-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_adv-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_aui-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_html-3.1.a)
        target_link_libraries(MeshViewer libwxtiff-3.1.a)
        target_link_libraries(MeshViewer libwxjpeg-3.1.a)
        target_link_libraries(MeshViewer libwxpng-3.1.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshViewer FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshViewer freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshViewer WPGraphicsOgreNext.a)

        target_link_libraries(MeshViewer harWPuzz.a)
        target_link_libraries(MeshViewer harWPuzz-subset.a)
        target_link_libraries(MeshViewer icucommon.a)
        target_link_libraries(MeshViewer sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshViewer WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(MeshViewer libWPGraphicsSoftware.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshViewer WPImGui.a)
        target_link_libraries(MeshViewer imgui.a)
    endif()

    if (WP_BUILD_OISINPUT)
        target_link_libraries(MeshViewer libWPOISInput.a)
        #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
    endif()

    target_link_libraries(MeshViewer libWPSQLite.a)

    target_link_libraries(MeshViewer libboost_thread.a)
    target_link_libraries(MeshViewer libboost_date_time.a)
    target_link_libraries(MeshViewer libboost_chrono.a)
    target_link_libraries(MeshViewer libboost_context.a)
    target_link_libraries(MeshViewer libboost_timer.a)
    target_link_libraries(MeshViewer libboost_filesystem.a)
    target_link_libraries(MeshViewer libboost_json.a)

    target_link_libraries(MeshViewer libPxTask.a)
    target_link_libraries(MeshViewer libPhysXVisualDebuggerSDK.a)
    target_link_libraries(MeshViewer libPvdRuntime.a)

    target_link_libraries(MeshViewer minizip.a)
    target_link_libraries(MeshViewer nativefiledialog.a)
    target_link_libraries(MeshViewer rttr.a)
    target_link_libraries(MeshViewer tinyxml.a)
    target_link_libraries(MeshViewer yaml-cpp.a)
    target_link_libraries(MeshViewer zlib.a)
    target_link_libraries(MeshViewer zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(MeshViewer libtbb.a)
        target_link_libraries(MeshViewer libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(MeshViewer libWm5Core.a)
        target_link_libraries(MeshViewer libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(MeshViewer libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(MeshViewer -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(MeshViewer libiconv.a)

    #target_link_libraries(MeshViewer stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(MeshViewer ${AUDIO_UNIT})
    target_link_libraries(MeshViewer stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(MeshViewer PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(MeshViewer -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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
        target_link_libraries(MeshViewer libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(MeshViewer ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(MeshViewer ${CMAKE_DL_LIBS})
endif ()
else ()
if (WIN32)
    target_link_libraries(MeshViewer Workphone.lib)
    target_link_libraries(MeshViewer WPProcedural.lib)
    target_link_libraries(MeshViewer WPSQLite.lib)
    target_link_libraries(MeshViewer WPWxWidgets.lib)
    target_link_libraries(MeshViewer WPGraphics.lib)
    target_link_libraries(MeshViewer WPRenderUI.lib)
    target_link_libraries(MeshViewer WPAssimp.lib)

    if (WP_BUILD_SDL)
        target_link_libraries(MeshViewer SDL2main.lib)
        target_link_libraries(MeshViewer SDL2_image.lib)
        target_link_libraries(MeshViewer debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshViewer WPImGui.lib)
        target_link_libraries(MeshViewer imgui.lib)
    endif()

    if (WP_BUILD_OISINPUT)
        target_link_libraries(MeshViewer WPOISInput.lib)

        #target_link_libraries(MeshViewer debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(MeshViewer dinput8.lib)
        target_link_libraries(MeshViewer dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(MeshViewer libWPxsdk.lib)
        target_link_libraries(MeshViewer libWPxsdk-mt.lib)
        target_link_libraries(MeshViewer libxml2-mt.lib)
        target_link_libraries(MeshViewer tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshViewer FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshViewer freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshViewer WPGraphicsOgreNext.lib)

        target_link_libraries(MeshViewer harWPuzz.lib)
        target_link_libraries(MeshViewer harWPuzz-subset.lib)
        target_link_libraries(MeshViewer icucommon.lib)
        target_link_libraries(MeshViewer sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(MeshViewer d3d11.lib)
            target_link_libraries(MeshViewer dxgi.lib)
            target_link_libraries(MeshViewer dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(MeshViewer opengl32.lib)
            target_link_libraries(MeshViewer glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            target_link_libraries(MeshViewer amd_ags_x64.lib)
        endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshViewer WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(MeshViewer debug OgreMainStatic_d.lib
                                optimized OgreMainStatic.lib)

            target_link_libraries(MeshViewer debug OgreOverlayStatic_d.lib
                                optimized OgreOverlayStatic.lib)
        
            target_link_libraries(MeshViewer debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(MeshViewer debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(MeshViewer debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(MeshViewer debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(MeshViewer debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            #target_link_libraries(MeshViewer OgreHLMSStatic_d.lib)

            target_link_libraries(MeshViewer debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(MeshViewer debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(MeshViewer debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            #target_link_libraries(MeshViewer debug OgreTerrainStatic_d.lib
            #                            optimized OgreTerrainStatic.lib)
    
            #target_link_libraries(MeshViewer Plugin_CgProgramManagerStatic_d.lib)
            #target_link_libraries(MeshViewer cg.lib)
        endif()

        #target_link_libraries(MeshViewer freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(MeshViewer d3d9.lib)
            target_link_libraries(MeshViewer d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(MeshViewer d3d11.lib)
            target_link_libraries(MeshViewer dxgi.lib)
            target_link_libraries(MeshViewer dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(MeshViewer opengl32.lib)
            target_link_libraries(MeshViewer glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(MeshViewer amd_ags_x64.lib)
            endif()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(MeshViewer opengl32.lib)
            target_link_libraries(MeshViewer glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(MeshViewer amd_ags_x64.lib)
            endif()
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(MeshViewer WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(MeshViewer bcrypt.lib)
    target_link_libraries(MeshViewer libboost_thread.lib)
    target_link_libraries(MeshViewer libboost_date_time.lib)
    target_link_libraries(MeshViewer libboost_chrono.lib)
    target_link_libraries(MeshViewer libboost_timer.lib)
    target_link_libraries(MeshViewer libboost_filesystem.lib)
    target_link_libraries(MeshViewer libboost_json.lib)
    target_link_libraries(MeshViewer minizip.lib)
    target_link_libraries(MeshViewer nativefiledialog.lib)
    target_link_libraries(MeshViewer rttr.lib)
    target_link_libraries(MeshViewer tinyxml.lib)   
    target_link_libraries(MeshViewer yaml-cpp.lib)
    target_link_libraries(MeshViewer zlib.lib)
    target_link_libraries(MeshViewer zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(MeshViewer tbb_static.lib)
			target_link_libraries(MeshViewer tbbmalloc_static.lib)
		else()
			target_link_libraries(MeshViewer tbb.lib)
			target_link_libraries(MeshViewer tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    target_link_libraries(MeshViewer libWPCore.a)
    target_link_libraries(MeshViewer libWPGraphics.a)
    target_link_libraries(MeshViewer libWPMesh.a)
    target_link_libraries(MeshViewer libWPProcedural.a)
    target_link_libraries(MeshViewer libWPPhysx.a)
    target_link_libraries(MeshViewer libWPAero.a)
    target_link_libraries(MeshViewer libWPVehicle.a)
    target_link_libraries(MeshViewer libWPAudio.a)

    target_link_libraries(MeshViewer libWPAssimp.a)

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(MeshViewer libWPWxWidgets.a)

        target_link_libraries(MeshViewer libwx_osx_cocoau_core-3.1.a)
        target_link_libraries(MeshViewer libwx_baseu-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_xrc-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_propgrid-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_richtext-3.1.a)
        target_link_libraries(MeshViewer libwx_baseu_xml-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_adv-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_aui-3.1.a)
        target_link_libraries(MeshViewer libwx_osx_cocoau_html-3.1.a)
        target_link_libraries(MeshViewer libwxtiff-3.1.a)
        target_link_libraries(MeshViewer libwxjpeg-3.1.a)
        target_link_libraries(MeshViewer libwxpng-3.1.a)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(MeshViewer FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(MeshViewer freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(MeshViewer WPGraphicsOgreNext.a)

        target_link_libraries(MeshViewer harWPuzz.a)
        target_link_libraries(MeshViewer harWPuzz-subset.a)
        target_link_libraries(MeshViewer icucommon.a)
        target_link_libraries(MeshViewer sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(MeshViewer WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(MeshViewer libWPGraphicsSoftware.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(MeshViewer WPImGui.a)
        target_link_libraries(MeshViewer imgui.a)
    endif()

    if (WP_BUILD_OISINPUT)
        target_link_libraries(MeshViewer libWPOISInput.a)
        #target_link_libraries(Editor debug libOIS_d.a optimized libOIS.a)
    endif()

    target_link_libraries(MeshViewer libWPSQLite.a)

    target_link_libraries(MeshViewer libboost_thread.a)
    target_link_libraries(MeshViewer libboost_date_time.a)
    target_link_libraries(MeshViewer libboost_chrono.a)
    target_link_libraries(MeshViewer libboost_context.a)
    target_link_libraries(MeshViewer libboost_timer.a)
    target_link_libraries(MeshViewer libboost_filesystem.a)
    target_link_libraries(MeshViewer libboost_json.a)

    target_link_libraries(MeshViewer libPxTask.a)
    target_link_libraries(MeshViewer libPhysXVisualDebuggerSDK.a)
    target_link_libraries(MeshViewer libPvdRuntime.a)

    target_link_libraries(MeshViewer minizip.a)
    target_link_libraries(MeshViewer nativefiledialog.a)
    target_link_libraries(MeshViewer rttr.a)
    target_link_libraries(MeshViewer tinyxml.a)
    target_link_libraries(MeshViewer yaml-cpp.a)
    target_link_libraries(MeshViewer zlib.a)
    target_link_libraries(MeshViewer zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(MeshViewer libtbb.a)
        target_link_libraries(MeshViewer libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(MeshViewer libWm5Core.a)
        target_link_libraries(MeshViewer libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(MeshViewer libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(MeshViewer -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(MeshViewer libiconv.a)

    #target_link_libraries(MeshViewer stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(MeshViewer ${AUDIO_UNIT})
    target_link_libraries(MeshViewer stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(MeshViewer PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(MeshViewer -Wl,--start-group WPCore.a WPVehiclePhysics.a 
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
        target_link_libraries(MeshViewer libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(MeshViewer ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(MeshViewer ${CMAKE_DL_LIBS})
endif()
endif()

if (WIN32)
    set_property(TARGET MeshViewer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}/Bin/Media/OgreNext" WP_MESHVIEWER_OGRENEXT_MEDIA_DIR)
    file(GENERATE OUTPUT "$<TARGET_FILE_DIR:MeshViewer>/resources2.cfg" CONTENT
"# Generated for MeshViewer.
[Essential]
Zip=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/packs/DebugPack.zip
Zip=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/packs/CornellBox.zip

[Popular]
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/Compositors
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/models
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/materials/textures
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/materials/textures/Cubemaps
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Cubemaps

[General]
FileSystem=./
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common/Any
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common/GLSL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common/GLSLES
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common/HLSL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Common/Metal
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Hlms/Common/Any
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Hlms/Common/GLSL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Hlms/Common/HLSL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Hlms/Common/Metal
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Compute/Algorithms/IBL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/Compute/Tools/Any
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Tutorial_Terrain
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Tutorial_Terrain/HLSL
FileSystem=${WP_MESHVIEWER_OGRENEXT_MEDIA_DIR}/2.0/scripts/materials/Postprocessing/SceneAssets
")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(MeshViewer Workphone)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        add_dependencies(MeshViewer WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(MeshViewer WPGraphicsOgre)
    endif()

    if (WP_BUILD_ODE)
        add_dependencies(MeshViewer WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(MeshViewer WPPhysx)
    endif()
endif()

set_target_properties (MeshViewer PROPERTIES FOLDER Tools/Cpp)
