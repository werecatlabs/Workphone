# Deliberately separate from dependency projects' install components.
if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR WP_STATIC_LIB
   OR NOT TARGET Editor OR NOT WP_GRAPHICS_SYSTEM_CLAW OR NOT WP_ENABLE_LUA)
    message(FATAL_ERROR "Editor packaging requires the shared Windows x64 Claw/Lua Editor build")
endif()

set(_editor_plugins WPGraphics WPAssimp WPSQLite WPPhysics WPLua WPAudio WPVehiclePhysics WPProcedural)
set(_editor_runtime_targets Editor Workphone WPNetwork ${_editor_plugins})
if(NOT TARGET WorkphoneResourcePackage)
    message(FATAL_ERROR "Editor package requires the checked resource package tool")
endif()
list(APPEND _editor_runtime_targets WorkphoneResourcePackage)
set(_editor_config "# Workphone Editor package plugin profile\n")
foreach(_plugin IN LISTS _editor_plugins)
    if(NOT TARGET ${_plugin})
        message(FATAL_ERROR "Editor package requires plugin target ${_plugin}")
    endif()
    string(APPEND _editor_config "Plugin=${_plugin}\n")
endforeach()
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/editor-package.cfg" "${_editor_config}")
add_custom_target(WorkphoneEditorPackageBinaries DEPENDS ${_editor_runtime_targets})

install(CODE "if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL \"RelWithDebInfo\")\n message(FATAL_ERROR \"Editor package requires RelWithDebInfo; Debug Editor is a test harness\")\nendif()" COMPONENT WorkphoneEditor)
install(TARGETS ${_editor_runtime_targets} RUNTIME_DEPENDENCY_SET WorkphoneEditorRuntime
    RUNTIME DESTINATION bin COMPONENT WorkphoneEditor CONFIGURATIONS RelWithDebInfo
    RESOURCE DESTINATION unused COMPONENT WorkphoneEditorDevelopment EXCLUDE_FROM_ALL)
# Resolve static PE imports AND explicitly seeded dynamic plugins. System/CRT DLLs
# remain Windows / Microsoft VC++ Redistributable prerequisites.
get_filename_component(_editor_linker_directory "${CMAKE_LINKER}" DIRECTORY)
install(CODE "set(CMAKE_GET_RUNTIME_DEPENDENCIES_COMMAND \"${_editor_linker_directory}/dumpbin.exe\")" COMPONENT WorkphoneEditor)
install(RUNTIME_DEPENDENCY_SET WorkphoneEditorRuntime
    DIRECTORIES "$<TARGET_FILE_DIR:Editor>"
    PRE_EXCLUDE_REGEXES "[Aa][Pp][Ii]-[Mm][Ss]-" "[Ee][Xx][Tt]-[Mm][Ss]-"
    POST_EXCLUDE_REGEXES "^[Cc]:[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\]"
    RUNTIME DESTINATION bin COMPONENT WorkphoneEditor CONFIGURATIONS RelWithDebInfo)
foreach(_target Editor ${_editor_plugins} Workphone WPNetwork WorkphoneResourcePackage)
    if(TARGET ${_target})
        install(FILES "$<TARGET_PDB_FILE:${_target}>" DESTINATION symbols
            COMPONENT WorkphoneEditorSymbols CONFIGURATIONS RelWithDebInfo)
    endif()
endforeach()
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/editor-package.cfg" DESTINATION . COMPONENT WorkphoneEditor)
install(DIRECTORY "${WP_ROOT}/bin/Media/Scripts/" DESTINATION Media/Scripts COMPONENT WorkphoneEditor
    FILES_MATCHING PATTERN "*.lua" PATTERN "*.sql")
file(GLOB _editor_media "${WP_ROOT}/bin/Media/*.mat" "${WP_ROOT}/bin/Media/*.png"
    "${WP_ROOT}/bin/Media/*.ttf" "${WP_ROOT}/bin/Media/*.fbmeshbin")
install(FILES ${_editor_media} DESTINATION Media COMPONENT WorkphoneEditor)
install(FILES "${WP_ROOT}/LICENSE" DESTINATION licenses COMPONENT WorkphoneEditor RENAME Workphone-LICENSE)
foreach(_notice "Dependencies/boost/LICENSE_1_0.txt" "Dependencies/imgui/LICENSE.txt"
        "Dependencies/rapidjson/license.txt"
        "Dependencies/meshoptimizer/LICENSE.md" "Dependencies/earcut-hpp/LICENSE"
        "Dependencies/tinyusdz/LICENSE"
        "docs/WPGRAPHICS_THIRD_PARTY_NOTICES.txt" "docs/WPGRAPHICS_EDITOR_THIRD_PARTY_NOTICES.txt"
        "Dependencies/cJSON/LICENSE" "Engine/cpp/Source/WPAssimp/Extern/LICENSE")
    if(EXISTS "${WP_ROOT}/${_notice}")
        string(REPLACE "/" "-" _notice_name "${_notice}")
        install(FILES "${WP_ROOT}/${_notice}" DESTINATION licenses COMPONENT WorkphoneEditor RENAME "${_notice_name}")
    endif()
endforeach()
install(FILES "${WP_ROOT}/docs/WPGRAPHICS_EDITOR_PACKAGE.md" DESTINATION . COMPONENT WorkphoneEditor RENAME README.md)
install(FILES "${WP_ROOT}/docs/WPGRAPHICS_CPP_SDK.md" DESTINATION . COMPONENT WorkphoneEditor)

include("${CMAKE_CURRENT_LIST_DIR}/InstallWorkphoneCppSDK.cmake")
