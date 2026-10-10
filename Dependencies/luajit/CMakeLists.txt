project(luajit LANGUAGES C)

if(NOT MSVC OR NOT CMAKE_GENERATOR MATCHES "Visual Studio" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The optional bundled LuaJIT build currently supports MSVC x64. Lua remains available on all existing platforms.")
endif()

# LuaJIT generates VM code and definition tables. A source glob cannot build it.
# Build a private copy per configuration, preserving the vendored source tree.
set(luajit_build_root "${CMAKE_CURRENT_BINARY_DIR}/upstream")
set(luajit_library "${luajit_build_root}/$<CONFIG>/lua51.lib")
set(luajit_executable "${luajit_build_root}/$<CONFIG>/luajit.exe")
file(GLOB_RECURSE luajit_inputs CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/dynasm/*")
add_custom_command(OUTPUT "${luajit_library}" "${luajit_executable}"
    COMMAND "${CMAKE_COMMAND}"
        "-DWP_LUAJIT_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}"
        "-DWP_LUAJIT_BUILD_ROOT=${luajit_build_root}"
        "-DWP_LUAJIT_CONFIG=$<CONFIG>"
        "-DWP_LUAJIT_VS=${CMAKE_GENERATOR_INSTANCE}"
        "-DWP_LUAJIT_CRT=${WP_STATIC_LINK_CRT_TYPE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/BuildLuaJit.cmake"
    DEPENDS ${luajit_inputs} "${CMAKE_CURRENT_SOURCE_DIR}/BuildLuaJit.cmake"
    COMMENT "Building optional LuaJIT with upstream VM generation" VERBATIM)
add_custom_target(LuaJitBuild DEPENDS "${luajit_library}" "${luajit_executable}")
add_library(luajit STATIC IMPORTED GLOBAL)
set_target_properties(luajit PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src")
foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${config}" upper_config)
    set_property(TARGET luajit PROPERTY "IMPORTED_LOCATION_${upper_config}"
        "${luajit_build_root}/${config}/lua51.lib")
endforeach()
add_dependencies(luajit LuaJitBuild)
set_target_properties(LuaJitBuild PROPERTIES FOLDER Dependencies/lua)
if(WP_BUILD_TESTS)
    add_test(NAME WPLua.luajit_backend_contracts
        COMMAND "${luajit_executable}" "${WP_ROOT}/Tests/lua/RuntimeBackendContracts.lua")
endif()
