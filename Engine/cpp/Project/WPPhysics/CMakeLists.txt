project(WPPhysics)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPhysics/*.h" "../../Include/WPPhysics/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPhysics/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPhysics/../../Include/WPPhysics")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPPhysics/../../Source/WPPhysics")

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

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/box2d/Include )
include_directories( ${WP_ROOT}/Dependencies/box2d/Source )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ../../Include )
include_directories( ../../Include/WPPhysics )
include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneCore )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphonePhysics )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPPhysics ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})

        if (WP_BUILD_PHYSICS_DEBUG)
            link_directories(WPPhysics ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
        endif()
    endif()
endif()

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPPhysics "WPPhysics/WPPhysicsPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPPhysics/WPPhysicsPCH.cpp")
endif()

if (WP_STATIC_LIB)
    add_library(WPPhysics STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPPhysics SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPPhysics Workphone)
        target_link_libraries(WPPhysics WorkphoneCore.lib)
        target_link_libraries(WPPhysics WorkphonePhysics.lib)
        target_link_libraries(WPPhysics box2d)

        if (WP_USE_ONETBB)
            target_link_libraries(WPPhysics debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPPhysics debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPPhysics debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPPhysics tbb_static.lib)
                target_link_libraries(WPPhysics tbbmalloc_static.lib)
            else()
                target_link_libraries(WPPhysics tbb.lib)
			    target_link_libraries(WPPhysics tbbmalloc.lib)
            endif()
        endif()
    elseif (APPLE)
        target_link_libraries(WPPhysics libWorkphone.a)
        target_link_libraries(WPPhysics libFBMesh.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPPhysics libWPPhysics.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPPhysics libWPODE3.a)
        endif()

        target_link_libraries(WPPhysics libWPVehiclePhysics.a)

        target_link_libraries(WPPhysics libboost_thread.a)
        target_link_libraries(WPPhysics libboost_date_time.a)
        target_link_libraries(WPPhysics libboost_chrono.a)
        target_link_libraries(WPPhysics libboost_timer.a)
        target_link_libraries(WPPhysics libboost_filesystem.a)
        target_link_libraries(WPPhysics libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPPhysics WPPhysics.a)

            target_link_libraries(WPPhysics PhysX3.a)
            target_link_libraries(WPPhysics PhysX3Extensions.a)
            target_link_libraries(WPPhysics PhysX3Common.a)
            target_link_libraries(WPPhysics PhysX3Vehicle.a)
            target_link_libraries(WPPhysics PhysX3Cooking.a)
            target_link_libraries(WPPhysics PhysXProfileSDK.a)
            target_link_libraries(WPPhysics PxTask.a)
            target_link_libraries(WPPhysics PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPPhysics PhysX3MetaData.a)
            target_link_libraries(WPPhysics PhysX3Foundation.a)
            target_link_libraries(WPPhysics PxSceneQuery.a)
            target_link_libraries(WPPhysics PxSimulationController.a)
            target_link_libraries(WPPhysics PhysX3LowLevel.a)
            target_link_libraries(WPPhysics PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPPhysics libtbb.a)
            target_link_libraries(WPPhysics libtbbmalloc.a)
        endif()

        target_link_libraries(WPPhysics minizip.a)
        target_link_libraries(WPPhysics nativefiledialog.a)
        target_link_libraries(WPPhysics rttr.a)
        target_link_libraries(WPPhysics tinyxml.a)
        target_link_libraries(WPPhysics yaml-cpp.a)
        target_link_libraries(WPPhysics zlib.a)
        target_link_libraries(WPPhysics zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPPhysics libWm5Core.a)
            target_link_libraries(WPPhysics libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPPhysics debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPPhysics FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPPhysics freetype.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPPhysics stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPPhysics PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPPhysics -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysics.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPPhysics.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
            libboost_thread-gcc5-mt-s-x64-1_70.a
            libboost_date_time-gcc5-mt-s-x64-1_70.a
            libboost_chrono-gcc5-mt-s-x64-1_70.a
            libboost_timer-gcc5-mt-s-x64-1_70.a
            libboost_filesystem-gcc5-mt-s-x64-1_70.a
            debug libtbb_debug.a optimized libtbb.a
            debug libtbbmalloc_debug.a optimized libtbbmalloc.a
            libWm5Core.a libWm5Mathematics.a
            libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a 
            libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a libPxTask.a
            -Wl,--end-group )

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(WPPhysics WPPhysics.a)
            target_link_libraries(WPPhysics freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPPhysics libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPPhysics png)
        target_link_libraries(WPPhysics Xaw)
        target_link_libraries(WPPhysics Xrandr)
        target_link_libraries(WPPhysics Xt)
        target_link_libraries(WPPhysics xcb)
        target_link_libraries(WPPhysics GL)
        target_link_libraries(WPPhysics ${X11_LIBRARIES})
        target_link_libraries(WPPhysics ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPPhysics ${CMAKE_DL_LIBS})
    endif ()
endif()

target_compile_definitions(WPPhysics PRIVATE WPPhysics_EXPORTS)
target_link_libraries(WPPhysics WorkphonePhysics)
set_target_properties (WPPhysics PROPERTIES FOLDER Engine/Cpp/Physics)
