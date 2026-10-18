add_executable(WorkphoneResourcePackage main.cpp PackageProfile.cpp)
target_include_directories(WorkphoneResourcePackage PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost
    ${WP_ROOT}/Dependencies/rapidjson/include)
target_link_libraries(WorkphoneResourcePackage PRIVATE
    $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPSQLite>
    $<TARGET_LINKER_FILE:WPGraphics> $<TARGET_LINKER_FILE:WPLua>)
add_dependencies(WorkphoneResourcePackage Workphone WPSQLite WPGraphics WPLua)
set_target_properties(WorkphoneResourcePackage PROPERTIES FOLDER Tools/Cpp)
if(BUILD_TESTING)
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME WorkphoneAssets.package_cli
            COMMAND ${Python3_EXECUTABLE} ${WP_ROOT}/Tests/python/resource_package_cli.py
                    $<TARGET_FILE:WorkphoneResourcePackage>)
        set_tests_properties(WorkphoneAssets.package_cli PROPERTIES TIMEOUT 120)
    endif()
endif()
