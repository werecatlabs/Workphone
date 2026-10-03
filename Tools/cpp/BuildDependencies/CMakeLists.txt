set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(ZipObfuscator)


#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/zziplib)
include_directories( ${WP_ROOT}/WPEngine/cpp/Include)
include_directories(Source)

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
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(BuildDependencies ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(BuildDependencies ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(BuildDependencies PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(BuildDependencies PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)


# fireblade libraries
if (WIN32)
    target_link_libraries(BuildDependencies WPCore.lib)
    target_link_libraries(BuildDependencies WPMesh.lib)
    target_link_libraries(BuildDependencies WPProcedural.lib)
    target_link_libraries(BuildDependencies WPPhysx.lib)
    target_link_libraries(BuildDependencies WPSQLite.lib)
    target_link_libraries(BuildDependencies WPVehicle.lib)
    target_link_libraries(BuildDependencies WPWxWidgets.lib)
    target_link_libraries(BuildDependencies WPOISInput.lib)
    target_link_libraries(BuildDependencies WPGraphics.lib)
    target_link_libraries(BuildDependencies WPRenderUI.lib)
elseif (APPLE)
    target_link_libraries(BuildDependencies libWPCore.a)
    target_link_libraries(BuildDependencies libWPFileSystem.a)
    target_link_libraries(BuildDependencies libWPMesh.a)
    target_link_libraries(BuildDependencies libWPProcedural.a)
    target_link_libraries(BuildDependencies libWPPhysx.a)
    target_link_libraries(BuildDependencies libWPAero.a)
    target_link_libraries(BuildDependencies libWPVehicle.a)
    target_link_libraries(BuildDependencies libWPAudio.a)
    target_link_libraries(BuildDependencies libWPApplication.a)

    if(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(BuildDependencies libWPGraphicsSoftware.a)
    endif()

    target_link_libraries(BuildDependencies libWPSQLite.a)

    target_link_libraries(BuildDependencies libboost_thread.a)
    target_link_libraries(BuildDependencies libboost_date_time.a)
    target_link_libraries(BuildDependencies libboost_chrono.a)
    target_link_libraries(BuildDependencies libboost_timer.a)
    target_link_libraries(BuildDependencies libboost_filesystem.a)
    target_link_libraries(BuildDependencies libboost_unit_test_framework.a)
    target_link_libraries(BuildDependencies libboost_system.a)

    target_link_libraries(BuildDependencies libPxTask.a)
    target_link_libraries(BuildDependencies libPhysXVisualDebuggerSDK.a)
    target_link_libraries(BuildDependencies libPvdRuntime.a)

    target_link_libraries(BuildDependencies debug libtbb_debug.a optimized libtbb.a)
    target_link_libraries(BuildDependencies debug libtbbmalloc_debug.a optimized libtbbmalloc.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(BuildDependencies libWm5Core.a)
        target_link_libraries(BuildDependencies libWm5Mathematics.a)
    endif()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(BuildDependencies libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a )
    elseif (UNIX)
        target_link_libraries(BuildDependencies -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a -Wl,--end-group )
    endif (WIN32)
elseif (UNIX)
    target_link_libraries(BuildDependencies -Wl,--start-group WPCore.a WPAero.a 
        WPApplication.a WPMath.a WPInterface.a WPVehicle.a WPData.a WPMesh.a 
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
        target_link_libraries(BuildDependencies libWm5Core.a libWm5Mathematics.a)
    endif()

    target_link_libraries(BuildDependencies ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(BuildDependencies ${CMAKE_DL_LIBS})
endif ()


if (WIN32)
elseif (APPLE)
    target_link_directories(BuildDependencies PUBLIC ../Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(BuildDependencies -L../dependencies/lib/linux/boost/)
elseif (UNIX)
    target_link_libraries(BuildDependencies -L../dependencies/lib/linux/boost/)
    target_link_libraries(BuildDependencies -L../dependencies/lib/linux/physx/)
    target_link_libraries(BuildDependencies -L../dependencies/lib/linux/)
endif (WIN32)

if (WIN32)
    set_property(TARGET BuildDependencies PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${WP_ROOT}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/Debug)
endif()


add_dependencies(BuildDependencies WPRenderUI WPAero WPAudio WPCore WPMesh WPProcedural WPSQLite WPVehicle)

if (WP_BUILD_ODE)
    add_dependencies(BuildDependencies WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(BuildDependencies WPPhysx)
endif()

set_target_properties (BuildDependencies PROPERTIES FOLDER Tools/Cpp)
