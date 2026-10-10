if(WIN32 AND "${CMAKE_GENERATOR}" MATCHES "Visual Studio" AND WP_CSHARP)
    add_subdirectory(./FolderStructure)
    add_subdirectory(./AssetDatabaseTool)

endif()

if(WIN32 AND WP_BUILD_CSHARP AND "${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    add_subdirectory(./LuaDebuggerVsix)
endif()
