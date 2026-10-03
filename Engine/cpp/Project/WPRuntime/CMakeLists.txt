project(WPRuntime)

if (UNIX AND NOT ANDROID)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

add_compile_definitions( BOOST_ALL_NO_LIB=1 )

add_compile_definitions (PX_PHYSX_STATIC_LIB)
add_compile_definitions (PX_PHYSX_CORE_STATIC_LIB)
add_compile_definitions (PX_FOUNDATION_NO_EXPORTS)
add_compile_definitions (PX_COOKING)
#add_compile_definitions (PX_CALL_CONV)

# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "../../Include/WPRuntime/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "../../Source/WPRuntime/*.cpp")

if (ANDROID)
    set(WPRUNTIME_ANDROID_NATIVE_ACTIVITY_SOURCE "../../Source/WPRuntime/Android/AndroidNativeActivity.cpp")
    list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/Android/AndroidNativeActivity\\.cpp$")
endif()

include_directories( ${WP_ROOT}/Dependencies/boost )

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/tinyxml/include)
include_directories( ${WP_ROOT}/Dependencies/utf8/source )
include_directories( ../../Include )
include_directories( ../../Include/WPRuntime )

if (WIN32)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    #link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)

    if (WP_BUILD_PHYSX)
        if (WP_BUILD_PHYSICS_DEBUG)
            link_directories(Editor ${CMAKE_SOURCE_DIR}/Dependencies/physx/externals/nvToolsExt/1/lib/x64)
        endif()
    endif()

    if (WP_USE_ONETBB)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/oneTBB/)
    elseif (WP_USE_TBB)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    endif()
    
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/fbxsdk/2020.2/${CMAKE_BUILD_TYPE})

    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/python/)
    #link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/pypy/)
    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif ()
    endif ()
elseif (ANDROID)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/libs/android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)

    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ois/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(WPRuntime ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)

