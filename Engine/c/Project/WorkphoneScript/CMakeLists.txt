project(WorkphoneScript)

include_directories( ../../Include/WorkphoneCore/ )
include_directories( ../../Include/WorkphoneScript/ )
include_directories( ${WP_ROOT}/Dependencies/boost )
include_directories( ${WP_ROOT}/Dependencies/lua/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc )
include_directories( ${WP_ROOT}/Dependencies/luabind/inc/luabind )

file(GLOB_RECURSE HEADER_FILES "../../Include/WorkphoneScript/*.h" )
file(GLOB_RECURSE SOURCE_FILES
    "../../Source/WorkphoneScript/*.c"
    "../../Source/WorkphoneScript/*.cpp")

add_library(WorkphoneScript STATIC ${HEADER_FILES} ${SOURCE_FILES})
target_compile_definitions(WorkphoneScript PRIVATE BOOST_BIND_GLOBAL_PLACEHOLDERS)
set_target_properties(WorkphoneScript PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)

if (TARGET lua AND TARGET luabind)
    target_link_libraries(WorkphoneScript PRIVATE luabind lua)
elseif (WIN32)
    target_link_directories(WorkphoneScript PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/${CMAKE_BUILD_TYPE})
    target_link_libraries(WorkphoneScript PRIVATE lua.lib luabind.lib)
elseif (APPLE)
    target_link_directories(WorkphoneScript PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/macOS/${CMAKE_BUILD_TYPE})
    target_link_libraries(WorkphoneScript PRIVATE luabind lua)
elseif (UNIX)
    target_link_directories(WorkphoneScript PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/linux/${CMAKE_BUILD_TYPE})
    target_link_libraries(WorkphoneScript PRIVATE luabind lua)
endif()

set_target_properties (WorkphoneScript PROPERTIES FOLDER Engine/C)

if (NOT WP_STATIC_LIB)
	target_compile_definitions(WorkphoneScript PUBLIC WorkphoneScript_EXPORTS)
endif()

if (WP_BUILD_TESTS)
    add_executable(WorkphoneScriptTests
        "${CMAKE_SOURCE_DIR}/Tests/c/WorkphoneScriptTests.c")
    target_include_directories(WorkphoneScriptTests PRIVATE
        ../../Include/WorkphoneCore/
        ../../Include/WorkphoneScript/)
    target_link_libraries(WorkphoneScriptTests PRIVATE WorkphoneScript)
    set_target_properties(WorkphoneScriptTests PROPERTIES
        FOLDER Tests/C
        C_STANDARD 90
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF)
endif()
