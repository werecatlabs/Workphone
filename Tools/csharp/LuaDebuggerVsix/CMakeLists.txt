# VSIX packaging is owned by the SDK project, not a C# executable target.
if(NOT WIN32 OR NOT CMAKE_GENERATOR MATCHES "Visual Studio")
    return()
endif()
add_custom_target(LuaDebuggerVsix
    COMMAND "${CMAKE_VS_MSBUILD_COMMAND}"
        "${CMAKE_CURRENT_SOURCE_DIR}/LuaDebuggerVsix.csproj" /restore /m
        "/p:Configuration=$<CONFIG>"
        "/p:BaseIntermediateOutputPath=${CMAKE_CURRENT_BINARY_DIR}/obj/"
        "/p:OutputPath=${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" VERBATIM)
set_target_properties(LuaDebuggerVsix PROPERTIES FOLDER Tools/csharp)
