add_executable(WorkphoneResourcePackage main.cpp)
target_include_directories(WorkphoneResourcePackage PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
    ${WP_ROOT}/Engine/c/Include ${WP_ROOT}/Engine/c/Include/WorkphoneCore
    ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics ${WP_ROOT}/Engine/c/Include/WorkphonePlatformWin32
    ${WP_ROOT}/Dependencies/rapidjson/include)
target_link_libraries(WorkphoneResourcePackage PRIVATE
    $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPSQLite>
    $<TARGET_LINKER_FILE:WPGraphics> $<TARGET_LINKER_FILE:WPLua>)
add_dependencies(WorkphoneResourcePackage Workphone WPSQLite WPGraphics WPLua)
if(WIN32)
    target_link_libraries(WorkphoneResourcePackage PRIVATE psapi)
endif()
if(TARGET WPAudio)
    target_compile_definitions(WorkphoneResourcePackage PRIVATE WP_PACKAGE_AUDIO=1)
    target_link_libraries(WorkphoneResourcePackage PRIVATE $<TARGET_LINKER_FILE:WPAudio>)
    add_dependencies(WorkphoneResourcePackage WPAudio)
endif()
set_target_properties(WorkphoneResourcePackage PROPERTIES FOLDER Tools/Cpp)
if(TARGET Editor)
    add_dependencies(Editor WorkphoneResourcePackage)
endif()
if(BUILD_TESTING)
    find_program(WP_PACKAGE_LUA_VM NAMES luavm
        HINTS ${WP_ROOT}/bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/RelWithDebInfo)
    if(WP_PACKAGE_LUA_VM)
        add_test(NAME WorkphoneAssets.package_editor_workflow
            COMMAND ${WP_PACKAGE_LUA_VM} Tests/lua/PackageEditorTests.lua)
        set_tests_properties(WorkphoneAssets.package_editor_workflow PROPERTIES
            WORKING_DIRECTORY ${WP_ROOT} TIMEOUT 30)
    endif()
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        if(TARGET WPGraphicsCookedResourceTests)
            add_test(NAME WorkphoneAssets.typed_package_families
                COMMAND "${Python3_EXECUTABLE}" "${WP_ROOT}/Tests/python/resource_package_families.py"
                    "$<TARGET_FILE:WPGraphicsCookedResourceTests>" "$<TARGET_FILE:WorkphoneResourcePackage>")
            set_tests_properties(WorkphoneAssets.typed_package_families PROPERTIES TIMEOUT 240 RUN_SERIAL TRUE LABELS "resource;package;graphics")
        endif()
        if(WIN32)
        add_executable(ResourcePackageJobTests ${WP_ROOT}/Tests/cpp/ResourcePackageJobTests.cpp
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaManager.cpp
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaDebugger.cpp
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaObjectData.cpp
            ${WP_ROOT}/Engine/cpp/Source/WPLua/NullScriptObject.cpp
            ${WP_ROOT}/Engine/cpp/Source/WPLua/LuaScriptCompiler.cpp)
        target_compile_definitions(ResourcePackageJobTests PRIVATE WPLua_EXPORTS BOOST_ALL_NO_LIB
            BOOST_BIND_GLOBAL_PLACEHOLDERS WP_PROFILE_LUA_CALLS=0)
        target_include_directories(ResourcePackageJobTests PRIVATE
            ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
            ${WP_LUA_INCLUDE_DIR} ${WP_ROOT}/Dependencies/luabind/inc
            ${WP_ROOT}/Dependencies/luabind/inc/luabind)
        target_link_directories(ResourcePackageJobTests PRIVATE
            ${WP_ROOT}/Dependencies/lib/windows/boost
            ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
        target_link_libraries(ResourcePackageJobTests PRIVATE
            $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPSQLite>
            $<TARGET_LINKER_FILE:WPLuaBind> lua lua_cjson luabind Boost::json ws2_32)
        add_dependencies(ResourcePackageJobTests WorkphoneResourcePackage)
        set_target_properties(ResourcePackageJobTests PROPERTIES FOLDER Tests/Cpp)
        add_test(NAME WorkphoneAssets.package_jobs
            COMMAND ${Python3_EXECUTABLE} ${WP_ROOT}/Tests/python/resource_package_jobs.py
                    $<TARGET_FILE:ResourcePackageJobTests>)
        set_tests_properties(WorkphoneAssets.package_jobs PROPERTIES TIMEOUT 120)
        endif()
        add_test(NAME WorkphoneAssets.package_cli
            COMMAND ${Python3_EXECUTABLE} ${WP_ROOT}/Tests/python/resource_package_cli.py
                    $<TARGET_FILE:WorkphoneResourcePackage>)
        set_tests_properties(WorkphoneAssets.package_cli PROPERTIES TIMEOUT 120)
    endif()
endif()
