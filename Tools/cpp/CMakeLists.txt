set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(cpp_defines)
include(cpp_compiler_options)

if (WP_BUILD_EDITOR)
	add_subdirectory(./Editor)

	if (WP_BUILD_WXWIDGETS)
		#add_subdirectory(./WPGameEditorWx)
	endif()
endif()

if (WP_USE_HOUDINI)
	add_subdirectory(./Houdini)
endif()
	
add_subdirectory(./MeshConverter)
add_subdirectory(./MeshViewer)
add_subdirectory(./OSMToJson)
add_subdirectory(./ZipObfuscator)
#add_subdirectory(./BuildDependencies)

# Standalone C++17 port of the C# AssetDatabaseTool.  Re-uses the engine renderer
# (Workphone + WPGraphics), the ImGui based UI system (WPImGui) and the SQLite
# backed database interfaces (WPSQLite).  Only enabled when the engine and the
# editor/UI/database components are turned on.
if (WP_BUILD_EDITOR AND WP_BUILD_ENGINE AND WP_BUILD_SQLITE AND WP_BUILD_IMGUI)
    #add_subdirectory(./AssetDatabaseEditor)
endif()

