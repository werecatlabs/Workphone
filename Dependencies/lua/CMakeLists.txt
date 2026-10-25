project(lua)

include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ./ )
include_directories( ../ )

file(GLOB_RECURSE HEADER_FILES "./inc/*.h" "./inc/*.hpp" "./inc/*.inl" )
file(GLOB_RECURSE SOURCE_FILES "./src/*.c" "./src/*.cpp")

add_library(lua STATIC ${HEADER_FILES} ${SOURCE_FILES})
add_executable(luavm ${HEADER_FILES} ${SOURCE_FILES})
if(WP_BUILD_TESTS)
    add_test(NAME WPLua.editor_debugger_panel
        COMMAND luavm "${WP_ROOT}/Tests/lua/LuaDebuggerPanelContracts.lua")
    set_tests_properties(WPLua.editor_debugger_panel PROPERTIES WORKING_DIRECTORY "${WP_ROOT}" TIMEOUT 15)
endif()
