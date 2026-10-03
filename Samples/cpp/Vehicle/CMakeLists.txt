set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(SampleVehicle)

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
include_directories(Source)

if (WIN32)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinybind/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ois/)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib/)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib/)
        
        if (WP_PLATFORM_X64)
            link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
        else ()
            link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x86)
        endif()
    endif ()
elseif (APPLE)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/assimp/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ois/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogrenext/dependencies/lib/${CMAKE_BUILD_TYPE})
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/ogre/dependencies/lib/)
        link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()
elseif (UNIX)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(SampleVehicle ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(SampleVehicle ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(SampleVehicle Workphone.lib)  
    target_link_libraries(SampleVehicle WPSQLite.lib)

    if (WP_BUILD_OISINPUT)
        target_link_libraries(SampleVehicle WPOISInput.lib)

        #target_link_libraries(SampleVehicle debug OIS_d.lib optimized OIS.lib)
        target_link_libraries(SampleVehicle dinput8.lib)
        target_link_libraries(SampleVehicle dxguid.lib)
    endif()

    if (WP_USE_WPXSDK)
        #target_link_libraries(SampleVehicle libWPxsdk.lib)
        target_link_libraries(SampleVehicle libWPxsdk-mt.lib)
        target_link_libraries(SampleVehicle libxml2-mt.lib)
        target_link_libraries(SampleVehicle tinyxml2.lib)
    endif()

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleVehicle FreeImage.lib)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleVehicle freetype.lib)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleVehicle WPGraphicsOgreNext.lib)

        target_link_libraries(SampleVehicle harfbuzz.lib)
        target_link_libraries(SampleVehicle harfbuzz-subset.lib)
        target_link_libraries(SampleVehicle icucommon.lib)
        target_link_libraries(SampleVehicle sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(SampleVehicle d3d11.lib)
            target_link_libraries(SampleVehicle dxgi.lib)
            target_link_libraries(SampleVehicle dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(SampleVehicle opengl32.lib)
            target_link_libraries(SampleVehicle glu32.lib)
        endif ()
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleVehicle WPGraphicsOgre.lib)

        if (WP_GRAPHICS_SYSTEM_USE_STATIC_LIBS)
            target_link_libraries(SampleVehicle debug OgreMainStatic_d.lib optimized OgreMainStatic.lib)
            target_link_libraries(SampleVehicle debug OgreOverlayStatic_d.lib optimized OgreOverlayStatic.lib)

            target_link_libraries(SampleVehicle debug RenderSystem_Direct3D9Static_d.lib
                                optimized RenderSystem_Direct3D9Static.lib)

            target_link_libraries(SampleVehicle debug RenderSystem_Direct3D11Static_d.lib
                                optimized RenderSystem_Direct3D11Static.lib)

            target_link_libraries(SampleVehicle debug RenderSystem_GL3PlusStatic_d.lib 
                                optimized RenderSystem_GL3PlusStatic.lib)

            target_link_libraries(SampleVehicle debug RenderSystem_GLStatic_d.lib
                                optimized RenderSystem_GLStatic.lib)

            target_link_libraries(SampleVehicle debug OgreGLSupportStatic_d.lib
                                optimized OgreGLSupportStatic.lib)

            target_link_libraries(SampleVehicle debug OgreTerrainStatic_d.lib optimized OgreTerrainStatic.lib)

            target_link_libraries(SampleVehicle debug OgreRTShaderSystemStatic_d.lib
                                       optimized OgreRTShaderSystemStatic.lib)

            target_link_libraries(SampleVehicle debug Codec_STBIStatic_d.lib
                                       optimized Codec_STBIStatic.lib)

            target_link_libraries(SampleVehicle debug OgreBitesStatic_d.lib
                                       optimized OgreBitesStatic.lib)
        endif()

        #target_link_libraries(SampleVehicle freetype.lib)

        if (WP_BUILD_RENDERER_DX9)
            target_link_libraries(SampleVehicle d3d9.lib)
            target_link_libraries(SampleVehicle d3dx9.lib)
        endif ()

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(SampleVehicle d3d11.lib)
            target_link_libraries(SampleVehicle dxgi.lib)
            target_link_libraries(SampleVehicle dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_OPENGL)
            target_link_libraries(SampleVehicle opengl32.lib)
            target_link_libraries(SampleVehicle glu32.lib)
        endif ()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(SampleVehicle opengl32.lib)
            target_link_libraries(SampleVehicle glu32.lib)
        endif ()
    elseif(WP_BUILD_DX11)
        target_link_libraries(SampleVehicle WPGraphicsD3D11.lib)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleVehicle WPPhysx.lib)

        target_link_libraries(SampleVehicle PhysX3.lib)
        target_link_libraries(SampleVehicle PhysX3Extensions.lib)
        target_link_libraries(SampleVehicle PhysX3Common.lib)
        target_link_libraries(SampleVehicle PhysX3Vehicle.lib)
        target_link_libraries(SampleVehicle PhysX3Cooking.lib)
        target_link_libraries(SampleVehicle PhysXProfileSDK.lib)
        target_link_libraries(SampleVehicle PxTask.lib)
        target_link_libraries(SampleVehicle PhysXVisualDebuggerSDK.lib)
        target_link_libraries(SampleVehicle PhysX3MetaData.lib)
        target_link_libraries(SampleVehicle PhysX3Foundation.lib)
        target_link_libraries(SampleVehicle PxSceneQuery.lib)
        target_link_libraries(SampleVehicle PxSimulationController.lib)
        target_link_libraries(SampleVehicle PhysX3LowLevel.lib)
        target_link_libraries(SampleVehicle PhysX3LowLevelCloth.lib)
    elseif(WP_BUILD_ODE)
        target_link_libraries(SampleVehicle WPODE3.lib)
    elseif(WP_BUILD_PHYSICS3)
        target_link_libraries(SampleVehicle WPPhysics.lib)
    endif()

    target_link_libraries(SampleVehicle bcrypt.lib)
    target_link_libraries(SampleVehicle libboost_thread.lib)
    target_link_libraries(SampleVehicle libboost_date_time.lib)
    target_link_libraries(SampleVehicle libboost_chrono.lib)
    target_link_libraries(SampleVehicle libboost_timer.lib)
    target_link_libraries(SampleVehicle libboost_filesystem.lib)
    target_link_libraries(SampleVehicle libboost_json.lib)
    target_link_libraries(SampleVehicle minizip.lib)
    target_link_libraries(SampleVehicle nativefiledialog.lib)
    target_link_libraries(SampleVehicle rttr.lib)
    target_link_libraries(SampleVehicle tinyxml.lib)   
    target_link_libraries(SampleVehicle yaml-cpp.lib)
    target_link_libraries(SampleVehicle zlib.lib)
    target_link_libraries(SampleVehicle zzip.lib)

    if (WP_USE_TBB)
        if (WP_TBB_BUILD_STATIC)
			target_link_libraries(SampleVehicle tbb_static.lib)
			target_link_libraries(SampleVehicle tbbmalloc_static.lib)
		else()
			target_link_libraries(SampleVehicle tbb.lib)
			target_link_libraries(SampleVehicle tbbmalloc.lib)
        endif()
    endif()
