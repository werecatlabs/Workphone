if (APPLE)
    project(WPGraphicsOgreNext C CXX OBJC)
else()
    project(WPGraphicsOgreNext C CXX)
endif()

option(WP_OGRENEXT_LIBS "Build with ogre next libs" OFF)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_INCLUDE_STANDARD_IO)

add_compile_definitions (WP_OGRE_USE_PARTICLE_UNIVERSE=0)
add_compile_definitions (RAPIDJSON_HAS_STDSTRING=1)
add_compile_definitions (OGRE_STATIC_LIB)
add_compile_definitions (FREEIMAGE_LIB)
#add_compile_definitions (OGRE_USE_SIMD=0)
#add_compile_definitions (OGRE_THREAD_SUPPORT=0)
#add_compile_definitions (OGRE_THREAD_PROVIDER=0)
add_compile_definitions (U_STATIC_IMPLEMENTATION=1)

if (WP_BUILD_OGRENEXT_DEPRECATED)
    add_compile_definitions (OGRE_DEPRECATED_2_2)
endif()

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPGraphicsOgreNext/../../Include/WPGraphicsOgreNext")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPGraphicsOgreNext/../../Source/WPGraphicsOgreNext")

file(GLOB HEADER_FILES "../../Include/WPGraphicsOgreNext/*.hpp" "../../Include/WPGraphicsOgreNext/*.h" "../../Include/WPGraphicsOgreNext/*.inl")
file(GLOB SOURCE_FILES "../../Source/WPGraphicsOgreNext/*.h" "../../Source/WPGraphicsOgreNext/*.cpp")

if (WIN32)
    file(GLOB HEADER_FILES_OGRE_SETTINGS "../../Include/WPGraphicsOgreNext/BuildSettings/win32/*.h" "../../Include/WPGraphicsOgreNext/BuildSettings/win32/*.hpp" )
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_SETTINGS})
elseif(APPLE)
    file(GLOB HEADER_FILES_OGRE_SETTINGS "../../Include/WPGraphicsOgreNext/BuildSettings/osx/*.h" "../../Include/WPGraphicsOgreNext/BuildSettings/osx/*.hpp" )
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_SETTINGS})
else()
    file(GLOB HEADER_FILES_OGRE_SETTINGS "../../Include/WPGraphicsOgreNext/BuildSettings/linux/*.h" "../../Include/WPGraphicsOgreNext/BuildSettings/linux/*.hpp" )
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_SETTINGS})
endif()

if (WIN32)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_WIN32 "../../Include/WPGraphicsOgreNext/Window/Windows/*.hpp")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_WIN32 "../../Source/WPGraphicsOgreNext/Window/Windows/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_WIN32})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_WIN32})
elseif(APPLE)
    set(CMAKE_MACOSX_RPATH 1)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(IOKIT_LIBRARY IOKit)
    find_path(COCOA_INCLUDE_DIR Cocoa/Cocoa.hpp)
    find_library(CARBON_LIBRARY Carbon)

    #add_definitions(-x objective-c++)

    file(GLOB HEADER_FILES_OGRE_INCLUDE_OSX "../../Include/WPGraphicsOgreNext/Window/Apple/*.hpp")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_OSX "../../Source/WPGraphicsOgreNext/Window/Apple/*.cpp" "../../Source/WPGraphicsOgreNext/Window/Apple/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_OSX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_OSX})

    file(GLOB HEADER_FILES_OGRE_INCLUDE_OSX "../../Include/WPGraphicsOgreNext/Window/Apple/macOS/*.hpp")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_OSX "../../Source/WPGraphicsOgreNext/Window/Apple/macOS/*.cpp" "../../Source/WPGraphicsOgreNext/Window/Apple/macOS/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_OSX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_OSX})
elseif(ANDROID)
    # Android windowing is provided by OgreMain's Android support below.
elseif(UNIX)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_UNIX "../../Include/WPGraphicsOgreNext/Window/linux/*.hpp")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_UNIX "../../Source/WPGraphicsOgreNext/Window/linux/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_UNIX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_UNIX})
endif()

