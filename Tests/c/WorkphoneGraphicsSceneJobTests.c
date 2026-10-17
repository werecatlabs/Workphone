/* Real C90 scene/job algorithms, with a recording renderer boundary. */
#include "workphone_graphics_scene.h"
#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_transform_job.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_renderer.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#ifdef _WIN32
#    include <windows.h>
#endif
static int failures, draws;
static wp_graphics_object *order[16];
static wp_mat4f submitted_world, submitted_view, submitted_projection;
#define CHECK( c )                                                 \
    do                                                             \
    {                                                              \
        if( !( c ) )                                               \
        {                                                          \
            fprintf( stderr, "FAIL line %d: %s\n", __LINE__, #c ); \
            ++failures;                                            \
        }                                                          \
    } while( 0 )

void wp_renderer_set_world_matrix( wp_renderer *r, const wp_mat4f *m )
{
    (void)r;
    submitted_world = *m;
}

void wp_renderer_set_view_matrix( wp_renderer *r, const wp_mat4f *m )
{
    (void)r;
    submitted_view = *m;
}

void wp_renderer_set_projection_matrix( wp_renderer *r, const wp_mat4f *m )
{
    (void)r;
    submitted_projection = *m;
}

void wp_graphics_mesh_render( wp_graphics_mesh *m, wp_renderer *r, const wp_graphics_material *material )
{
    (void)m;
    (void)r;
    (void)material;
    ++draws;
}

static void record( wp_graphics_object *o, wp_renderer *r )
{
    (void)r;
    if( draws < 16 )
    {
        order[draws] = o;
    }
    ++draws;
}

static int equal_matrix( const wp_mat4f *a, const wp_mat4f *b )
{
    int i;
    for( i = 0; i < 16; ++i )
    {
        if( fabs( ( (const wp_f32 *)a )[i] - ( (const wp_f32 *)b )[i] ) > 1.0e-5 )
        {
            return 0;
        }
    }
    return 1;
}

static wp_s32 consume( wp_graphics_object *o, wp_renderer *r, const wp_mat4f *w, void *data )
{
    (void)r;
    (void)data;
    submitted_world = *w;
    if( draws < 16 )
    {
        order[draws] = o;
    }
    ++draws;
    return 1;
}

static void matrix_cache( void )
{
    wp_scenenode *root = wp_scenenode_create(), *child = wp_scenenode_create(),
                 *sibling = wp_scenenode_create(), *other = wp_scenenode_create();
    wp_scenenode_transform_stats root_stats, child_stats, sibling_stats;
    wp_vec3f position = { 3, 4, 5 }, scale = { -2, 3, 4 };
    wp_quatf orientation = { 1.41421356f, 0, 0, 1.41421356f };
    wp_mat4f translation, rotation, scaling, combined, parent, child_local, expected, actual;
    wp_u64 revision;
    int i;
    wp_scenenode_add_child( root, child );
    wp_scenenode_add_child( root, sibling );
    wp_scenenode_set_position( root, position );
    wp_scenenode_set_orientation( root, orientation );
    wp_scenenode_set_scale( root, scale );
    wp_mat4f_from_translation( &translation, 3, 4, 5 );
    wp_mat4f_from_rotation_z( &rotation, 1.57079632679f );
    wp_mat4f_from_scale( &scaling, -2, 3, 4 );
    wp_mat4f_mul( &combined, &translation, &rotation );
    wp_mat4f_mul( &parent, &combined, &scaling );
    position.x = 1;
    position.y = 2;
    position.z = 3;
    wp_scenenode_set_position( child, position );
    wp_mat4f_from_translation( &child_local, 1, 2, 3 );
    wp_mat4f_mul( &expected, &parent, &child_local );
    wp_scenenode_get_world_matrix( child, &actual );
    CHECK( equal_matrix( &actual, &expected ) );
    wp_scenenode_get_world_matrix( sibling, &actual );
    for( i = 0; i < 100; ++i )
    {
        wp_scenenode_get_world_matrix( child, &actual );
        wp_scenenode_get_local_matrix( root, &actual );
    }
    wp_scenenode_get_transform_stats( root, &root_stats );
    wp_scenenode_get_transform_stats( child, &child_stats );
    CHECK( root_stats.local_builds == 1 && root_stats.world_builds == 1 &&
           child_stats.local_builds == 1 && child_stats.world_builds == 1 );
    revision = wp_scenenode_get_generation( root );
    position.x = 3;
    position.y = 4;
    position.z = 5;
    wp_scenenode_set_position( root, position );
    wp_scenenode_set_orientation( root, orientation );
    wp_scenenode_set_scale( root, scale );
    CHECK( wp_scenenode_get_generation( root ) == revision );
    position.x = 2;
    position.y = 2;
    position.z = 3;
    wp_scenenode_set_position( child, position );
    wp_scenenode_get_world_matrix( child, &actual );
    wp_mat4f_from_translation( &child_local, 2, 2, 3 );
    wp_mat4f_mul( &expected, &parent, &child_local );
    CHECK( equal_matrix( &actual, &expected ) );
    wp_scenenode_get_transform_stats( root, &root_stats );
    wp_scenenode_get_transform_stats( child, &child_stats );
    wp_scenenode_get_transform_stats( sibling, &sibling_stats );
    CHECK( root_stats.world_builds == 1 && child_stats.world_builds == 2 &&
           sibling_stats.world_builds == 1 );
    /* A grandparent edit invalidates descendant worlds without rebuilding their locals. */
    position.x = 6;
    position.y = 4;
    position.z = 5;
    wp_scenenode_set_position( root, position );
    wp_scenenode_get_world_matrix( child, &actual );
    expected.m[0][3] += 3;
    CHECK( equal_matrix( &actual, &expected ) );
    wp_scenenode_get_transform_stats( child, &child_stats );
    CHECK( child_stats.local_builds == 2 && child_stats.world_builds == 3 );
    wp_scenenode_get_world_matrix( sibling, &actual );
    wp_scenenode_get_transform_stats( sibling, &sibling_stats );
    CHECK( sibling_stats.local_builds == 1 && sibling_stats.world_builds == 2 );
    /* Reparenting must replace the cached parent transform. */
    position.x = 20;
    position.y = 0;
    position.z = 0;
    wp_scenenode_set_position( other, position );
    wp_scenenode_get_world_matrix( other, &actual );
    wp_scenenode_set_parent( child, other );
    wp_scenenode_get_world_matrix( child, &actual );
    CHECK( actual.m[0][3] == 22 && actual.m[1][3] == 2 && actual.m[2][3] == 3 );
    wp_scenenode_destroy( other );
    wp_scenenode_get_world_matrix( child, &actual );
    CHECK( equal_matrix( &actual, &child_local ) );
    wp_scenenode_get_transform_stats( child, &child_stats );
    CHECK( child_stats.local_builds == 2 );
    wp_scenenode_destroy( root );
    wp_scenenode_destroy( sibling );
    wp_scenenode_destroy( child );
}

static void shared_transform_capture( void )
{
    enum
    {
        population = 256,
        repeats = 4
    };

    wp_scenenode *root = wp_scenenode_create(), *children[population];
    const wp_scenenode *nodes[population * repeats];
    wp_mat4f worlds[population * repeats], expected;
    wp_vec3f position = { 3, 4, 5 };
    wp_transform_job_system *job = wp_transform_job_system_create( NULL, 17 );
    wp_transform_job_stats stats;
    wp_scenenode_transform_stats node_stats;
    int i, j;
    wp_scenenode_set_position( root, position );
    for( i = 0; i < population; ++i )
    {
        children[i] = wp_scenenode_create();
        wp_scenenode_add_child( root, children[i] );
        position.x = (wp_f32)i;
        position.y = 2;
        position.z = 1;
        wp_scenenode_set_position( children[i], position );
        for( j = 0; j < repeats; ++j )
        {
            nodes[i * repeats + j] = children[i];
        }
    }
    CHECK( wp_transform_job_system_process( job, nodes, worlds, population * repeats ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.output_count == population * repeats && stats.local_matrix_count == population + 1 &&
           stats.path_reference_count == population * repeats * 2 );
    CHECK( stats.capture_peak_bytes <= 64u * 1024u * 1024u &&
           stats.retained_snapshot_bytes < stats.capture_peak_bytes );
    for( i = 0; i < population; ++i )
    {
        wp_mat4f_from_translation( &expected, 3 + (wp_f32)i, 6, 6 );
        for( j = 0; j < repeats; ++j )
        {
            CHECK( equal_matrix( &worlds[i * repeats + j], &expected ) );
        }
    }
    CHECK( wp_transform_job_system_process( job, nodes, worlds, population * repeats ) );
    wp_scenenode_get_transform_stats( root, &node_stats );
    CHECK( node_stats.local_builds == 1 );
    for( i = 0; i < population; ++i )
    {
        wp_scenenode_get_transform_stats( children[i], &node_stats );
        CHECK( node_stats.local_builds == 1 );
    }
    position.x = 9;
    position.y = 4;
    position.z = 5;
    wp_scenenode_set_position( root, position );
    CHECK( wp_transform_job_system_dispatch( job, nodes, worlds, population * repeats ) );
    for( i = 0; i < population; ++i )
    {
        wp_scenenode_destroy( children[i] );
    }
    wp_scenenode_destroy( root );
    wp_transform_job_system_wait( job );
    for( i = 0; i < population; ++i )
    {
        wp_mat4f_from_translation( &expected, 9 + (wp_f32)i, 6, 6 );
        CHECK( equal_matrix( &worlds[i * repeats], &expected ) );
    }
    printf(
        "Transform reuse fixture: %u outputs, %u path references, %u unique locals, %lu retained "
        "bytes.\n",
        (unsigned)stats.output_count, (unsigned)stats.path_reference_count,
        (unsigned)stats.local_matrix_count, (unsigned long)stats.retained_snapshot_bytes );
    wp_transform_job_system_destroy( job );
}

static void cache_depth_boundary( void )
{
    wp_scenenode *nodes[258];
    wp_mat4f world;
    wp_vec3f position = { 1, 0, 0 };
    wp_camera *camera = wp_camera_create();
    wp_mat4f view;
    int i;
    for( i = 0; i < 258; ++i )
    {
        nodes[i] = wp_scenenode_create();
        wp_scenenode_set_position( nodes[i], position );
        if( i )
        {
            wp_scenenode_add_child( nodes[i - 1], nodes[i] );
        }
    }
    position.x = 7;
    wp_scenenode_set_position( nodes[0], position );
    wp_scenenode_get_world_matrix( nodes[256], &world );
    CHECK( world.m[0][3] == 263 );
    wp_scenenode_get_world_matrix( nodes[257], &world );
    CHECK( world.m[0][3] == 257 );
    wp_scenenode_get_world_matrix( nodes[256], &world );
    CHECK( world.m[0][3] == 263 );
    wp_camera_attach_to_node( camera, nodes[2] );
    wp_camera_get_view_matrix( camera, &view );
    CHECK( view.m[0][3] == -9 );
    position.x = 10;
    wp_scenenode_set_position( nodes[0], position );
    wp_scenenode_get_world_matrix( nodes[256], &world );
    CHECK( world.m[0][3] == 266 );
    wp_scenenode_get_world_matrix( nodes[257], &world );
    CHECK( world.m[0][3] == 257 );
    wp_camera_get_view_matrix( camera, &view );
    CHECK( view.m[0][3] == -12 );
    wp_scenenode_set_parent( nodes[257], nodes[0] );
    wp_scenenode_get_world_matrix( nodes[257], &world );
    CHECK( world.m[0][3] == 11 );
    wp_camera_attach_to_node( camera, NULL );
    wp_camera_destroy( camera );
    for( i = 257; i >= 0; --i )
    {
        wp_scenenode_destroy( nodes[i] );
    }
}

static void transforms( void )
{
    wp_threadpool *pool = wp_threadpool_create( 2 );
    wp_transform_job_system *job = wp_transform_job_system_create( pool, 1 );
    wp_scenenode *parent = wp_scenenode_create(), *child = wp_scenenode_create();
    const wp_scenenode *nodes[3];
    wp_mat4f expected, out[3], sentinel;
    wp_vec3f p = { 3, 4, 5 }, scale = { -2, 3, 4 };
    wp_quatf q = { 0, 0, 0.70710678f, 0.70710678f };
    wp_u32 count;
    wp_u64 generation;
    const wp_mat4f *result;
    CHECK( pool && job && parent && child );
    wp_scenenode_add_child( parent, child );
    wp_scenenode_set_position( parent, p );
    wp_scenenode_set_orientation( parent, q );
    wp_scenenode_set_scale( parent, scale );
    p.x = 1;
    p.y = 2;
    p.z = 3;
    wp_scenenode_set_position( child, p );
    wp_scenenode_get_world_matrix( child, &expected );
    memset( &sentinel, 0xAB, sizeof( sentinel ) );
    out[0] = out[1] = out[2] = sentinel;
    nodes[0] = child;
    nodes[1] = child;
    nodes[2] = NULL;
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 3 ) );
    CHECK( !memcmp( &out[0], &sentinel, sizeof( sentinel ) ) );
    CHECK( !wp_transform_job_system_dispatch( job, nodes, out, 3 ) );
    /* No worker reads the hierarchy after dispatch. */
    wp_scenenode_destroy( child );
    wp_scenenode_destroy( parent );
    wp_transform_job_system_wait( job );
    CHECK( equal_matrix( &out[0], &expected ) && equal_matrix( &out[1], &expected ) );
    wp_mat4f_identity( &expected );
    CHECK( equal_matrix( &out[2], &expected ) );
    result = wp_transform_job_system_get_result( job, &count, &generation );
    CHECK( result && count == 3 );
    CHECK( !wp_transform_job_system_dispatch( job, nodes, out, UINT_MAX ) );
    CHECK( wp_transform_job_system_get_status( job ) == WP_TRANSFORM_COMPLETE );
    nodes[0] = NULL;
    out[0] = sentinel;
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 1 ) );
    wp_transform_job_system_cancel( job );
    CHECK( !memcmp( out, &sentinel, sizeof( sentinel ) ) );
    CHECK( !wp_transform_job_system_get_result( job, NULL, NULL ) );
    CHECK( wp_transform_job_system_dispatch_snapshot( job, nodes, 1, 987 ) );
    wp_transform_job_system_wait( job );
    result = wp_transform_job_system_get_result( job, &count, &generation );
    CHECK( result && count == 1 && generation == 987 && equal_matrix( result, &expected ) );
    CHECK( wp_transform_job_system_dispatch_snapshot( job, NULL, 0, 123 ) );
    wp_transform_job_system_wait( job );
    CHECK( wp_transform_job_system_get_status( job ) == WP_TRANSFORM_COMPLETE );
    wp_transform_job_system_get_result( job, &count, &generation );
    CHECK( !count && generation == 123 );
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 1 ) );
    out[0] = sentinel;
    wp_transform_job_system_destroy( job );
    CHECK( !memcmp( out, &sentinel, sizeof( sentinel ) ) );
    wp_threadpool_destroy( pool );
}
#ifdef _WIN32
typedef struct blocker
{
    HANDLE started, release;
} blocker;

