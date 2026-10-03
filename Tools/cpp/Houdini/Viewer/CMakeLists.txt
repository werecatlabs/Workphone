project( HoudiniViewer )
set(CMAKE_CXX_STANDARD 17)


#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (_UNICODE)

find_library(CORW_GRAPHICS CoreGraphics)

if (APPLE)
    add_compile_definitions(__WXMAC__)
    add_compile_definitions(__WXOSX__)
    add_compile_definitions(__WXOSX_COCOA__)
endif()

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp")

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets  )
include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include )
include_directories( ${WP_ROOT}/WPEngine/cpp/Include)
include_directories(Source)

if (WP_GRAPHICS_SYSTEM_OGRE)
    include_directories( ${WP_ROOT}/WPEngine/cpp/Include/WPGraphicsOgre/Ogre/OgreMain/include )

    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
        include_directories( ${WP_ROOT}/WPEngine/cpp/Include/WPGraphicsOgre/BuildSettings/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
        include_directories( ${WP_ROOT}/WPEngine/cpp/Include/WPGraphicsOgre/BuildSettings/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
        include_directories( ${WP_ROOT}/WPEngine/cpp/Include/WPGraphicsOgre/BuildSettings/linux )
    endif()
endif()

if (WIN32)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc/wx )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
elseif (APPLE)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1  )
elseif (UNIX)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1 )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1/wx )
endif ()

if (WIN32)
    link_directories(HoudiniViewer ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wxWidgets/vc_x64_lib)

    if (WP_BUILD_OGRENEXT)
        link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_BUILD_OGRE)
        link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(HoudiniViewer ${WP_ROOT}/WPEngine/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/macOS)
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/macOS/boost)
    link_directories(HoudiniViewer ${WP_ROOT}/Dependencies/lib/macOS/tbb)
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/macOS)
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/macOS/physx)
    link_directories(HoudiniViewer ${WP_ROOT}/AdditionalLibraries/lib/macOS/wildmagic)
elseif (UNIX)
endif (WIN32)

# Add a library with two source files.
set( library_name HoudiniViewer )

if (WIN32)
    add_executable(${library_name} WIN32 ${HEADER_FILES} ${SOURCE_FILES})
