cmake_minimum_required(VERSION 3.15)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(WorkphoneProceduralTextureExportTests LANGUAGES CXX)
    enable_testing()
endif()

find_program(WP_PROCEDURAL_LUA_VM NAMES luavm
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../../bin/windows/v145/x64/MD/Debug")
if(WP_PROCEDURAL_LUA_VM)
    add_test(NAME WPProcedural.texture_editor_chain
        COMMAND "${WP_PROCEDURAL_LUA_VM}" Tests/lua/ProceduralTextureEditorTests.lua)
    set_tests_properties(WPProcedural.texture_editor_chain PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../.."
        LABELS "procedural;texture;lua;contract")
endif()

if(NOT TARGET WPProceduralTextureExportTests)
    add_executable(WPProceduralTextureExportTests ../ProceduralTextureExportTests.cpp)
    target_include_directories(WPProceduralTextureExportTests PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../Engine/cpp/Include")
    target_compile_features(WPProceduralTextureExportTests PRIVATE cxx_std_17)
    set_target_properties(WPProceduralTextureExportTests PROPERTIES FOLDER Tests/Procedural)
    add_test(NAME WPProcedural.texture_export
        COMMAND WPProceduralTextureExportTests "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(WPProcedural.texture_export PROPERTIES LABELS "procedural;texture;contract")
endif()
