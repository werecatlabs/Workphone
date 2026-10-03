set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(TestRacknetProgram)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (_UNICODE)

if (APPLE)
    add_compile_definitions(__WXMAC__)
    add_compile_definitions(__WXOSX__)
    add_compile_definitions(__WXOSX_COCOA__)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)

if (WP_ENABLE_RAKNET)
    include_directories( ${WP_ROOT}/AdditionalLibraries/raknet/Source )
endif()

include_directories( ${WP_ROOT}/WPEngine/cpp/Include )
include_directories(Source)

if (WP_BUILD_WXWIDGETS)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets  )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include )

    if (WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc )
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc/wx )
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
    elseif (APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1  )
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1/wx  )
    elseif (UNIX)
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1 )
        include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1/wx )
    endif ()
endif ()

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

if (WIN32)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/raknet/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/raknet/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wxwidgets/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(TestRacknetProgram ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

if (WIN32)
    add_executable(TestRacknetProgram WIN32 ${HEADER_FILES} ${SOURCE_FILES})
else()
    add_executable(TestRacknetProgram ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (WIN32)
target_link_options(TestRacknetProgram PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(TestRacknetProgram PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(TestRacknetProgram WPCore.lib)
    target_link_libraries(TestRacknetProgram WPMesh.lib)
    target_link_libraries(TestRacknetProgram WPProcedural.lib)
    target_link_libraries(TestRacknetProgram WPPhysx.lib)
    target_link_libraries(TestRacknetProgram WPSQLite.lib)
    target_link_libraries(TestRacknetProgram WPVehicle.lib)
    target_link_libraries(TestRacknetProgram WPWxWidgets.lib)
    target_link_libraries(TestRacknetProgram WPOISInput.lib)
    target_link_libraries(TestRacknetProgram WPGraphics.lib)
    target_link_libraries(TestRacknetProgram WPRenderUI.lib)
    target_link_libraries(TestRacknetProgram WPRakNet.lib)

    #target_link_libraries(TestRacknetProgram debug OIS_d.lib optimized OIS.lib)
    target_link_libraries(TestRacknetProgram dinput8.lib)
    target_link_libraries(TestRacknetProgram dxguid.lib)

    if (WP_USE_WPXSDK)
        #target_link_libraries(TestRacknetProgram libWPxsdk.lib)
        target_link_libraries(TestRacknetProgram libWPxsdk-mt.lib)
        target_link_libraries(TestRacknetProgram libxml2-mt.lib)
        target_link_libraries(TestRacknetProgram tinyxml2.lib)
    endif()

    target_link_libraries(TestRacknetProgram ws2_32.lib)

    if (WP_ENABLE_RAKNET)
        target_link_libraries(TestRacknetProgram debug RakNet_VS2008_LibStatic_Debug_x64.lib optimized RakNet_VS2008_LibStatic_Release_x64.lib)
    endif()

    if (WP_BUILD_WXWIDGETS)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_core.lib optimized wxmsw31u_core.lib)
        target_link_libraries(TestRacknetProgram debug wxbase31ud.lib optimized wxbase31u.lib)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_propgrid.lib optimized wxmsw31u_propgrid.lib)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_richtext.lib optimized wxmsw31u_richtext.lib)
        target_link_libraries(TestRacknetProgram debug wxbase31ud_xml.lib optimized wxbase31u_xml.lib)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_adv.lib optimized wxmsw31u_adv.lib)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_aui.lib optimized wxmsw31u_aui.lib)
        target_link_libraries(TestRacknetProgram debug wxmsw31ud_html.lib optimized wxmsw31u_html.lib)
        target_link_libraries(TestRacknetProgram debug wxtiffd.lib optimized wxtiff.lib)
        target_link_libraries(TestRacknetProgram debug wxjpegd.lib optimized wxjpeg.lib)
        target_link_libraries(TestRacknetProgram debug wxpngd.lib optimized wxpng.lib)
        target_link_libraries(TestRacknetProgram debug wxzlibd.lib optimized wxzlib.lib)
        target_link_libraries(TestRacknetProgram debug wxregexud.lib optimized wxregexu.lib)
        target_link_libraries(TestRacknetProgram debug wxexpatd.lib optimized wxexpat.lib)
    endif()

    target_link_libraries(TestRacknetProgram kernel32.lib)
    target_link_libraries(TestRacknetProgram user32.lib)
    target_link_libraries(TestRacknetProgram gdi32.lib)
    target_link_libraries(TestRacknetProgram comdlg32.lib)
    target_link_libraries(TestRacknetProgram winspool.lib)
    target_link_libraries(TestRacknetProgram winmm.lib)
    target_link_libraries(TestRacknetProgram shell32.lib)
    target_link_libraries(TestRacknetProgram comctl32.lib)
    target_link_libraries(TestRacknetProgram ole32.lib)
    target_link_libraries(TestRacknetProgram oleaut32.lib)
    target_link_libraries(TestRacknetProgram uuid.lib)
    target_link_libraries(TestRacknetProgram rpcrt4.lib)
    target_link_libraries(TestRacknetProgram advapi32.lib)
    target_link_libraries(TestRacknetProgram wsock32.lib)
    target_link_libraries(TestRacknetProgram wininet.lib)

    target_link_libraries(TestRacknetProgram bcrypt.lib)
    target_link_libraries(TestRacknetProgram libboost_thread.lib)
    target_link_libraries(TestRacknetProgram libboost_date_time.lib)
    target_link_libraries(TestRacknetProgram libboost_chrono.lib)
    target_link_libraries(TestRacknetProgram libboost_timer.lib)
    target_link_libraries(TestRacknetProgram libboost_filesystem.lib)
    target_link_libraries(TestRacknetProgram libboost_json.lib)
    target_link_libraries(TestRacknetProgram minizip.lib)
    target_link_libraries(TestRacknetProgram nativefiledialog.lib)
    target_link_libraries(TestRacknetProgram rttr.lib)
    target_link_libraries(TestRacknetProgram tinyxml.lib)   
    target_link_libraries(TestRacknetProgram yaml-cpp.lib)
    target_link_libraries(TestRacknetProgram zlib.lib)
    target_link_libraries(TestRacknetProgram zzip.lib)

    if (WP_USE_TBB)
        target_link_libraries(TestRacknetProgram tbb_static.lib)
        target_link_libraries(TestRacknetProgram tbbmalloc_static.lib)
    endif()