static void block_task( void *p )
{
    blocker *b = (blocker *)p;
    SetEvent( b->started );
    WaitForSingleObject( b->release, INFINITE );
}

static DWORD WINAPI unblock_later( LPVOID p )
{
    blocker *b = (blocker *)p;
    Sleep( 2000 );
    SetEvent( b->release );
    return 0;
}

static void own_completion( void )
{
    blocker b;
    HANDLE unblock;
    ULONGLONG start, elapsed;
    wp_threadpool *pool = wp_threadpool_create( 2 );
    wp_transform_job_system *job = wp_transform_job_system_create( pool, UINT_MAX );
    const wp_scenenode *nodes[1] = { NULL };
    wp_mat4f out;
    b.started = CreateEvent( NULL, TRUE, FALSE, NULL );
    b.release = CreateEvent( NULL, TRUE, FALSE, NULL );
    CHECK( wp_threadpool_submit( pool, block_task, &b ) );
    CHECK( WaitForSingleObject( b.started, 3000 ) == WAIT_OBJECT_0 );
    unblock = CreateThread( NULL, 0, unblock_later, &b, 0, NULL );
    start = GetTickCount64();
    CHECK( wp_transform_job_system_process( job, nodes, &out, 1 ) );
    elapsed = GetTickCount64() - start;
    CHECK( elapsed < 1500 ); /* Old whole-pool wait blocks until the unrelated release. */
    SetEvent( b.release );
    WaitForSingleObject( unblock, 3000 );
    CloseHandle( unblock );
    wp_transform_job_system_destroy( job );
    wp_threadpool_destroy( pool );
    CloseHandle( b.started );
    CloseHandle( b.release );
}
#endif
static void hierarchy_limit( void )
{
    wp_scenenode *nodes[258];
    const wp_scenenode *leaf[1];
    wp_mat4f out;
    wp_transform_job_system *job = wp_transform_job_system_create( NULL, 0 );
    int i;
    for( i = 0; i < 258; ++i )
    {
        nodes[i] = wp_scenenode_create();
        if( i )
        {
            wp_scenenode_add_child( nodes[i - 1], nodes[i] );
        }
    }
    leaf[0] = nodes[256];
    CHECK( wp_transform_job_system_process( job, leaf, &out, 1 ) );
    leaf[0] = nodes[257];
    CHECK( !wp_transform_job_system_process( job, leaf, &out, 1 ) );
    for( i = 257; i >= 0; --i )
    {
        wp_scenenode_destroy( nodes[i] );
    }
    wp_transform_job_system_destroy( job );
}

