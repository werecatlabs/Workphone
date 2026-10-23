add_executable(WorkphoneSceneWorld main.cpp)
target_include_directories(WorkphoneSceneWorld PRIVATE
    ${WP_ROOT}/Engine/cpp/Include ${WP_ROOT}/Dependencies/boost)
target_compile_definitions(WorkphoneSceneWorld PRIVATE BOOST_ALL_NO_LIB)
if(WIN32)
    target_link_directories(WorkphoneSceneWorld PRIVATE
        ${WP_ROOT}/libs/windows/${WP_TOOLSET}/${WP_ARCH}/${WP_STATIC_LINK_CRT_TYPE}/$<CONFIG>)
endif()
target_link_libraries(WorkphoneSceneWorld PRIVATE Workphone)
set_target_properties(WorkphoneSceneWorld PROPERTIES FOLDER Tools/Cpp)