elseif(APPLE)
    add_executable(${library_name} ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(${library_name} ${HEADER_FILES} ${SOURCE_FILES})
endif()

# Link against the Houdini libraries, and add required include directories and compile definitions.
target_link_libraries( ${library_name} Houdini )

# Configure several common target properties, such as its output directory.
houdini_configure_target( ${library_name} )

# fireblade libraries
if (WIN32)
    target_link_libraries(HoudiniViewer WPCore.lib)
    target_link_libraries(HoudiniViewer WPFileSystem.lib)
    target_link_libraries(HoudiniViewer WPMesh.lib)
    target_link_libraries(HoudiniViewer WPProcedural.lib)
    target_link_libraries(HoudiniViewer WPPhysx.lib)
    target_link_libraries(HoudiniViewer WPSQLite.lib)
    target_link_libraries(HoudiniViewer WPVehicle.lib)
    target_link_libraries(HoudiniViewer WPOISInput.lib)
    target_link_libraries(HoudiniViewer WPGraphics.lib)
    target_link_libraries(HoudiniViewer WPRenderUI.lib)

    #target_link_libraries(HoudiniViewer debug OIS_d.lib optimized OIS.lib)
    target_link_libraries(HoudiniViewer dinput8.lib)
    target_link_libraries(HoudiniViewer dxguid.lib)

    if (WP_USE_WPXSDK)
        #target_link_libraries(HoudiniViewer libWPxsdk.lib)
        target_link_libraries(HoudiniViewer libWPxsdk-mt.lib)
        target_link_libraries(HoudiniViewer libxml2-mt.lib)
        target_link_libraries(HoudiniViewer tinyxml2.lib)
    endif()

    if (WP_BUILD_OGRENEXT)
        target_link_libraries(HoudiniViewer WPGraphicsOgreNext.lib)
        #target_link_libraries(HoudiniViewer debug freetype_d.lib optimized freetype.lib)
    elseif(WP_BUILD_OGRE)
        target_link_libraries(HoudiniViewer WPGraphicsOgre.lib)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(HoudiniViewer debug FreeImage_d.lib
                                optimized FreeImage.lib)
        endif ()

        #target_link_libraries(HoudiniViewer freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(HoudiniViewer d3d9.lib)
            target_link_libraries(HoudiniViewer d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(HoudiniViewer d3d11.lib)
            target_link_libraries(HoudiniViewer dxgi.lib)
            target_link_libraries(HoudiniViewer dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(HoudiniViewer opengl32.lib)
            target_link_libraries(HoudiniViewer glu32.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(HoudiniViewer amd_ags_x64.lib)
            endif()
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(HoudiniViewer opengl32.lib)
            target_link_libraries(HoudiniViewer glu32.lib)
            
            if (WP_PLATFORM_X64)
                target_link_libraries(HoudiniViewer amd_ags_x64.lib)
            endif()
        endif ()

        if (WP_USE_FREETYPE)
            target_link_libraries(HoudiniViewer freetype.lib)
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(HoudiniViewer WPGraphicsD3D11.lib)
    endif()

    target_link_libraries(HoudiniViewer kernel32.lib)
    target_link_libraries(HoudiniViewer user32.lib)
    target_link_libraries(HoudiniViewer gdi32.lib)
    target_link_libraries(HoudiniViewer comdlg32.lib)
    target_link_libraries(HoudiniViewer winspool.lib)
    target_link_libraries(HoudiniViewer winmm.lib)
    target_link_libraries(HoudiniViewer shell32.lib)
    target_link_libraries(HoudiniViewer comctl32.lib)
    target_link_libraries(HoudiniViewer ole32.lib)
    target_link_libraries(HoudiniViewer oleaut32.lib)
    target_link_libraries(HoudiniViewer uuid.lib)
    target_link_libraries(HoudiniViewer rpcrt4.lib)
    target_link_libraries(HoudiniViewer advapi32.lib)
    target_link_libraries(HoudiniViewer wsock32.lib)
    target_link_libraries(HoudiniViewer wininet.lib)

    target_link_libraries(HoudiniViewer opengl32.lib)
    target_link_libraries(HoudiniViewer glu32.lib)
    target_link_libraries(HoudiniViewer d3d11.lib)
    target_link_libraries(HoudiniViewer dxgi.lib)
    target_link_libraries(HoudiniViewer dxguid.lib)
    #target_link_libraries(HoudiniViewer amd_ags_x64.lib)
elseif (APPLE)
    target_link_libraries(HoudiniViewer libWPCore.a)
    target_link_libraries(HoudiniViewer libWPFileSystem.a)
    target_link_libraries(HoudiniViewer libWPMath.a)
    target_link_libraries(HoudiniViewer libWPMesh.a)
    target_link_libraries(HoudiniViewer libWPProcedural.a)
    target_link_libraries(HoudiniViewer libWPPhysx.a)
    target_link_libraries(HoudiniViewer libWPData.a)
    target_link_libraries(HoudiniViewer libWPObjectTemplates.a)
    target_link_libraries(HoudiniViewer libWPAero.a)
    target_link_libraries(HoudiniViewer libWPVehicle.a)
    target_link_libraries(HoudiniViewer libWPAudio.a)
    target_link_libraries(HoudiniViewer libWPApplication.a)
    target_link_libraries(HoudiniViewer libWPWxWidgets.a)

    target_link_libraries(HoudiniViewer libwx_osx_cocoau_core-3.1.a)
    target_link_libraries(HoudiniViewer libwx_baseu-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_xrc-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_propgrid-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_richtext-3.1.a)
    target_link_libraries(HoudiniViewer libwx_baseu_xml-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_adv-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_aui-3.1.a)
    target_link_libraries(HoudiniViewer libwx_osx_cocoau_html-3.1.a)
    target_link_libraries(HoudiniViewer libwxtiff-3.1.a)
    target_link_libraries(HoudiniViewer libwxjpeg-3.1.a)
    target_link_libraries(HoudiniViewer libwxpng-3.1.a)

    if(WP_BUILD_SOFTWARE_RENDERER)
        target_link_libraries(HoudiniViewer libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(HoudiniViewer libWPSQLite.a)

    target_link_libraries(HoudiniViewer libboost_thread.a)
    target_link_libraries(HoudiniViewer libboost_date_time.a)
    target_link_libraries(HoudiniViewer libboost_chrono.a)
    target_link_libraries(HoudiniViewer libboost_timer.a)
    target_link_libraries(HoudiniViewer libboost_filesystem.a)
    target_link_libraries(HoudiniViewer libboost_system.a)

    target_link_libraries(HoudiniViewer libPxTask.a)
    target_link_libraries(HoudiniViewer libPhysXVisualDebuggerSDK.a)
    target_link_libraries(HoudiniViewer libPvdRuntime.a)

    target_link_libraries(HoudiniViewer debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(HoudiniViewer debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(HoudiniViewer libWm5Core.a)
        target_link_libraries(HoudiniViewer libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(HoudiniViewer libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(HoudiniViewer -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(HoudiniViewer libiconv.a)

    #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
    target_link_libraries(HoudiniViewer stdc++ "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(HoudiniViewer PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(HoudiniViewer -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPFileSystem.a WPMath.a WPInterface.a WPVehicle.a WPData.a WPMesh.a 
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
        target_link_libraries(HoudiniViewer libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(HoudiniViewer ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(HoudiniViewer ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    add_custom_command(TARGET  ${library_name} POST_BUILD
        COMMAND COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${library_name}> ${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/Debug
    )
endif ()


set_target_properties (HoudiniViewer PROPERTIES FOLDER Tools/Cpp)