# Compile the production C90 scene and jobs without a device; the test records
# matrices/draws only at the renderer boundary. Used by full and isolated builds.
if(NOT TARGET WorkphoneGraphicsSceneJobTests)
    set(WP_JOB_ROOT "${CMAKE_SOURCE_DIR}")
    add_executable(WorkphoneGraphicsSceneJobTests
        ${WP_JOB_ROOT}/Tests/c/WorkphoneGraphicsSceneJobTests.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneCore/workphone_threadpool.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneCore/workphone_math.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneCore/workphone_matrix.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneCore/workphone_vector.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneCore/workphone_quat.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_scenenode.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_object.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_camera.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_scene.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_mesh.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_cull_job.c
        ${WP_JOB_ROOT}/Engine/c/Source/WorkphoneGraphics/workphone_graphics_transform_job.c)
    target_include_directories(WorkphoneGraphicsSceneJobTests PRIVATE
        ${WP_JOB_ROOT}/Engine/c/Include/WorkphoneCore
        ${WP_JOB_ROOT}/Engine/c/Include/WorkphoneGraphics)
    target_compile_definitions(WorkphoneGraphicsSceneJobTests PRIVATE WP_GRAPHICS_MESH_SERIALIZATION_ONLY)
    set_target_properties(WorkphoneGraphicsSceneJobTests PROPERTIES
        C_STANDARD 90 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF FOLDER Tests/C)
    if(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(WorkphoneGraphicsSceneJobTests PRIVATE m Threads::Threads)
    endif()
    add_test(NAME WorkphoneGraphics.scene_jobs COMMAND WorkphoneGraphicsSceneJobTests)
    set_tests_properties(WorkphoneGraphics.scene_jobs PROPERTIES LABELS "graphics;headless;cull;transform" TIMEOUT 30)
endif()
