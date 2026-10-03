project(WPVehiclePhysics)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPVehiclePhysics/*.hpp" "../../Include/WPVehiclePhysics/*.inl")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPVehiclePhysics/*.cpp")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPVehiclePhysics/../../Include/WPVehiclePhysics")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPVehiclePhysics/../../Source/WPVehiclePhysics")

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

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinybind )
include_directories( ${WP_ROOT}/Dependencies/tinyxml )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ${WP_ROOT}/AdditionalLibraries/wildmagic/SDK/Include )
include_directories( ../../Include )
include_directories( ../../Include/WPVehiclePhysics )

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPVehiclePhysics ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPVehiclePhysics STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPVehiclePhysics SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPVehiclePhysics Workphone.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(WPVehiclePhysics debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPVehiclePhysics debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPVehiclePhysics debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPVehiclePhysics tbb_static.lib)
                target_link_libraries(WPVehiclePhysics tbbmalloc_static.lib)
            else()
                target_link_libraries(WPVehiclePhysics tbb.lib)
			    target_link_libraries(WPVehiclePhysics tbbmalloc.lib)
            endif()
        endif()
    elseif (APPLE)
        target_link_libraries(WPVehiclePhysics libWorkphone.a)
        target_link_libraries(WPVehiclePhysics libFBMesh.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPVehiclePhysics libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPVehiclePhysics libWPODE3.a)
        endif()

        target_link_libraries(WPVehiclePhysics libWPVehiclePhysics.a)

        target_link_libraries(WPVehiclePhysics libboost_thread.a)
        target_link_libraries(WPVehiclePhysics libboost_date_time.a)
        target_link_libraries(WPVehiclePhysics libboost_chrono.a)
        target_link_libraries(WPVehiclePhysics libboost_timer.a)
        target_link_libraries(WPVehiclePhysics libboost_filesystem.a)
        target_link_libraries(WPVehiclePhysics libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPVehiclePhysics WPVehiclePhysics.a)

            target_link_libraries(WPVehiclePhysics PhysX3.a)
            target_link_libraries(WPVehiclePhysics PhysX3Extensions.a)
            target_link_libraries(WPVehiclePhysics PhysX3Common.a)
            target_link_libraries(WPVehiclePhysics PhysX3Vehicle.a)
            target_link_libraries(WPVehiclePhysics PhysX3Cooking.a)
            target_link_libraries(WPVehiclePhysics PhysXProfileSDK.a)
            target_link_libraries(WPVehiclePhysics PxTask.a)
            target_link_libraries(WPVehiclePhysics PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPVehiclePhysics PhysX3MetaData.a)
            target_link_libraries(WPVehiclePhysics PhysX3Foundation.a)
            target_link_libraries(WPVehiclePhysics PxSceneQuery.a)
            target_link_libraries(WPVehiclePhysics PxSimulationController.a)
            target_link_libraries(WPVehiclePhysics PhysX3LowLevel.a)
            target_link_libraries(WPVehiclePhysics PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPVehiclePhysics libtbb.a)
            target_link_libraries(WPVehiclePhysics libtbbmalloc.a)
        endif()

        target_link_libraries(WPVehiclePhysics minizip.a)
        target_link_libraries(WPVehiclePhysics nativefiledialog.a)
        target_link_libraries(WPVehiclePhysics rttr.a)
        target_link_libraries(WPVehiclePhysics tinyxml.a)
        target_link_libraries(WPVehiclePhysics yaml-cpp.a)
        target_link_libraries(WPVehiclePhysics zlib.a)
        target_link_libraries(WPVehiclePhysics zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPVehiclePhysics libWm5Core.a)
            target_link_libraries(WPVehiclePhysics libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPVehiclePhysics debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPVehiclePhysics FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPVehiclePhysics freetype.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPVehiclePhysics stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPVehiclePhysics PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPVehiclePhysics -Wl,--start-group Workphone.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPVehiclePhysics.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPVehiclePhysics WPVehiclePhysics.a)
            target_link_libraries(WPVehiclePhysics freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPVehiclePhysics libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPVehiclePhysics png)
        target_link_libraries(WPVehiclePhysics Xaw)
        target_link_libraries(WPVehiclePhysics Xrandr)
        target_link_libraries(WPVehiclePhysics Xt)
        target_link_libraries(WPVehiclePhysics xcb)
        target_link_libraries(WPVehiclePhysics GL)
        target_link_libraries(WPVehiclePhysics ${X11_LIBRARIES})
        target_link_libraries(WPVehiclePhysics ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPVehiclePhysics ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPVehiclePhysics PROPERTIES FOLDER Engine/Cpp/Physics)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPVehiclePhysics PUBLIC WPVehicle_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPVehiclePhysics Workphone)
endif()
