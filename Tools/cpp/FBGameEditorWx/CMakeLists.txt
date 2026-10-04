project(WPGameEditorWx)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

add_compile_definitions (_WP_STATIC_LIB_)
add_compile_definitions (_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)
add_compile_definitions (_UNICODE)

if (APPLE)
    add_compile_definitions(__WXMAC__)
    add_compile_definitions(__WXOSX__)
    add_compile_definitions(__WXOSX_COCOA__)
elseif (UNIX)
    add_compile_definitions(__WXUNIX__)
    add_compile_definitions(__WXGTK__)
endif()

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "src/*.h")
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

set(WP_HEADER_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/WPGameEditorWx/src")
set(WP_SOURCE_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/WPGameEditorWx/src")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Header Files\\${_group_file_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${SOURCE_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_SOURCE_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "/" "\\" _group_file_path "${_group_file_path}")
    source_group("Source Files\\${_group_file_path}" FILES "${_source}")
endforeach()

include_directories( ../../Dependencies/boost)
include_directories( ../../Dependencies/tinyxml/include)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ../../AdditionalLibraries/cppsqlite/include)
include_directories( ../../AdditionalLibraries/sqlite/include)
include_directories( ../../AdditionalLibraries/wxWidgets  )
include_directories( ../../AdditionalLibraries/wxWidgets/include )

if (WIN32)
    include_directories( ../../AdditionalLibraries/wxWidgets/include/msvc )
    include_directories( ../../AdditionalLibraries/wxWidgets/include/msvc/wx )
    include_directories( ../../AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
elseif (APPLE)
    include_directories( ../../AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1  )
elseif (UNIX)
    include_directories( ../../AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1 )
    include_directories( ../../AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1/wx )
endif ()

include_directories( ../../WPEngine/Include)
include_directories(${CMAKE_SOURCE_DIR}/Tools/WPGameEditorWx/src)

if (WIN32)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})   
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()

    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tinybind)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(WPGameEditorWx ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPGameEditorWx "GameEditorPCH.h" "${CMAKE_SOURCE_DIR}/Tools/WPGameEditorWx/src/GameEditorPCH.cpp")
endif()

if (WIN32)
    add_executable(WPGameEditorWx WIN32 ${HEADER_FILES} ${SOURCE_FILES})