elseif (APPLE)
    target_link_libraries(TestRacknetProgram libWPCore.a)
    target_link_libraries(TestRacknetProgram libWPFileSystem.a)
    target_link_libraries(TestRacknetProgram libWPMath.a)
    target_link_libraries(TestRacknetProgram libWPMesh.a)
    target_link_libraries(TestRacknetProgram libWPProcedural.a)
    target_link_libraries(TestRacknetProgram libWPPhysx.a)
    target_link_libraries(TestRacknetProgram libWPData.a)
    target_link_libraries(TestRacknetProgram libWPAero.a)
    target_link_libraries(TestRacknetProgram libWPVehicle.a)
    target_link_libraries(TestRacknetProgram libWPAudio.a)
    target_link_libraries(TestRacknetProgram libWPApplication.a)
    target_link_libraries(TestRacknetProgram libWPRaknet.a)
    target_link_libraries(TestRacknetProgram libWPSQLite.a)

    target_link_libraries(TestRacknetProgram libRakNetLibStatic.a)

    if(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(TestRacknetProgram libWPGraphicsSoftware.a)
    endif()


    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_core-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_baseu-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_xrc-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_propgrid-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_richtext-3.1.a)
    target_link_libraries(TestRacknetProgram libwxexpat-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_baseu_xml-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_baseu_net-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_webview-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_xrc-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_adv-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_aui-3.1.a)
    target_link_libraries(TestRacknetProgram libwx_osx_cocoau_html-3.1.a)
    target_link_libraries(TestRacknetProgram libwxtiff-3.1.a)
    target_link_libraries(TestRacknetProgram libwxjpeg-3.1.a)
    target_link_libraries(TestRacknetProgram libwxpng-3.1.a)

    target_link_libraries(TestRacknetProgram libiconv.a)


    target_link_libraries(TestRacknetProgram libboost_thread.a)
    target_link_libraries(TestRacknetProgram libboost_date_time.a)
    target_link_libraries(TestRacknetProgram libboost_chrono.a)
    target_link_libraries(TestRacknetProgram libboost_timer.a)
    target_link_libraries(TestRacknetProgram libboost_filesystem.a)
    target_link_libraries(TestRacknetProgram libboost_unit_test_framework.a)
    target_link_libraries(TestRacknetProgram libboost_system.a)

    target_link_libraries(TestRacknetProgram libPxTask.a)
    target_link_libraries(TestRacknetProgram libPhysXVisualDebuggerSDK.a)
    target_link_libraries(TestRacknetProgram libPvdRuntime.a)

    target_link_libraries(TestRacknetProgram debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(TestRacknetProgram debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(TestRacknetProgram libWm5Core.a)
        target_link_libraries(TestRacknetProgram libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(TestRacknetProgram libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(TestRacknetProgram -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)

    target_link_libraries(TestRacknetProgram stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(TestRacknetProgram PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
elseif (UNIX)
    target_link_libraries(TestRacknetProgram -Wl,--start-group WPCore.a WPAero.a 
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
        target_link_libraries(TestRacknetProgram libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(TestRacknetProgram ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(TestRacknetProgram ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET TestRacknetProgram PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(TestRacknetProgram Workphone)

set_target_properties (TestRacknetProgram PROPERTIES FOLDER Tests/Cpp)
