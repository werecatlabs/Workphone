set(WP_GRAPHICS_CONTRACT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_library(WPGraphicsProductionReference STATIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_skinning.c
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_particle_simulation.c)
target_include_directories(WPGraphicsProductionReference PUBLIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneCore
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneGraphics)
set_target_properties(WPGraphicsProductionReference PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
if(NOT MSVC)
    target_link_libraries(WPGraphicsProductionReference PUBLIC m)
    target_compile_options(WPGraphicsProductionReference PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
foreach(contract IN ITEMS Skinning ParticleSimulation)
    add_executable(WPGraphics${contract}Tests ${contract}Tests.c)
    target_link_libraries(WPGraphics${contract}Tests PRIVATE WPGraphicsProductionReference)
    set_target_properties(WPGraphics${contract}Tests PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    add_test(NAME WPGraphics.production_${contract} COMMAND WPGraphics${contract}Tests)
    set_tests_properties(WPGraphics.production_${contract} PROPERTIES LABELS "graphics;headless;production" TIMEOUT 30)
endforeach()