static wp_graphics_cull_result *capture( wp_graphics_scene *s, wp_camera *c, wp_s32 pass )
{
    wp_graphics_cull_result *r = wp_graphics_scene_prepare_cull( s, c, pass );
    CHECK( r != NULL );
    return r;
}

static void culling( void )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_object *a = wp_graphics_scene_create_object( scene ),
                       *b = wp_graphics_scene_create_object( scene ),
                       *c = wp_graphics_scene_create_object( scene ),
                       *outside = wp_graphics_scene_create_object( scene );
    wp_scenenode *parent = wp_graphics_scene_create_node( scene ),
                 *node = wp_graphics_scene_create_node( scene );
    wp_camera *camera = wp_camera_create(), *second = wp_camera_create();
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_mesh_vertex_p vertices[3] = { { { -0.2f, -0.2f, -2 } },
                                              { { 0.2f, -0.2f, -2 } },
                                              { { 0, 0.2f, -2 } } };
    wp_graphics_cull_result *main, *shadow, *other;
    wp_transform_job_stats prepared_stats;
    wp_scenenode_transform_stats node_stats;
    wp_graphics_cull_job *job = wp_graphics_cull_job_create();
    wp_mat4f world, frozen;
    wp_vec3f p = { 100, 0, 0 };
    wp_u64 revision;
    wp_renderer *renderer = (wp_renderer *)scene; /* recording boundary never dereferences */
    CHECK( scene && a && b && c && outside && camera && second && job && mesh );
    wp_graphics_object_set_render_func( a, record );
    wp_graphics_object_set_render_func( b, record );
    wp_graphics_object_set_render_func( c, record );
    wp_graphics_object_set_render_queue_group( a, 2 );
    wp_graphics_object_set_render_queue_group( b, 1 );
    wp_graphics_object_set_render_queue_group( c, 1 );
    wp_graphics_object_set_z_order( b, 5 );
    wp_graphics_object_set_z_order( c, 5 );
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 3 );
    wp_graphics_mesh_compute_aabb( mesh );
    wp_graphics_object_set_mesh( outside, mesh );
    wp_scenenode_add_child( parent, node );
    wp_scenenode_set_position( parent, p );
    wp_scenenode_attach_object( node, outside );
    wp_graphics_scene_set_active_camera( scene, camera );
    main = capture( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN );
    shadow = capture( scene, camera, WP_GRAPHICS_CULL_PASS_SHADOW );
    wp_graphics_cull_result_get_transform_stats( main, &prepared_stats );
    CHECK( prepared_stats.output_count == 4 && prepared_stats.local_matrix_count == 2 &&
           prepared_stats.path_reference_count == 2 );
    wp_scenenode_get_transform_stats( parent, &node_stats );
    CHECK( node_stats.local_builds == 1 );
    wp_scenenode_set_position( parent, wp_scenenode_get_position( parent ) );
    CHECK( wp_graphics_cull_result_validate( main, scene,
                                             wp_graphics_cull_result_get_scene_generation( main ) ) ==
           WP_GRAPHICS_CULL_OK );

    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 3 &&
           wp_graphics_cull_result_get_visible_count( shadow ) == 4 );
    CHECK( wp_graphics_cull_result_get_object( main, 0 ) == b &&
           wp_graphics_cull_result_get_object( main, 1 ) == c &&
           wp_graphics_cull_result_get_object( main, 2 ) == a );
    draws = 0;
    wp_graphics_scene_render_with_submit( scene, renderer, NULL, NULL );
    CHECK( draws == 3 && order[0] == b && order[1] == c && order[2] == a );
    draws = 0;
    CHECK( wp_graphics_scene_render_cull_result( scene, renderer, main, consume, NULL ) ==
           WP_GRAPHICS_CULL_OK );
    CHECK( draws == 3 && order[0] == b && order[1] == c && order[2] == a );
    wp_camera_set_position( second, p );
    other = capture( scene, second, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( wp_graphics_cull_result_get_visible_count( other ) == 4 );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 3 );
    wp_graphics_cull_result_destroy( other );
    wp_graphics_cull_result_get_world_matrix( shadow, 3, &frozen );
    wp_scenenode_get_world_matrix( node, &world );
    CHECK( equal_matrix( &frozen, &world ) );
    revision = wp_graphics_scene_get_generation( scene );
    p.x = 50;
    wp_scenenode_set_position( parent, p );
    CHECK( wp_graphics_scene_get_generation( scene ) == revision );
    CHECK( wp_graphics_cull_result_validate( main, scene, (wp_u32)revision ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    CHECK( wp_graphics_cull_result_validate( shadow, scene, (wp_u32)revision ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_get_world_matrix( shadow, 3, &world );
    CHECK( equal_matrix( &world, &frozen ) );
    draws = 0;
    CHECK( wp_graphics_scene_render_cull_result( scene, renderer, main, consume, NULL ) ==
               WP_GRAPHICS_CULL_ERROR_STALE &&
           draws == 0 );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_cull_result_destroy( shadow );
    main = capture( scene, camera, 0 );
    vertices[0].position.x = -0.3f;
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 3 );
    CHECK( wp_graphics_cull_result_validate( main, scene, (wp_u32)revision ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_object_set_visible( a, 0 );
    main = capture( scene, camera, 0 );
    wp_graphics_object_set_visible( a, 1 );
    CHECK( wp_graphics_cull_result_validate( main, scene, (wp_u32)revision ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_object_set_visibility_flags( b, 2 );
    wp_camera_set_visibility_mask( camera, 1 );
    main = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 2 );
    wp_graphics_cull_result_destroy( main );
    main = capture( scene, camera, 0 );
    wp_graphics_scene_destroy_object( scene, c );
    c = wp_graphics_scene_create_object( scene );
    wp_graphics_object_set_render_func( c, record );
    CHECK( wp_graphics_cull_result_validate( main, scene,
                                             wp_graphics_cull_result_get_scene_generation( main ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( main );
    CHECK( wp_graphics_cull_job_configure( job, scene, camera, WP_GRAPHICS_CULL_PASS_MAIN, 99 ) ==
           WP_GRAPHICS_CULL_OK );
    CHECK( wp_graphics_cull_job_configure( job, scene, camera, WP_GRAPHICS_CULL_PASS_MAIN, 99 ) ==
           WP_GRAPHICS_CULL_ERROR_INVALID_STATE );
    wp_graphics_cull_job_cancel( job );
    CHECK( wp_graphics_cull_job_execute( job ) == WP_GRAPHICS_CULL_ERROR_INVALID_STATE );
    wp_graphics_cull_job_reset( job );
    CHECK( wp_graphics_cull_job_configure( job, scene, camera, WP_GRAPHICS_CULL_PASS_MAIN, 99 ) ==
           WP_GRAPHICS_CULL_OK );
    /* execute does not read any live object, even after scene clear */
    wp_graphics_scene_clear( scene );
    CHECK( wp_graphics_cull_job_execute( job ) == WP_GRAPHICS_CULL_OK );
    main = wp_graphics_cull_job_take_result( job );
    CHECK( wp_graphics_cull_result_validate( main, scene, 99 ) == WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_job_destroy( job );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_scene_destroy( scene );
    wp_camera_destroy( camera );
    wp_camera_destroy( second );
    wp_graphics_mesh_destroy( mesh );
}

static void conservative_bounds( void )
{
    wp_graphics_scene *s = wp_graphics_scene_create();
    wp_camera *camera = wp_camera_create();
    wp_graphics_object *o = wp_graphics_scene_create_object( s );
    wp_scenenode *n = wp_graphics_scene_create_node( s );
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_cull_result *r;
    wp_aabb3f bounds = { { -1, -1, -3 }, { 1, 1, -2 } };
    wp_vec3f p = { 100, 0, 0 };
    wp_mat4f view, projection;
    wp_graphics_object_set_mesh( o, mesh );
    wp_scenenode_attach_object( n, o );
    wp_scenenode_set_position( n, p );
    wp_graphics_scene_set_active_camera( s, camera );
    wp_graphics_mesh_set_local_aabb( mesh, bounds );
    r = capture( s, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 0 );
    wp_graphics_cull_result_destroy( r );
    bounds.min.x = 2;
    bounds.max.x = -2;
    wp_graphics_mesh_set_local_aabb( mesh, bounds );
    r = capture( s, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 1 );
    wp_graphics_cull_result_get_camera_matrices( r, &view, &projection );
    p.x = 1;
    wp_camera_set_position( camera, p );
    CHECK( wp_graphics_cull_result_validate( r, s, wp_graphics_cull_result_get_scene_generation( r ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( r );
    r = capture( s, camera, 0 );
    bounds.min.x = -1;
    bounds.max.x = 1;
    wp_graphics_mesh_set_local_aabb( mesh, bounds );
    CHECK( wp_graphics_cull_result_validate( r, s, wp_graphics_cull_result_get_scene_generation( r ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( r );
    r = capture( s, camera, 1 );
    wp_graphics_object_set_cast_shadows( o, 0 );
    CHECK( wp_graphics_cull_result_validate( r, s, wp_graphics_cull_result_get_scene_generation( r ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( r );
    r = capture( s, camera, 1 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 0 );
    wp_graphics_cull_result_destroy( r );
    wp_graphics_scene_destroy( s );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
}

static wp_s32 clear_during_submit( wp_graphics_object *o, wp_renderer *r, const wp_mat4f *w, void *data )
{
    (void)o;
    (void)r;
    (void)w;
    wp_graphics_scene_clear( (wp_graphics_scene *)data );
    return 0;
}

static void retirement( void )
{
    wp_graphics_scene *s = wp_graphics_scene_create();
    wp_graphics_object *o = wp_graphics_scene_create_object( s );
    wp_graphics_cull_result *r;
    wp_graphics_object_set_render_func( o, record );
    r = capture( s, NULL, 0 );
    draws = 0;
    CHECK( wp_graphics_scene_render_cull_result( s, (wp_renderer *)s, r, clear_during_submit, s ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    CHECK( !draws );
    wp_graphics_cull_result_destroy( r );
    wp_graphics_scene_destroy( s );
}

int main( void )
{
    matrix_cache();
    cache_depth_boundary();
    shared_transform_capture();
    transforms();
    hierarchy_limit();
    culling();
    conservative_bounds();
    retirement();
#ifdef _WIN32
    own_completion();
#endif
    printf( "Native transform/cull contracts: %s\n", failures ? "FAIL" : "PASS" );
    return failures ? 1 : 0;
}
