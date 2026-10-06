project(WorkphonePhysicsCollisionTests C)

add_executable(WorkphonePhysicsCollisionTests WorkphonePhysicsCollisionTests.c)
target_include_directories(
    WorkphonePhysicsCollisionTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphonePhysics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCollision
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_link_libraries(WorkphonePhysicsCollisionTests PRIVATE WorkphonePhysics)
set_target_properties(WorkphonePhysicsCollisionTests PROPERTIES FOLDER Tests/C)

add_test(NAME WorkphonePhysics.collision COMMAND WorkphonePhysicsCollisionTests)

add_executable(
    WorkphoneGraphicsMeshTests
    WorkphoneGraphicsMeshTests.c
    ${WP_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh.c
    ${WP_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_reader.c
    ${WP_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_ogre_reader.c
    ${WP_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh_writer.c)
target_include_directories(
    WorkphoneGraphicsMeshTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_compile_definitions(
    WorkphoneGraphicsMeshTests PRIVATE WP_GRAPHICS_MESH_SERIALIZATION_ONLY)
set_target_properties(WorkphoneGraphicsMeshTests PROPERTIES FOLDER Tests/C)

add_test(
    NAME WorkphoneGraphics.mesh_serializer
    COMMAND WorkphoneGraphicsMeshTests)
add_test(
    NAME WorkphoneGraphics.mesh_import_assets
    COMMAND WorkphoneGraphicsMeshTests
            ${WP_ROOT}/Bin/Media/Ogre/models/Barrel.mesh
            ${WP_ROOT}/Bin/Media/Ogre/models/athene.mesh
            ${WP_ROOT}/Bin/Media/Ogre/models/ninja.mesh
            ${WP_ROOT}/Bin/Media/Ogre/models/ogrehead.mesh
            ${WP_ROOT}/Bin/Media/Ogre/mygui/Common/Scene/Mikki.mesh
            ${WP_ROOT}/Bin/Media/OgreNext/models/char_feet.mesh)
set_tests_properties(WorkphoneGraphics.mesh_import_assets PROPERTIES SKIP_RETURN_CODE 77 LABELS "graphics;external-assets")

add_executable(WorkphonePhysicsMaterialTests WorkphonePhysicsMaterialTests.c)
target_include_directories(
    WorkphonePhysicsMaterialTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphonePhysics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCollision
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_link_libraries(WorkphonePhysicsMaterialTests PRIVATE WorkphonePhysics)
set_target_properties(WorkphonePhysicsMaterialTests PROPERTIES FOLDER Tests/C)

add_test(NAME WorkphonePhysics.material COMMAND WorkphonePhysicsMaterialTests)

add_executable(WorkphoneGraphicsRendererTests WorkphoneGraphicsRendererTests.c)
target_include_directories(
    WorkphoneGraphicsRendererTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_link_libraries(
    WorkphoneGraphicsRendererTests
    PRIVATE WorkphoneGraphics WorkphonePlatformWin32 WorkphoneCore)
set_target_properties(WorkphoneGraphicsRendererTests PROPERTIES FOLDER Tests/C)

add_test(NAME WorkphoneGraphics.renderer_contract COMMAND WorkphoneGraphicsRendererTests)

add_executable(WorkphoneGraphicsShaderTests WorkphoneGraphicsShaderTests.c)
target_include_directories(
    WorkphoneGraphicsShaderTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_link_libraries(
    WorkphoneGraphicsShaderTests
    PRIVATE WorkphoneGraphics WorkphonePlatformWin32 WorkphoneCore)
set_target_properties(WorkphoneGraphicsShaderTests PROPERTIES FOLDER Tests/C)
add_test(NAME WorkphoneGraphics.shader_contract COMMAND WorkphoneGraphicsShaderTests)

add_executable(WorkphoneGraphicsPipelineTests WorkphoneGraphicsPipelineTests.c)
target_include_directories(
    WorkphoneGraphicsPipelineTests
    PRIVATE ${WP_ROOT}/Engine/c/Include/WorkphoneGraphics
            ${WP_ROOT}/Engine/c/Include/WorkphoneCore)
target_link_libraries(WorkphoneGraphicsPipelineTests PRIVATE WorkphoneGraphics)
set_target_properties(WorkphoneGraphicsPipelineTests PROPERTIES FOLDER Tests/C)
add_test(NAME WorkphoneGraphics.aaa_pipeline COMMAND WorkphoneGraphicsPipelineTests)
