set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SampleVehicleAdvanced)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)

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
    if(WIN32)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
    endif()

    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/Components/Overlay/include )
endif()

include_directories( ${WP_ROOT}/Engine/cpp/Include )
if (WIN32 AND WP_GRAPHICS_SYSTEM_CLAW)
    include_directories( ${WP_ROOT}/Engine/c/Include )
endif()
include_directories(Source)

if (WIN32)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ois/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicleAdvanced ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

cmake_policy(PUSH)
cmake_policy(SET CMP0112 NEW)
add_executable(SampleVehicleAdvanced ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(SampleVehicleAdvanced Workphone)
elseif (APPLE)
    # fireblade libraries
    target_link_libraries(SampleVehicleAdvanced libWPCore.a)
    target_link_libraries(SampleVehicleAdvanced libWPGraphics.a)
    target_link_libraries(SampleVehicleAdvanced libWPMesh.a)
    target_link_libraries(SampleVehicleAdvanced libWPPhysx.a)
    target_link_libraries(SampleVehicleAdvanced libWPSQLite.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleVehicleAdvanced FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleVehicleAdvanced freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleVehicleAdvanced WPGraphicsOgreNext.a)
        target_link_libraries(SampleVehicleAdvanced harWPuzz.a)
        target_link_libraries(SampleVehicleAdvanced harWPuzz-subset.a)
        target_link_libraries(SampleVehicleAdvanced icucommon.a)
        target_link_libraries(SampleVehicleAdvanced sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleVehicleAdvanced WPGraphicsOgre.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleVehicleAdvanced WPPhysx.a)

        target_link_libraries(SampleVehicleAdvanced PhysX3.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3Extensions.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3Common.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3Vehicle.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3Cooking.a)
        target_link_libraries(SampleVehicleAdvanced PhysXProfileSDK.a)
        target_link_libraries(SampleVehicleAdvanced PxTask.a)
        target_link_libraries(SampleVehicleAdvanced PhysXVisualDebuggerSDK.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3MetaData.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3Foundation.a)
        target_link_libraries(SampleVehicleAdvanced PxSceneQuery.a)
        target_link_libraries(SampleVehicleAdvanced PxSimulationController.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3LowLevel.a)
        target_link_libraries(SampleVehicleAdvanced PhysX3LowLevelCloth.a)
    endif()

    target_link_libraries(SampleVehicleAdvanced libboost_thread.a)
    target_link_libraries(SampleVehicleAdvanced libboost_date_time.a)
    target_link_libraries(SampleVehicleAdvanced libboost_chrono.a)
    target_link_libraries(SampleVehicleAdvanced libboost_context.a)
    target_link_libraries(SampleVehicleAdvanced libboost_timer.a)
    target_link_libraries(SampleVehicleAdvanced libboost_filesystem.a)
    target_link_libraries(SampleVehicleAdvanced libboost_json.a)

    target_link_libraries(SampleVehicleAdvanced minizip.a)
    target_link_libraries(SampleVehicleAdvanced nativefiledialog.a)
    target_link_libraries(SampleVehicleAdvanced tinyxml.a)
    target_link_libraries(SampleVehicleAdvanced zlib.a)
    target_link_libraries(SampleVehicleAdvanced zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(SampleVehicleAdvanced tbb_static.a)
        target_link_libraries(SampleVehicleAdvanced tbbmalloc_static.a)
    endif ()

    target_link_libraries(SampleVehicleAdvanced debug libOIS_d.a optimized libOIS.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(SampleVehicleAdvanced libWm5Core.a)
        target_link_libraries(SampleVehicleAdvanced libWm5Mathematics.a)
    endif()

    #target_link_libraries(SampleVehicleAdvanced stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(SampleVehicleAdvanced ${AUDIO_UNIT})
    target_link_libraries(SampleVehicleAdvanced stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(SampleVehicleAdvanced PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(SampleVehicleAdvanced -Wl,--start-group WPCore.a WPMesh.a 
        libWPSQLite.a libWPPhysx.a WPAudio.a         
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
        libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
        libWm5Core.a libWm5Mathematics.a
        -Wl,--end-group )
        
    target_link_libraries(SampleVehicleAdvanced ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(SampleVehicleAdvanced ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET SampleVehicleAdvanced PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_STATIC_LIB)
    target_link_libraries(SampleVehicleAdvanced WPVehiclePhysics)
endif()

set_target_properties (SampleVehicleAdvanced PROPERTIES FOLDER Samples)

if(WP_GRAPHICS_SYSTEM_CLAW)
    target_include_directories(SampleVehicleAdvanced PRIVATE
        ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
        ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
    target_link_libraries(SampleVehicleAdvanced WPGraphics WorkphoneGraphics)
endif()

add_dependencies(SampleVehicleAdvanced WPProcedural)
target_link_libraries(SampleVehicleAdvanced WPProcedural)

# Keep the sample self-contained, including asset-only incremental builds.
add_custom_target(VehicleAdvancedAudio
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SampleVehicleAdvanced>/Audio/VehicleAdvanced"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Audio"
        "$<TARGET_FILE_DIR:SampleVehicleAdvanced>/Audio/VehicleAdvanced"
    VERBATIM)
cmake_policy(POP)
set_target_properties(VehicleAdvancedAudio PROPERTIES FOLDER Samples)
add_dependencies(SampleVehicleAdvanced VehicleAdvancedAudio)
if(TARGET WPAudio)
    add_dependencies(SampleVehicleAdvanced WPAudio)
endif()
