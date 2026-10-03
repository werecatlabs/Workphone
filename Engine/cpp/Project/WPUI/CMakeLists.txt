project(WPImGui)

if (APPLE)
    project(WPImGui C CXX OBJC)
else()
    project(WPImGui C CXX)
endif()

add_compile_definitions (IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)

if (WP_GRAPHICS_SYSTEM_OGRENEXT)
    add_compile_definitions (WP_OGRE_USE_PARTICLE_UNIVERSE=0)
    add_compile_definitions (RAPIDJSON_HAS_STDSTRING=1)
    add_compile_definitions (OGRE_STATIC_LIB)
    add_compile_definitions (FREEIMAGE_LIB)
    add_compile_definitions (OGRE_DEPRECATED_2_2)
    #add_compile_definitions (IM_VEC2_CLASS_EXTRA)
endif()

# define header and source files for the library
if (WIN32)
    file(GLOB ROOT_HEADER_FILES "../../Include/WPImGui/*.h" "../../Include/WPImGui/*.hpp" "../../Include/WPImGui/*.inl")
    file(GLOB ROOT_SOURCE_FILES "../../Source/WPImGui/*.cpp")
elseif(APPLE)
    file(GLOB ROOT_HEADER_FILES "../../Include/WPImGui/*.h" "../../Include/WPImGui/*.hpp" "../../Include/WPImGui/*.inl")
    file(GLOB ROOT_SOURCE_FILES "../../Source/WPImGui/*.cpp" "../../Source/WPImGui/*.mm")
elseif (UNIX)
    file(GLOB ROOT_HEADER_FILES "../../Include/WPImGui/*.h" "../../Include/WPImGui/*.hpp" "../../Include/WPImGui/*.inl")
    file(GLOB ROOT_SOURCE_FILES "../../Source/WPImGui/*.cpp")
endif()

list(APPEND HEADER_FILES ${ROOT_HEADER_FILES})
list(APPEND SOURCE_FILES ${ROOT_SOURCE_FILES})

if (WIN32)
    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_dx11.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_dx9.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    else()
        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_win32.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_GRAPHICS_SYSTEM_OGRE AND NOT WP_GRAPHICS_SYSTEM_OGRENEXT)
        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_freetype.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()
elseif(APPLE)
    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_metal.mm")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_osx.mm")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Apple/MainMenu.storyboard")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Apple/ImGuiApplicationOSX.mm")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})
elseif(UNIX)
    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_demo.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_draw.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_opengl3.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    if (WP_BUILD_SDL2)
        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdl.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})

        file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_impl_sdlrenderer.cpp")
        list(APPEND SOURCE_FILES ${EXTERN_FILE})
    endif()

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_tables.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})

    file(GLOB EXTERN_FILE "../../Source/WPImGui/Extern/imgui_widgets.cpp")
    list(APPEND SOURCE_FILES ${EXTERN_FILE})
endif()

file(GLOB IMGUI_HEADER_FILES "../../Include/WPImGui/ImguiOgre/*.hpp")
file(GLOB IMGUI_SOURCE_FILES "../../Source/WPImGui/ImguiOgre/*.cpp")

list(APPEND HEADER_FILES ${IMGUI_HEADER_FILES})
list(APPEND SOURCE_FILES ${IMGUI_SOURCE_FILES})

file(GLOB IMGUI_HEADER_FILES "../../Include/WPImGui/ImGuizmo/*.hpp")
file(GLOB IMGUI_SOURCE_FILES "../../Source/WPImGui/ImGuizmo/*.cpp")

list(APPEND HEADER_FILES ${IMGUI_HEADER_FILES})
list(APPEND SOURCE_FILES ${IMGUI_SOURCE_FILES})

set(WP_HEADER_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPImGui/../../Include/WPImGui")
set(WP_SOURCE_REL_PATH "${WP_ROOT}/Engine/cpp/Project/WPImGui/../../Source/WPImGui")

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

if (WP_BUILD_IMGUI)
    include_directories( ${WP_ROOT}/Dependencies/imgui )
    include_directories( ${WP_ROOT}/Dependencies/imgui/misc/freetype )
endif()

include_directories( ${WP_ROOT}/Dependencies/zlib )
include_directories( ${WP_ROOT}/Dependencies/zziplib )
include_directories( ../../Include )

include_directories( ${WP_ROOT}/Engine/cpp/Include/WPImGui )
include_directories( ${WP_ROOT}/Engine/cpp/Include/WPImGui/Extern )
include_directories( ${WP_ROOT}/Engine/cpp/Include/WPImGui/ImGuizmo )

if (WP_USE_PRECOMPILED_HEADERS)
    wp_use_precompiled_header(Workphone "WPImGui/WPImGuiPCH.hpp" "${CMAKE_SOURCE_DIR}/Engine/cpp/Source/WPImGui/WPImGuiPCH.cpp")

    file(GLOB_RECURSE SOURCE_FILES_EXTERN "../../Source/WPImGui/Extern/*.cpp" "../../Source/WPImGui/Extern/*.c" )
    foreach(_source IN ITEMS ${SOURCE_FILES_EXTERN})
        set_source_files_properties(${_source}
            PPROPERTIES COMPILE_FLAGS /Y-
	    )
    endforeach()

    file(GLOB_RECURSE SOURCE_FILES_EXTERN "../../Source/WPImGui/ImguiOgre/*.cpp" "../../Source/WPImGui/ImguiOgre/*.c" )
    foreach(_source IN ITEMS ${SOURCE_FILES_EXTERN})
        set_source_files_properties(${_source}
            PPROPERTIES COMPILE_FLAGS /Y-
	    )
    endforeach()

    file(GLOB_RECURSE SOURCE_FILES_EXTERN "../../Source/WPImGui/ImGuizmo/*.cpp" "../../Source/WPImGui/ImGuizmo/*.c" )
    foreach(_source IN ITEMS ${SOURCE_FILES_EXTERN})
        set_source_files_properties(${_source}
            PPROPERTIES COMPILE_FLAGS /Y-
	    )
    endforeach()
endif()

if (NOT WP_STATIC_LIB)
    if (WIN32)
        link_directories(WPImGui ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    endif()
endif()

add_library(WPImGui STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties (WPImGui PROPERTIES FOLDER Engine/Cpp/Render)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WPImGui PUBLIC WPImGui_EXPORTS)
endif()

if (NOT WP_STATIC_LIB)
    add_dependencies(WPImGui Workphone)
endif()
