set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(UnitTests)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

add_compile_definitions (BOOST_PYTHON_STATIC_LIB)
#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/Dependencies/cppsqlite/include)
include_directories( ${WP_ROOT}/Dependencies/entt/src )
include_directories( ${WP_ROOT}/Dependencies/python/PC )
include_directories( ${WP_ROOT}/Dependencies/python/Include )
include_directories( ${WP_ROOT}/Dependencies/sqlite/include)
include_directories( ${WP_ROOT}/Dependencies/fmod/studio/api/studio/inc )
include_directories( ${WP_ROOT}/Dependencies/fmod/studio/api/lowlevel/inc )
include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc/luabind )
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneCore )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphonePhysics )
include_directories( ../../Include/WPFMODStudio )
include_directories(./)

include_directories( ${WP_ROOT}/Dependencies/physx )
include_directories( ${WP_ROOT}/Dependencies/physx/Include )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/common )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/extensions )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/foundation )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/foundation/include )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/physxvisualdebuggersdk )

include_directories( ../../Include/WPGraphicsOgre/Plugins/ParticleUniverse/include )

if (WP_GRAPHICS_SYSTEM_OGRE)
    include_directories( ${WP_ROOT}/Dependencies/ogre/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/OgreMain/include )

    if(WIN32)
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/win32 )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/osx )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/osx )
    else()
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/linux )
        include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgre/BuildSettings/linux )
    endif()
endif()

include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/nativefiledialog/include )

if (WIN32)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/python/)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/fmod/)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        
        if (WP_PLATFORM_X64)
            link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/directx/Lib/x64)
        else ()
            link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(UnitTests ${WP_ROOT}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS)

    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/boost)
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS)
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/${CMAKE_BUILD_TYPE}/fmod)

    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${WP_ROOT}/Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/ogre/)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/ogre/dependencies/lib/)
        link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost/${WP_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/physx/${WP_BUILD_TYPE})
    link_directories(UnitTests ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "*.hpp" "*.hpp")
