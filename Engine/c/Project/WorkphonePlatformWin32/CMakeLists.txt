project(WorkphonePlatformWin32)

add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_ALLOCATOR)
add_compile_definitions (WORKPHONE_INCLUDE_FONT_BAKING)
add_compile_definitions (WORKPHONE_INCLUDE_DEFAULT_FONT)
add_compile_definitions (WORKPHONE_INCLUDE_VERTEX_BUFFER_OUTPUT)
add_compile_definitions (WORKPHONE_D3D11_IMPLEMENTATION)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneGraphics/ )
include_directories( ../../Include/WorkphonePlatformWin32/ )

if (WIN32)
	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_dx11_pixel_shader.h" )
	list(APPEND HEADER_FILES ${HEADERS})

	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_dx11_vertex_shader.h" )
	list(APPEND HEADER_FILES ${HEADERS})

	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.h" )
	file(GLOB SOURCES "../../Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx11.c")
	list(APPEND HEADER_FILES ${HEADERS})
	list(APPEND SOURCE_FILES ${SOURCES})

	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_renderer_dx12.h" )
	file(GLOB SOURCES "../../Source/WorkphonePlatformWin32/workphone_graphics_renderer_dx12.c")
	list(APPEND HEADER_FILES ${HEADERS})
	list(APPEND SOURCE_FILES ${SOURCES})

	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_platform_window_win32.h")
	file(GLOB SOURCES "../../Source/WorkphonePlatformWin32/workphone_platform_window_win32.c")
	list(APPEND HEADER_FILES ${HEADERS})
	list(APPEND SOURCE_FILES ${SOURCES})
elseif (APPLE)
	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_renderer_metal.h" )
	file(GLOB SOURCES "../../Source/WorkphonePlatformWin32/workphone_graphics_renderer_metal.c")
	list(APPEND HEADER_FILES ${HEADERS})
	list(APPEND SOURCE_FILES ${SOURCES})
else()
	file(GLOB HEADERS "../../Include/WorkphonePlatformWin32/workphone_graphics_renderer_vulkan.h" )
	file(GLOB SOURCES "../../Source/WorkphonePlatformWin32/workphone_graphics_renderer_vulkan.c")
	list(APPEND HEADER_FILES ${HEADERS})
	list(APPEND SOURCE_FILES ${SOURCES})
endif()

add_library(WorkphonePlatformWin32 STATIC ${HEADER_FILES} ${SOURCE_FILES})

if (WIN32)
    target_link_libraries(WorkphonePlatformWin32 PUBLIC d3d11 dxgi d3dcompiler dxguid)
endif()

set_target_properties (WorkphonePlatformWin32 PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphonePlatformWin32 PUBLIC WorkphonePlatformWin32_EXPORTS)
endif()
