set(CMAKE_USE_RELATIVE_PATHS true)
set(CMAKE_SUPPRESS_REGENERATION true)

project(ZipObfuscator)

#ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories( ${WP_ROOT}/Dependencies/boost)
include_directories( ${WP_ROOT}/Dependencies/tbb/include)
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/Dependencies/cppsqlite/include)
include_directories( ${WP_ROOT}/Dependencies/sqlite/include)
include_directories( ${WP_ROOT}/Dependencies/zzip/include)
include_directories( ${WP_ROOT}/Engine/cpp/Include)
include_directories(Source)

if (WP_GRAPHICS_SYSTEM_OGRE)
    include_directories( ${WP_ROOT}/Dependencies/ogre/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/OgreMain/include )

    include_directories( ${WP_ROOT}/Dependencies/ogre/Components/Overlay/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/Components/Terrain/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/Components/Paging/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/Components/Bites/include )
    include_directories( ${WP_ROOT}/Dependencies/particle_universe/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/PlugIns/CgProgramManager/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/RenderSystems/Direct3D9/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/RenderSystems/Direct3D11/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/RenderSystems/GL/include )
    include_directories( ${WP_ROOT}/Dependencies/directx/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/PlugIns/STBICodec/include )
    include_directories( ${WP_ROOT}/Dependencies/ogre/Components/RTShaderSystem/include )

    if(WIN32)
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/win32 )
    elseif(APPLE)
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/osx )
    else()
        include_directories( ${WP_ROOT}/Dependencies/ogre/include/linux )
    endif()
endif()

if (WIN32)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/physx/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tbb/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/boost/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/tinyxml2/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/wildmagic/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/assimp/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/WPxsdk/2020.2/${CMAKE_BUILD_TYPE})

    if (WP_GRAPHICS_SYSTEM_OGRENEXT)
        link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/${CMAKE_BUILD_TYPE})
        link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogrenext/dependencies/lib)
    elseif(WP_GRAPHICS_SYSTEM_OGRE)
        link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/${CMAKE_BUILD_TYPE})
        link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/ogre/dependencies/lib)
        link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/directx/Lib/x64)
    endif ()
elseif (APPLE)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/boost/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/tbb/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/physx/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/macOS/wildmagic/${CMAKE_BUILD_TYPE})
elseif (UNIX)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/boost)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/tbb)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux)
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/physx/${CMAKE_BUILD_TYPE})
    link_directories(ZipObfuscator ${CMAKE_SOURCE_DIR}/Dependencies/lib/linux/wildmagic)
endif ()

file(GLOB_RECURSE HEADER_FILES "./*.h")
file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

add_executable(ZipObfuscator ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
target_link_options(ZipObfuscator PUBLIC 
$<$<CONFIG:Debug>:/LTCG>
$<$<CONFIG:Release>:/LTCG>
$<$<CONFIG:MinSizeRel>:/LTCG>
$<$<CONFIG:RelWithDebInfo>:/LTCG>
)

#target_link_options(ZipObfuscator PUBLIC 
#$<$<CONFIG:Debug>:/force:multiple>
#$<$<CONFIG:Release>:/force:multiple>
#$<$<CONFIG:MinSizeRel>:/force:multiple>
#$<$<CONFIG:RelWithDebInfo>:/force:multiple>
#)

endif (WIN32)

if (WIN32)
    target_link_libraries(ZipObfuscator Workphone.lib)
elseif (APPLE)
    target_link_libraries(ZipObfuscator WPCore.a)
elseif (UNIX)
    target_link_libraries(ZipObfuscator -Wl,--start-group WPCore.a 
        -Wl,--end-group )

    target_link_libraries(ZipObfuscator ${CMAKE_THREAD_LIBS_INIT} )
    target_link_libraries(ZipObfuscator ${CMAKE_DL_LIBS})
endif ()


if (WIN32)
elseif (APPLE)
    target_link_directories(ZipObfuscator PUBLIC ../Dependencies/lib/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(ZipObfuscator -L../dependencies/lib/linux/boost/)
elseif (UNIX)
    target_link_libraries(ZipObfuscator -L../dependencies/lib/linux/boost/)
    target_link_libraries(ZipObfuscator -L../dependencies/lib/linux/physx/)
    target_link_libraries(ZipObfuscator -L../dependencies/lib/linux/)
endif (WIN32)

if (WIN32)
    set_property(TARGET ZipObfuscator PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration)")
endif()

if (WP_BUILD_ENGINE)
    add_dependencies(ZipObfuscator Workphone)
endif()

set_target_properties (ZipObfuscator PROPERTIES FOLDER Tools/Cpp)
