add_executable(WorkphoneSceneWorld main.cpp)
target_include_directories(WorkphoneSceneWorld PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
target_link_libraries(WorkphoneSceneWorld PRIVATE Workphone)
set_target_properties(WorkphoneSceneWorld PROPERTIES FOLDER Tools/Cpp)
