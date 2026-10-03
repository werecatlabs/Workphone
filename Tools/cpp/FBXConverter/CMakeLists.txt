cmake_minimum_required(VERSION 3.13)
project(WPXConverter)

add_compile_definitions (_WP_STATIC_LIB_)
add_compile_definitions (_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS)


# define header and source files for the library
file(GLOB_RECURSE HEADER_FILES "Include/*.h")
file(GLOB_RECURSE SOURCE_FILES "Source/*.cpp")

set(WP_HEADER_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/WPXConverter/Include")
set(WP_SOURCE_REL_PATH "${CMAKE_SOURCE_DIR}/Tools/WPXConverter/Source")

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

include_directories( ../../Dependencies/boost)
include_directories( ../../Dependencies/tinyxml/include)
include_directories( ../../Dependencies/tinyxml2)

if (WP_USE_THREADING_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_THREADING_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

if (WP_BUILD_OGRE)
    include_directories( ../../AdditionalLibraries/ogre/include )
    include_directories( ../../AdditionalLibraries/ogre/OgreMain/include )

    if(WIN32)
        include_directories( ../../AdditionalLibraries/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ../../AdditionalLibraries/ogre/include/osx )
    else()
        include_directories( ../../AdditionalLibraries/ogre/include/linux )
    endif()
endif()

include_directories( ../../AdditionalLibraries/WPxsdk/2020.2/include )

include_directories( ../../AdditionalLibraries/cppsqlite/include)
include_directories( ../../AdditionalLibraries/sqlite/include)
include_directories( ../../AdditionalLibraries/wxWidgets  )
include_directories( ../../AdditionalLibraries/wxWidgets/include )
include_directories( ../../AdditionalLibraries/wxWidgets/include/msvc )
include_directories( ../../AdditionalLibraries/wxWidgets/include/msvc/wx )
include_directories( ../../AdditionalLibraries/wxWidgets/lib/vc_x64_lib/mswud  )
include_directories( ../../WPEngine/Include)
include_directories(${CMAKE_SOURCE_DIR}/Tools/WPXConverter/Include)
include_directories(${CMAKE_SOURCE_DIR}/Tools/WPXConverter/Source)

if (WIN32)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/physx/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/boost/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/tinybind/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/WPxsdk/2020.0.1/${CMAKE_BUILD_TYPE})

    if (WP_BUILD_OGRENEXT)
        link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/ogrenext/dependencies/lib)
    elseif(WP_BUILD_OGRE)
        link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/ogre/dependencies/lib)
        link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/directx/Lib/x64)
    endif ()

    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/wxWidgets/lib/vc_x64_lib)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/windows/${WP_TOOLSET}/${WP_ARCH}/assimp/${CMAKE_BUILD_TYPE})
elseif (APPLE)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/WPEngine/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tinybind)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/physx)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/macOS/wildmagic)
elseif (UNIX)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/WPEngine/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tinybind)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/physx)
    link_directories(WPXConverter ${CMAKE_SOURCE_DIR}/AdditionalLibraries/lib/linux/wildmagic)
endif (WIN32)


if (WIN32)
    #add_executable(WPXConverter WIN32 ${HEADER_FILES} ${SOURCE_FILES})
    add_library(WPXConverter STATIC ${HEADER_FILES} ${SOURCE_FILES})
else()
    #add_executable(WPXConverter ${HEADER_FILES} ${SOURCE_FILES})
    add_library(WPXConverter STATIC ${HEADER_FILES} ${SOURCE_FILES})
endif()




