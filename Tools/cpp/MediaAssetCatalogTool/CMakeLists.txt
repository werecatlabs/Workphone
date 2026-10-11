add_executable(WorkphoneMediaCatalog main.cpp)
target_include_directories(WorkphoneMediaCatalog PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
target_link_libraries(WorkphoneMediaCatalog PRIVATE $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPSQLite>)
add_dependencies(WorkphoneMediaCatalog Workphone WPSQLite)
set_target_properties(WorkphoneMediaCatalog PROPERTIES FOLDER Tools)
