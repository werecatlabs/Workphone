set(WP_GRAPHICS_CONTRACT_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../..")
add_library(WPGraphicsProductionReference STATIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_skinning.c
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_particle_simulation.c
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_irradiance.c)
target_include_directories(WPGraphicsProductionReference PUBLIC
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneCore
    ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneGraphics)
set_target_properties(WPGraphicsProductionReference PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
if(NOT MSVC)
    target_link_libraries(WPGraphicsProductionReference PUBLIC m)
    target_compile_options(WPGraphicsProductionReference PRIVATE -Wall -Wextra -Werror -pedantic)
endif()
foreach(contract IN ITEMS Skinning ParticleSimulation Irradiance)
    add_executable(WPGraphics${contract}C89Tests ${contract}Tests.c)
    target_link_libraries(WPGraphics${contract}C89Tests PRIVATE WPGraphicsProductionReference)
    set_target_properties(WPGraphics${contract}C89Tests PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
    add_test(NAME WPGraphics.production_${contract} COMMAND WPGraphics${contract}C89Tests)
    set_tests_properties(WPGraphics.production_${contract} PROPERTIES LABELS "graphics;headless;production" TIMEOUT 30)
endforeach()

# Exercise the native geometry conversion without linking the C++ engine or a
# graphics device, including buffer aliasing and index-only cache invalidation.
if(NOT TARGET WorkphoneGraphicsMeshTests)
    add_executable(WorkphoneGraphicsMeshTests
        ${WP_GRAPHICS_CONTRACT_ROOT}/Tests/c/WorkphoneGraphicsMeshTests.c
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh.c
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_reader.c
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_ogre_reader.c
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_writer.c)
    target_include_directories(WorkphoneGraphicsMeshTests PRIVATE
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneGraphics
        ${WP_GRAPHICS_CONTRACT_ROOT}/Engine/c/Include/WorkphoneCore)
    target_compile_definitions(WorkphoneGraphicsMeshTests PRIVATE WP_GRAPHICS_MESH_SERIALIZATION_ONLY)
    set_target_properties(WorkphoneGraphicsMeshTests PROPERTIES
        C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
    if(UNIX)
        target_link_libraries(WorkphoneGraphicsMeshTests PRIVATE m)
    endif()
    add_test(NAME WorkphoneGraphics.mesh_serializer COMMAND WorkphoneGraphicsMeshTests)
    set_tests_properties(WorkphoneGraphics.mesh_serializer PROPERTIES LABELS "graphics;headless;mesh" TIMEOUT 30)
endif()

# Full engine builds already register this contract against the real scene
# implementation in Tests/c. Only use the stub scene boundary in isolation.
if(NOT TARGET WorkphoneGraphicsFrameTests)
    add_executable(WorkphoneGraphicsFrameTests
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
endif()

if(NOT TARGET WorkphoneGraphicsRTTJobTests)
    add_executable(WorkphoneGraphicsRTTJobTests
        ${CMAKE_SOURCE_DIR}/Tests/c/WorkphoneGraphicsRTTJobTests.c
        ${CMAKE_SOURCE_DIR}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_rtt_job.c)
    target_include_directories(WorkphoneGraphicsRTTJobTests PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine/c/Include/WorkphoneCore
        ${CMAKE_SOURCE_DIR}/Engine/c/Include/WorkphoneGraphics)
    set_target_properties(WorkphoneGraphicsRTTJobTests PROPERTIES
        C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
    if(NOT MSVC)
        target_compile_options(WorkphoneGraphicsRTTJobTests PRIVATE -Wall -Wextra -Werror -pedantic)
    endif()
    add_test(NAME WorkphoneGraphics.rtt_job COMMAND WorkphoneGraphicsRTTJobTests)
    set_tests_properties(WorkphoneGraphics.rtt_job PROPERTIES LABELS "graphics;headless;rtt" TIMEOUT 30)
endif()

include(${CMAKE_SOURCE_DIR}/Tests/c/SceneJobContracts.cmake)