if (WIN32)
    target_link_libraries(WPXConverter WPCore.lib)
    target_link_libraries(WPXConverter WPFileSystem.lib)
    target_link_libraries(WPXConverter WPMath.lib)
    target_link_libraries(WPXConverter WPMesh.lib)
    target_link_libraries(WPXConverter WPInterface.lib)
    target_link_libraries(WPXConverter WPProcedural.lib)
    target_link_libraries(WPXConverter WPSystem.lib)
    target_link_libraries(WPXConverter WPPhysx.lib)
    target_link_libraries(WPXConverter WPData.lib)
    target_link_libraries(WPXConverter WPSQLite.lib)
    target_link_libraries(WPXConverter WPObjectTemplates.lib)
    target_link_libraries(WPXConverter WPVehicle.lib)
    target_link_libraries(WPXConverter WPWxWidgets.lib)
    target_link_libraries(WPXConverter WPObjectTemplates.lib)
    target_link_libraries(WPXConverter.lib)
    target_link_libraries(WPXConverter WPStateMessages.lib)

    target_link_libraries(WPXConverter WPGraphics.lib)

    target_link_libraries(WPXConverter libWPxsdk.lib)
    target_link_libraries(WPXConverter libWPxsdk-mt.lib)
    target_link_libraries(WPXConverter libxml2-mt.lib)

    if (WP_BUILD_OGRENEXT)
        target_link_libraries(WPXConverter WPGraphicsOgreNext.lib)

        target_link_libraries(WPXConverter OgreMainStatic_d.lib)
        target_link_libraries(WPXConverter OgreOverlayStatic_d.lib)
        target_link_libraries(WPXConverter RenderSystem_Direct3D11Static_d.lib)
        target_link_libraries(WPXConverter RenderSystem_GL3PlusStatic_d.lib)

        target_link_libraries(WPXConverter OgreHlmsPbsStatic_d.lib)
        target_link_libraries(WPXConverter OgreHlmsUnlitStatic_d.lib)

        target_link_libraries(WPXConverter FreeImage_d.lib)
        target_link_libraries(WPXConverter freetype_d.lib)

    elseif(WP_BUILD_OGRE)
        target_link_libraries(WPXConverter WPGraphicsOgre.lib)

        target_link_libraries(WPXConverter debug OgreMainStatic_d.lib
                            optimized OgreMainStatic.lib)

        target_link_libraries(WPXConverter debug OgreOverlayStatic_d.lib
                            optimized OgreOverlayStatic.lib)
        
        target_link_libraries(WPXConverter debug RenderSystem_Direct3D9Static_d.lib
                            optimized RenderSystem_Direct3D9Static.lib)

        target_link_libraries(WPXConverter debug RenderSystem_Direct3D11Static_d.lib
                            optimized RenderSystem_Direct3D11Static.lib)

        target_link_libraries(WPXConverter debug RenderSystem_GL3PlusStatic_d.lib 
                            optimized RenderSystem_GL3PlusStatic.lib)

        target_link_libraries(WPXConverter debug RenderSystem_GLStatic_d.lib
                            optimized RenderSystem_GLStatic.lib)

        target_link_libraries(WPXConverter debug OgreGLSupportStatic_d.lib
                            optimized OgreGLSupportStatic.lib)

        #target_link_libraries(WPXConverter OgreHLMSStatic_d.lib)

        target_link_libraries(WPXConverter debug OgreRTShaderSystemStatic_d.lib
                                   optimized OgreRTShaderSystemStatic.lib)

        target_link_libraries(WPXConverter debug Codec_STBIStatic_d.lib
                                   optimized Codec_STBIStatic.lib)

        target_link_libraries(WPXConverter debug OgreBitesStatic_d.lib
                                   optimized OgreBitesStatic.lib)

        target_link_libraries(WPXConverter debug OgreTerrainStatic_d.lib
                                    optimized OgreTerrainStatic.lib)
    
        #target_link_libraries(WPXConverter Plugin_CgProgramManagerStatic_d.lib)
        #target_link_libraries(WPXConverter cg.lib)

        #target_link_libraries(WPXConverter debug FreeImage_d.lib
         #                       optimized FreeImage.lib)
        target_link_libraries(WPXConverter freetype.lib)

        target_link_libraries(WPXConverter d3d9.lib)
        target_link_libraries(WPXConverter d3dx9.lib)
    elseif(WP_BUILD_DX11)
        target_link_libraries(WPXConverter WPGraphicsD3D11.lib)
    endif()

    #target_link_libraries(WPXConverter tinybind.lib)

    if (WP_BUILD_PHYSX)
        target_link_libraries(WPXConverter WPPhysx.lib)

        if (WP_PLATFORM_X64)
            target_link_libraries(WPXConverter 
                        debug PhysX3DEBUG_x64.lib
                        optimized PhysX3_x64.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3CommonDEBUG_x64.lib 
                        optimized PhysX3Common_x64.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3CookingDEBUG_x64.lib
                        optimized PhysX3Cooking_x64.lib)
            target_link_libraries(WPXConverter 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(WPXConverter 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(WPXConverter 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        else ()
            target_link_libraries(WPXConverter 
                        debug PhysX3DEBUG_x86.lib
                        optimized PhysX3_x86.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3ExtensionsDEBUG.lib
                        optimized PhysX3Extensions.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3CommonDEBUG_x86.lib 
                        optimized PhysX3Common_x86.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3VehicleDEBUG.lib 
                        optimized PhysX3Vehicle.lib)
            target_link_libraries(WPXConverter 
                        debug PhysX3CookingDEBUG_x86.lib
                        optimized PhysX3Cooking_x86.lib)
            target_link_libraries(WPXConverter 
                        debug PhysXProfileSDKDEBUG.lib
                        optimized PhysXProfileSDK.lib)
            target_link_libraries(WPXConverter 
                        debug PxTaskDEBUG.lib
                        optimized PxTask.lib)
            target_link_libraries(WPXConverter 
                        debug PhysXVisualDebuggerSDKDEBUG.lib
                        optimized PhysXVisualDebuggerSDK.lib)
        endif()
    endif()

    target_link_libraries(WPXConverter debug wxmsw31ud_core.lib optimized wxmsw31u_core.lib)
    target_link_libraries(WPXConverter debug wxbase31ud.lib optimized wxbase31u.lib)
    target_link_libraries(WPXConverter debug wxmsw31ud_propgrid.lib optimized wxmsw31u_propgrid.lib)
    target_link_libraries(WPXConverter debug wxmsw31ud_richtext.lib optimized wxmsw31u_richtext.lib)
    target_link_libraries(WPXConverter debug wxbase31ud_xml.lib optimized wxbase31u_xml.lib)
    target_link_libraries(WPXConverter debug wxmsw31ud_adv.lib optimized wxmsw31u_adv.lib)
    target_link_libraries(WPXConverter debug wxmsw31ud_aui.lib optimized wxmsw31u_aui.lib)
    target_link_libraries(WPXConverter debug wxmsw31ud_html.lib optimized wxmsw31u_html.lib)
    target_link_libraries(WPXConverter debug wxtiffd.lib optimized wxtiff.lib)
    target_link_libraries(WPXConverter debug wxjpegd.lib optimized wxjpeg.lib)
    target_link_libraries(WPXConverter debug wxpngd.lib optimized wxpng.lib)
    target_link_libraries(WPXConverter debug wxzlibd.lib optimized wxzlib.lib)
    target_link_libraries(WPXConverter debug wxregexud.lib optimized wxregexu.lib)
    target_link_libraries(WPXConverter debug wxexpatd.lib optimized wxexpat.lib)

    target_link_libraries(WPXConverter kernel32.lib)
    target_link_libraries(WPXConverter user32.lib)
    target_link_libraries(WPXConverter gdi32.lib)
    target_link_libraries(WPXConverter comdlg32.lib)
    target_link_libraries(WPXConverter winspool.lib)
    target_link_libraries(WPXConverter winmm.lib)
    target_link_libraries(WPXConverter shell32.lib)
    target_link_libraries(WPXConverter comctl32.lib)
    target_link_libraries(WPXConverter ole32.lib)
    target_link_libraries(WPXConverter oleaut32.lib)
    target_link_libraries(WPXConverter uuid.lib)
    target_link_libraries(WPXConverter rpcrt4.lib)
    target_link_libraries(WPXConverter advapi32.lib)
    target_link_libraries(WPXConverter wsock32.lib)
    target_link_libraries(WPXConverter wininet.lib)

    target_link_libraries(WPXConverter opengl32.lib)
    target_link_libraries(WPXConverter glu32.lib)
    target_link_libraries(WPXConverter d3d11.lib)
    target_link_libraries(WPXConverter dxgi.lib)
    target_link_libraries(WPXConverter dxguid.lib)
    target_link_libraries(WPXConverter amd_ags_x64.lib)

    target_link_libraries(WPXConverter debug assimp-vc142-mtd.lib
                            optimized assimp-vc142-mt.lib)

    target_link_libraries(WPXConverter 
                        debug Wm5CoreD.lib
                        optimized Wm5Core.lib)
    target_link_libraries(WPXConverter 
                        debug Wm5MathematicsD.lib
                        optimized Wm5Mathematics.lib)
    target_link_libraries(WPXConverter 
                        debug Wm5PhysicsD.lib
                        optimized Wm5Physics.lib)

elseif (APPLE)
    # fireblade libraries
    target_link_libraries(WPXConverter libWPCore.a)
    target_link_libraries(WPXConverter libWPFileSystem.a)
    target_link_libraries(WPXConverter libWPMath.a)
    target_link_libraries(WPXConverter libWPMesh.a)
    target_link_libraries(WPXConverter libWPInterface.a)
    target_link_libraries(WPXConverter libWPProcedural.a)
    target_link_libraries(WPXConverter libWPSystem.a)
    target_link_libraries(WPXConverter libWPPhysx.a)
    target_link_libraries(WPXConverter libWPData.a)
    target_link_libraries(WPXConverter libWPObjectTemplates.a)
    target_link_libraries(WPXConverter libWPVehicle.a)

    if (WIN32)
        target_link_libraries(WPXConverter libboost_thread-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_date_time-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_chrono-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_timer-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_filesystem-gcc5-mt-s-x64-1_70.a)
    elseif (APPLE)
        target_link_libraries(WPXConverter libboost_thread.a)
        target_link_libraries(WPXConverter libboost_date_time.a)
        target_link_libraries(WPXConverter libboost_chrono.a)
        target_link_libraries(WPXConverter libboost_timer.a)
        target_link_libraries(WPXConverter libboost_filesystem.a)
    elseif (UNIX)
        target_link_libraries(WPXConverter libboost_thread-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_date_time-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_chrono-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_timer-gcc5-mt-s-x64-1_70.a)
        target_link_libraries(WPXConverter libboost_filesystem-gcc5-mt-s-x64-1_70.a)
    endif ()

    # physx libraries
    if (WIN32)
    elseif (APPLE)
        target_link_libraries(WPXConverter libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a)
    elseif (UNIX)
        target_link_libraries(WPXConverter -Wl,--start-group libLowLevel.a libLowLevelCloth.a libPhysXProfileSDK.a libPhysX3Common.a libSceneQuery.a libSimulationController.a libPhysX3Cooking.a libPhysX3Extensions.a libPhysX3.a libPhysX3Vehicle.a -Wl,--end-group )
    endif ()

    target_link_libraries(WPXConverter libPxTask.a)
    target_link_libraries(WPXConverter libPhysXVisualDebuggerSDK.a)
    target_link_libraries(WPXConverter libPvdRuntime.a)


    target_link_libraries(WPXConverter libtinybind.a)

    target_link_libraries(WPXConverter libtbb.a)
    target_link_libraries(WPXConverter libtbbmalloc.a)

    target_link_libraries(WPXConverter libWm5Core.a)
    target_link_libraries(WPXConverter libWm5Mathematics.a)

endif (WIN32)

add_dependencies(WPXConverterWPWxWidgets WPAero WPAi WPAudio WPCore WPData WPInterface WPFileSystem WPMath WPMesh WPObjectTemplates WPProcedural WPSQLite WPObjectTemplates WPSystem WPVehicle)

if (WP_BUILD_OGRENEXT) 
    add_dependencies(WPXConverter WPGraphicsOgreNext)
elseif (WP_BUILD_OGRE)
    add_dependencies(WPXConverter WPGraphicsOgre)
endif()

if (WP_BUILD_ODE)
    add_dependencies(WPXConverter WPODE3)
elseif (WP_BUILD_PHYSX)
    add_dependencies(WPXConverter WPPhysx)
endif()

set_property(TARGET WPXConverter PROPERTY VS_DEBUGGER_COMMAND ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug/WPXConverter.exe)
set_property(TARGET WPXConverter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Bin/windows/v142/x64/Debug)
set_target_properties (WPXConverter PROPERTIES FOLDER Tools)