project(WPLuaJit)

add_compile_definitions (WP_PROFILE_LUA_CALLS=0)
add_compile_definitions (BOOST_BIND_GLOBAL_PLACEHOLDERS)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPLua/*.h" "../../Include/WPLua/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPLua/*.cpp" "../../Source/WPLua/*.c")

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPLua/../../Include/WPLua")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPLua/../../Source/WPLua")

foreach(_source IN ITEMS ${HEADER_FILES})
    get_filename_component(_source_file_path "${_source}" DIRECTORY)
    string(REPLACE "${WP_HEADER_REL_PATH}" "" _group_file_path "${_source_file_path}")
    string(REPLACE "\\" "/" _group_file_path "${_group_file_path}")
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
include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc/luabind )

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
include_directories( ../../Include/WPLuaJit )

#if (WP_USE_PRECOMPILED_HEADERS)
#    wp_use_precompiled_header(WPProcedural "WPLua/WPLuaPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPLua/WPLuaPCH.cpp")
#endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPLuaJit ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

if (WP_STATIC_LIB)
    add_library(WPLuaJit STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPLuaJit SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPLuaJit Workphone)
        target_link_libraries(WPLuaJit WPLuaBind)
        target_link_libraries(WPLuaJit WPProcedural)

        if (WP_USE_ONETBB)
            target_link_libraries(WPLuaJit debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPLuaJit debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPLuaJit debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (TBB_BUILD__STATIC)
                target_link_libraries(WPLuaJit tbb_static.lib)
                target_link_libraries(WPLuaJit tbbmalloc_static.lib)
            else()
                target_link_libraries(WPLuaJit tbb.lib)
			    target_link_libraries(WPLuaJit tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPLuaJit bcrypt.lib)
        target_link_libraries(WPLuaJit libboost_thread.lib)
        target_link_libraries(WPLuaJit libboost_date_time.lib)
        target_link_libraries(WPLuaJit libboost_chrono.lib)
        target_link_libraries(WPLuaJit libboost_timer.lib)
        target_link_libraries(WPLuaJit libboost_filesystem.lib)
        target_link_libraries(WPLuaJit lua.lib)
        target_link_libraries(WPLuaJit lua_cjson.lib)
        target_link_libraries(WPLuaJit luabind.lib)
    elseif (APPLE)
        target_link_libraries(WPLuaJit libWorkphone.a)
        target_link_libraries(WPLuaJit libFBMesh.a)
        target_link_libraries(WPLuaJit libWPProcedural.a)
        target_link_libraries(WPLuaJit libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(WPLuaJit FBAudio.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(WPLuaJit WPLua.a)
            target_link_libraries(WPLuaJit WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(WPLuaJit WPPython.a)
            target_link_libraries(WPLuaJit WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(WPLuaJit SDL2main.a)
            target_link_libraries(WPLuaJit SDL2_image.a)
            target_link_libraries(WPLuaJit debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(WPLuaJit libWPImGui.a)
            target_link_libraries(WPLuaJit imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPLuaJit libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPLuaJit libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            #target_link_libraries(WPLuaJit debug libOIS_d.a optimized libOIS.a)
        endif()

        target_link_libraries(WPLuaJit libWPVehiclePhysics.a)

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(WPLuaJit libWPWxWidgets.a)

            target_link_libraries(WPLuaJit libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(WPLuaJit libwx_baseu-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(WPLuaJit libwxexpat-3.1.a)
            target_link_libraries(WPLuaJit libwx_baseu_xml-3.1.a)
            target_link_libraries(WPLuaJit libwx_baseu_net-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(WPLuaJit libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(WPLuaJit libwxtiff-3.1.a)
            target_link_libraries(WPLuaJit libwxjpeg-3.1.a)
            target_link_libraries(WPLuaJit libwxpng-3.1.a)

            target_link_libraries(WPLuaJit libiconv.a)
        endif()

        target_link_libraries(WPLuaJit libFBRenderUI.a)
        target_link_libraries(WPLuaJit libWPSQLite.a)
        target_link_libraries(WPLuaJit libFBCPlusPlus.a)
        target_link_libraries(WPLuaJit libFBGraphics.a)

        target_link_libraries(WPLuaJit libboost_thread.a)
        target_link_libraries(WPLuaJit libboost_date_time.a)
        target_link_libraries(WPLuaJit libboost_chrono.a)
        target_link_libraries(WPLuaJit libboost_timer.a)
        target_link_libraries(WPLuaJit libboost_filesystem.a)
        target_link_libraries(WPLuaJit libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPLuaJit WPPhysx.a)

            target_link_libraries(WPLuaJit PhysX3.a)
            target_link_libraries(WPLuaJit PhysX3Extensions.a)
            target_link_libraries(WPLuaJit PhysX3Common.a)
            target_link_libraries(WPLuaJit PhysX3Vehicle.a)
            target_link_libraries(WPLuaJit PhysX3Cooking.a)
            target_link_libraries(WPLuaJit PhysXProfileSDK.a)
            target_link_libraries(WPLuaJit PxTask.a)
            target_link_libraries(WPLuaJit PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPLuaJit PhysX3MetaData.a)
            target_link_libraries(WPLuaJit PhysX3Foundation.a)
            target_link_libraries(WPLuaJit PxSceneQuery.a)
            target_link_libraries(WPLuaJit PxSimulationController.a)
            target_link_libraries(WPLuaJit PhysX3LowLevel.a)
            target_link_libraries(WPLuaJit PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPLuaJit libtbb.a)
            target_link_libraries(WPLuaJit libtbbmalloc.a)
        endif()

        target_link_libraries(WPLuaJit minizip.a)
        target_link_libraries(WPLuaJit nativefiledialog.a)
        target_link_libraries(WPLuaJit tinyxml.a)
        target_link_libraries(WPLuaJit zlib.a)
        target_link_libraries(WPLuaJit zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPLuaJit libWm5Core.a)
            target_link_libraries(WPLuaJit libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPLuaJit debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPLuaJit FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPLuaJit freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(WPLuaJit WPLua.a)
            target_link_libraries(WPLuaJit harfbuzz.a)
            target_link_libraries(WPLuaJit harfbuzz-subset.a)
            target_link_libraries(WPLuaJit icucommon.a)
            target_link_libraries(WPLuaJit sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(WPLuaJit WPLua.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPLuaJit stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPLuaJit PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPLuaJit -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPLua.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPLuaJit WPLua.a)
            target_link_libraries(WPLuaJit freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPLuaJit libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPLuaJit png)
        target_link_libraries(WPLuaJit Xaw)
        target_link_libraries(WPLuaJit Xrandr)
        target_link_libraries(WPLuaJit Xt)
        target_link_libraries(WPLuaJit xcb)
        target_link_libraries(WPLuaJit GL)
        target_link_libraries(WPLuaJit ${X11_LIBRARIES})
        target_link_libraries(WPLuaJit ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPLuaJit ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPLuaJit PROPERTIES FOLDER Engine/Cpp/Scripting)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPLuaJit PUBLIC WPLua_EXPORTS)
endif()

if (WP_BUILD_DEPENDENCIES)
	if (NOT WP_STATIC_LIB)
		add_dependencies(WPLuaJit lua)
		add_dependencies(WPLuaJit luabind)
	endif()
endif()

if (NOT WP_STATIC_LIB)
	#add_dependencies(WPLuaJit Workphone)
	add_dependencies(WPLuaJit WPLuaBind)
endif()