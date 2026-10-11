project(WPResourceTests)

add_executable(WPResourceTests ResourceSystemSmoke.cpp)
target_include_directories(WPResourceTests PRIVATE ${WP_ROOT}/Engine/cpp/Include)

# Link the produced libraries without inheriting the legacy engine targets' very
# large interface link sets. Workphone consumes only the database interface;
# this test selects the SQLite implementation explicitly.
target_link_libraries(WPResourceTests PRIVATE
    $<TARGET_LINKER_FILE:WPSQLite>
    $<TARGET_LINKER_FILE:Workphone>)
add_dependencies(WPResourceTests WPSQLite Workphone)

set_target_properties(WPResourceTests PROPERTIES FOLDER Tests/Cpp)
if (WIN32)
    set_property(TARGET WPResourceTests PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
        ${WP_ROOT}/Bin/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$(Configuration))
endif()

add_test(NAME WPResourceTests COMMAND WPResourceTests)

find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME WPResource.writer_lease
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/ResourceWriterLeaseContracts.py"
            "$<TARGET_FILE:WPResourceTests>")
    add_test(NAME WPResource.index_migration
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/ResourceCompilationMigrationContracts.py"
            "$<TARGET_FILE:WPResourceTests>")
endif()
