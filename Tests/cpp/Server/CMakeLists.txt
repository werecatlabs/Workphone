set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(Server)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)
add_compile_definitions (_UNICODE)
add_compile_definitions (wxUSE_UNICODE=1)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets  )
include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include )

if (WIN32)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/msvc/wx )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
elseif (APPLE)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1 )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/gtk3-unicode-3.1/wx )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/lib/osx/wx/include/osx_cocoa-unicode-static-3.1/wx  )
elseif (UNIX)
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1 )
    include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include/osx_cocoa-unicode-static-3.1/wx )
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
    link_directories(Server ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(Server ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS//${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(Server ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(Server ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(Server ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(Server ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(Server PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(Server PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(Server WPCore.lib)
    target_link_libraries(Server WPMesh.lib)
    target_link_libraries(Server WPProcedural.lib)
    target_link_libraries(Server WPPhysx.lib)
    target_link_libraries(Server WPSQLite.lib)
    target_link_libraries(Server WPVehicle.lib)
    target_link_libraries(Server WPWxWidgets.lib)
    target_link_libraries(Server WPOISInput.lib)
    target_link_libraries(Server WPGraphics.lib)
    target_link_libraries(Server WPRenderUI.lib)
elseif (APPLE)
    target_link_libraries(Server libWPCore.a)
    target_link_libraries(Server libWPMesh.a)
    target_link_libraries(Server libWPProcedural.a)
    target_link_libraries(Server libWPPhysx.a)
    target_link_libraries(Server libWPAero.a)
    target_link_libraries(Server libWPVehicle.a)
    target_link_libraries(Server libWPAudio.a)

    if(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(Server libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(Server libWPSQLite.a)

    target_link_libraries(Server libboost_thread.a)
    target_link_libraries(Server libboost_date_time.a)
    target_link_libraries(Server libboost_chrono.a)
    target_link_libraries(Server libboost_timer.a)
    target_link_libraries(Server libboost_filesystem.a)
    target_link_libraries(Server libboost_unit_test_framework.a)

    target_link_libraries(Server minizip.a)
    target_link_libraries(Server nativefiledialog.a)
    target_link_libraries(Server rttr.a)
    target_link_libraries(Server tinyxml.a)
    target_link_libraries(Server yaml-cpp.a)
    target_link_libraries(Server zlib.a)
    target_link_libraries(Server zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(Server libtbb_static.a)
        target_link_libraries(Server libtbbmalloc_static.a)
    endif()
elseif (UNIX)
    target_link_libraries(Server -Wl,--start-group WPCore.a WPAero.a 
        WPVehicle.a WPMesh.a
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
        target_link_libraries(Server libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(Server ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(Server ${CMAKE_DL_LIBS})
endif ()


if (WIN32)
elseif (APPLE)
    target_link_directories(Server PUBLIC ../Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(Server -L../dependencies/lib/linux/boost/)
elseif (UNIX)
    target_link_libraries(Server -L../dependencies/lib/linux/boost/)
    target_link_libraries(Server -L../dependencies/lib/linux/physx/)
    target_link_libraries(Server -L../dependencies/lib/linux/)
endif (WIN32)

if (WIN32)
    set_property(TARGET Server PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(Server Workphone)

set_target_properties (Server PROPERTIES FOLDER Tests/Cpp )