add_executable(WPRuntime ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties (WPRuntime PROPERTIES FOLDER Engine/Cpp)

if (ANDROID)
    set(WPRUNTIME_ANDROID_APP_GLUE_DIR "${ANDROID_NDK}/sources/android/native_app_glue")
    if (NOT TARGET android_native_app_glue)
        add_library(android_native_app_glue STATIC
            "${WPRUNTIME_ANDROID_APP_GLUE_DIR}/android_native_app_glue.c")
        target_include_directories(android_native_app_glue PUBLIC "${WPRUNTIME_ANDROID_APP_GLUE_DIR}")
    endif()

    add_library(WPRuntimeAndroid SHARED
        ${HEADER_FILES}
        ${SOURCE_FILES}
        ${WPRUNTIME_ANDROID_NATIVE_ACTIVITY_SOURCE})
    set_target_properties(WPRuntimeAndroid PROPERTIES
        FOLDER Engine/Cpp
        OUTPUT_NAME WPRuntime)
    target_include_directories(WPRuntimeAndroid PRIVATE "${WPRUNTIME_ANDROID_APP_GLUE_DIR}")
    target_link_options(WPRuntimeAndroid PRIVATE "-u" "ANativeActivity_onCreate")

    target_include_directories(WPRuntime PRIVATE
        ${WP_ROOT}/Dependencies/ogre-next/BuildSettings/linux
        ${WP_ROOT}/Dependencies/harfbuzz/src
        ${WP_ROOT}/Dependencies/icu
        ${WP_ROOT}/Dependencies/icu/common
        ${WP_ROOT}/Dependencies/icu/common/unicode
        ${WP_ROOT}/Dependencies/sds_library/include
        ${WP_ROOT}/Dependencies/freetype/include
        ${WP_ROOT}/Dependencies/imgui
        ${WP_ROOT}/Dependencies/imgui/backends
        ${WP_ROOT}/Dependencies/imgui/misc/freetype
        ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty
        ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang
        ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang/glslang
        ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang/OGLCompilersDLL
        ../../Include/WPGraphicsOgreNext
        ../../Include/WPGraphicsOgreNext/ColibriGui
        ../../Include/WPGraphicsOgreNext/Deferred
        ../../Include/WPGraphicsOgreNext/Extern
        ../../Include/WPGraphicsOgreNext/Ogre/include
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Android
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Animation
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/CommandBuffer
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor/Pass
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor/Pass/PassClear
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compute
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/GLX
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/C
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple/C
        ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/ogrestd
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Bites/include
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Common/include
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Pbs/include
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Unlit/include
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Overlay/include
        ../../Include/WPGraphicsOgreNext/Ogre/Components/Paging/include
        ../../Include/WPGraphicsOgreNext/Ogre/PlugIns/CgProgramManager/include
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/include
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/include/spirv/unified1
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Vao
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing
        ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing/Android
        ../../Include/WPGraphicsOgreNext/UI
        ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src
        ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/SPIRV-Reflect
        ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Vao
        ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing
        ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing/Android
    )

    target_include_directories(WPRuntimeAndroid PRIVATE
        $<TARGET_PROPERTY:WPRuntime,INCLUDE_DIRECTORIES>)
endif()

if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(WPRuntime Workphone.lib)

    target_link_libraries(WPRuntime bcrypt.lib)
    target_link_libraries(WPRuntime libboost_thread.lib)
    target_link_libraries(WPRuntime libboost_date_time.lib)
    target_link_libraries(WPRuntime libboost_chrono.lib)
    target_link_libraries(WPRuntime libboost_context.lib)
    target_link_libraries(WPRuntime libboost_timer.lib)
    target_link_libraries(WPRuntime libboost_filesystem.lib)
    target_link_libraries(WPRuntime libboost_json.lib)
    target_link_libraries(WPRuntime nativefiledialog.lib)
    target_link_libraries(WPRuntime tinyxml.lib)  
    target_link_libraries(WPRuntime minizip.lib)
    target_link_libraries(WPRuntime zlib.lib)
    target_link_libraries(WPRuntime zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
            target_link_libraries(WPRuntime tbb_static.lib)
            target_link_libraries(WPRuntime tbbmalloc_static.lib)
        else()
            target_link_libraries(WPRuntime tbb.lib)
			target_link_libraries(WPRuntime tbbmalloc.lib)
        endif()
    endif()
elseif (ANDROID)
    target_link_libraries(WPRuntime
        -Wl,--start-group
        Workphone
        WPGraphicsOgreNext
        WPImGui
        WPLua
        WPLuaBind
        WPPhysics
        WorkphonePhysics
        WPProcedural
        WPSQLite
        WPVehiclePhysics
        boost_chrono
        boost_context
        boost_date_time
        boost_filesystem
        boost_json
        boost_thread
        boost_timer
        freetype
        harfbuzz
        harfbuzz-subset
        icucommon
        lua
        lua_cjson
        luabind
        minizip
        nativefiledialog
        rttr
        sds_library
        tinyxml
        yaml-cpp
        zlib
        zzip
        -Wl,--end-group
        android
        log
        vulkan
        dl
        m
        atomic
    )
elseif (APPLE)
    target_link_libraries(WPRuntime libWorkphone.a)
    target_link_libraries(WPRuntime libFBMesh.a)
    target_link_libraries(WPRuntime libWPProcedural.a)
    target_link_libraries(WPRuntime libWPVehiclePhysics.a)
    target_link_libraries(WPRuntime libWPSQLite.a)
    target_link_libraries(WPRuntime libFBCPlusPlus.a)
    target_link_libraries(WPRuntime libFBGraphics.a)
    target_link_libraries(WPRuntime WPAssimp.a)
    target_link_libraries(WPRuntime FBRenderUI.a)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(WPRuntime libWPOISInput.a)
        target_link_libraries(WPRuntime debug libOIS_d.a optimized libOIS.a)
    endif()

    if (WP_ENABLE_LUA)
        target_link_libraries(WPRuntime WPLua.a)
        target_link_libraries(WPRuntime WPLuaBind.a)
    elseif (WP_ENABLE_PYTHON)
        target_link_libraries(WPRuntime WPPython.a)
        target_link_libraries(WPRuntime WPPythonBind.a)
    endif()

    target_link_libraries(WPRuntime libboost_thread.a)
    target_link_libraries(WPRuntime libboost_date_time.a)
    target_link_libraries(WPRuntime libboost_chrono.a)
    target_link_libraries(WPRuntime libboost_context.a)
    target_link_libraries(WPRuntime libboost_timer.a)
    target_link_libraries(WPRuntime libboost_filesystem.a)
    target_link_libraries(WPRuntime libboost_json.a)
    target_link_libraries(WPRuntime minizip.a)
    target_link_libraries(WPRuntime nativefiledialog.a)
    target_link_libraries(WPRuntime rttr.a)
    target_link_libraries(WPRuntime tinyxml.a)
    target_link_libraries(WPRuntime yaml-cpp.a)
    target_link_libraries(WPRuntime zlib.a)
    target_link_libraries(WPRuntime zzip.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPRuntime WPGraphicsOgreNext.a)
        target_link_libraries(WPRuntime harfbuzz.a)
        target_link_libraries(WPRuntime harfbuzz-subset.a)
        target_link_libraries(WPRuntime icucommon.a)
        target_link_libraries(WPRuntime sds_library.a)

        target_link_libraries(WPRuntime freetype.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(WPRuntime WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(WPRuntime libFBGraphicsSoftware.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPRuntime libWPPhysx.a)

        target_link_libraries(WPRuntime PhysX3.a)
        target_link_libraries(WPRuntime PhysX3Extensions.a)
        target_link_libraries(WPRuntime PhysX3Common.a)
        target_link_libraries(WPRuntime PhysX3Vehicle.a)
        target_link_libraries(WPRuntime PhysX3Cooking.a)
        target_link_libraries(WPRuntime PhysXProfileSDK.a)
        target_link_libraries(WPRuntime PxTask.a)
        target_link_libraries(WPRuntime PhysXVisualDebuggerSDK.a)
        target_link_libraries(WPRuntime PhysX3MetaData.a)
        target_link_libraries(WPRuntime PhysX3Foundation.a)
        target_link_libraries(WPRuntime PxSceneQuery.a)
        target_link_libraries(WPRuntime PxSimulationController.a)
        target_link_libraries(WPRuntime PhysX3LowLevel.a)
        target_link_libraries(WPRuntime PhysX3LowLevelCloth.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(WPRuntime libWPODE3.a)
    endif()

    if (WP_USE_TBB)
        target_link_libraries(WPRuntime libtbb.a)
        target_link_libraries(WPRuntime libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(WPRuntime libWm5Core.a)
        target_link_libraries(WPRuntime libWm5Mathematics.a)
    endif()

    target_link_libraries(WPRuntime stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(WPRuntime PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(WPRuntime -Wl,--start-group Workphone.a WPVehiclePhysics.a 
        WPApplication.a FBMath.a FBState.a WPVehiclePhysics.a FBData.a FBMesh.a FBGraphics.a
        libWPVehiclePhysics.a libWPProcedural.a libWPSQLite.a libWPPhysx.a FBAudio.a
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

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPRuntime WPGraphicsOgreNext.a)
        target_link_libraries(WPRuntime freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(WPRuntime libFBGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(WPRuntime libWm5Core.a libWm5Mathematics.a)
    endif()
        
    target_link_libraries(WPRuntime png)
    target_link_libraries(WPRuntime Xaw)
    target_link_libraries(WPRuntime Xrandr)
    target_link_libraries(WPRuntime Xt)
    target_link_libraries(WPRuntime xcb)
    target_link_libraries(WPRuntime ${X11_LIBRARIES})
    target_link_libraries(WPRuntime ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(WPRuntime ${CMAKE_DL_LIBS})
endif ()
else()
if (WIN32)
    target_link_libraries(WPRuntime Workphone.lib)
    target_link_libraries(WPRuntime FBMesh.lib)
    target_link_libraries(WPRuntime WPProcedural.lib)
    target_link_libraries(WPRuntime WPSQLite.lib)
    target_link_libraries(WPRuntime WPVehiclePhysics.lib)
    target_link_libraries(WPRuntime WPOISInput.lib)
    target_link_libraries(WPRuntime FBGraphics.lib)
    target_link_libraries(WPRuntime WPAssimp.lib)
    target_link_libraries(WPRuntime FBRenderUI.lib)

    if (WP_ENABLE_LUA)
        target_link_libraries(WPRuntime WPLua.lib)
        target_link_libraries(WPRuntime WPLuaBind.lib)
    endif()
        
    if (WP_ENABLE_PYTHON)
        target_link_libraries(WPRuntime WPPython.lib)
        target_link_libraries(WPRuntime WPPythonBind.lib)
    endif()

    if (WP_BUILD_OISINPUT)
        #target_link_libraries(WPRuntime debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(WPRuntime dinput8.lib)
        target_link_libraries(WPRuntime dxguid.lib)
    endif()

    if (WP_USE_FBXSDK)
        #target_link_libraries(WPRuntime libfbxsdk.lib)
        target_link_libraries(WPRuntime libfbxsdk-mt.lib)
        target_link_libraries(WPRuntime libxml2-mt.lib)
        target_link_libraries(WPRuntime tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(WPRuntime FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(WPRuntime freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPRuntime WPGraphicsOgreNext.lib)

        target_link_libraries(WPRuntime harfbuzz.lib)
        target_link_libraries(WPRuntime harfbuzz-subset.lib)
        target_link_libraries(WPRuntime icucommon.lib)
        target_link_libraries(WPRuntime sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(WPRuntime d3d11.lib)
            target_link_libraries(WPRuntime dxgi.lib)
            target_link_libraries(WPRuntime dxguid.lib)
        endif()

        #if (WP_PLATFORM_X64)
        #    target_link_libraries(WPRuntime amd_ags_x64.lib)
        #endif()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(WPRuntime WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(WPRuntime debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(WPRuntime debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(WPRuntime debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(WPRuntime debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(WPRuntime debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(WPRuntime debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(WPRuntime debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(WPRuntime debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(WPRuntime debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(WPRuntime debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(WPRuntime debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)

            target_link_libraries(WPRuntime freetype.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(WPRuntime opengl32.lib)
            target_link_libraries(WPRuntime glu32.lib)
        endif()

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(WPRuntime d3d9.lib)
            target_link_libraries(WPRuntime d3dx9.lib)
            target_link_libraries(WPRuntime dxgi.lib)
            target_link_libraries(WPRuntime dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(WPRuntime d3d11.lib)
            target_link_libraries(WPRuntime dxgi.lib)
            target_link_libraries(WPRuntime dxguid.lib)
        endif()

        #if (WP_PLATFORM_X64)
        #    target_link_libraries(WPRuntime amd_ags_x64.lib)
        #endif()
    elseif(WP_BUILD_DX11)
        target_link_libraries(WPRuntime FBGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPRuntime WPPhysx.lib)

        target_link_libraries(WPRuntime PhysX3.lib)
        target_link_libraries(WPRuntime PhysX3Extensions.lib)
        target_link_libraries(WPRuntime PhysX3Common.lib)
        target_link_libraries(WPRuntime PhysX3Vehicle.lib)
        target_link_libraries(WPRuntime PhysX3Cooking.lib)
        target_link_libraries(WPRuntime PxTask.lib)        
        target_link_libraries(WPRuntime PhysX3MetaData.lib)
        target_link_libraries(WPRuntime PhysX3Foundation.lib)
        target_link_libraries(WPRuntime PxSceneQuery.lib)
        target_link_libraries(WPRuntime PxSimulationController.lib)
        target_link_libraries(WPRuntime PhysX3LowLevel.lib)
        target_link_libraries(WPRuntime PhysX3LowLevelCloth.lib)
        target_link_libraries(WPRuntime PhysXProfileSDK.lib)

        if(WP_BUILD_PHYSICS_DEBUG)
            target_link_libraries(WPRuntime PhysXVisualDebuggerSDK.lib)
            target_link_libraries(WPRuntime PvdRuntime.lib)

            if (WP_PLATFORM_X64)
                target_link_libraries(WPRuntime nvToolsExt64_1.lib)
            endif()
        endif()
    elseif(WP_BUILD_ODE)
        target_link_libraries(WPRuntime WPODE3.lib)
    elseif(WP_BUILD_PHYSICS3)
        target_link_libraries(WPRuntime FBPhysics3.lib)
    endif()

    target_link_libraries(WPRuntime bcrypt.lib)
    target_link_libraries(WPRuntime libboost_thread.lib)
    target_link_libraries(WPRuntime libboost_date_time.lib)
    target_link_libraries(WPRuntime libboost_chrono.lib)
    target_link_libraries(WPRuntime libboost_context.lib)
    target_link_libraries(WPRuntime libboost_timer.lib)
    target_link_libraries(WPRuntime libboost_filesystem.lib)
    target_link_libraries(WPRuntime libboost_json.lib)
    target_link_libraries(WPRuntime nativefiledialog.lib)
    target_link_libraries(WPRuntime rttr.lib)
    target_link_libraries(WPRuntime tinyxml.lib)    
    target_link_libraries(WPRuntime yaml-cpp.lib)
    target_link_libraries(WPRuntime minizip.lib)
    target_link_libraries(WPRuntime zlib.lib)
    target_link_libraries(WPRuntime zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
            target_link_libraries(WPRuntime tbb_static.lib)
            target_link_libraries(WPRuntime tbbmalloc_static.lib)
        else()
            target_link_libraries(WPRuntime tbb.lib)
			target_link_libraries(WPRuntime tbbmalloc.lib)
        endif()
    endif()

    target_link_libraries(WPRuntime dxgi.lib)
    target_link_libraries(WPRuntime dxguid.lib)

    target_link_libraries(WPRuntime opengl32.lib)
    target_link_libraries(WPRuntime glu32.lib)
    target_link_libraries(WPRuntime d3d11.lib)
    target_link_libraries(WPRuntime dxgi.lib)
    target_link_libraries(WPRuntime dxguid.lib)
    #target_link_libraries(WPRuntime amd_ags_x64.lib)
    #target_link_libraries(WPRuntime debug assimp-vc142-mtd.lib optimized assimp-vc142-mt.lib)
elseif (ANDROID)
    target_link_libraries(WPRuntime
        -Wl,--start-group
        Workphone
        WPGraphicsOgreNext
        WPImGui
        WPLua
        WPLuaBind
        WPPhysics
        WorkphonePhysics
        WPProcedural
        WPSQLite
        WPVehiclePhysics
        boost_chrono
        boost_context
        boost_date_time
        boost_filesystem
        boost_json
        boost_thread
        boost_timer
        freetype
        harfbuzz
        harfbuzz-subset
        icucommon
        lua
        lua_cjson
        luabind
        minizip
        nativefiledialog
        rttr
        sds_library
        tinyxml
        yaml-cpp
        zlib
        zzip
        -Wl,--end-group
        android
        log
        vulkan
        dl
        m
        atomic
    )
elseif (APPLE)
    target_link_libraries(WPRuntime libWorkphone.a)
    target_link_libraries(WPRuntime libFBMesh.a)
    target_link_libraries(WPRuntime libWPProcedural.a)
    target_link_libraries(WPRuntime libWPVehiclePhysics.a)
    target_link_libraries(WPRuntime libWPSQLite.a)
    target_link_libraries(WPRuntime libFBCPlusPlus.a)
    target_link_libraries(WPRuntime libFBGraphics.a)
    target_link_libraries(WPRuntime WPAssimp.a)
    target_link_libraries(WPRuntime FBRenderUI.a)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(WPRuntime libWPOISInput.a)
        target_link_libraries(WPRuntime debug libOIS_d.a optimized libOIS.a)
    endif()

    if (WP_ENABLE_LUA)
        target_link_libraries(WPRuntime WPLua.a)
        target_link_libraries(WPRuntime WPLuaBind.a)
    elseif (WP_ENABLE_PYTHON)
        target_link_libraries(WPRuntime WPPython.a)
        target_link_libraries(WPRuntime WPPythonBind.a)
    endif()

    target_link_libraries(WPRuntime libboost_thread.a)
    target_link_libraries(WPRuntime libboost_date_time.a)
    target_link_libraries(WPRuntime libboost_chrono.a)
    target_link_libraries(WPRuntime libboost_context.a)
    target_link_libraries(WPRuntime libboost_timer.a)
    target_link_libraries(WPRuntime libboost_filesystem.a)
    target_link_libraries(WPRuntime libboost_json.a)
    target_link_libraries(WPRuntime minizip.a)
    target_link_libraries(WPRuntime nativefiledialog.a)
    target_link_libraries(WPRuntime rttr.a)
    target_link_libraries(WPRuntime tinyxml.a)
    target_link_libraries(WPRuntime yaml-cpp.a)
    target_link_libraries(WPRuntime zlib.a)
    target_link_libraries(WPRuntime zzip.a)

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPRuntime WPGraphicsOgreNext.a)
        target_link_libraries(WPRuntime harfbuzz.a)
        target_link_libraries(WPRuntime harfbuzz-subset.a)
        target_link_libraries(WPRuntime icucommon.a)
        target_link_libraries(WPRuntime sds_library.a)

        target_link_libraries(WPRuntime freetype.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(WPRuntime WPGraphicsOgre.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(WPRuntime libFBGraphicsSoftware.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPRuntime libWPPhysx.a)

        target_link_libraries(WPRuntime PhysX3.a)
        target_link_libraries(WPRuntime PhysX3Extensions.a)
        target_link_libraries(WPRuntime PhysX3Common.a)
        target_link_libraries(WPRuntime PhysX3Vehicle.a)
        target_link_libraries(WPRuntime PhysX3Cooking.a)
        target_link_libraries(WPRuntime PhysXProfileSDK.a)
        target_link_libraries(WPRuntime PxTask.a)
        target_link_libraries(WPRuntime PhysXVisualDebuggerSDK.a)
        target_link_libraries(WPRuntime PhysX3MetaData.a)
        target_link_libraries(WPRuntime PhysX3Foundation.a)
        target_link_libraries(WPRuntime PxSceneQuery.a)
        target_link_libraries(WPRuntime PxSimulationController.a)
        target_link_libraries(WPRuntime PhysX3LowLevel.a)
        target_link_libraries(WPRuntime PhysX3LowLevelCloth.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(WPRuntime libWPODE3.a)
    endif()

    if (WP_USE_TBB)
        target_link_libraries(WPRuntime libtbb.a)
        target_link_libraries(WPRuntime libtbbmalloc.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(WPRuntime libWm5Core.a)
        target_link_libraries(WPRuntime libWm5Mathematics.a)
    endif()

    target_link_libraries(WPRuntime stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(WPRuntime PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(WPRuntime -Wl,--start-group Workphone.a WPVehiclePhysics.a 
        WPApplication.a FBMath.a FBState.a WPVehiclePhysics.a FBData.a FBMesh.a FBGraphics.a
        libWPVehiclePhysics.a libWPProcedural.a libWPSQLite.a libWPPhysx.a FBAudio.a
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

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(WPRuntime WPGraphicsOgreNext.a)
        target_link_libraries(WPRuntime freetype.a)
    elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
        target_link_libraries(WPRuntime libFBGraphicsSoftware.a)
    endif()

    if(WP_BUILD_WILDMAGIC5)
        target_link_libraries(WPRuntime libWm5Core.a libWm5Mathematics.a)
    endif()
        
    target_link_libraries(WPRuntime png)
    target_link_libraries(WPRuntime Xaw)
    target_link_libraries(WPRuntime Xrandr)
    target_link_libraries(WPRuntime Xt)
    target_link_libraries(WPRuntime xcb)
    target_link_libraries(WPRuntime ${X11_LIBRARIES})
    target_link_libraries(WPRuntime ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(WPRuntime ${CMAKE_DL_LIBS})
endif ()
endif()

if (ANDROID)
    target_link_libraries(WPRuntimeAndroid
        -Wl,--start-group
        Workphone
        WPGraphicsOgreNext
        WPImGui
        WPLua
        WPLuaBind
        WPPhysics
        WorkphonePhysics
        WPProcedural
        WPSQLite
        WPVehiclePhysics
        boost_chrono
        boost_context
        boost_date_time
        boost_filesystem
        boost_json
        boost_thread
        boost_timer
        freetype
        harfbuzz
        harfbuzz-subset
        icucommon
        lua
        lua_cjson
        luabind
        minizip
        nativefiledialog
        rttr
        sds_library
        tinyxml
        yaml-cpp
        zlib
        zzip
        android_native_app_glue
        -Wl,--end-group
        android
        log
        vulkan
        dl
        m
        atomic
    )
endif()

if (WIN32)
    set_property(TARGET WPRuntime PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${WP_ROOT}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration))
endif()

add_dependencies(WPRuntime Workphone)

if (WP_STATIC_LIB)
    if (ANDROID)
        add_dependencies(WPRuntime
            Workphone
            WPGraphicsOgreNext
            WPImGui
            WPLua
            WPLuaBind
            WPPhysics
            WorkphonePhysics
            WPProcedural
            WPSQLite
            WPVehiclePhysics
            boost_chrono
            boost_context
            boost_date_time
            boost_filesystem
            boost_json
            boost_thread
            boost_timer
            freetype
            harfbuzz
            harfbuzz-subset
            icucommon
            lua
            lua_cjson
            luabind
            minizip
            nativefiledialog
            rttr
            sds_library
            tinyxml
            yaml-cpp
            zlib
            zzip
        )
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
        add_dependencies(WPRuntime WPGraphicsOgreNext)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        add_dependencies(WPRuntime WPGraphicsOgre)
    endif()

    if (WP_BUILD_ODE)
        add_dependencies(WPRuntime WPODE3)
    elseif (WP_BUILD_PHYSX)
        add_dependencies(WPRuntime WPPhysx)
    endif()

	add_dependencies(WPLua lua)
	add_dependencies(WPLua luabind)
 endif()

if (ANDROID)
    set(WP_ANDROID_PACKAGE_NAME "com.workphone.wpruntime" CACHE STRING "Android package name for WPRuntime")
    set(WP_ANDROID_SDK_ROOT "$ENV{ANDROID_HOME}" CACHE PATH "Android SDK root")
    if (NOT WP_ANDROID_SDK_ROOT)
        set(WP_ANDROID_SDK_ROOT "$ENV{LOCALAPPDATA}/Android/Sdk" CACHE PATH "Android SDK root" FORCE)
    endif()

    set(WP_ANDROID_BUILD_TOOLS_VERSION "35.0.0" CACHE STRING "Android build-tools version used for APK packaging")
    set(WP_ANDROID_PLATFORM_VERSION "android-35" CACHE STRING "Android SDK platform used for APK packaging")
    set(WP_ANDROID_JAVA_HOME "C:/Program Files/Android/Android Studio/jbr" CACHE PATH "Java runtime used for APK signing")

    set(WP_ANDROID_BUILD_TOOLS_DIR "${WP_ANDROID_SDK_ROOT}/build-tools/${WP_ANDROID_BUILD_TOOLS_VERSION}")
    set(WP_ANDROID_AAPT "${WP_ANDROID_BUILD_TOOLS_DIR}/aapt.exe")
    set(WP_ANDROID_ZIPALIGN "${WP_ANDROID_BUILD_TOOLS_DIR}/zipalign.exe")
    set(WP_ANDROID_APKSIGNER_JAR "${WP_ANDROID_BUILD_TOOLS_DIR}/lib/apksigner.jar")
    set(WP_ANDROID_ANDROID_JAR "${WP_ANDROID_SDK_ROOT}/platforms/${WP_ANDROID_PLATFORM_VERSION}/android.jar")
    set(WP_ANDROID_JAVA "${WP_ANDROID_JAVA_HOME}/bin/java.exe")
    set(WP_ANDROID_KEYTOOL "${WP_ANDROID_JAVA_HOME}/bin/keytool.exe")

    foreach(_wp_android_tool
            WP_ANDROID_AAPT
            WP_ANDROID_ZIPALIGN
            WP_ANDROID_APKSIGNER_JAR
            WP_ANDROID_ANDROID_JAR
            WP_ANDROID_JAVA
            WP_ANDROID_KEYTOOL)
        if (NOT EXISTS "${${_wp_android_tool}}")
            message(FATAL_ERROR "Required Android APK packaging tool not found: ${_wp_android_tool}=${${_wp_android_tool}}")
        endif()
    endforeach()

    set(WP_ANDROID_APK_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    set(WP_ANDROID_APK_STAGE_DIR "${CMAKE_BINARY_DIR}/apk/WPRuntime")
    set(WP_ANDROID_MANIFEST "${CMAKE_BINARY_DIR}/apk/AndroidManifest.xml")
    set(WP_ANDROID_DEBUG_KEYSTORE "${CMAKE_BINARY_DIR}/apk/debug.keystore")
    set(WP_ANDROID_UNSIGNED_APK "${CMAKE_BINARY_DIR}/apk/WPRuntime-unsigned.apk")
    set(WP_ANDROID_ALIGNED_APK "${CMAKE_BINARY_DIR}/apk/WPRuntime-aligned.apk")
    set(WP_ANDROID_FINAL_APK "${WP_ANDROID_APK_DIR}/WPRuntime-debug.apk")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/Android/AndroidManifest.xml.in"
        "${WP_ANDROID_MANIFEST}"
        @ONLY)

    add_custom_command(
        OUTPUT "${WP_ANDROID_DEBUG_KEYSTORE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_BINARY_DIR}/apk"
        COMMAND "${CMAKE_COMMAND}" -E rm -f "${WP_ANDROID_DEBUG_KEYSTORE}"
        COMMAND "${WP_ANDROID_KEYTOOL}"
            -genkeypair
            -keystore "${WP_ANDROID_DEBUG_KEYSTORE}"
            -storepass android
            -keypass android
            -alias androiddebugkey
            -keyalg RSA
            -keysize 2048
            -validity 10000
            -dname "CN=Android Debug,O=Android,C=US"
        VERBATIM)

    add_custom_command(
        OUTPUT "${WP_ANDROID_UNSIGNED_APK}"
        COMMAND "${CMAKE_COMMAND}"
            "-DAAPT_EXECUTABLE=${WP_ANDROID_AAPT}"
            "-DANDROID_ABI=${ANDROID_ABI}"
            "-DANDROID_JAR=${WP_ANDROID_ANDROID_JAR}"
            "-DAPK_STAGE_DIR=${WP_ANDROID_APK_STAGE_DIR}"
            "-DAPK_UNSIGNED=${WP_ANDROID_UNSIGNED_APK}"
            "-DMANIFEST_FILE=${WP_ANDROID_MANIFEST}"
            "-DWPRUNTIME_NATIVE_LIBRARY=$<TARGET_FILE:WPRuntimeAndroid>"
            -P "${WP_ROOT}/CMake/PackageAndroidApk.cmake"
        DEPENDS WPRuntimeAndroid "${WP_ANDROID_MANIFEST}" "${WP_ROOT}/CMake/PackageAndroidApk.cmake"
        VERBATIM)

    add_custom_command(
        OUTPUT "${WP_ANDROID_FINAL_APK}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${WP_ANDROID_APK_DIR}"
        COMMAND "${WP_ANDROID_ZIPALIGN}" -f -p 4
            "${WP_ANDROID_UNSIGNED_APK}"
            "${WP_ANDROID_ALIGNED_APK}"
        COMMAND "${WP_ANDROID_JAVA}" -jar "${WP_ANDROID_APKSIGNER_JAR}"
            sign
            --ks "${WP_ANDROID_DEBUG_KEYSTORE}"
            --ks-key-alias androiddebugkey
            --ks-pass pass:android
            --key-pass pass:android
            --v4-signing-enabled false
            --out "${WP_ANDROID_FINAL_APK}"
            "${WP_ANDROID_ALIGNED_APK}"
        COMMAND "${WP_ANDROID_JAVA}" -jar "${WP_ANDROID_APKSIGNER_JAR}"
            verify
            "${WP_ANDROID_FINAL_APK}"
        DEPENDS "${WP_ANDROID_UNSIGNED_APK}" "${WP_ANDROID_DEBUG_KEYSTORE}"
        VERBATIM)

    add_custom_target(WPRuntimeAPK ALL DEPENDS "${WP_ANDROID_FINAL_APK}")
    add_dependencies(WPRuntimeAPK WPRuntimeAndroid)
endif()

