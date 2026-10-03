if (APPLE)
    project(imgui C CXX OBJC)
else()
    project(imgui C CXX)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Add the flag for enabling weak references (adjust as needed)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fobjc-weak")
endif()

file(GLOB_RECURSE HEADER_FILES "./*.h")
#file(GLOB_RECURSE SOURCE_FILES "./*.c" "./*.cpp")

if (WIN32)
    file(GLOB EXTERN_FILE "imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_dx11.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_dx9.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "backends/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "backends/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    else()
        file(GLOB EXTERN_FILE "backends/imgui_impl_win32.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_USE_FREETYPE)
        file(GLOB EXTERN_FILE "misc/freetype/imgui_freetype.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()
elseif(APPLE)
    file(GLOB EXTERN_FILE "imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "backends/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "backends/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_metal.mm")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "backends/imgui_impl_osx.mm")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "MainMenu.storyboard")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})
elseif(UNIX)
    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/FBImGui/Extern/imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})
endif()

include_directories( ./ )
include_directories( ../freetype/include )

if (UNIX)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

add_library(imgui ${HEADER_FILES} ${SOURCE_FILES})
