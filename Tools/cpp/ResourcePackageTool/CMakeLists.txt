add_executable(WorkphoneResourcePackage main.cpp)
target_include_directories(WorkphoneResourcePackage PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
target_link_libraries(WorkphoneResourcePackage PRIVATE
    $<TARGET_LINKER_FILE:Workphone> $<TARGET_LINKER_FILE:WPSQLite>
    $<TARGET_LINKER_FILE:WPGraphics> $<TARGET_LINKER_FILE:WPLuaBind>)
add_dependencies(WorkphoneResourcePackage Workphone WPSQLite WPGraphics WPLuaBind)
set_target_properties(WorkphoneResourcePackage PROPERTIES FOLDER Tools/Cpp)