elseif(APPLE)
    add_executable(WPGameEditorWx ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(WPGameEditorWx ${HEADER_FILES} ${SOURCE_FILES})
endif()




if (WIN32)
    target_link_libraries(WPGameEditorWx WPCore.lib)
    target_link_libraries(WPGameEditorWx WPFileSystem.lib)
    target_link_libraries(WPGameEditorWx WPMath.lib)
    target_link_libraries(WPGameEditorWx WPMesh.lib)
    target_link_libraries(WPGameEditorWx WPProcedural)
    target_link_libraries(WPGameEditorWx WPSQLite)
    target_link_libraries(WPGameEditorWx WPPhysx.lib)
    target_link_libraries(WPGameEditorWx WPData.lib)
    target_link_libraries(WPGameEditorWx WPSQLite)
    target_link_libraries(WPGameEditorWx WPObjectTemplates.lib)
    target_link_libraries(WPGameEditorWx WPVehicle.lib)
    target_link_libraries(WPGameEditorWx WPWxWidgets.lib)
    target_link_libraries(WPGameEditorWx WPObjectTemplates.lib)
    target_link_libraries(WPGameEditorWx.lib)
    target_link_libraries(WPGameEditorWx WPState.lib)
    target_link_libraries(WPGameEditorWx WPGraphics)
    target_link_libraries(WPGameEditorWx WPRenderUI.lib)
    target_link_libraries(WPGameEditorWx WPCPlusPlus.lib)
    target_link_libraries(WPGameEditorWx WPAssimp)

    if (WP_USE_WPXSDK)
        #target_link_libraries(WPGameEditorWx libWPxsdk.lib)
        target_link_libraries(WPGameEditorWx libWPxsdk-mt.lib)
        target_link_libraries(WPGameEditorWx libxml2-mt.lib)
        target_link_libraries(WPGameEditorWx tinyxml2.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPGameEditorWx WPGraphicsOgreNext.lib)

        #target_link_libraries(WPGameEditorWx debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
        #target_link_libraries(WPGameEditorWx debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)
        #target_link_libraries(WPGameEditorWx debug RenderSystem_Direct3D11Static_d.lib optimized RenderSystem_Direct3D11Static.lib)
        #target_link_libraries(WPGameEditorWx debug RenderSystem_GL3PlusStatic_d.lib optimized RenderSystem_GL3PlusStatic.lib)
        #target_link_libraries(WPGameEditorWx debug OgreHlmsPbsStatic_d.lib optimized OgreHlmsPbsStatic.lib)
        #target_link_libraries(WPGameEditorWx debug OgreHlmsUnlitStatic_d.lib optimized OgreHlmsUnlitStatic.lib)

        target_link_libraries(WPGameEditorWx debug FreeImage_d.lib optimized FreeImage.lib)
        target_link_libraries(WPGameEditorWx debug freetype_d.lib optimized freetype.lib)
        target_link_libraries(WPGameEditorWx debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(WPGameEditorWx WPGraphicsOgre.lib)

        target_link_libraries(WPGameEditorWx debug OgreMainStatic_d.lib
                            optimized OgreMainStatic.lib)

        target_link_libraries(WPGameEditorWx debug OgreOverlayStatic_d.lib
                            optimized OgreOverlayStatic.lib)
        
        target_link_libraries(WPGameEditorWx debug RenderSystem_Direct3D9Static_d.lib
                            optimized RenderSystem_Direct3D9Static.lib)

        target_link_libraries(WPGameEditorWx debug RenderSystem_Direct3D11Static_d.lib
                            optimized RenderSystem_Direct3D11Static.lib)

        target_link_libraries(WPGameEditorWx debug RenderSystem_GL3PlusStatic_d.lib 
                            optimized RenderSystem_GL3PlusStatic.lib)

        target_link_libraries(WPGameEditorWx debug RenderSystem_GLStatic_d.lib
                            optimized RenderSystem_GLStatic.lib)

        target_link_libraries(WPGameEditorWx debug OgreGLSupportStatic_d.lib
                            optimized OgreGLSupportStatic.lib)

        #target_link_libraries(WPGameEditorWx OgreHLMSStatic_d.lib)

        target_link_libraries(WPGameEditorWx debug OgreRTShaderSystemStatic_d.lib
                                   optimized OgreRTShaderSystemStatic.lib)

        target_link_libraries(WPGameEditorWx debug Codec_STBIStatic_d.lib
                                   optimized Codec_STBIStatic.lib)

        target_link_libraries(WPGameEditorWx debug OgreBitesStatic_d.lib
                                   optimized OgreBitesStatic.lib)

        #target_link_libraries(WPGameEditorWx debug OgreTerrainStatic_d.lib
        #                            optimized OgreTerrainStatic.lib)
    
        #target_link_libraries(WPGameEditorWx Plugin_CgProgramManagerStatic_d.lib)
        #target_link_libraries(WPGameEditorWx cg.lib)

        #target_link_libraries(WPGameEditorWx debug FreeImage_d.lib
         #                       optimized FreeImage.lib)
        target_link_libraries(WPGameEditorWx freetype.lib)

        target_link_libraries(WPGameEditorWx d3d9.lib)
        target_link_libraries(WPGameEditorWx d3dx9.lib)
    elseif(WP_BUILD_DX11)
        target_link_libraries(WPGameEditorWx WPGraphicsD3D11.lib)
    endif()

    #target_link_libraries(WPGameEditorWx tinybind.lib)

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPGameEditorWx WPPhysx.lib)

        if (WP_PLATFORM_X64)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3DEBUG_x64.lib
                        optimized PhysX3_x64.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3CommonDEBUG_x64.lib 
                        optimized PhysX3Common_x64.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3CookingDEBUG_x64.lib
                        optimized PhysX3Cooking_x64.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        else ()
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3DEBUG_x86.lib
                        optimized PhysX3_x86.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3CommonDEBUG_x86.lib 
                        optimized PhysX3Common_x86.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysX3CookingDEBUG_x86.lib
                        optimized PhysX3Cooking_x86.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(WPGameEditorWx 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        endif()
    endif()

    target_link_libraries(WPGameEditorWx debug wxmsw31ud_core.lib optimized wxmsw31u_core.lib)
    target_link_libraries(WPGameEditorWx debug wxbase31ud.lib optimized wxbase31u.lib)
    target_link_libraries(WPGameEditorWx debug wxmsw31ud_propgrid.lib optimized wxmsw31u_propgrid.lib)
    target_link_libraries(WPGameEditorWx debug wxmsw31ud_richtext.lib optimized wxmsw31u_richtext.lib)
    target_link_libraries(WPGameEditorWx debug wxbase31ud_xml.lib optimized wxbase31u_xml.lib)
    target_link_libraries(WPGameEditorWx debug wxmsw31ud_adv.lib optimized wxmsw31u_adv.lib)
    target_link_libraries(WPGameEditorWx debug wxmsw31ud_aui.lib optimized wxmsw31u_aui.lib)
    target_link_libraries(WPGameEditorWx debug wxmsw31ud_html.lib optimized wxmsw31u_html.lib)
    target_link_libraries(WPGameEditorWx debug wxtiffd.lib optimized wxtiff.lib)
    target_link_libraries(WPGameEditorWx debug wxjpegd.lib optimized wxjpeg.lib)
    target_link_libraries(WPGameEditorWx debug wxpngd.lib optimized wxpng.lib)
    target_link_libraries(WPGameEditorWx debug wxzlibd.lib optimized wxzlib.lib)
    target_link_libraries(WPGameEditorWx debug wxregexud.lib optimized wxregexu.lib)
    target_link_libraries(WPGameEditorWx debug wxexpatd.lib optimized wxexpat.lib)

    target_link_libraries(WPGameEditorWx kernel32.lib)
    target_link_libraries(WPGameEditorWx user32.lib)
    target_link_libraries(WPGameEditorWx gdi32.lib)
    target_link_libraries(WPGameEditorWx comdlg32.lib)
    target_link_libraries(WPGameEditorWx winspool.lib)
    target_link_libraries(WPGameEditorWx winmm.lib)
    target_link_libraries(WPGameEditorWx shell32.lib)
    target_link_libraries(WPGameEditorWx comctl32.lib)
    target_link_libraries(WPGameEditorWx ole32.lib)
    target_link_libraries(WPGameEditorWx oleaut32.lib)
    target_link_libraries(WPGameEditorWx uuid.lib)
    target_link_libraries(WPGameEditorWx rpcrt4.lib)
    target_link_libraries(WPGameEditorWx advapi32.lib)
    target_link_libraries(WPGameEditorWx wsock32.lib)
    target_link_libraries(WPGameEditorWx wininet.lib)

    target_link_libraries(WPGameEditorWx opengl32.lib)
    target_link_libraries(WPGameEditorWx glu32.lib)
    target_link_libraries(WPGameEditorWx d3d11.lib)
    target_link_libraries(WPGameEditorWx dxgi.lib)
    target_link_libraries(WPGameEditorWx dxguid.lib)

    target_link_libraries(WPGameEditorWx amd_ags_x64.lib)

    target_link_libraries(WPGameEditorWx debug assimp-vc142-mtd.lib
                            optimized assimp-vc142-mt.lib)

    target_link_libraries(WPGameEditorWx 
                        debug Wm5CoreD.lib
                        optimized Wm5Core.lib)
    target_link_libraries(WPGameEditorWx 
                        debug Wm5MathematicsD.lib
                        optimized Wm5Mathematics.lib)
    target_link_libraries(WPGameEditorWx 
                        debug Wm5PhysicsD.lib
                        optimized Wm5Physics.lib)

elseif (APPLE)
    # fireblade libraries
    target_link_libraries(WPGameEditorWx libWPAnimation.a)
    target_link_libraries(WPGameEditorWx libWPApplication.a)
    target_link_libraries(WPGameEditorWx libWPCore.a)
    target_link_libraries(WPGameEditorWx libWPFileSystem.a)
    target_link_libraries(WPGameEditorWx libWPMath.a)
    target_link_libraries(WPGameEditorWx libWPMesh.a)
    target_link_libraries(WPGameEditorWx libWPProcedural.a)

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPGameEditorWx libWPPhysx.a)
    elseif (WP_BUILD_ODE)
            target_link_libraries(WPGameEditorWx libWPODE3.a)
    endif()

    target_link_libraries(WPGameEditorWx libWPData.a)
    target_link_libraries(WPGameEditorWx libWPObjectTemplates.a)
    target_link_libraries(WPGameEditorWx libWPVehicle.a)
    target_link_libraries(WPGameEditorWx libWPState.a)
    target_link_libraries(WPGameEditorWx libWPWxWidgets.a)
    target_link_libraries(WPGameEditorWx libWPRenderUI.a)
    target_link_libraries(WPGameEditorWx libWPSQLite.a)
    target_link_libraries(WPGameEditorWx libWPCPlusPlus.a)
    target_link_libraries(WPGameEditorWx libWPGraphics.a)

    target_link_libraries(WPGameEditorWx libboost_thread.a)
    target_link_libraries(WPGameEditorWx libboost_date_time.a)
    target_link_libraries(WPGameEditorWx libboost_chrono.a)
    target_link_libraries(WPGameEditorWx libboost_timer.a)
    target_link_libraries(WPGameEditorWx libboost_filesystem.a)

    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_core-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_baseu-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_xrc-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_propgrid-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_richtext-3.1.a)
    target_link_libraries(WPGameEditorWx libwxexpat-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_baseu_xml-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_baseu_net-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_webview-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_xrc-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_adv-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_aui-3.1.a)
    target_link_libraries(WPGameEditorWx libwx_osx_cocoau_html-3.1.a)
    target_link_libraries(WPGameEditorWx libwxtiff-3.1.a)
    target_link_libraries(WPGameEditorWx libwxjpeg-3.1.a)
    target_link_libraries(WPGameEditorWx libwxpng-3.1.a)

    target_link_libraries(WPGameEditorWx libiconv.a)

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPGameEditorWx libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a)

        target_link_libraries(WPGameEditorWx libPxTask.a)
        target_link_libraries(WPGameEditorWx libPhysXVisualDebuggerSDK.a)
        target_link_libraries(WPGameEditorWx libPvdRuntime.a)
    endif()



    target_link_libraries(WPGameEditorWx libtbb.a)
    target_link_libraries(WPGameEditorWx libtbbmalloc.a)

    target_link_libraries(WPGameEditorWx libWm5Core.a)
    target_link_libraries(WPGameEditorWx libWm5Mathematics.a)

    target_link_libraries(WPGameEditorWx debug libassimpd.a optimized libassimp.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPGameEditorWx WPGraphicsOgreNext.a)

        #target_link_libraries(WPGameEditorWx libOgreMainStatic.a)
        #target_link_libraries(WPGameEditorWx libOgreOverlayStatic.a)
        #target_link_libraries(WPGameEditorWx libRenderSystem_GL3PlusStatic.a)
        #target_link_libraries(WPGameEditorWx libRenderSystem_MetalStatic.a)
        #target_link_libraries(WPGameEditorWx libOgreHlmsPbsStatic.a)
        #target_link_libraries(WPGameEditorWx libOgreHlmsUnlitStatic.a)

        #target_link_libraries(WPGameEditorWx FreeImage.a)
        target_link_libraries(WPGameEditorWx freetype.a)
    endif()

    #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
    target_link_libraries(WPGameEditorWx stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(WPGameEditorWx PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(WPGameEditorWx -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPFileSystem.a WPMath.a WPMesh.a WPVehicle.a WPData.a WPState.a 
        libWPObjectTemplates.a libWPVehicle.a WPGraphics.a WPGraphicsOgreNext.a         
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libWm5Core.a libWm5Mathematics.a
        -Wl,--end-group )

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPGameEditorWx WPGraphicsOgreNext.a)
        target_link_libraries(WPGameEditorWx freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(WPGameEditorWx libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(WPGameEditorWx png)
    target_link_libraries(WPGameEditorWx Xaw)
    target_link_libraries(WPGameEditorWx Xrandr)
    target_link_libraries(WPGameEditorWx Xt)
    target_link_libraries(WPGameEditorWx xcb)
    target_link_libraries(WPGameEditorWx GL)
    target_link_libraries(WPGameEditorWx ${X11_LIBRARIES})
    target_link_libraries(WPGameEditorWx ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(WPGameEditorWx ${CMAKE_DL_LIBS})
endif ()

add_dependencies(WPGameEditorWx WPAnimationWPCPlusPlus WPGraphics WPAero WPAi WPAudio WPCore WPData WPFileSystem WPMath WPMesh WPObjectTemplates WPProcedural WPSQLite WPObjectTemplates WPState WPVehicle WPRenderUI)

if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
    add_dependencies(WPGameEditorWx WPGraphicsOgreNext)
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(WPGameEditorWx WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(WPGameEditorWx WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(WPGameEditorWx WPPhysx)
endif()

if (WP_BUILD_WXWIDGETS)
    add_dependencies(WPGameEditorWx WPWxWidgets)
endif()

if (WIN32)
    #set_property(TARGET WPGameEditorWx PROPERTY VS_DEBUGGER_COMMAND ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug/WPGameEditorWx.exe)
    #set_property(TARGET WPGameEditorWx PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug)

    set_target_properties(WPGameEditorWx PROPERTIES 
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/$(Configuration)")

    set_target_properties (WPGameEditorWx PROPERTIES FOLDER Tools)
endif()