cmake_minimum_required(VERSION 3.16)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(CarVisualPortTests LANGUAGES CXX)
    enable_testing()
endif()

get_filename_component(CARVISUAL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
add_executable(CarVisualPortTests CarVisualTests.cpp CarVisualSources.cpp)
target_include_directories(CarVisualPortTests PRIVATE "${CARVISUAL_ROOT}/Engine/cpp/Include")
target_compile_features(CarVisualPortTests PRIVATE cxx_std_17)
target_compile_definitions(CarVisualPortTests PRIVATE _WP_STATIC_LIB_ CARVISUAL_STANDALONE=1 WP_USE_PRECOMPILED_HEADERS=0 WP_USE_CPP17=1)

if(MSVC)
    target_compile_options(CarVisualPortTests PRIVATE /Y- /Zc:__cplusplus)
endif()

add_test(NAME CarVisual.port COMMAND CarVisualPortTests)
set_tests_properties(CarVisual.port PROPERTIES LABELS "vehicle;visual;headless" TIMEOUT 30)

target_link_libraries(CarVisualPortTests PRIVATE Meshopt)
target_include_directories(CarVisualPortTests PRIVATE "${CARVISUAL_ROOT}/Dependencies/tinyusdz/src/external/jsonhpp")
set_target_properties(CarVisualPortTests PROPERTIES FOLDER Tests/cpp/CarVisual)