file(GLOB HEADER_FILES_CORE "../../Include/WPGraphicsOgreNext/Core/*.hpp")
file(GLOB SOURCE_FILES_CORE "../../Source/WPGraphicsOgreNext/Core/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_CORE})
list(APPEND SOURCE_FILES ${SOURCE_FILES_CORE})

file(GLOB_RECURSE HEADER_FILES_ColibriGui "../../Include/WPGraphicsOgreNext/ColibriGui/*.h")
file(GLOB_RECURSE SOURCE_FILES_ColibriGui "../../Source/WPGraphicsOgreNext/ColibriGui/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_ColibriGui})
list(APPEND SOURCE_FILES ${SOURCE_FILES_ColibriGui})

file(GLOB_RECURSE HEADER_FILES_ui "../../Include/WPGraphicsOgreNext/UI/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_ui "../../Source/WPGraphicsOgreNext/UI/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_ui})
list(APPEND SOURCE_FILES ${SOURCE_FILES_ui})

file(GLOB HEADER_FILES_ADDONS "../../Include/WPGraphicsOgreNext/AddOns/*.hpp")
file(GLOB SOURCE_FILES_ADDONS "../../Source/WPGraphicsOgreNext/AddOns/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_ADDONS})
list(APPEND SOURCE_FILES ${SOURCE_FILES_ADDONS})

file(GLOB HEADER_FILES_JOBS "../../Include/WPGraphicsOgreNext/Jobs/*.hpp")
file(GLOB SOURCE_FILES_JOBS "../../Source/WPGraphicsOgreNext/Jobs/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_JOBS})
list(APPEND SOURCE_FILES ${SOURCE_FILES_JOBS})

file(GLOB HEADER_FILES_LOD "../../Include/WPGraphicsOgreNext/LOD/*.hpp")
file(GLOB SOURCE_FILES_LOD "../../Source/WPGraphicsOgreNext/LOD/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_LOD})
list(APPEND SOURCE_FILES ${SOURCE_FILES_LOD})

file(GLOB_RECURSE HEADER_FILES_TERRA "../../Include/WPGraphicsOgreNext/Terra/*.hpp")
file(GLOB_RECURSE SOURCE_FILES_TERRA "../../Source/WPGraphicsOgreNext/Terra/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_TERRA})
list(APPEND SOURCE_FILES ${SOURCE_FILES_TERRA})

file(GLOB HEADER_FILES_UTILS "../../Include/WPGraphicsOgreNext/Utils/*.hpp")
file(GLOB SOURCE_FILES_UTILS "../../Source/WPGraphicsOgreNext/Utils/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_UTILS})
list(APPEND SOURCE_FILES ${SOURCE_FILES_UTILS})

file(GLOB HEADER_FILES_WRAPPER "../../Include/WPGraphicsOgreNext/Wrapper/*.hpp" "../../Include/WPGraphicsOgreNext/Wrapper/*.inl")
file(GLOB SOURCE_FILES_WRAPPER "../../Source/WPGraphicsOgreNext/Wrapper/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_WRAPPER})
list(APPEND SOURCE_FILES ${SOURCE_FILES_WRAPPER})

if(APPLE)
    file(GLOB HEADER_FILES_WRAPPER "../../Include/WPGraphicsOgreNext/Wrapper/Apple/*.hpp" "../../Include/WPGraphicsOgreNext/WrapperApple/*.inl")
    file(GLOB SOURCE_FILES_WRAPPER "../../Source/WPGraphicsOgreNext/Wrapper/Apple/*.cpp" "../../Source/WPGraphicsOgreNext/Wrapper/Apple/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_WRAPPER})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_WRAPPER})
endif()

if (NOT WP_OGRENEXT_LIBS)
file(GLOB HEADER_FILES_OGRE "../../Include/WPGraphicsOgreNext/Ogre/*.h")
file(GLOB SOURCE_FILES_OGRE "../../Source/WPGraphicsOgreNext/Ogre/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGRE})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE})

file(GLOB HEADER_FILES_OGRE_INCLUDE "../../Include/WPGraphicsOgreNext/Ogre/include/*.h")
file(GLOB SOURCE_FILES_OGRE_INCLUDE "../../Source/WPGraphicsOgreNext/Ogre/src/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE})

if (WIN32)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_WIN32 "../../Include/WPGraphicsOgreNext/Ogre/include/win32/*.h")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_WIN32 "../../Source/WPGraphicsOgreNext/Ogre/include/win32/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_WIN32})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_WIN32})
elseif(APPLE)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_OSX "../../Include/WPGraphicsOgreNext/Ogre/include/osx/*.h")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_OSX "../../Source/WPGraphicsOgreNext/Ogre/include/osx/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_OSX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_OSX})
elseif(ANDROID)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_ANDROID "../../Include/WPGraphicsOgreNext/Ogre/include/Android/*.h")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_ANDROID "../../Source/WPGraphicsOgreNext/Ogre/include/Android/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_ANDROID})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_ANDROID})
elseif(UNIX)
    file(GLOB HEADER_FILES_OGRE_INCLUDE_UNIX "../../Include/WPGraphicsOgreNext/Ogre/include/linux/*.h")
    file(GLOB SOURCE_FILES_OGRE_INCLUDE_UNIX "../../Source/WPGraphicsOgreNext/Ogre/include/linux/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_INCLUDE_UNIX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_INCLUDE_UNIX})
endif()

file(GLOB_RECURSE HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Common/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Hlms/Common/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB_RECURSE HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Pbs/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Hlms/Pbs/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

#file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/PbsMobile/include/*.h")
#file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Hlms/PbsMobile/src/*.cpp")
#list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
#list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB_RECURSE HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Unlit/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Hlms/Unlit/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

#file(GLOB_RECURSE HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/UnlitMobile/*.h")
#file(GLOB_RECURSE SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Hlms/UnlitMobile/*.cpp")
#list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
#list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/Components/Overlay/include/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/Components/Overlay/src/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_MESH_LOD_GENERATOR
    "../../Include/WPGraphicsOgreNext/Ogre/Components/MeshLodGenerator/include/*.h")
file(GLOB SOURCE_FILES_MESH_LOD_GENERATOR
    "../../Source/WPGraphicsOgreNext/Ogre/Components/MeshLodGenerator/src/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_MESH_LOD_GENERATOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_MESH_LOD_GENERATOR})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Animation/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Animation/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/CommandBuffer/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/CommandBuffer/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB_RECURSE HEADER_FILES_OGREMAIN_COMPOSITOR "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN_COMPOSITOR "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Compositor/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_COMPOSITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_COMPOSITOR})

file(GLOB_RECURSE HEADER_FILES_OGREMAIN_COMPOSITOR "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compute/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN_COMPOSITOR "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Compute/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_COMPOSITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_COMPOSITOR})


if (WP_BUILD_OGRENEXT_DEPRECATED)
    file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Deprecated/*.h")
    file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Deprecated/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})
endif()

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Hash/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Hash/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Array/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

if (WP_USE_C_MATH)
    file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/C/*.h")
    file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Array/C/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})
elseif (WP_USE_NEON_MATH)
    file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/NEON/Single/*.h")
    file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Array/NEON/Single/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})
elseif (WP_USE_SIMD)
    file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/SSE2/Single/*.h")
    file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Array/SSE2/Single/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})
endif()

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Simple/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple/C/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Math/Simple/C/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB HEADER_FILES_OGREMAIN "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/ParticleSystem/*.h")
file(GLOB SOURCE_FILES_OGREMAIN "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/ParticleSystem/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN})

file(GLOB_RECURSE HEADER_FILES_OGREMAIN_COMPOSITOR "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/ogrestd/*.h")
file(GLOB_RECURSE SOURCE_FILES_OGREMAIN_COMPOSITOR "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/ogrestd/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_COMPOSITOR})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_COMPOSITOR})

if (WIN32)
    set(THREAD_SOURCE_FILES
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreBarrierWin.cpp            
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreDefaultWorkQueueStandard.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreLightweightMutexWin.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreSemaphore.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreThreadsWin.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreWaitableEvent.cpp
            )

    list(APPEND SOURCE_FILES ${THREAD_SOURCE_FILES})
elseif(APPLE)
    set(THREAD_SOURCE_FILES
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreBarrierPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreDefaultWorkQueueStandard.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreLightweightMutexPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreSemaphoreApple.mm
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreThreadsPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreWaitableEvent.cpp
            )

    list(APPEND SOURCE_FILES ${THREAD_SOURCE_FILES})
elseif(ANDROID)
    set(THREAD_SOURCE_FILES
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreBarrierPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreDefaultWorkQueueStandard.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreLightweightMutexPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreThreadsPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreWaitableEvent.cpp
            )

    list(APPEND SOURCE_FILES ${THREAD_SOURCE_FILES})
elseif(UNIX)
    set(THREAD_SOURCE_FILES
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreBarrierPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreDefaultWorkQueueStandard.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreLightweightMutexPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreThreadsPThreads.cpp
            ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/Threading/OgreWaitableEvent.cpp
            )

    list(APPEND SOURCE_FILES ${THREAD_SOURCE_FILES})
endif()


#file(GLOB HEADER_FILES_OGREMAIN_THREADING "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Threading/*.h")
#file(GLOB SOURCE_FILES_OGREMAIN_THREADING "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Threading/*.cpp")
#list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_THREADING})
#list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_THREADING})

file(GLOB HEADER_FILES_OGREMAIN_THREADING "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Vao/*.h")
file(GLOB SOURCE_FILES_OGREMAIN_THREADING "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Vao/*.cpp")
list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_THREADING})
list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_THREADING})

if (WIN32)
    file(GLOB_RECURSE HEADER_FILES_OGREMAIN_WIN32 "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/WIN32/*.h")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_WIN32})

    set(SOURCE_FILES_OGREMAIN_WIN32 
        ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/WIN32/OgreConfigDialog.cpp
        ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/WIN32/OgreErrorDialog.cpp
        ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/WIN32/OgreFileSystemLayer.cpp
        ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/WIN32/OgreOptimisedUtilDirectXMath.cpp
        ${WP_SOURCE_REL_PATH}/Ogre/OgreMain/src/WIN32/OgreTimer.cpp)
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_WIN32})

    if (MINGW)
        set(SOURCE_FILES_OGREMAIN_WIN32_MINGW "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/WIN32/OgreMinGWSupport.cpp")
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_WIN32_MINGW})
    endif()
elseif(APPLE)
    file(GLOB_RECURSE HEADER_FILES_OGREMAIN_MACOS "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/OSX/*.h")
    file(GLOB_RECURSE SOURCE_FILES_OGREMAIN_MACOS "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/OSX/*.cpp" "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/OSX/*.mm")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_MACOS})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_MACOS})
elseif(ANDROID)
    file(GLOB HEADER_FILES_OGREMAIN_ANDROID "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Android/*.h")
    file(GLOB SOURCE_FILES_OGREMAIN_ANDROID "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/Android/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_ANDROID})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_ANDROID})
elseif(UNIX)
    file(GLOB_RECURSE HEADER_FILES_OGREMAIN_LINUX "../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/GLX/*.h")
    file(GLOB_RECURSE SOURCE_FILES_OGREMAIN_LINUX "../../Source/WPGraphicsOgreNext/Ogre/OgreMain/src/GLX/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGREMAIN_LINUX})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGREMAIN_LINUX})
endif()

if (WIN32)
    if (WP_BUILD_RENDERER_DX11)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/src/Vao/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/VendorExtensions/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/src/VendorExtensions/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Windowing/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/src/Windowing/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Windowing/WIN32/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/src/Windowing/WIN32/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    endif()

    if (WP_BUILD_RENDERER_OPENGL)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GLSL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/Vao/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/WIN32/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    endif()
	
	if (WP_BUILD_RENDERER_GL3PLUS)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GLSL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/Vao/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/WIN32/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    endif()

    if (WP_BUILD_RENDERER_OPENGL OR WP_BUILD_RENDERER_GL3PLUS)
        file(GLOB HEADER_FILES_SUBFOLDER "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/include/*.h")
        file(GLOB SOURCE_FILES_SUBFOLDER "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_SUBFOLDER})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_SUBFOLDER})

        file(GLOB HEADER_FILES_SUBFOLDER "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/include/GLSL/*.h")
        file(GLOB SOURCE_FILES_SUBFOLDER "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/src/GLSL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_SUBFOLDER})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_SUBFOLDER})

        file(GLOB HEADER_FILES_SUBFOLDER "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/include/win32/*.h")
        file(GLOB SOURCE_FILES_SUBFOLDER "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GLSupport/src/win32/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_SUBFOLDER})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_SUBFOLDER})
    endif()
elseif(APPLE)
    if (WP_BUILD_RENDERER_OPENGL)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GLSL/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/Vao/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/OSX/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/OSX/*.cpp" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/OSX/*.mm")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    endif()

    if (WP_BUILD_RENDERER_METAL)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/src/*.mm")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/src/Vao/*.mm")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/Windowing/OSX/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/src/Windowing/OSX/*.cpp" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/src/Windowing/OSX/*.mm")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
    endif()
elseif(ANDROID)
    if (WP_BUILD_RENDERER_VULKAN)
        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/*.h" "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Vao/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Vao/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing/Android/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing/Android/*.cpp")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/*.h" "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/include/spirv/unified1/*.h")
        file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/SPIRV-Reflect/*.c")
        list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
        list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

        set(WP_GLSLANG_ROOT "${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang")
        file(GLOB SOURCE_FILES_GLSLANG
            "${WP_GLSLANG_ROOT}/OGLCompilersDLL/*.cpp"
            "${WP_GLSLANG_ROOT}/SPIRV/*.cpp"
            "${WP_GLSLANG_ROOT}/glslang/CInterface/*.cpp"
            "${WP_GLSLANG_ROOT}/glslang/GenericCodeGen/*.cpp"
            "${WP_GLSLANG_ROOT}/glslang/MachineIndependent/*.cpp"
            "${WP_GLSLANG_ROOT}/glslang/MachineIndependent/preprocessor/*.cpp"
            "${WP_GLSLANG_ROOT}/glslang/OSDependent/Unix/*.cpp"
        )
        list(APPEND SOURCE_FILES ${SOURCE_FILES_GLSLANG})
    endif()
elseif(UNIX)
    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GL/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GL/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/GLSL/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/Vao/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})

    file(GLOB HEADER_FILES_OGRE_RENDERSYSTEM "../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/GLX/*.h")
    file(GLOB SOURCE_FILES_OGRE_RENDERSYSTEM "../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/GLX/*.cpp")
    list(APPEND HEADER_FILES ${HEADER_FILES_OGRE_RENDERSYSTEM})
    list(APPEND SOURCE_FILES ${SOURCE_FILES_OGRE_RENDERSYSTEM})
endif()
endif()

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

#filter_items(${HEADER_FILES} "OgreMain/include/Android/.*")
#filter_items(${SOURCE_FILES} "OgreMain/src/Android/.*")

include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/tinyxml/include )
include_directories( ${WP_ROOT}/Dependencies/utf8/source )

include_directories( ${WP_ROOT}/Engine/c/Include )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneCore )
include_directories( ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics )

include_directories( ${WP_ROOT}/Dependencies/ogre-next )
include_directories( ${WP_ROOT}/Dependencies/ogre-next/BuildSettings/win32 )

include_directories( ${WP_ROOT}/Dependencies/harfbuzz/src )
include_directories( ${WP_ROOT}/Dependencies/sds_library/include )
include_directories( ${WP_ROOT}/Dependencies/icu )
include_directories( ${WP_ROOT}/Dependencies/icu/common )
include_directories( ${WP_ROOT}/Dependencies/icu/common/unicode )
include_directories( ${WP_ROOT}/Dependencies/zzip/include )
include_directories( ${WP_ROOT}/Dependencies/zlib/include )

if (WP_CODEC_FREEIMAGE)
    include_directories( ${WP_ROOT}/Dependencies/FreeImage/Source )
endif()

if (WP_USE_FREETYPE)
    include_directories( ${WP_ROOT}/Dependencies/freetype/include )
endif()

if(WIN32)
    include_directories( ${WP_ROOT}/Dependencies/windows )
elseif(APPLE)
    include_directories( ${WP_ROOT}/Dependencies/apple )
endif()

if (WP_USE_ONETBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/onetbb/include )
elseif (WP_USE_TBB)
    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/tbb/include )
endif()

include_directories( ${WP_ROOT}/Dependencies/assimp/include )
include_directories( ${WP_ROOT}/Dependencies/ogrenext/Dependencies/include )
include_directories( ${WP_ROOT}/Dependencies/rapidjson/include )
include_directories( ${WP_ROOT}/Dependencies/rapidjson/include/rapidjson )

if(WIN32)
    include_directories( ${WP_ROOT}/Dependencies/assimp/include/vc142 )
elseif(APPLE)
    include_directories( ${WP_ROOT}/Dependencies/assimp/include/vc142 )
else()
    include_directories( ${WP_ROOT}/Dependencies/assimp/include/vc142 )
endif()

if (WIN32)
    include_directories( ${WP_ROOT}/Engine/cpp/Include/WPGraphicsOgreNext/BuildSettings/win32 )
elseif(APPLE)
    include_directories( ../../Include/WPGraphicsOgreNext/BuildSettings/osx )
else()
    include_directories( ../../Include/WPGraphicsOgreNext/BuildSettings/linux )
endif()

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/include )

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Android )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Animation)
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/CommandBuffer )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor/Pass )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compositor/Pass/PassClear )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Compute )
#include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Deprecated )

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math )
include_directories( ../../Include/WPGraphicsOgreNext )

if (WP_USE_C_MATH)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/C )
elseif (WP_USE_NEON_MATH)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/NEON )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/NEON/Single )
elseif (WP_USE_SIMD)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/SSE2 )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Array/SSE2/Single )
endif()

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/Math/Simple/C )

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/ogrestd )

include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Overlay/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/MeshLodGenerator/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Paging/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Bites/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Common/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Pbs/include )
#include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/PbsMobile/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/Components/Hlms/Unlit/include )
include_directories( ../../Include/WPGraphicsOgreNext/Ogre/PlugIns/CgProgramManager/include )

if (WP_BUILD_IMGUI)
    include_directories( ${WP_ROOT}/Dependencies/imgui )
    include_directories( ${WP_ROOT}/Dependencies/imgui/backends )
    include_directories( ${WP_ROOT}/Dependencies/imgui/misc/freetype )
endif()

if (WIN32)
    if (WP_BUILD_RENDERER_DX11)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Deprecated )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Vao )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/VenderExtensions )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Direct3D11/include/Windowing )
    endif ()

    if (WP_BUILD_RENDERER_OPENGL)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/WIN32 )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src )
        #include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32 )
    endif()
	
	if (WP_BUILD_RENDERER_GL3PLUS)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/WIN32 )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src )
        #include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/WIN32 )
    endif()

    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL/src/atifs/include )

    if (WP_BUILD_RENDERER_NULL)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/NULL/include )
    endif()
elseif(APPLE)
    if (WP_BUILD_RENDERER_OPENGL)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/OSX )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src )
        #include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/OSX )
    endif()

    #include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GLES2/include )

    if (WP_BUILD_RENDERER_METAL)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Metal/include/Windowing/OSX )
    endif()
elseif(ANDROID)
    if (WP_BUILD_RENDERER_VULKAN)
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Vao )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/Windowing/Android )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/include )
        include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/include/SPIRV-Reflect/include/spirv/unified1 )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Vao )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/Windowing/Android )
        include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/Vulkan/src/SPIRV-Reflect )
        include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang )
        include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang/glslang )
        include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty/glslang/OGLCompilersDLL )
        include_directories( ${WP_ROOT}/Dependencies/bgfx/bgfx/3rdparty )
    endif()
elseif(UNIX)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/GLSL )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/Vao )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing )
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/include/windowing/GLX )
    include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src )
    include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing )
    include_directories( ../../Source/WPGraphicsOgreNext/Ogre/RenderSystems/GL3Plus/src/windowing/GLX )
endif()

include_directories( ../../Include )
include_directories( ../../Include/WPGraphicsOgreNext )
include_directories( ../../Include/WPGraphicsOgreNext/Extern )
include_directories( ../../Include/WPGraphicsOgreNext/Deferred )
include_directories( ../../Include/WPGraphicsOgreNext/ColibriGui )
include_directories( ../../Include/WPGraphicsOgreNext/UI )

if(WIN32)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/WIN32 )
elseif(APPLE)
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/OSX )
else()
    include_directories( ../../Include/WPGraphicsOgreNext/Ogre/OgreMain/include/GLX )
endif()

#if (WP_BUILD_IMGUI)
#    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/imgui )
#    include_directories( ${CMAKE_SOURCE_DIR}/Dependencies/imgui/backends )
#endif()

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(WPGraphicsOgreNext "WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPGraphicsOgreNext/WPGraphicsOgreNextPCH.cpp")

    file(GLOB_RECURSE OGRE_SOURCE_FILES "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPGraphicsOgreNext/Ogre/*.cpp")
    set_source_files_properties(${OGRE_SOURCE_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
 
    file(GLOB_RECURSE OGRE_SOURCE_FILES "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPGraphicsOgreNext/ColibriGui/*.cpp")
    set_source_files_properties(${OGRE_SOURCE_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    
    file(GLOB_RECURSE OGRE_SOURCE_FILES "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPGraphicsOgreNext/Terra/*.cpp")
    set_source_files_properties(${OGRE_SOURCE_FILES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPGraphicsOgreNext ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

file(GLOB_RECURSE OGRELIB_HEADER_FILES "../../Include/WPGraphicsOgreNext/Ogre/*.h" "../../Include/WPGraphicsOgreNext/Ogre/*.inl")
file(GLOB_RECURSE OGRELIB_SOURCE_FILES "../../Source/WPGraphicsOgreNext/Ogre/*.h" "../../Source/WPGraphicsOgreNext/Ogre/*.cpp")

if(MSVC)
    set_source_files_properties(${OGRELIB_HEADER_FILES} PROPERTIES COMPILE_FLAGS "/W0")
    set_source_files_properties(${OGRELIB_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "/W0")
else()
    set_source_files_properties(${OGRELIB_HEADER_FILES} PROPERTIES COMPILE_FLAGS "-w")
    set_source_files_properties(${OGRELIB_SOURCE_FILES} PROPERTIES COMPILE_FLAGS "-w")
endif()

if (WP_STATIC_LIB)
    add_library(WPGraphicsOgreNext STATIC ${HEADER_FILES} ${SOURCE_FILES})
else ()
    add_library(WPGraphicsOgreNext SHARED ${HEADER_FILES} ${SOURCE_FILES})
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        target_link_libraries(WPGraphicsOgreNext Workphone.lib)

        target_link_libraries(WPGraphicsOgreNext WorkphoneCore.lib)
        target_link_libraries(WPGraphicsOgreNext WorkphoneGraphics.lib)

        if (WP_USE_ONETBB)
            target_link_libraries(WPGraphicsOgreNext debug tbb12_debug.lib optimized tbb12.lib)
            target_link_libraries(WPGraphicsOgreNext debug tbbmalloc_debug.lib optimized tbbmalloc.lib)
            target_link_libraries(WPGraphicsOgreNext debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy.lib)
        elseif (WP_USE_TBB)
            if (WP_TBB_BUILD_STATIC)
                target_link_libraries(WPGraphicsOgreNext tbb_static.lib)
                target_link_libraries(WPGraphicsOgreNext tbbmalloc_static.lib)
            else()
                target_link_libraries(WPGraphicsOgreNext tbb.lib)
			    target_link_libraries(WPGraphicsOgreNext tbbmalloc.lib)
            endif()
        endif()

        target_link_libraries(WPGraphicsOgreNext WPImGui.lib)
        target_link_libraries(WPGraphicsOgreNext imgui.lib)

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPGraphicsOgreNext FreeImage.lib)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPGraphicsOgreNext freetype.lib)
        endif()

        target_link_libraries(WPGraphicsOgreNext harfbuzz.lib)
        target_link_libraries(WPGraphicsOgreNext harfbuzz.lib)

        target_link_libraries(WPGraphicsOgreNext harfbuzz.lib)
        target_link_libraries(WPGraphicsOgreNext harfbuzz-subset.lib)
        target_link_libraries(WPGraphicsOgreNext icucommon.lib)
        target_link_libraries(WPGraphicsOgreNext sds_library.lib)

        if (WP_BUILD_RENDERER_DX11)
            target_link_libraries(WPGraphicsOgreNext d3d11.lib)
            target_link_libraries(WPGraphicsOgreNext dxgi.lib)
            target_link_libraries(WPGraphicsOgreNext dxguid.lib)
        endif()

        if (WP_BUILD_RENDERER_GL3PLUS)
            target_link_libraries(WPGraphicsOgreNext opengl32.lib)
            target_link_libraries(WPGraphicsOgreNext glu32.lib)
        endif ()

        if (WP_PLATFORM_X64)
            #target_link_libraries(WPGraphicsOgreNext amd_ags_x64.lib)
        endif()

        target_link_libraries(WPGraphicsOgreNext zlib.lib)
        target_link_libraries(WPGraphicsOgreNext zzip.lib)
    elseif (APPLE)
        target_link_libraries(WPGraphicsOgreNext libWorkphone.a)
        target_link_libraries(WPGraphicsOgreNext libFBMesh.a)
        target_link_libraries(WPGraphicsOgreNext libWPProcedural.a)
        target_link_libraries(WPGraphicsOgreNext libWPAssimp.a)

        if (WP_BUILD_AUDIO)
            target_link_libraries(WPGraphicsOgreNext FBAudio.a)
        endif()

        if (WP_ENABLE_LUA)
            target_link_libraries(WPGraphicsOgreNext WPLua.a)
            target_link_libraries(WPGraphicsOgreNext WPLuaBind.a)
        elseif (WP_ENABLE_PYTHON)
            target_link_libraries(WPGraphicsOgreNext WPPython.a)
            target_link_libraries(WPGraphicsOgreNext WPPythonBind.a)
        endif()

        if (WP_BUILD_SDL2)
            target_link_libraries(WPGraphicsOgreNext SDL2main.a)
            target_link_libraries(WPGraphicsOgreNext SDL2_image.a)
            target_link_libraries(WPGraphicsOgreNext debug SDL2d.a optimized SDL2.a)
        endif()

        if (WP_BUILD_IMGUI)
            target_link_libraries(WPGraphicsOgreNext libWPImGui.a)
            target_link_libraries(WPGraphicsOgreNext imgui.a)
        endif()

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPGraphicsOgreNext libWPPhysx.a)
        elseif (WP_BUILD_ODE)
            target_link_libraries(WPGraphicsOgreNext libWPODE3.a)
        endif()

        if (WP_BUILD_OISINPUT)
            target_link_libraries(WPGraphicsOgreNext libWPOISInput.a)
            #target_link_libraries(WPGraphicsOgreNext debug libOIS_d.a optimized libOIS.a)
        endif()

        target_link_libraries(WPGraphicsOgreNext libWPVehiclePhysics.a)

        if (WP_BUILD_WXWIDGETS)
            #target_link_libraries(WPGraphicsOgreNext libWPWxWidgets.a)

            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_core-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_baseu-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_propgrid-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_richtext-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwxexpat-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_baseu_xml-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_baseu_net-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_webview-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_xrc-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_adv-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_aui-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwx_osx_cocoau_html-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwxtiff-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwxjpeg-3.1.a)
            target_link_libraries(WPGraphicsOgreNext libwxpng-3.1.a)

            target_link_libraries(WPGraphicsOgreNext libiconv.a)
        endif()

        target_link_libraries(WPGraphicsOgreNext libFBRenderUI.a)
        target_link_libraries(WPGraphicsOgreNext libWPSQLite.a)
        target_link_libraries(WPGraphicsOgreNext libFBCPlusPlus.a)
        target_link_libraries(WPGraphicsOgreNext libFBGraphics.a)

        target_link_libraries(WPGraphicsOgreNext libboost_thread.a)
        target_link_libraries(WPGraphicsOgreNext libboost_date_time.a)
        target_link_libraries(WPGraphicsOgreNext libboost_chrono.a)
        target_link_libraries(WPGraphicsOgreNext libboost_timer.a)
        target_link_libraries(WPGraphicsOgreNext libboost_filesystem.a)
        target_link_libraries(WPGraphicsOgreNext libboost_json.a)

        if (WP_BUILD_PHYSX)
            target_link_libraries(WPGraphicsOgreNext WPPhysx.a)

            target_link_libraries(WPGraphicsOgreNext PhysX3.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3Extensions.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3Common.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3Vehicle.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3Cooking.a)
            target_link_libraries(WPGraphicsOgreNext PhysXProfileSDK.a)
            target_link_libraries(WPGraphicsOgreNext PxTask.a)
            target_link_libraries(WPGraphicsOgreNext PhysXVisualDebuggerSDK.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3MetaData.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3Foundation.a)
            target_link_libraries(WPGraphicsOgreNext PxSceneQuery.a)
            target_link_libraries(WPGraphicsOgreNext PxSimulationController.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3LowLevel.a)
            target_link_libraries(WPGraphicsOgreNext PhysX3LowLevelCloth.a)
        endif()

        if (WP_USE_TBB)
            target_link_libraries(WPGraphicsOgreNext libtbb.a)
            target_link_libraries(WPGraphicsOgreNext libtbbmalloc.a)
        endif()

        target_link_libraries(WPGraphicsOgreNext minizip.a)
        target_link_libraries(WPGraphicsOgreNext nativefiledialog.a)
        target_link_libraries(WPGraphicsOgreNext rttr.a)
        target_link_libraries(WPGraphicsOgreNext tinyxml.a)
        target_link_libraries(WPGraphicsOgreNext yaml-cpp.a)
        target_link_libraries(WPGraphicsOgreNext zlib.a)
        target_link_libraries(WPGraphicsOgreNext zzip.a)

        if (WP_BUILD_WILDMAGIC5)
            target_link_libraries(WPGraphicsOgreNext libWm5Core.a)
            target_link_libraries(WPGraphicsOgreNext libWm5Mathematics.a)
        endif()

        if (WP_USE_ASSET_IMPORT)
            #target_link_libraries(WPGraphicsOgreNext debug libassimpd.a optimized libassimp.a)
        endif()

        if (WP_CODEC_FREEIMAGE)
            target_link_libraries(WPGraphicsOgreNext FreeImage.a)
        endif()

        if (WP_USE_FREETYPE)
            target_link_libraries(WPGraphicsOgreNext freetype.a)
        endif()

        if (WP_GRAPHICS_SYSTEM_OGRENEXT)
            target_link_libraries(WPGraphicsOgreNext WPGraphicsOgreNext.a)
            target_link_libraries(WPGraphicsOgreNext harfbuzz.a)
            target_link_libraries(WPGraphicsOgreNext harfbuzz-subset.a)
            target_link_libraries(WPGraphicsOgreNext icucommon.a)
            target_link_libraries(WPGraphicsOgreNext sds_library.a)
        elseif (WP_GRAPHICS_SYSTEM_OGRE)
            target_link_libraries(WPGraphicsOgreNext WPGraphicsOgre.a)
        endif()

        #target_link_libraries(OgreOverlayTest stdc++ "-framework CoreGraphics" objc)
        #target_link_libraries(OgreOverlayTest ${AUDIO_UNIT})
        target_link_libraries(WPGraphicsOgreNext stdc++ "-framework AppKit" "-framework Metal" "-framework MetalKit" "-framework OpenGL" "-framework Foundation" "-framework QuartzCore" "-framework CoreGraphics" "-framework CoreServices" "-framework Cocoa" "-framework Carbon" "-framework IOKit" "-framework GameController" objc)
        set_target_properties(WPGraphicsOgreNext PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    elseif (UNIX)
        target_link_libraries(WPGraphicsOgreNext -Wl,--start-group Workphone.a WPVehiclePhysics.a libWPProcedural.a WPImGui.a
            FBMath.a FBMesh.a WPOISInput.a WPVehiclePhysics.a FBData.a FBState.a libWPPhysx.a libWPSQLite.a
            libWPVehiclePhysics.a FBGraphics.a WPGraphicsOgreNext.a WPAssimp.a FBRenderUI.a libFBCPlusPlus.a
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
            target_link_libraries(WPGraphicsOgreNext WPGraphicsOgreNext.a)
            target_link_libraries(WPGraphicsOgreNext freetype.a)
        elseif(WP_GRAPHICS_SYSTEM_SOFTWARE)
            target_link_libraries(WPGraphicsOgreNext libFBGraphicsSoftware.a)
        endif()

        target_link_libraries(WPGraphicsOgreNext png)
        target_link_libraries(WPGraphicsOgreNext Xaw)
        target_link_libraries(WPGraphicsOgreNext Xrandr)
        target_link_libraries(WPGraphicsOgreNext Xt)
        target_link_libraries(WPGraphicsOgreNext xcb)
        target_link_libraries(WPGraphicsOgreNext GL)
        target_link_libraries(WPGraphicsOgreNext ${X11_LIBRARIES})
        target_link_libraries(WPGraphicsOgreNext ${CMAKE_THREAD_LIBS_INIT} )
        target_link_libraries(WPGraphicsOgreNext ${CMAKE_DL_LIBS})
    endif ()
endif()

set_target_properties (WPGraphicsOgreNext PROPERTIES FOLDER Engine/Cpp/Render)

if (APPLE)
    target_compile_options(WPGraphicsOgreNext PRIVATE "-fobjc-arc")
    set_target_properties(WPGraphicsOgreNext PROPERTIES LINK_FLAGS "-fobjc-arc")
endif()

if (APPLE)
    set_target_properties(WPGraphicsOgreNext PROPERTIES
            LINK_FLAGS "-framework Cocoa -framework OpenGL")
endif ()

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPGraphicsOgreNext PUBLIC WPGraphicsOgreNext_EXPORTS)
endif()

if (WP_BUILD_IMGUI)
    add_dependencies(WPGraphicsOgreNext WPImGui)
endif()

if (WP_BUILD_DEPENDENCIES)
	if (NOT WP_STATIC_LIB)
		add_dependencies(WPGraphicsOgreNext Workphone)

		if (WP_USE_ONETBB)
			add_dependencies(WPGraphicsOgreNext debug tbb12_debug.lib optimized tbb12)
			add_dependencies(WPGraphicsOgreNext debug tbbmalloc_debug.lib optimized tbbmalloc)
			add_dependencies(WPGraphicsOgreNext debug tbbmalloc_proxy_debug.lib optimized tbbmalloc_proxy)
		elseif (WP_USE_TBB)
			if (TBB_BUILD_STATIC)
				add_dependencies(WPGraphicsOgreNext tbb_static)
				add_dependencies(WPGraphicsOgreNext tbbmalloc_static)
			else()
				add_dependencies(WPGraphicsOgreNext tbb)
				add_dependencies(WPGraphicsOgreNext tbbmalloc)
			endif()
		endif()

		if (WP_BUILD_IMGUI)
			add_dependencies(WPGraphicsOgreNext WPImGui)
			add_dependencies(WPGraphicsOgreNext imgui)
		endif()

		if (WP_CODEC_FREEIMAGE)
			add_dependencies(WPGraphicsOgreNext FreeImage)
		endif()

		if (WP_USE_FREETYPE)
			add_dependencies(WPGraphicsOgreNext freetype)
		endif()

		add_dependencies(WPGraphicsOgreNext harfbuzz)
		add_dependencies(WPGraphicsOgreNext harfbuzz-subset)
		add_dependencies(WPGraphicsOgreNext icucommon)
		add_dependencies(WPGraphicsOgreNext sds_library)
	endif()
endif()

