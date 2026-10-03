project(WPPhysx)

add_compile_definitions (WP_USE_PHYSX_ACTIVE_TRANSFORMS=0)
add_compile_definitions (WP_TRACK_LOCKS=0)
add_compile_definitions (PX_PHYSX_STATIC_LIB)
add_compile_definitions (PX_PHYSX_CORE_STATIC_LIB)
add_compile_definitions (PX_FOUNDATION_NO_EXPORTS)
add_compile_definitions (PX_COOKING)
#add_compile_definitions (PX_CALL_CONV)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPPhysx/*.hpp" "../../Include/WPPhysx/*.inl")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPPhysx/*.cpp")

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/physx )
include_directories( ${WP_ROOT}/Dependencies/physx/Include )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/common )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/extensions )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/foundation )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/foundation/include )
include_directories( ${WP_ROOT}/Dependencies/physx/Include/physxvisualdebuggersdk )
include_directories( ${WP_ROOT}/Dependencies/wildmagic/SDK/Include )
include_directories( ../../Include )

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPPhysx "WPPhysx/WPPhysxPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPPhysx/WPPhysxPCH.cpp")
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPPhysx ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})

        if (WP_BUILD_PHYSICS_DEBUG)
            link_directories(WPPhysx ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
        endif()
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPPhysx STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPPhysx SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPPhysx Workphone.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(WPPhysx debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPPhysx debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPPhysx debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPPhysx tbb_static.lib)
                target_link_libraries(WPPhysx tbbmalloc_static.lib)
            else()
                target_link_libraries(WPPhysx tbb.lib)
			    target_link_libraries(WPPhysx tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPPhysx PhysX3.lib)
        target_link_libraries(WPPhysx PhysX3Extensions.lib)
        target_link_libraries(WPPhysx PhysX3Common.lib)
        target_link_libraries(WPPhysx PhysX3Vehicle.lib)
        target_link_libraries(WPPhysx PhysX3Cooking.lib)
        target_link_libraries(WPPhysx PxTask.lib)
        target_link_libraries(WPPhysx PhysX3MetaData.lib)
        target_link_libraries(WPPhysx PhysX3Foundation.lib)
        target_link_libraries(WPPhysx PxSceneQuery.lib)
        target_link_libraries(WPPhysx PxSimulationController.lib)
        target_link_libraries(WPPhysx PhysX3LowLevel.lib)
        target_link_libraries(WPPhysx PhysX3LowLevelCloth.lib)
        target_link_libraries(WPPhysx PhysXProfileSDK.lib)

        if(WP_BUILD_PHYSICS_DEBUG)
            target_link_libraries(WPPhysx PhysXVisualDebuggerSDK.lib)
            target_link_libraries(WPPhysx PvdRuntime.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(WPPhysx nvToolsExt64_1.lib)
            endif()
        endif()
    elseif (APPLE)
        target_link_libraries(WPPhysx libWorkphone.a)
        target_link_libraries(WPPhysx libFBMesh.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPPhysx libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPPhysx libWPODE3.a)
        endif()

        target_link_libraries(WPPhysx libWPVehiclePhysics.a)

        target_link_libraries(WPPhysx libboost_thread.a)
        target_link_libraries(WPPhysx libboost_date_time.a)
        target_link_libraries(WPPhysx libboost_chrono.a)
        target_link_libraries(WPPhysx libboost_timer.a)
        target_link_libraries(WPPhysx libboost_filesystem.a)
        target_link_libraries(WPPhysx libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPPhysx WPPhysx.a)

            target_link_libraries(WPPhysx PhysX3.a)
            target_link_libraries(WPPhysx PhysX3Extensions.a)
            target_link_libraries(WPPhysx PhysX3Common.a)
            target_link_libraries(WPPhysx PhysX3Vehicle.a)
            target_link_libraries(WPPhysx PhysX3Cooking.a)
            target_link_libraries(WPPhysx PhysXProfileSDK.a)
            target_link_libraries(WPPhysx PxTask.a)
            target_link_libraries(WPPhysx PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPPhysx PhysX3MetaData.a)
            target_link_libraries(WPPhysx PhysX3Foundation.a)
            target_link_libraries(WPPhysx PxSceneQuery.a)
            target_link_libraries(WPPhysx PxSimulationController.a)
            target_link_libraries(WPPhysx PhysX3LowLevel.a)
            target_link_libraries(WPPhysx PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPPhysx libtbb.a)
            target_link_libraries(WPPhysx libtbbmalloc.a)
        endif()

        target_link_libraries(WPPhysx minizip.a)
        target_link_libraries(WPPhysx nativefiledialog.a)
        target_link_libraries(WPPhysx rttr.a)
        target_link_libraries(WPPhysx tinyxml.a)
        target_link_libraries(WPPhysx yaml-cpp.a)
        target_link_libraries(WPPhysx zlib.a)
        target_link_libraries(WPPhysx zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPPhysx libWm5Core.a)
            target_link_libraries(WPPhysx libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPPhysx debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPPhysx FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPPhysx freetype.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPPhysx stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPPhysx PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPPhysx -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPPhysx.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPPhysx WPPhysx.a)
            target_link_libraries(WPPhysx freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPPhysx libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPPhysx png)
        target_link_libraries(WPPhysx Xaw)
        target_link_libraries(WPPhysx Xrandr)
        target_link_libraries(WPPhysx Xt)
        target_link_libraries(WPPhysx xcb)
        target_link_libraries(WPPhysx GL)
        target_link_libraries(WPPhysx ${X11_LIBRARIES})
        target_link_libraries(WPPhysx ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPPhysx ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPPhysx PROPERTIES FOLDER Engine/Cpp/Physics)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPPhysx PUBLIC WPPhysx_EXPORTS)
endif()

if (WP_BUILD_DEPENDENCIES)
	if (NOT WP_STATIC_LIB)
		add_dependencies(WPPhysx Workphone)

		add_dependencies(WPPhysx PhysX3)
		add_dependencies(WPPhysx PhysX3Extensions)
		add_dependencies(WPPhysx PhysX3Common)
		add_dependencies(WPPhysx PhysX3Vehicle)
		add_dependencies(WPPhysx PhysX3Cooking)
		add_dependencies(WPPhysx PxTask)
		add_dependencies(WPPhysx PhysX3MetaData)
		add_dependencies(WPPhysx PhysX3Foundation)
		add_dependencies(WPPhysx PxSceneQuery)
		add_dependencies(WPPhysx PxSimulationController)
		add_dependencies(WPPhysx PhysX3LowLevel)
		add_dependencies(WPPhysx PhysX3LowLevelCloth)
		add_dependencies(WPPhysx PhysXProfileSDK)

		if(WP_BUILD_PHYSICS_DEBUG)
			add_dependencies(WPPhysx PhysXVisualDebuggerSDK)
			add_dependencies(WPPhysx PvdRuntime)
		endif()
	endif()
endif()