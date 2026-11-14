cmake_minimum_required(VERSION 3.15)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(WorkphoneProceduralTextureExportTests LANGUAGES CXX)
    enable_testing()
endif()

get_filename_component(WP_TEXTURE_EXPORT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
find_program(WP_PROCEDURAL_LUA_VM NAMES luavm
    HINTS "${WP_TEXTURE_EXPORT_ROOT}/bin/windows/v145/x64/MD/Debug")
if(WP_PROCEDURAL_LUA_VM)
    add_test(NAME WPProcedural.texture_editor_chain
        COMMAND "${WP_PROCEDURAL_LUA_VM}" Tests/lua/ProceduralTextureEditorTests.lua)
    set_tests_properties(WPProcedural.texture_editor_chain PROPERTIES
        WORKING_DIRECTORY "${WP_TEXTURE_EXPORT_ROOT}"
        LABELS "procedural;texture;lua;contract")
endif()

if(NOT TARGET WPProceduralTextureExportTests)
    add_executable(WPProceduralTextureExportTests ../ProceduralTextureExportTests.cpp)
    target_include_directories(WPProceduralTextureExportTests PRIVATE "${WP_TEXTURE_EXPORT_ROOT}/Engine/cpp/Include")
    target_compile_features(WPProceduralTextureExportTests PRIVATE cxx_std_17)
    set_target_properties(WPProceduralTextureExportTests PROPERTIES FOLDER Tests/cpp/Procedural)
    add_test(NAME WPProcedural.texture_export
        COMMAND WPProceduralTextureExportTests "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(WPProcedural.texture_export PROPERTIES LABELS "procedural;texture;contract")
endif()
