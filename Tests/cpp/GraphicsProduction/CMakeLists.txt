set(WP_GRAPHICS_CONTRACT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_library(WPGraphicsProductionReference STATIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_skinning.c
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_particle_simulation.c)
target_include_directories(WPGraphicsProductionReference PUBLIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneCore
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneGraphics)
set_target_properties(WPGraphicsProductionReference PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
if(NOT MSVC)
    target_link_libraries(WPGraphicsProductionReference PUBLIC m)
    target_compile_options(WPGraphicsProductionReference PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
foreach(contract IN ITEMS Skinning ParticleSimulation)
    add_executable(WPGraphics${contract}C89Tests ${contract}Tests.c)
    target_link_libraries(WPGraphics${contract}C89Tests PRIVATE WPGraphicsProductionReference)
    set_target_properties(WPGraphics${contract}C89Tests PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
    add_test(NAME WPGraphics.production_${contract} COMMAND WPGraphics${contract}C89Tests)
    set_tests_properties(WPGraphics.production_${contract} PROPERTIES LABELS "graphics;headless;production" TIMEOUT 30)
endforeach()

add_executable(WorkphoneGraphicsFrameCppTests
    ${WP_GRAPHICS_CONTRACT_ROOT}/Tests/c/WorkphoneGraphicsFrameTests.c
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_system.c)
target_include_directories(WorkphoneGraphicsFrameTests PRIVATE
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneGraphics
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneCore)
target_compile_definitions(WorkphoneGraphicsFrameTests PRIVATE WP_FRAME_CONTRACT_STUB_SCENES)
set_target_properties(WorkphoneGraphicsFrameTests PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
if(NOT MSVC)
    target_compile_options(WorkphoneGraphicsFrameTests PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
add_test(NAME WorkphoneGraphics.native_frame COMMAND WorkphoneGraphicsFrameTests)
set_tests_properties(WorkphoneGraphics.native_frame PROPERTIES LABELS "graphics;headless;frame" TIMEOUT 30)