file(GLOB_RECURSE SOURCE_FILES "*.c" "*.cpp")
list(APPEND SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/ParameterTests.cpp")
list(REMOVE_DUPLICATES SOURCE_FILES)

set(WP_HEADER_REL_PATH "${WP_ROOT}/Tests/cpp/UnitTests")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Tests/cpp/UnitTests")

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

add_executable(UnitTests ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(UnitTests PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(UnitTests PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(UnitTests Workphone.lib )
        target_link_libraries(UnitTests WorkphoneCollision.lib )
        target_link_libraries(UnitTests WorkphoneCore.lib )
        target_link_libraries(UnitTests WorkphonePhysics.lib )
        target_link_libraries(UnitTests WPVehiclePhysics.lib )
        target_link_libraries(UnitTests WPSQLite.lib )
        target_link_libraries(UnitTests WPProcedural.lib )

        target_link_libraries(UnitTests bcrypt.lib)
        target_link_libraries(UnitTests libboost_thread.lib)
        target_link_libraries(UnitTests libboost_date_time.lib)
        target_link_libraries(UnitTests libboost_chrono.lib)
        target_link_libraries(UnitTests libboost_context.lib)
        target_link_libraries(UnitTests libboost_timer.lib)
        target_link_libraries(UnitTests libboost_filesystem.lib)
        target_link_libraries(UnitTests libboost_json.lib)
        target_link_libraries(UnitTests minizip.lib)
        target_link_libraries(UnitTests nativefiledialog.lib)
        target_link_libraries(UnitTests rttr.lib)
        target_link_libraries(UnitTests tinyxml.lib)   
        target_link_libraries(UnitTests yaml-cpp.lib)
        target_link_libraries(UnitTests zlib.lib)
        target_link_libraries(UnitTests zzip.lib)

        if (WP_ENABLE_LUA)
            target_link_libraries(UnitTests WPLuaBind.lib)
            target_link_libraries(UnitTests lua.lib)
            target_link_libraries(UnitTests luabind.lib)
        endif()
    elseif (APPLE)
        target_link_libraries(UnitTests libWPAero.a)
        target_link_libraries(UnitTests libWPCore.a)
        target_link_libraries(UnitTests libWPMesh.a)
        target_link_libraries(UnitTests libWPProcedural.a)
        target_link_libraries(UnitTests libWPVehicle.a)
        target_link_libraries(UnitTests libWPAudio.a)
        target_link_libraries(UnitTests libWPTest.a )
        target_link_libraries(UnitTests WPGraphics.a)
        target_link_libraries(UnitTests WPRenderUI.a)

        if (WP_ENABLE_LUA)
            target_link_libraries(UnitTests WPLua.a)
            target_link_libraries(UnitTests WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(UnitTests WPPython.a)
            target_link_libraries(UnitTests WPPythonBind.a)
        endif()

        if (WP_BUILD_FMOD)
            target_link_libraries(UnitTests libfmod.dylib )
            target_link_libraries(UnitTests libfmodstudio.dylib )
        endif()

        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(UnitTests WPOISInput.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(UnitTests WPGraphicsOgreNext.a)

            target_link_libraries(UnitTests harWPuzz.a)
            target_link_libraries(UnitTests harWPuzz-subset.a)
            target_link_libraries(UnitTests icucommon.a)
            target_link_libraries(UnitTests sds_library.a)

            target_link_libraries(UnitTests FreeImage.a)
            target_link_libraries(UnitTests freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(UnitTests WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(UnitTests libWPGraphicsSoftware.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(UnitTests WPPhysx.a)

            target_link_libraries(UnitTests PhysX3.a)
            target_link_libraries(UnitTests PhysX3Extensions.a)
            target_link_libraries(UnitTests PhysX3Common.a)
            target_link_libraries(UnitTests PhysX3Vehicle.a)
            target_link_libraries(UnitTests PhysX3Cooking.a)
            target_link_libraries(UnitTests PhysXProfileSDK.a)
            target_link_libraries(UnitTests PxTask.a)
            target_link_libraries(UnitTests PhysXVisualDebuggerSDK.a)
            target_link_libraries(UnitTests PhysX3MetaData.a)
            target_link_libraries(UnitTests PhysX3Foundation.a)
            target_link_libraries(UnitTests PxSceneQuery.a)
            target_link_libraries(UnitTests PxSimulationController.a)
            target_link_libraries(UnitTests PhysX3LowLevel.a)
            target_link_libraries(UnitTests PhysX3LowLevelCloth.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(UnitTests libWPODE3.a)
        endif()

        target_link_libraries(UnitTests libWPSQLite.a)

        target_link_libraries(UnitTests libboost_thread.a)
        target_link_libraries(UnitTests libboost_date_time.a)
        target_link_libraries(UnitTests libboost_chrono.a)
        target_link_libraries(UnitTests libboost_context.a)
        target_link_libraries(UnitTests libboost_timer.a)
        target_link_libraries(UnitTests libboost_filesystem.a)
        target_link_libraries(UnitTests libboost_unit_test_framework.a)
        target_link_libraries(UnitTests libboost_json.a)

        target_link_libraries(UnitTests minizip.a)
        target_link_libraries(UnitTests nativefiledialog.a)
        target_link_libraries(UnitTests rttr.a)
        target_link_libraries(UnitTests tinyxml.a)
        target_link_libraries(UnitTests yaml-cpp.a)
        target_link_libraries(UnitTests zlib.a)
        target_link_libraries(UnitTests zzip.a)

        if (WP_USE_TBB)
            target_link_libraries(UnitTests tbb.a)
            target_link_libraries(UnitTests tbbmalloc.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
            #target_link_libraries(UnitTests debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(UnitTests libWm5Core.a)
            target_link_libraries(UnitTests libWm5Mathematics.a)
        endif()

        target_link_libraries(UnitTests libzlib.a)
        target_link_libraries(UnitTests libzziplib.a)

        target_link_libraries(UnitTests stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(UnitTests PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(UnitTests -Wl,--start-group WPCore.a WPVehiclePhysics.a WPAssimp.a libWPTest.a
                WPApplication.a WPVehicle.a WPMesh.a WPGraphics.a
                libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a
                WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
                png Xaw Xrandr Xt xcb GL
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

            #target_link_libraries(UnitTests -Wl,--start-group WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -Wl,--end-group )

            target_link_libraries(UnitTests freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(UnitTests libWPGraphicsSoftware.a)
        else()
            target_link_libraries(UnitTests -Wl,--start-group WPCore.a WPVehiclePhysics.a 
                WPApplication.a.a WPVehicle.a WPMesh.a WPGraphics.a
                libWPObjectTemplates.a libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a        
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
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(UnitTests libWm5Core.a)
            target_link_libraries(UnitTests libWm5Mathematics.a)
        endif()
    
        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
        endif()

        target_link_libraries(UnitTests png)
        target_link_libraries(UnitTests Xaw)
        target_link_libraries(UnitTests Xrandr)
        target_link_libraries(UnitTests Xt)
        target_link_libraries(UnitTests xcb)
        target_link_libraries(UnitTests ${X11_LIBRARIES})
        target_link_libraries(UnitTests ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(UnitTests ${CMAKE_DL_LIBS})
    endif ()
else()
    if (WIN32)
        target_link_libraries(UnitTests Workphone.lib )
        target_link_libraries(UnitTests WPAerodynamics.lib )
        target_link_libraries(UnitTests WPVehicle.lib )
        target_link_libraries(UnitTests WPSQLite.lib )
        target_link_libraries(UnitTests WPProcedural.lib )
        target_link_libraries(UnitTests WPGraphics.lib)

        target_link_libraries(UnitTests WPAudio.lib)
        target_link_libraries(UnitTests winmm.lib)
        target_link_libraries(UnitTests xaudio2.lib)

        if (WP_ENABLE_LUA)
            target_link_libraries(UnitTests WPLua.lib)
            target_link_libraries(UnitTests WPLuaBind.lib)
            target_link_libraries(UnitTests lua.lib)
            target_link_libraries(UnitTests luabind.lib)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(UnitTests WPPython.lib)
            target_link_libraries(UnitTests WPPythonBind.lib)
        endif()

        if (WP_BUILD_FMOD)
            target_link_libraries(UnitTests fmod64_vc.lib )
            target_link_libraries(UnitTests fmodstudio64_vc.lib )
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(UnitTests WPOISInput.lib)

            #target_link_libraries(UnitTests debug OIS_d.lib optimized OIS.lib)
            target_link_libraries(UnitTests dinput8.lib)
            target_link_libraries(UnitTests dxguid.lib)
        endif()

        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.lib)
        endif()
    
        target_link_libraries(UnitTests WPRenderUI.lib)

        if (WP_USE_WPXSDK)
            #target_link_libraries(UnitTests libWPxsdk.lib)
            target_link_libraries(UnitTests libWPxsdk-mt.lib)
            target_link_libraries(UnitTests libxml2-mt.lib)
            target_link_libraries(UnitTests tinyxml2.lib)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(UnitTests debug FreeImage_d.lib optimized FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(UnitTests debug freetype_d.lib optimized freetype.lib)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(UnitTests WPGraphicsOgreNext.lib)

            target_link_libraries(UnitTests harWPuzz.lib)
            target_link_libraries(UnitTests harWPuzz-subset.lib)
            target_link_libraries(UnitTests icucommon.lib)
            target_link_libraries(UnitTests sds_library.lib)

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(UnitTests d3d11.lib)
                target_link_libraries(UnitTests dxgi.lib)
                target_link_libraries(UnitTests dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(UnitTests opengl32.lib)
                target_link_libraries(UnitTests glu32.lib)
            endif ()

            if (WP_PLATFORM_X64)
                target_link_libraries(UnitTests amd_ags_x64.lib)
            endif()
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(UnitTests WPGraphicsOgre.lib)

            if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
                target_link_libraries(UnitTests debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
                target_link_libraries(UnitTests debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

                target_link_libraries(UnitTests debug RenderSystem_Direct3D9Static_d.lib
                                    optimized RenderSystem_Direct3D9Static.lib)

                target_link_libraries(UnitTests debug RenderSystem_Direct3D11Static_d.lib
                                    optimized RenderSystem_Direct3D11Static.lib)

                target_link_libraries(UnitTests debug RenderSystem_GL3PlusStatic_d.lib 
                                    optimized RenderSystem_GL3PlusStatic.lib)

                target_link_libraries(UnitTests debug RenderSystem_GLStatic_d.lib
                                    optimized RenderSystem_GLStatic.lib)

                target_link_libraries(UnitTests debug OgreGLSupportStatic_d.lib
                                    optimized OgreGLSupportStatic.lib)

                target_link_libraries(UnitTests debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

                target_link_libraries(UnitTests debug OgreRTShaderSystemStatic_d.lib
                                           optimized OgreRTShaderSystemStatic.lib)

                target_link_libraries(UnitTests debug Codec_STBIStatic_d.lib
                                           optimized Codec_STBIStatic.lib)

                target_link_libraries(UnitTests debug OgreBitesStatic_d.lib
                                           optimized OgreBitesStatic.lib)

                target_link_libraries(UnitTests d3d9.lib)
                target_link_libraries(UnitTests d3dx9.lib)
    
                target_link_libraries(UnitTests freetype.lib)
                target_link_libraries(UnitTests debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
            endif()

            if (WP_BUILD_RENDERER_DX9)
                target_link_libraries(UnitTests d3d9.lib)
                target_link_libraries(UnitTests d3dx9.lib)
            endif ()

            if (WP_BUILD_RENDERER_DX11)
                target_link_libraries(UnitTests d3d11.lib)
                target_link_libraries(UnitTests dxgi.lib)
                target_link_libraries(UnitTests dxguid.lib)
            endif()

            if (WP_BUILD_RENDERER_OPENGL)
                target_link_libraries(UnitTests opengl32.lib)
                target_link_libraries(UnitTests glu32.lib)
            endif ()

            if (WP_BUILD_RENDERER_GL3PLUS)
                target_link_libraries(UnitTests opengl32.lib)
                target_link_libraries(UnitTests glu32.lib)
            endif ()

            if (WP_PLATFORM_X64)
                target_link_libraries(UnitTests amd_ags_x64.lib)
            endif()
        elseif(WP_BUILD_DX11)
            target_link_libraries(UnitTests WPGraphicsD3D11.lib)

            target_link_libraries(UnitTests Hieroglyph3_Desktop.lib)
            target_link_libraries(UnitTests DirectXTK.lib)
            target_link_libraries(UnitTests D3DCompiler.lib)
            target_link_libraries(UnitTests DXGUID.lib)
            target_link_libraries(UnitTests DevIL.lib)
            target_link_libraries(UnitTests ILU.lib)
            target_link_libraries(UnitTests ILUT.lib)

            target_link_libraries(UnitTests atk-1.0.lib)
            target_link_libraries(UnitTests cairo.lib)
            target_link_libraries(UnitTests fontconfig.lib)
            target_link_libraries(UnitTests gailutil.lib)
            target_link_libraries(UnitTests gdk_pixbuf-2.0.lib)
            target_link_libraries(UnitTests gdk-win32-3.0.lib)
            target_link_libraries(UnitTests gio-2.0.lib)
            target_link_libraries(UnitTests glib-2.0.lib)
            target_link_libraries(UnitTests gmodule-2.0.lib)
            target_link_libraries(UnitTests gobject-2.0.lib)
            target_link_libraries(UnitTests gthread-2.0.lib)
            target_link_libraries(UnitTests gtk-win32-3.0.lib)
            target_link_libraries(UnitTests pango-1.0.lib)
            target_link_libraries(UnitTests pangocairo-1.0.lib)
            target_link_libraries(UnitTests pangoft2-1.0.lib)
            target_link_libraries(UnitTests pangowin32-1.0.lib)

            target_link_libraries(UnitTests libxml2.lib)
            target_link_libraries(UnitTests librsvg.lib)
            target_link_libraries(UnitTests croco.lib)
            target_link_libraries(UnitTests libiconv.lib)
        elseif (WP_USE_SOFTWARE_RENDERER)
            target_link_libraries(UnitTests WPGraphicsSoftware.lib )
        endif()

        target_link_libraries(UnitTests WPAudio.lib )
                    
        if (WP_BUILD_BULLETPHYSICS)
            target_link_libraries(UnitTests WPBulletPhysics.lib)
        endif()

        if (WP_BUILD_ODE)
            target_link_libraries(UnitTests WPODE3.lib)
        endif()

        if (WP_BUILD_PHYSICS3)
            target_link_libraries(UnitTests WPPhysics.lib)
        endif()

        if (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
			    target_link_libraries(UnitTests tbb_static.lib)
			    target_link_libraries(UnitTests tbbmalloc_static.lib)
		    else()
			    target_link_libraries(UnitTests tbb.lib)
			    target_link_libraries(UnitTests tbbmalloc.lib)
            endif()
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(UnitTests WPPhysx.lib)

            target_link_libraries(UnitTests PhysX3.lib)
            target_link_libraries(UnitTests PhysX3Extensions.lib)
            target_link_libraries(UnitTests PhysX3Common.lib)
            target_link_libraries(UnitTests PhysX3Vehicle.lib)
            target_link_libraries(UnitTests PhysX3Cooking.lib)
            target_link_libraries(UnitTests PhysXProfileSDK.lib)
            target_link_libraries(UnitTests PxTask.lib)
            target_link_libraries(UnitTests PhysXVisualDebuggerSDK.lib)
            target_link_libraries(UnitTests PhysX3MetaData.lib)
            target_link_libraries(UnitTests PhysX3Foundation.lib)
            target_link_libraries(UnitTests PxSceneQuery.lib)
            target_link_libraries(UnitTests PxSimulationController.lib)
            target_link_libraries(UnitTests PhysX3LowLevel.lib)
            target_link_libraries(UnitTests PhysX3LowLevelCloth.lib)
        endif()

        target_link_libraries(UnitTests bcrypt.lib)
        target_link_libraries(UnitTests libboost_thread.lib)
        target_link_libraries(UnitTests libboost_date_time.lib)
        target_link_libraries(UnitTests libboost_chrono.lib)
        target_link_libraries(UnitTests libboost_context.lib)
        target_link_libraries(UnitTests libboost_timer.lib)
        target_link_libraries(UnitTests libboost_filesystem.lib)
        target_link_libraries(UnitTests libboost_json.lib)
        target_link_libraries(UnitTests minizip.lib)
        target_link_libraries(UnitTests nativefiledialog.lib)
        target_link_libraries(UnitTests rttr.lib)
        target_link_libraries(UnitTests tinyxml.lib)   
        target_link_libraries(UnitTests yaml-cpp.lib)
        target_link_libraries(UnitTests zlib.lib)
        target_link_libraries(UnitTests zzip.lib)
    elseif (APPLE)
        target_link_libraries(UnitTests libWPAero.a)
        target_link_libraries(UnitTests libWPCore.a)
        target_link_libraries(UnitTests libWPMesh.a)
        target_link_libraries(UnitTests libWPProcedural.a)
        target_link_libraries(UnitTests libWPVehicle.a)
        target_link_libraries(UnitTests libWPAudio.a)
        target_link_libraries(UnitTests libWPTest.a )
        target_link_libraries(UnitTests WPGraphics.a)
        target_link_libraries(UnitTests WPRenderUI.a)

        if (WP_ENABLE_LUA)
            target_link_libraries(UnitTests WPLua.a)
            target_link_libraries(UnitTests WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(UnitTests WPPython.a)
            target_link_libraries(UnitTests WPPythonBind.a)
        endif()

        if (WP_BUILD_FMOD)
            target_link_libraries(UnitTests libfmod.dylib )
            target_link_libraries(UnitTests libfmodstudio.dylib )
        endif()

        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(UnitTests WPOISInput.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(UnitTests WPGraphicsOgreNext.a)

            target_link_libraries(UnitTests harWPuzz.a)
            target_link_libraries(UnitTests harWPuzz-subset.a)
            target_link_libraries(UnitTests icucommon.a)
            target_link_libraries(UnitTests sds_library.a)

            target_link_libraries(UnitTests FreeImage.a)
            target_link_libraries(UnitTests freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(UnitTests WPGraphicsOgre.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(UnitTests libWPGraphicsSoftware.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(UnitTests WPPhysx.a)

            target_link_libraries(UnitTests PhysX3.a)
            target_link_libraries(UnitTests PhysX3Extensions.a)
            target_link_libraries(UnitTests PhysX3Common.a)
            target_link_libraries(UnitTests PhysX3Vehicle.a)
            target_link_libraries(UnitTests PhysX3Cooking.a)
            target_link_libraries(UnitTests PhysXProfileSDK.a)
            target_link_libraries(UnitTests PxTask.a)
            target_link_libraries(UnitTests PhysXVisualDebuggerSDK.a)
            target_link_libraries(UnitTests PhysX3MetaData.a)
            target_link_libraries(UnitTests PhysX3Foundation.a)
            target_link_libraries(UnitTests PxSceneQuery.a)
            target_link_libraries(UnitTests PxSimulationController.a)
            target_link_libraries(UnitTests PhysX3LowLevel.a)
            target_link_libraries(UnitTests PhysX3LowLevelCloth.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(UnitTests libWPODE3.a)
        endif()

        target_link_libraries(UnitTests libWPSQLite.a)

        target_link_libraries(UnitTests libboost_thread.a)
        target_link_libraries(UnitTests libboost_date_time.a)
        target_link_libraries(UnitTests libboost_chrono.a)
        target_link_libraries(UnitTests libboost_context.a)
        target_link_libraries(UnitTests libboost_timer.a)
        target_link_libraries(UnitTests libboost_filesystem.a)
        target_link_libraries(UnitTests libboost_unit_test_framework.a)
        target_link_libraries(UnitTests libboost_json.a)

        target_link_libraries(UnitTests minizip.a)
        target_link_libraries(UnitTests nativefiledialog.a)
        target_link_libraries(UnitTests rttr.a)
        target_link_libraries(UnitTests tinyxml.a)
        target_link_libraries(UnitTests yaml-cpp.a)
        target_link_libraries(UnitTests zlib.a)
        target_link_libraries(UnitTests zzip.a)

        if (WP_USE_TBB)
            target_link_libraries(UnitTests tbb.a)
            target_link_libraries(UnitTests tbbmalloc.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
            #target_link_libraries(UnitTests debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(UnitTests libWm5Core.a)
            target_link_libraries(UnitTests libWm5Mathematics.a)
        endif()

        target_link_libraries(UnitTests libzlib.a)
        target_link_libraries(UnitTests libzziplib.a)

        target_link_libraries(UnitTests stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
        set_target_properties(UnitTests PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(UnitTests -Wl,--start-group WPCore.a WPVehiclePhysics.a WPAssimp.a libWPTest.a
                WPApplication.a WPVehicle.a WPMesh.a WPGraphics.a
                libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a
                WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS}
                png Xaw Xrandr Xt xcb GL
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

            #target_link_libraries(UnitTests -Wl,--start-group WPGraphicsOgreNext.a ${X11_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} -Wl,--end-group )

            target_link_libraries(UnitTests freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(UnitTests libWPGraphicsSoftware.a)
        else()
            target_link_libraries(UnitTests -Wl,--start-group WPCore.a WPVehiclePhysics.a 
                WPApplication.a.a WPVehicle.a WPMesh.a WPGraphics.a
                libWPObjectTemplates.a libWPVehicle.a libWPProcedural.a libWPSQLite.a libWPPhysx.a WPPhysics3.a WPAudio.a WPRenderUI.a        
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
        endif()

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(UnitTests libWm5Core.a)
            target_link_libraries(UnitTests libWm5Mathematics.a)
        endif()
    
        if (WP_USE_ASSET_IMPORT)
            target_link_libraries(UnitTests WPAssimp.a)
        endif()

        target_link_libraries(UnitTests png)
        target_link_libraries(UnitTests Xaw)
        target_link_libraries(UnitTests Xrandr)
        target_link_libraries(UnitTests Xt)
        target_link_libraries(UnitTests xcb)
        target_link_libraries(UnitTests ${X11_LIBRARIES})
        target_link_libraries(UnitTests ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(UnitTests ${CMAKE_DL_LIBS})
    endif ()
endif ()

if (WIN32)
    set_property(TARGET UnitTests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${WP_ROOT}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration))
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(UnitTests Workphone)

    if (WP_BUILD_PHYSICS3)
        add_dependencies(UnitTests WPPhysics)
    elseif (WP_BUILD_ODE)
        add_dependencies(UnitTests WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(UnitTests WPPhysx)
    endif()
endif()

set_target_properties (UnitTests PROPERTIES FOLDER Tests/Cpp)

if(WIN32 AND WP_GRAPHICS_SYSTEM_CLAW)
    target_include_directories(UnitTests PRIVATE
        ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
        ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32)
    target_link_libraries(UnitTests WPGraphics WorkphoneGraphics WorkphonePlatformWin32 WorkphoneCore)
endif()
