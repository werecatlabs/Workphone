set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SampleApplication)

if (UNIX)
    if (NOT APPLE)
        find_package(X11 REQUIRED)
    endif()
endif()

add_compile_definitions (_UNICODE)
add_compile_definitions (wxUSE_UNICODE=1)
#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/cppsqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/sqlite/include)
include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets  )
include_directories( ${WP_ROOT}/AdditionalLibraries/wxWidgets/include )
include_directories( ${WP_ROOT}/Engine/cpp/Include )
include_directories(Source)

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

if(WIN32)
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/win32 )
elseif(APPLE)
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/osx )
else()
    include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include/linux )
endif()

include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/include )
include_directories( ${WP_ROOT}/AdditionalLibraries/ogre/OgreMain/include )

if (WIN32)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/sdl2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(SampleApplication ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(SampleApplication ${HEADER_FILES} ${SOURCE_FILES})

if (NOT WP_STATIC_LIB)
if (WIN32)
    target_link_libraries(SampleApplication Workphone)

    target_link_libraries(SampleApplication bcrypt.lib)
    target_link_libraries(SampleApplication libboost_thread.lib)
    target_link_libraries(SampleApplication libboost_date_time.lib)
    target_link_libraries(SampleApplication libboost_chrono.lib)
    target_link_libraries(SampleApplication libboost_timer.lib)
    target_link_libraries(SampleApplication libboost_filesystem.lib)
    target_link_libraries(SampleApplication libboost_json.lib)
    target_link_libraries(SampleApplication minizip.lib)
    target_link_libraries(SampleApplication nativefiledialog.lib)
    target_link_libraries(SampleApplication tinyxml.lib)   
    target_link_libraries(SampleApplication zlib.lib)
    target_link_libraries(SampleApplication zzip.lib)
        
    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(SampleApplication tbb_static.lib)
			target_link_libraries(SampleApplication tbbmalloc_static.lib)
		else()
			target_link_libraries(SampleApplication tbb.lib)
			target_link_libraries(SampleApplication tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    # fireblade libraries
    target_link_libraries(SampleApplication libWPCore.a)
    target_link_libraries(SampleApplication libWPGraphics.a)
    target_link_libraries(SampleApplication libWPMesh.a)
    target_link_libraries(SampleApplication libWPPhysx.a)
    target_link_libraries(SampleApplication libWPSQLite.a)

    target_link_libraries(SampleApplication libWPAssimp.a)

    if (WP_BUILD_OISINPUT)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleApplication FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleApplication freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleApplication WPGraphicsOgreNext.a)

        target_link_libraries(SampleApplication harWPuzz.a)
        target_link_libraries(SampleApplication harWPuzz-subset.a)
        target_link_libraries(SampleApplication icucommon.a)
        target_link_libraries(SampleApplication sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleApplication WPGraphicsOgre.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(SampleApplication WPImGui.a)
        target_link_libraries(SampleApplication imgui.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleApplication WPPhysx.a)

        target_link_libraries(SampleApplication PhysX3.a)
        target_link_libraries(SampleApplication PhysX3Extensions.a)
        target_link_libraries(SampleApplication PhysX3Common.a)
        target_link_libraries(SampleApplication PhysX3Vehicle.a)
        target_link_libraries(SampleApplication PhysX3Cooking.a)
        target_link_libraries(SampleApplication PhysXProfileSDK.a)
        target_link_libraries(SampleApplication PxTask.a)
        target_link_libraries(SampleApplication PhysXVisualDebuggerSDK.a)
        target_link_libraries(SampleApplication PhysX3MetaData.a)
        target_link_libraries(SampleApplication PhysX3Foundation.a)
        target_link_libraries(SampleApplication PxSceneQuery.a)
        target_link_libraries(SampleApplication PxSimulationController.a)
        target_link_libraries(SampleApplication PhysX3LowLevel.a)
        target_link_libraries(SampleApplication PhysX3LowLevelCloth.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(SampleApplication libWPODE3.a)
    endif()

    target_link_libraries(SampleApplication libboost_thread.a)
    target_link_libraries(SampleApplication libboost_date_time.a)
    target_link_libraries(SampleApplication libboost_chrono.a)
    target_link_libraries(SampleApplication libboost_context.a)
    target_link_libraries(SampleApplication libboost_timer.a)
    target_link_libraries(SampleApplication libboost_filesystem.a)
    target_link_libraries(SampleApplication libboost_unit_test_framework.a)
    target_link_libraries(SampleApplication libboost_json.a)

    target_link_libraries(SampleApplication minizip.a)
    target_link_libraries(SampleApplication nativefiledialog.a)
    target_link_libraries(SampleApplication tinyxml.a)
    target_link_libraries(SampleApplication zlib.a)
    target_link_libraries(SampleApplication zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(SampleApplication libtbb_static.a)
        target_link_libraries(SampleApplication libtbbmalloc_static.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(SampleApplication libWm5Core.a)
        target_link_libraries(SampleApplication libWm5Mathematics.a)
    endif ()

    #target_link_libraries(SampleApplication stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(SampleApplication ${AUDIO_UNIT})
    target_link_libraries(SampleApplication stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(SampleApplication PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(SampleApplication -Wl,--start-group WPCore.a 
        WPGraphics.a WPGraphicsOgreNext.a WPImGui.a WPMesh.a       
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        freetype.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libWm5Core.a libWm5Mathematics.a
        -Wl,--end-group )

    target_link_libraries(SampleApplication png)
    target_link_libraries(SampleApplication Xaw)
    target_link_libraries(SampleApplication Xrandr)
    target_link_libraries(SampleApplication Xt)
    target_link_libraries(SampleApplication xcb)
    target_link_libraries(SampleApplication GL)
    target_link_libraries(SampleApplication ${X11_LIBRARIES})
    target_link_libraries(SampleApplication ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(SampleApplication ${CMAKE_DL_LIBS})
endif ()
else ()
if (WIN32)
    target_link_libraries(SampleApplication Workphone)
    target_link_libraries(SampleApplication WPPhysx.lib)
    target_link_libraries(SampleApplication WPSQLite)
    target_link_libraries(SampleApplication WPWxWidgets.lib)
    target_link_libraries(SampleApplication WPGraphics)

    if (WP_BUILD_OISINPUT)
        #target_link_libraries(SampleApplication debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(SampleApplication dinput8.lib)
        target_link_libraries(SampleApplication dxguid.lib)
    endif ()

    if (WP_USE_WPXSDK)
        #target_link_libraries(SampleApplication libWPxsdk.lib)
        target_link_libraries(SampleApplication libWPxsdk-mt.lib)
        target_link_libraries(SampleApplication libxml2-mt.lib)
        target_link_libraries(SampleApplication tinyxml2.lib)
    endif()

    if (WP_BUILD_SDL2)
        target_link_libraries(SampleApplication debug SDL2d.lib optimized SDL2.lib)
    endif()

    if (WP_BUILD_IMGUI)        
        target_link_libraries(SampleApplication WPImGui)
        target_link_libraries(SampleApplication imgui.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleApplication FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleApplication freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleApplication WPGraphicsOgreNext.lib)

        target_link_libraries(SampleApplication harWPuzz.lib)
        target_link_libraries(SampleApplication harWPuzz-subset.lib)
        target_link_libraries(SampleApplication icucommon.lib)
        target_link_libraries(SampleApplication sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(SampleApplication d3d11.lib)
            target_link_libraries(SampleApplication dxgi.lib)
            target_link_libraries(SampleApplication dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(SampleApplication opengl32.lib)
            target_link_libraries(SampleApplication glu32.lib)
        endif ()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleApplication WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(SampleApplication debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(SampleApplication debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(SampleApplication debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(SampleApplication debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(SampleApplication debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(SampleApplication debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(SampleApplication debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(SampleApplication debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(SampleApplication debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(SampleApplication debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(SampleApplication debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)
        endif()

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(SampleApplication d3d9.lib)
            target_link_libraries(SampleApplication d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(SampleApplication d3d11.lib)
            target_link_libraries(SampleApplication dxgi.lib)
            target_link_libraries(SampleApplication dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(SampleApplication opengl32.lib)
            target_link_libraries(SampleApplication glu32.lib)
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(SampleApplication opengl32.lib)
            target_link_libraries(SampleApplication glu32.lib)
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(SampleApplication WPGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleApplication WPPhysx.lib)

        target_link_libraries(SampleApplication PhysX3.lib)
        target_link_libraries(SampleApplication PhysX3Extensions.lib)
        target_link_libraries(SampleApplication PhysX3Common.lib)
        target_link_libraries(SampleApplication PhysX3Vehicle.lib)
        target_link_libraries(SampleApplication PhysX3Cooking.lib)
        target_link_libraries(SampleApplication PhysXProfileSDK.lib)
        target_link_libraries(SampleApplication PxTask.lib)
        target_link_libraries(SampleApplication PhysXVisualDebuggerSDK.lib)
        target_link_libraries(SampleApplication PhysX3MetaData.lib)
        target_link_libraries(SampleApplication PhysX3Foundation.lib)
        target_link_libraries(SampleApplication PxSceneQuery.lib)
        target_link_libraries(SampleApplication PxSimulationController.lib)
        target_link_libraries(SampleApplication PhysX3LowLevel.lib)
        target_link_libraries(SampleApplication PhysX3LowLevelCloth.lib)
    elseif(WP_BUILD_ODE)
        target_link_libraries(SampleApplication WPODE3.lib)
    elseif(WP_BUILD_PHYSICS3)
        target_link_libraries(SampleApplication WPPhysics3.lib)
    endif()

    target_link_libraries(SampleApplication bcrypt.lib)
    target_link_libraries(SampleApplication libboost_thread.lib)
    target_link_libraries(SampleApplication libboost_date_time.lib)
    target_link_libraries(SampleApplication libboost_chrono.lib)
    target_link_libraries(SampleApplication libboost_timer.lib)
    target_link_libraries(SampleApplication libboost_filesystem.lib)
    target_link_libraries(SampleApplication libboost_json.lib)
    target_link_libraries(SampleApplication minizip.lib)
    target_link_libraries(SampleApplication nativefiledialog.lib)
    target_link_libraries(SampleApplication tinyxml.lib)   
    target_link_libraries(SampleApplication zlib.lib)
    target_link_libraries(SampleApplication zzip.lib)
        
    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(SampleApplication tbb_static.lib)
			target_link_libraries(SampleApplication tbbmalloc_static.lib)
		else()
			target_link_libraries(SampleApplication tbb.lib)
			target_link_libraries(SampleApplication tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    # fireblade libraries
    target_link_libraries(SampleApplication libWPCore.a)
    target_link_libraries(SampleApplication libWPGraphics.a)
    target_link_libraries(SampleApplication libWPMesh.a)
    target_link_libraries(SampleApplication libWPPhysx.a)
    target_link_libraries(SampleApplication libWPSQLite.a)

    target_link_libraries(SampleApplication libWPAssimp.a)

    if (WP_BUILD_OISINPUT)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleApplication FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleApplication freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleApplication WPGraphicsOgreNext.a)

        target_link_libraries(SampleApplication harWPuzz.a)
        target_link_libraries(SampleApplication harWPuzz-subset.a)
        target_link_libraries(SampleApplication icucommon.a)
        target_link_libraries(SampleApplication sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleApplication WPGraphicsOgre.a)
    endif()

    if (WP_BUILD_IMGUI)
        target_link_libraries(SampleApplication WPImGui.a)
        target_link_libraries(SampleApplication imgui.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleApplication WPPhysx.a)

        target_link_libraries(SampleApplication PhysX3.a)
        target_link_libraries(SampleApplication PhysX3Extensions.a)
        target_link_libraries(SampleApplication PhysX3Common.a)
        target_link_libraries(SampleApplication PhysX3Vehicle.a)
        target_link_libraries(SampleApplication PhysX3Cooking.a)
        target_link_libraries(SampleApplication PhysXProfileSDK.a)
        target_link_libraries(SampleApplication PxTask.a)
        target_link_libraries(SampleApplication PhysXVisualDebuggerSDK.a)
        target_link_libraries(SampleApplication PhysX3MetaData.a)
        target_link_libraries(SampleApplication PhysX3Foundation.a)
        target_link_libraries(SampleApplication PxSceneQuery.a)
        target_link_libraries(SampleApplication PxSimulationController.a)
        target_link_libraries(SampleApplication PhysX3LowLevel.a)
        target_link_libraries(SampleApplication PhysX3LowLevelCloth.a)
    elseif (WP_BUILD_ODE)
        target_link_libraries(SampleApplication libWPODE3.a)
    endif()

    target_link_libraries(SampleApplication libboost_thread.a)
    target_link_libraries(SampleApplication libboost_date_time.a)
    target_link_libraries(SampleApplication libboost_chrono.a)
    target_link_libraries(SampleApplication libboost_context.a)
    target_link_libraries(SampleApplication libboost_timer.a)
    target_link_libraries(SampleApplication libboost_filesystem.a)
    target_link_libraries(SampleApplication libboost_unit_test_framework.a)
    target_link_libraries(SampleApplication libboost_json.a)

    target_link_libraries(SampleApplication minizip.a)
    target_link_libraries(SampleApplication nativefiledialog.a)
    target_link_libraries(SampleApplication tinyxml.a)
    target_link_libraries(SampleApplication zlib.a)
    target_link_libraries(SampleApplication zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(SampleApplication libtbb_static.a)
        target_link_libraries(SampleApplication libtbbmalloc_static.a)
    endif()

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(SampleApplication libWm5Core.a)
        target_link_libraries(SampleApplication libWm5Mathematics.a)
    endif ()

    #target_link_libraries(SampleApplication stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(SampleApplication ${AUDIO_UNIT})
    target_link_libraries(SampleApplication stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
    set_target_properties(SampleApplication PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(SampleApplication -Wl,--start-group WPCore.a 
        WPGraphics.a WPGraphicsOgreNext.a WPImGui.a WPMesh.a       
        libboost_thread-gcc5-mt-s-x64-1_70.a
        libboost_date_time-gcc5-mt-s-x64-1_70.a
        libboost_chrono-gcc5-mt-s-x64-1_70.a
        libboost_timer-gcc5-mt-s-x64-1_70.a
        libboost_filesystem-gcc5-mt-s-x64-1_70.a
        freetype.a
        debug libtbb_debug.a optimized libtbb.a
        debug libtbbmalloc_debug.a optimized libtbbmalloc.a
        libWm5Core.a libWm5Mathematics.a
        -Wl,--end-group )

    target_link_libraries(SampleApplication png)
    target_link_libraries(SampleApplication Xaw)
    target_link_libraries(SampleApplication Xrandr)
    target_link_libraries(SampleApplication Xt)
    target_link_libraries(SampleApplication xcb)
    target_link_libraries(SampleApplication GL)
    target_link_libraries(SampleApplication ${X11_LIBRARIES})
    target_link_libraries(SampleApplication ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(SampleApplication ${CMAKE_DL_LIBS})
endif ()
endif()

if (WIN32)
    set_property(TARGET SampleApplication PROPERTY VS_DEBUGGER_WORKING_DIRECTORY 
        "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
    add_dependencies(SampleApplication WPGraphicsOgreNext)
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(SampleApplication WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(SampleApplication WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(SampleApplication WPPhysx)
endif()

set_target_properties (SampleApplication PROPERTIES FOLDER Samples)