elseif (APPLE)
    # fireblade libraries
    target_link_libraries(SampleVehicle libWPCore.a)
    target_link_libraries(SampleVehicle libWPGraphics.a)
    target_link_libraries(SampleVehicle libWPMesh.a)
    target_link_libraries(SampleVehicle libWPPhysx.a)
    target_link_libraries(SampleVehicle libWPSQLite.a)
    target_link_libraries(SampleVehicle libWPOISInput.a)

    if (WP_CODEC_FREEIMAGE)
        target_link_libraries(SampleVehicle FreeImage.a)
    endif()

    if (WP_USE_FREETYPE)
        target_link_libraries(SampleVehicle freetype.a)
    endif()

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        target_link_libraries(SampleVehicle WPGraphicsOgreNext.a)
        target_link_libraries(SampleVehicle harWPuzz.a)
        target_link_libraries(SampleVehicle harWPuzz-subset.a)
        target_link_libraries(SampleVehicle icucommon.a)
        target_link_libraries(SampleVehicle sds_library.a)
    elseif (WP_GRAPHICS_SYSTEM_OGRE)
        target_link_libraries(SampleVehicle WPGraphicsOgre.a)
    endif()

    if (WP_BUILD_PHYSX)
        target_link_libraries(SampleVehicle WPPhysx.a)

        target_link_libraries(SampleVehicle PhysX3.a)
        target_link_libraries(SampleVehicle PhysX3Extensions.a)
        target_link_libraries(SampleVehicle PhysX3Common.a)
        target_link_libraries(SampleVehicle PhysX3Vehicle.a)
        target_link_libraries(SampleVehicle PhysX3Cooking.a)
        target_link_libraries(SampleVehicle PhysXProfileSDK.a)
        target_link_libraries(SampleVehicle PxTask.a)
        target_link_libraries(SampleVehicle PhysXVisualDebuggerSDK.a)
        target_link_libraries(SampleVehicle PhysX3MetaData.a)
        target_link_libraries(SampleVehicle PhysX3Foundation.a)
        target_link_libraries(SampleVehicle PxSceneQuery.a)
        target_link_libraries(SampleVehicle PxSimulationController.a)
        target_link_libraries(SampleVehicle PhysX3LowLevel.a)
        target_link_libraries(SampleVehicle PhysX3LowLevelCloth.a)
    endif()

    target_link_libraries(SampleVehicle libboost_thread.a)
    target_link_libraries(SampleVehicle libboost_date_time.a)
    target_link_libraries(SampleVehicle libboost_chrono.a)
    target_link_libraries(SampleVehicle libboost_context.a)
    target_link_libraries(SampleVehicle libboost_timer.a)
    target_link_libraries(SampleVehicle libboost_filesystem.a)
    target_link_libraries(SampleVehicle libboost_json.a)

    target_link_libraries(SampleVehicle minizip.a)
    target_link_libraries(SampleVehicle nativefiledialog.a)
    target_link_libraries(SampleVehicle rttr.a)
    target_link_libraries(SampleVehicle tinyxml.a)
    target_link_libraries(SampleVehicle yaml-cpp.a)
    target_link_libraries(SampleVehicle zlib.a)
    target_link_libraries(SampleVehicle zzip.a)

    if (WP_USE_TBB)
        target_link_libraries(SampleVehicle tbb_static.a)
        target_link_libraries(SampleVehicle tbbmalloc_static.a)
    endif ()

    target_link_libraries(SampleVehicle debug libOIS_d.a optimized libOIS.a)

    if (WP_BUILD_WILDMAGIC5)
        target_link_libraries(SampleVehicle libWm5Core.a)
        target_link_libraries(SampleVehicle libWm5Mathematics.a)
    endif()

    #target_link_libraries(SampleVehicle stdc++ "-framework CoreGraphics" objc)
    #target_link_libraries(SampleVehicle ${AUDIO_UNIT})
    target_link_libraries(SampleVehicle stdc++ "-framework Metal" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" objc)
    set_target_properties(SampleVehicle PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
else ()
    target_link_libraries(SampleVehicle -Wl,--start-group WPCore.a WPMesh.a 
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
        
    target_link_libraries(SampleVehicle ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(SampleVehicle ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
    set_property(TARGET SampleVehicle PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

add_dependencies(SampleVehicle WPOISInput)
add_dependencies(SampleVehicle WPVehiclePhysics)
if (WP_STATIC_LIB)
    target_link_libraries(SampleVehicle WPVehiclePhysics)
endif()

if (WP_GRAPHICS_SYSTEM_OGRENEXT) 
    add_dependencies(SampleVehicle WPGraphicsOgreNext)
elseif (WP_GRAPHICS_SYSTEM_OGRE)
    add_dependencies(SampleVehicle WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(SampleVehicle WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(SampleVehicle WPPhysx)
endif()

set_target_properties (SampleVehicle PROPERTIES FOLDER Samples)

if(WP_GRAPHICS_SYSTEM_CLAW)
    target_include_directories(SampleVehicle PRIVATE
        ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
        ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
    target_link_libraries(SampleVehicle WPGraphics WorkphoneGraphics)
endif()
