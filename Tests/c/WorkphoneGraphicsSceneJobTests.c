/* Real C90 scene/job algorithms, with a recording renderer boundary. */
#include "workphone_graphics_scene.h"
#include "workphone_graphics_cull_job.h"
#include "workphone_graphics_transform_job.h"
#include "workphone_graphics_object.h"
#include "workphone_graphics_mesh.h"
#include "workphone_graphics_camera.h"
#include "workphone_graphics_renderer.h"
/* Full-axis comparison for the private SAT compaction optimization. */
#include "../../Engine/c/Source/WorkphoneGraphics/graphics_cull_bounds.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdlib.h>
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
    CHECK( stats.batch_count == 61 && stats.matrix_multiply_count == 362 );
    CHECK( stats.capture_scratch_allocations == 1 && !stats.capture_scratch_reuses &&
           stats.capture_scratch_bytes > 0 );
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
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_scratch_allocations && stats.capture_scratch_reuses == 1 );
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
        "bytes, %u matrix multiplies versus %u unshared; reference scratch %lu bytes, allocs=%u "
        "reuses=%u.\n",
        (unsigned)stats.output_count, (unsigned)stats.path_reference_count,
        (unsigned)stats.local_matrix_count, (unsigned long)stats.retained_snapshot_bytes,
        (unsigned)stats.matrix_multiply_count, (unsigned)stats.path_reference_count,
        (unsigned long)stats.capture_scratch_bytes, (unsigned)stats.capture_scratch_allocations,
        (unsigned)stats.capture_scratch_reuses );
    wp_transform_job_system_destroy( job );
}

/* Independent root-to-leaf reference, without the job's deduplication or cache. */
static void reference_world( const wp_scenenode *node, wp_mat4f *out )
{
    wp_mat4f locals[257], combined;
    int depth = 0;
    while( node && depth < 257 )
    {
        wp_scenenode_get_local_matrix( node, &locals[depth++] );
        node = wp_scenenode_get_parent( node );
    }
    wp_mat4f_identity( out );
    while( depth )
    {
        wp_mat4f_mul( &combined, out, &locals[--depth] );
        *out = combined;
    }
}

static void shared_prefixes( void )
{
    enum
    {
        population = 48,
        outputs = 193
    };

    wp_scenenode *hierarchy[population];
    const wp_scenenode *nodes[outputs], *seen[population], *node;
    wp_mat4f expected[outputs], actual[outputs];
    wp_vec3f position, scale;
    wp_quatf orientation;
    wp_threadpool *pool = wp_threadpool_create( 3 );
    wp_transform_job_system *job;
    wp_transform_job_stats stats;
    wp_u32 batch_sizes[4] = { 1, 7, 64, 1000 };
    wp_u32 expected_multiplies, unshared, unique, begin, end, output, j;
    int i, mode, size, found;
    CHECK( pool != NULL );
    for( i = 0; i < population; ++i )
    {
        hierarchy[i] = wp_scenenode_create();
        /* Two roots, long shared prefixes, and several diverging branches. */
        if( i > 1 )
        {
            wp_scenenode_add_child( hierarchy[( i - 2 ) / 2], hierarchy[i] );
        }
        position.x = (wp_f32)( i % 7 ) * 0.13f;
        position.y = -0.21f;
        position.z = 0.37f;
        scale.x = ( i & 1 ) ? -0.9f : 1.1f;
        scale.y = 1.2f;
        scale.z = ( i % 11 ) ? 0.8f : 0;
        orientation.w = (wp_f32)cos( i * 0.031 );
        orientation.x = 0;
        orientation.y = (wp_f32)sin( i * 0.031 );
        orientation.z = 0;
        wp_scenenode_set_position( hierarchy[i], position );
        wp_scenenode_set_scale( hierarchy[i], scale );
        wp_scenenode_set_orientation( hierarchy[i], orientation );
    }
    for( i = 0; i < outputs; ++i )
    {
        nodes[i] = ( i % 17 ) ? hierarchy[( i * 11 ) % population] : NULL;
        reference_world( nodes[i], &expected[i] );
    }
    for( mode = 0; mode < 2; ++mode )
    {
        for( size = 0; size < 4; ++size )
        {
            expected_multiplies = 0;
            unshared = 0;
            for( begin = 0; begin < outputs; begin += batch_sizes[size] )
            {
                unique = 0;
                end = begin + batch_sizes[size];
                if( end > outputs )
                {
                    end = outputs;
                }
                for( output = begin; output < end; ++output )
                {
                    node = nodes[output];
                    while( node )
                    {
                        ++unshared;
                        found = 0;
                        for( j = 0; j < unique; ++j )
                        {
                            if( seen[j] == node )
                            {
                                found = 1;
                            }
                        }
                        if( !found )
                        {
                            seen[unique++] = node;
                        }
                        node = wp_scenenode_get_parent( node );
                    }
                }
                expected_multiplies += unique;
            }
            job = wp_transform_job_system_create( mode ? pool : NULL, batch_sizes[size] );
            CHECK( wp_transform_job_system_process( job, nodes, actual, outputs ) );
            wp_transform_job_system_get_stats( job, &stats );
            CHECK( stats.matrix_multiply_count == expected_multiplies );
            CHECK( stats.path_reference_count == unshared );
            CHECK( stats.batch_count == ( outputs + batch_sizes[size] - 1 ) / batch_sizes[size] );
            CHECK( stats.capture_peak_bytes <= 64u * 1024u * 1024u );
            for( i = 0; i < outputs; ++i )
            {
                CHECK( !memcmp( &expected[i], &actual[i], sizeof( actual[i] ) ) );
            }
            if( size == 3 )
            {
                CHECK( stats.matrix_multiply_count == population );
            }
            wp_transform_job_system_destroy( job );
        }
    }
    for( i = population - 1; i >= 0; --i )
    {
        wp_scenenode_destroy( hierarchy[i] );
    }
    wp_threadpool_destroy( pool );
    puts(
        "Shared prefixes: bit-identical reference matrices across four batch sizes, inline and three "
        "workers." );
}

static void retained_capture_scratch( void )
{
    wp_scenenode *root = wp_scenenode_create(), *child = wp_scenenode_create();
    const wp_scenenode *nodes[4];
    wp_threadpool *pool = wp_threadpool_create( 2 );
    wp_transform_job_system *job = wp_transform_job_system_create( pool, 2 );
    wp_transform_job_stats stats, before;
    wp_mat4f out[4], expected, sentinel;
    const wp_mat4f *saved;
    wp_u64 full_bytes;
    wp_vec3f position = { 3, 4, 5 };
    int i;
    wp_scenenode_add_child( root, child );
    wp_scenenode_set_position( root, position );
    reference_world( child, &expected );
    for( i = 0; i < 4; ++i )
    {
        nodes[i] = child;
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    full_bytes = stats.capture_scratch_bytes;
    CHECK( stats.capture_scratch_allocations == 1 && !stats.capture_scratch_reuses && full_bytes > 0 );
    CHECK( stats.retained_snapshot_bytes + full_bytes == stats.capture_peak_bytes );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_scratch_allocations && stats.capture_scratch_reuses == 1 );
    saved = wp_transform_job_system_get_result( job, NULL, NULL );
    before = stats;
    CHECK( !wp_transform_job_system_dispatch_snapshot( job, nodes, UINT_MAX, 7 ) );
    CHECK( wp_transform_job_system_get_result( job, NULL, NULL ) == saved );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_scratch_allocations == before.capture_scratch_allocations &&
           stats.capture_scratch_reuses == before.capture_scratch_reuses &&
           stats.capture_scratch_bytes == before.capture_scratch_bytes &&
           stats.output_count == before.output_count &&
           stats.capture_peak_bytes == before.capture_peak_bytes &&
           stats.matrix_multiply_count == before.matrix_multiply_count );
    CHECK( wp_transform_job_system_process( job, nodes, out, 1 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_scratch_allocations == 1 && stats.capture_scratch_bytes < full_bytes );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_scratch_allocations == 1 && stats.capture_scratch_bytes == full_bytes );
    memset( &sentinel, 0xAB, sizeof( sentinel ) );
    for( i = 0; i < 4; ++i )
    {
        out[i] = sentinel;
    }
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 4 ) );
    wp_transform_job_system_cancel( job );
    for( i = 0; i < 4; ++i )
    {
        CHECK( !memcmp( &out[i], &sentinel, sizeof( sentinel ) ) );
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_scratch_reuses == 1 && !stats.capture_scratch_allocations );
    for( i = 0; i < 4; ++i )
    {
        CHECK( !memcmp( &out[i], &expected, sizeof( expected ) ) );
    }
    CHECK( wp_transform_job_system_dispatch_snapshot( job, NULL, 0, 9 ) );
    wp_transform_job_system_wait( job );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_scratch_allocations && !stats.capture_scratch_reuses &&
           !stats.capture_scratch_bytes );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_scratch_allocations == 1 && !stats.capture_scratch_reuses );
    wp_transform_job_system_destroy( job );
    wp_threadpool_destroy( pool );
    wp_scenenode_destroy( child );
    wp_scenenode_destroy( root );
    puts(
        "Transform capture scratch: cold allocation, warm reuse, exact-size resize, failed dispatch, "
        "cancellation and empty release passed." );
}

static void retained_capture_paths( void )
{
    wp_scenenode *root = wp_scenenode_create(), *child = wp_scenenode_create(), *deep[258];
    const wp_scenenode *nodes[4];
    wp_threadpool *pool = wp_threadpool_create( 2 );
    wp_transform_job_system *job = wp_transform_job_system_create( pool, 2 );
    wp_transform_job_stats stats;
    wp_mat4f out[4], saved[4], expected, sentinel;
    const wp_mat4f *published;
    wp_vec3f p = { 3, 4, 5 };
    int i;
    wp_scenenode_add_child( root, child );
    wp_scenenode_set_position( root, p );
    for( i = 0; i < 258; ++i )
    {
        deep[i] = wp_scenenode_create();
        if( i )
        {
            wp_scenenode_add_child( deep[i - 1], deep[i] );
        }
    }
    for( i = 0; i < 4; ++i )
    {
        nodes[i] = child;
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_allocations == 1 && !stats.capture_path_reuses );
    published = wp_transform_job_system_get_result( job, NULL, NULL );
    memcpy( saved, published, sizeof( saved ) );
    /* Reuse writes two valid paths, then rejects a third path that is too deep.
     * The old public matrix array must remain readable and bit-identical. */
    nodes[0] = root;
    nodes[1] = NULL;
    nodes[2] = deep[257];
    CHECK( !wp_transform_job_system_dispatch_snapshot( job, nodes, 4, 77 ) );
    CHECK( wp_transform_job_system_get_result( job, NULL, NULL ) == published );
    CHECK( !memcmp( saved, published, sizeof( saved ) ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_allocations == 1 && !stats.capture_path_reuses );
    nodes[0] = child;
    nodes[1] = root;
    nodes[2] = NULL;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_path_allocations && stats.capture_path_reuses == 1 &&
           stats.capture_scratch_allocations == 1 );
    for( i = 0; i < 4; ++i )
    {
        reference_world( nodes[i], &expected );
        CHECK( !memcmp( &expected, &out[i], sizeof( expected ) ) );
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_reuses == 1 && stats.capture_scratch_reuses == 1 &&
           !stats.capture_path_allocations && !stats.capture_scratch_allocations );
    CHECK( wp_transform_job_system_process( job, nodes, out, 1 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_allocations == 1 && !stats.capture_path_reuses );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_allocations == 1 && !stats.capture_path_reuses );
    memset( &sentinel, 0xAB, sizeof( sentinel ) );
    for( i = 0; i < 4; ++i )
    {
        out[i] = sentinel;
    }
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 4 ) );
    wp_transform_job_system_cancel( job );
    for( i = 0; i < 4; ++i )
    {
        CHECK( !memcmp( &out[i], &sentinel, sizeof( sentinel ) ) );
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_reuses == 1 && !stats.capture_path_allocations );
    CHECK( wp_transform_job_system_dispatch_snapshot( job, NULL, 0, 78 ) );
    wp_transform_job_system_wait( job );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_path_allocations && !stats.capture_path_reuses );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_path_allocations == 1 && !stats.capture_path_reuses );
    wp_transform_job_system_destroy( job );
    wp_threadpool_destroy( pool );
    for( i = 257; i >= 0; --i )
    {
        wp_scenenode_destroy( deep[i] );
    }
    wp_scenenode_destroy( child );
    wp_scenenode_destroy( root );
    puts(
        "Transform paths: warm reuse, changed hierarchy, partial failed capture preserves old matrices, "
        "resize/cancel/empty release passed." );
}

static void retained_capture_indices_and_batches( void )
{
    wp_scenenode *root = wp_scenenode_create(), *child = wp_scenenode_create();
    const wp_scenenode *nodes[4];
    wp_threadpool *pool = wp_threadpool_create( 2 );
    wp_transform_job_system *job = wp_transform_job_system_create( pool, 2 );
    wp_transform_job_stats stats;
    wp_mat4f out[4], expected, sentinel;
    const wp_mat4f *published;
    wp_vec3f p = { 7, 2, -3 };
    int i;
    wp_scenenode_add_child( root, child );
    wp_scenenode_set_position( child, p );
    nodes[0] = child;
    nodes[1] = root;
    nodes[2] = NULL;
    nodes[3] = root;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_allocations == 1 && !stats.capture_index_reuses &&
           stats.capture_batch_allocations == 1 && !stats.capture_batch_reuses );
    CHECK( stats.matrix_multiply_count == 3 );
    /* Same four references and two batches, different output count and mapping.
     * Reused indices must switch from prefix slots to uncached local indices. */
    nodes[0] = child;
    nodes[1] = NULL;
    nodes[2] = child;
    CHECK( wp_transform_job_system_process( job, nodes, out, 3 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_reuses == 1 && !stats.capture_index_allocations &&
           stats.capture_batch_reuses == 1 && !stats.capture_batch_allocations );
    CHECK( stats.matrix_multiply_count == 4 );
    for( i = 0; i < 3; ++i )
    {
        reference_world( nodes[i], &expected );
        CHECK( !memcmp( &out[i], &expected, sizeof( expected ) ) );
    }
    published = wp_transform_job_system_get_result( job, NULL, NULL );
    expected = published[1];
    CHECK( !wp_transform_job_system_dispatch_snapshot( job, nodes, UINT_MAX, 99 ) );
    CHECK( wp_transform_job_system_get_result( job, NULL, NULL ) == published &&
           !memcmp( &published[1], &expected, sizeof( expected ) ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_reuses == 1 && stats.capture_batch_reuses == 1 );
    memset( &sentinel, 0xAB, sizeof( sentinel ) );
    for( i = 0; i < 3; ++i )
    {
        out[i] = sentinel;
    }
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 3 ) );
    wp_transform_job_system_cancel( job );
    for( i = 0; i < 3; ++i )
    {
        CHECK( !memcmp( &out[i], &sentinel, sizeof( sentinel ) ) );
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 3 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_reuses == 1 && stats.capture_batch_reuses == 1 );
    /* Reference count changes independently of batch count; then both shrink. */
    nodes[1] = root;
    CHECK( wp_transform_job_system_process( job, nodes, out, 3 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_allocations == 1 && !stats.capture_index_reuses &&
           stats.capture_batch_reuses == 1 );
    CHECK( wp_transform_job_system_process( job, nodes, out, 1 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_allocations == 1 && stats.capture_batch_allocations == 1 );
    nodes[0] = NULL;
    CHECK( wp_transform_job_system_process( job, nodes, out, 1 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_index_allocations && !stats.capture_index_reuses &&
           stats.capture_batch_reuses == 1 );
    wp_mat4f_identity( &expected );
    CHECK( !memcmp( &out[0], &expected, sizeof( expected ) ) );
    CHECK( wp_transform_job_system_dispatch_snapshot( job, NULL, 0, 100 ) );
    wp_transform_job_system_wait( job );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.capture_index_allocations && !stats.capture_index_reuses &&
           !stats.capture_batch_allocations && !stats.capture_batch_reuses );
    nodes[0] = child;
    CHECK( wp_transform_job_system_process( job, nodes, out, 1 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_index_allocations == 1 && stats.capture_batch_allocations == 1 );
    wp_transform_job_system_destroy( job );
    wp_threadpool_destroy( pool );
    wp_scenenode_destroy( child );
    wp_scenenode_destroy( root );
    puts(
        "Transform indices/batches: exact-size reuse, changed mappings, rejection, cancel, "
        "independent resize and empty release passed." );
}

static void retained_capture_matrices( wp_u32 workers )
{
    wp_scenenode *root_a = wp_scenenode_create(), *root_b = wp_scenenode_create();
    wp_scenenode *leaf_a = wp_scenenode_create(), *leaf_b = wp_scenenode_create();
    const wp_scenenode *nodes[4];
    wp_threadpool *pool = workers ? wp_threadpool_create( workers ) : NULL;
    wp_transform_job_system *job = wp_transform_job_system_create( pool, 2 );
    wp_transform_job_stats stats;
    wp_mat4f out[4], expected, sentinel;
    const wp_mat4f *published;
    wp_vec3f p = { 7, -2, 3 }, scale = { -2, 3, 0.5f };
    int i, step;
    wp_scenenode_add_child( root_a, leaf_a );
    wp_scenenode_add_child( root_b, leaf_b );
    wp_scenenode_set_position( root_b, p );
    wp_scenenode_set_scale( root_b, scale );
    for( i = 0; i < 4; ++i )
    {
        nodes[i] = leaf_a;
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_allocations == 1 && !stats.capture_local_reuses &&
           stats.capture_prefix_allocations == 1 && !stats.capture_prefix_reuses &&
           stats.matrix_multiply_count == 4 );
    for( step = 0; step < 3; ++step )
    {
        /* Equal counts with entirely different source identities, then reparent,
         * then a local edit. Ready prefixes must never reuse previous worlds. */
        if( step == 1 )
        {
            wp_scenenode_set_parent( leaf_b, root_a );
        }
        p.x += 2;
        wp_scenenode_set_position( leaf_b, p );
        for( i = 0; i < 4; ++i )
        {
            nodes[i] = leaf_b;
        }
        CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
        wp_transform_job_system_get_stats( job, &stats );
        CHECK( stats.capture_local_reuses == 1 && !stats.capture_local_allocations &&
               stats.capture_prefix_reuses == 1 && !stats.capture_prefix_allocations );
        reference_world( leaf_b, &expected );
        for( i = 0; i < 4; ++i )
        {
            CHECK( !memcmp( &expected, &out[i], sizeof( expected ) ) );
        }
    }
    published = wp_transform_job_system_get_result( job, NULL, NULL );
    CHECK( !wp_transform_job_system_dispatch_snapshot( job, nodes, UINT_MAX, 101 ) );
    CHECK( wp_transform_job_system_get_result( job, NULL, NULL ) == published &&
           !memcmp( &expected, &published[0], sizeof( expected ) ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_reuses == 1 && stats.capture_prefix_reuses == 1 );
    memset( &sentinel, 0xAB, sizeof( sentinel ) );
    for( i = 0; i < 4; ++i )
    {
        out[i] = sentinel;
    }
    CHECK( wp_transform_job_system_dispatch( job, nodes, out, 4 ) );
    wp_transform_job_system_cancel( job );
    for( i = 0; i < 4; ++i )
    {
        CHECK( !memcmp( &out[i], &sentinel, sizeof( sentinel ) ) );
    }
    p.y = 9;
    wp_scenenode_set_position( root_a, p );
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_reuses == 1 && stats.capture_prefix_reuses == 1 );
    reference_world( leaf_b, &expected );
    CHECK( !memcmp( &out[3], &expected, sizeof( expected ) ) );
    /* Two leaves sharing a root require three locals and six cached prefixes. */
    nodes[0] = leaf_a;
    nodes[2] = leaf_a;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.local_matrix_count == 3 && stats.matrix_multiply_count == 6 &&
           stats.capture_local_allocations == 1 && !stats.capture_local_reuses &&
           stats.capture_prefix_allocations == 1 && !stats.capture_prefix_reuses );
    /* Same locals, no repeated nodes within a batch: drop the optional cache. */
    nodes[1] = NULL;
    nodes[3] = NULL;
    nodes[2] = leaf_b;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_reuses == 1 && !stats.capture_prefix_allocations &&
           !stats.capture_prefix_reuses && stats.matrix_multiply_count == 4 );
    for( i = 0; i < 4; ++i )
    {
        reference_world( nodes[i], &expected );
        CHECK( !memcmp( &out[i], &expected, sizeof( expected ) ) );
    }
    nodes[0] = leaf_a;
    nodes[1] = leaf_b;
    nodes[2] = leaf_a;
    nodes[3] = leaf_b;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_reuses == 1 && stats.capture_prefix_allocations == 1 &&
           !stats.capture_prefix_reuses );
    for( i = 0; i < 4; ++i )
    {
        reference_world( nodes[i], &expected );
        CHECK( !memcmp( &out[i], &expected, sizeof( expected ) ) );
        nodes[i] = NULL;
    }
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( !stats.local_matrix_count && !stats.capture_local_allocations &&
           !stats.capture_local_reuses && !stats.capture_prefix_allocations &&
           !stats.capture_prefix_reuses );
    CHECK( wp_transform_job_system_dispatch_snapshot( job, NULL, 0, 102 ) );
    wp_transform_job_system_wait( job );
    nodes[0] = leaf_a;
    nodes[1] = leaf_a;
    nodes[2] = leaf_a;
    nodes[3] = leaf_a;
    CHECK( wp_transform_job_system_process( job, nodes, out, 4 ) );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.capture_local_allocations == 1 && stats.capture_prefix_allocations == 1 );
    wp_transform_job_system_destroy( job );
    wp_threadpool_destroy( pool );
    wp_scenenode_destroy( leaf_a );
    wp_scenenode_destroy( leaf_b );
    wp_scenenode_destroy( root_a );
    wp_scenenode_destroy( root_b );
    puts(
        "Transform local/prefix reuse: changed identities, reparent/edit, rejection, cancel, "
        "resize, optional-cache transitions and empty release passed." );
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

static void prefix_budget_fallback( void )
{
    enum
    {
        outputs = 350000
    };

    wp_scenenode *root = wp_scenenode_create(), *leaf = wp_scenenode_create();
    const wp_scenenode **nodes = (const wp_scenenode **)malloc( outputs * sizeof( *nodes ) );
    wp_transform_job_system *job = wp_transform_job_system_create( NULL, 2 );
    wp_transform_job_stats stats;
    wp_vec3f position = { 2, 3, 4 };
    wp_mat4f expected;
    const wp_mat4f *result;
    int i;
    CHECK( nodes && job );
    if( !nodes || !job )
    {
        free( nodes );
        wp_transform_job_system_destroy( job );
        wp_scenenode_destroy( leaf );
        wp_scenenode_destroy( root );
        return;
    }
    wp_scenenode_add_child( root, leaf );
    wp_scenenode_set_position( root, position );
    reference_world( leaf, &expected );
    for( i = 0; i < outputs; ++i )
    {
        nodes[i] = leaf;
    }
    /* Base capture fits; the optional per-batch prefix worlds would exceed 64 MiB. */
    CHECK( wp_transform_job_system_dispatch_snapshot( job, nodes, outputs, 17 ) );
    wp_transform_job_system_wait( job );
    wp_transform_job_system_get_stats( job, &stats );
    CHECK( stats.path_reference_count == outputs * 2 && stats.matrix_multiply_count == outputs * 2 );
    CHECK( stats.capture_peak_bytes <= 64u * 1024u * 1024u );
    result = wp_transform_job_system_get_result( job, NULL, NULL );
    CHECK( result != NULL );
    if( result )
    {
        CHECK( !memcmp( &result[0], &expected, sizeof( expected ) ) );
        CHECK( !memcmp( &result[outputs - 1], &expected, sizeof( expected ) ) );
    }
    free( nodes );
    wp_transform_job_system_destroy( job );
    wp_scenenode_destroy( leaf );
    wp_scenenode_destroy( root );
    puts( "Prefix cache budget: optional cache falls back without rejecting a valid base capture." );
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

static void cull_order_fast_path( void )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_object *objects[17];
    wp_graphics_cull_result *result, *derived;
    wp_graphics_cull_sort_stats stats;
    wp_u32 groups[17], depths[17];
    int expected[17], i, j, key, count, mode, n, ordered, passes, width;
    CHECK( scene != NULL );
    for( i = 0; i < 17; ++i )
    {
        objects[i] = wp_graphics_scene_create_object( scene );
        wp_graphics_object_set_render_func( objects[i], record );
    }
    for( mode = 0; mode < 6; ++mode )
    {
        for( count = 0; count <= 17; ++count )
        {
            n = 0;
            ordered = 1;
            for( i = 0; i < 17; ++i )
            {
                groups[i] = mode == 0                 ? 0
                            : mode == 1               ? (wp_u32)( i / 3 )
                            : mode == 2               ? (wp_u32)( ( 16 - i ) / 3 )
                            : mode == 3               ? (wp_u32)( ( i * 7 ) % 3 )
                            : mode == 4 && !( i % 2 ) ? 99u
                                                      : (wp_u32)i;
                depths[i] = mode == 2 ? (wp_u32)( 16 - i ) : mode == 3 ? (wp_u32)( i % 2 ) : 0u;
                if( mode == 5 && i == count - 1 )
                {
                    groups[i] = 0;
                }
                wp_graphics_object_set_render_queue_group( objects[i], groups[i] );
                wp_graphics_object_set_z_order( objects[i], depths[i] );
                wp_graphics_object_set_visible( objects[i], i < count && ( mode != 4 || i % 2 ) );
                if( i < count && ( mode != 4 || i % 2 ) )
                {
                    if( n && ( groups[i] < groups[expected[n - 1]] ||
                               ( groups[i] == groups[expected[n - 1]] &&
                                 depths[i] < depths[expected[n - 1]] ) ) )
                    {
                        ordered = 0;
                    }
                    expected[n++] = i;
                }
            }
            /* Independent stable insertion reference, including repeated keys. */
            for( i = 1; i < n; ++i )
            {
                key = expected[i];
                j = i;
                while( j && ( groups[key] < groups[expected[j - 1]] ||
                              ( groups[key] == groups[expected[j - 1]] &&
                                depths[key] < depths[expected[j - 1]] ) ) )
                {
                    expected[j] = expected[j - 1];
                    --j;
                }
                expected[j] = key;
            }
            result = capture( scene, NULL, WP_GRAPHICS_CULL_PASS_MAIN );
            CHECK( result != NULL && wp_graphics_cull_result_get_visible_count( result ) == n );
            for( i = 0; i < n; ++i )
            {
                CHECK( wp_graphics_cull_result_get_object( result, i ) == objects[expected[i]] );
            }
            wp_graphics_cull_result_get_sort_stats( result, &stats );
            if( ordered )
            {
                CHECK( !stats.merge_passes && !stats.scratch_allocations && !stats.scratch_bytes &&
                       stats.order_checks == (wp_u32)( n ? n - 1 : 0 ) );
            }
            else
            {
                passes = 0;
                for( width = 1; width < n; width *= 2 )
                {
                    ++passes;
                }
                CHECK( stats.merge_passes == (wp_u32)passes && stats.scratch_allocations == 1 &&
                       stats.scratch_bytes == (wp_u64)n * sizeof( wp_s32 ) );
            }
            derived = wp_graphics_cull_result_create_view( result, NULL, WP_GRAPHICS_CULL_PASS_MAIN );
            CHECK( derived != NULL && !wp_graphics_cull_result_get_transform_dispatch_count( derived ) );
            wp_graphics_cull_result_destroy( result );
            /* Independent sorted indices survive source-view retirement. */
            for( i = 0; i < n; ++i )
            {
                CHECK( wp_graphics_cull_result_get_object( derived, i ) == objects[expected[i]] );
            }
            wp_graphics_cull_result_get_sort_stats( derived, &stats );
            CHECK( stats.scratch_allocations == (wp_u32)!ordered );
            wp_graphics_cull_result_destroy( derived );
        }
    }
    wp_graphics_cull_result_get_sort_stats( NULL, &stats );
    CHECK( !stats.order_checks && !stats.merge_passes && !stats.scratch_allocations &&
           !stats.scratch_bytes );
    wp_graphics_scene_destroy( scene );
    puts(
        "Cull ordering: 108 ordered/reversed/tied/filtered/late-inversion fixtures, "
        "stable reference parity, exact scratch bytes and derived-view ownership passed." );
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
    wp_graphics_mesh_compute_aabb( mesh );
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

static double box_dot( const wp_vec3d *a, const wp_vec3d *b )
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static void check_enclosure( const wp_graphics_obb *box, const wp_graphics_mesh_vertex_p *vertices,
                             int count )
{
    int i, j;
    wp_vec3d d;
    double squared;
    for( i = 0; i < count; ++i )
    {
        d.x = vertices[i].position.x - box->center.x;
        d.y = vertices[i].position.y - box->center.y;
        d.z = vertices[i].position.z - box->center.z;
        for( j = 0; j < 3; ++j )
        {
            squared = box_dot( &box->half_axes[j], &box->half_axes[j] );
            CHECK( fabs( box_dot( &d, &box->half_axes[j] ) ) <= squared + 1.0e-10 );
        }
    }
}

static void refitted_bounds( void )
{
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_mesh_vertex_p vertices[8];
    wp_graphics_obb previous, current, invalid;
    wp_graphics_mesh_bounds_stats stats;
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( scene );
    wp_graphics_cull_result *captured;
    wp_u32 nan_bits = 0x7fc00000u;
    double x, y, z, projection, old_extent, new_extent;
    int i, pose, expanded = 0;
    for( i = 0; i < 8; ++i )
    {
        x = ( i & 1 ) ? 2 : -2;
        y = ( i & 2 ) ? 0.1 : -0.1;
        vertices[i].position.x = (wp_f32)( ( x - y ) * 0.70710678118655 );
        vertices[i].position.y = (wp_f32)( ( x + y ) * 0.70710678118655 );
        vertices[i].position.z = ( i & 4 ) ? 0.2f : -0.2f;
    }
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    CHECK( wp_graphics_mesh_refit_bounds( mesh, NULL ) );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &previous ) );
    wp_graphics_object_set_mesh( object, mesh );
    captured = capture( scene, NULL, 0 );
    old_extent = box_dot( &previous.half_axes[0], &previous.half_axes[0] );
    for( pose = 0; pose < 64; ++pose )
    {
        for( i = 0; i < 8; ++i )
        {
            x = ( ( i & 1 ) ? 1 : -1 ) * ( 1 + pose * 0.07 );
            y = ( ( i & 2 ) ? 1 : -1 ) * ( 0.1 + pose * 0.03 );
            z = ( ( i & 4 ) ? 1 : -1 ) * ( 0.2 + pose * 0.01 );
            vertices[i].position.x =
                (wp_f32)( x * cos( pose * 0.03 ) - y * sin( pose * 0.03 ) + pose * 2 );
            vertices[i].position.y = (wp_f32)( x * sin( pose * 0.03 ) + y * cos( pose * 0.03 ) - pose );
            vertices[i].position.z = (wp_f32)z;
        }
        wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
        CHECK( !wp_graphics_mesh_get_local_obb( mesh, &current ) );
        CHECK( wp_graphics_mesh_refit_bounds( mesh, &previous ) );
        CHECK( wp_graphics_mesh_get_local_obb( mesh, &current ) );
        check_enclosure( &current, vertices, 8 );
        new_extent = box_dot( &current.half_axes[0], &current.half_axes[0] );
        projection = box_dot( &previous.half_axes[0], &current.half_axes[0] );
        CHECK( fabs( projection * projection /
                         ( box_dot( &previous.half_axes[0], &previous.half_axes[0] ) * new_extent ) -
                     1 ) < 1.0e-10 );
        if( new_extent > old_extent * 2 )
        {
            expanded = 1;
        }
        previous = current;
    }
    CHECK( expanded && previous.center.x > 100 );
    CHECK( wp_graphics_cull_result_validate(
               captured, scene, wp_graphics_cull_result_get_scene_generation( captured ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( captured );
    wp_graphics_mesh_get_bounds_stats( mesh, &stats );
    CHECK( stats.pca_solves == 1 && stats.orientation_refits == 64 );
    /* Invalid or rank-deficient prior directions request a new full fit. */
    invalid = previous;
    invalid.half_axes[1] = invalid.half_axes[0];
    CHECK( wp_graphics_mesh_refit_bounds( mesh, &invalid ) );
    memset( &invalid, 0, sizeof( invalid ) );
    CHECK( wp_graphics_mesh_refit_bounds( mesh, &invalid ) );
    invalid = previous;
    invalid.half_axes[0].x = HUGE_VAL;
    CHECK( wp_graphics_mesh_refit_bounds( mesh, &invalid ) );
    CHECK( wp_graphics_mesh_refit_bounds( mesh, NULL ) );
    wp_graphics_mesh_get_bounds_stats( mesh, &stats );
    CHECK( stats.pca_solves == 5 && stats.orientation_refits == 64 );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &current ) );
    check_enclosure( &current, vertices, 8 );
    memcpy( &vertices[0].position.x, &nan_bits, sizeof( nan_bits ) );
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    CHECK( !wp_graphics_mesh_refit_bounds( mesh, &previous ) );
    CHECK( !wp_graphics_mesh_get_local_obb( mesh, &current ) );
    CHECK( wp_graphics_mesh_get_local_aabb( mesh ).max.x == 0 );
    wp_graphics_scene_destroy( scene );
    wp_graphics_mesh_destroy( mesh );
    puts(
        "Deformed bounds: 64 conservative orientation refits with one PCA solve; invalid basis fallback "
        "passed." );
}

static double box_surface_proxy( const wp_graphics_obb *box )
{
    double a = sqrt( box_dot( &box->half_axes[0], &box->half_axes[0] ) );
    double b = sqrt( box_dot( &box->half_axes[1], &box->half_axes[1] ) );
    double c = sqrt( box_dot( &box->half_axes[2], &box->half_axes[2] ) );
    return a * b + a * c + b * c;
}

static void adaptive_bounds( void )
{
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_mesh_vertex_p vertices[8];
    wp_graphics_obb previous, fixed, refreshed, actual;
    wp_graphics_mesh_bounds_stats stats, before;
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( scene );
    wp_camera *camera = wp_camera_create();
    wp_graphics_cull_result *r;
    wp_u64 generation;
    wp_u32 nan_bits = 0x7fc00000u;
    wp_f32 nan_value;
    double x, y;
    int i, pose, type;
    for( i = 0; i < 8; ++i )
    {
        x = ( i & 1 ) ? 2 : -2;
        y = ( i & 2 ) ? 0.05 : -0.05;
        vertices[i].position.x = (wp_f32)( ( x - y ) * 0.70710678118655 );
        vertices[i].position.y = (wp_f32)( ( x + y ) * 0.70710678118655 );
        vertices[i].position.z = ( i & 4 ) ? -1.95f : -2.05f;
    }
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    CHECK( wp_graphics_mesh_refit_bounds_adaptive( mesh, NULL, 1.5 ) );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &previous ) );
    for( i = 0; i < 8; ++i )
    {
        vertices[i].position.x = ( i & 2 ) ? 1.25f : 1.15f;
        vertices[i].position.y = ( i & 1 ) ? 2 : -2;
    }
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    CHECK( wp_graphics_mesh_refit_bounds( mesh, &previous ) );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &fixed ) );
    check_enclosure( &fixed, vertices, 8 );
    wp_graphics_object_set_mesh( object, mesh );
    wp_camera_set_projection_type( camera, WORKPHONE_PROJECTION_ORTHOGRAPHIC );
    wp_camera_set_ortho_width( camera, 1 );
    wp_camera_set_aspect_ratio( camera, 1 );
    r = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 1 );
    CHECK( wp_graphics_mesh_refit_bounds_adaptive( mesh, &fixed, 1.5 ) );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &refreshed ) );
    check_enclosure( &refreshed, vertices, 8 );
    CHECK( box_surface_proxy( &refreshed ) < box_surface_proxy( &fixed ) * 0.2 );
    CHECK( wp_graphics_cull_result_validate( r, scene,
                                             wp_graphics_cull_result_get_scene_generation( r ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( r );
    r = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 0 );
    wp_graphics_cull_result_destroy( r );
    previous = refreshed;
    for( pose = 0; pose < 64; ++pose )
    {
        for( i = 0; i < 8; ++i )
        {
            vertices[i].position.x += 0.01f;
        }
        wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
        CHECK( wp_graphics_mesh_refit_bounds_adaptive( mesh, &previous, 1.5 ) );
        CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
        check_enclosure( &actual, vertices, 8 );
        previous = actual;
    }
    wp_graphics_mesh_get_bounds_stats( mesh, &stats );
    CHECK( stats.pca_solves == 2 && stats.orientation_refits == 66 && stats.adaptive_refreshes == 1 );
    generation = wp_graphics_mesh_get_content_generation( mesh );
    memcpy( &nan_value, &nan_bits, sizeof( nan_value ) );
    CHECK( !wp_graphics_mesh_refit_bounds_adaptive( mesh, &previous, 0.5 ) );
    CHECK( !wp_graphics_mesh_refit_bounds_adaptive( mesh, &previous, HUGE_VAL ) );
    CHECK( !wp_graphics_mesh_refit_bounds_adaptive( mesh, &previous, nan_value ) );
    CHECK( wp_graphics_mesh_get_content_generation( mesh ) == generation );
    wp_graphics_mesh_get_bounds_stats( mesh, &before );
    CHECK( !memcmp( &stats, &before, sizeof( stats ) ) );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
    CHECK( !memcmp( &actual, &previous, sizeof( actual ) ) );
    /* Numerical padding alone must not trigger refreshes for flat/linear/point data. */
    for( type = 0; type < 3; ++type )
    {
        for( i = 0; i < 8; ++i )
        {
            vertices[i].position.x = type == 2 ? 4 : ( ( i & 1 ) ? 2 : -2 );
            vertices[i].position.y = type >= 1 ? 0 : ( ( i & 2 ) ? 1 : -1 );
            vertices[i].position.z = 0;
        }
        wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
        CHECK( wp_graphics_mesh_refit_bounds_adaptive( mesh, NULL, 1.0 ) );
        CHECK( wp_graphics_mesh_get_local_obb( mesh, &previous ) );
        wp_graphics_mesh_get_bounds_stats( mesh, &before );
        for( pose = 0; pose < 8; ++pose )
        {
            CHECK( wp_graphics_mesh_refit_bounds_adaptive( mesh, &previous, 1.0 ) );
            CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
            check_enclosure( &actual, vertices, 8 );
            previous = actual;
        }
        wp_graphics_mesh_get_bounds_stats( mesh, &stats );
        CHECK( stats.pca_solves == before.pca_solves &&
               stats.adaptive_refreshes == before.adaptive_refreshes );
    }
    wp_graphics_scene_destroy( scene );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    puts(
        "Adaptive OBB: tighter cull after one refresh, 64 stable refits without more PCA, degenerate "
        "stability passed." );
}

static void oriented_bounds( void )
{
    wp_graphics_scene *s = wp_graphics_scene_create();
    wp_camera *camera = wp_camera_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( s );
    wp_scenenode *node = wp_graphics_scene_create_node( s );
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_mesh_vertex_p vertices[8];
    wp_graphics_obb fitted, invalid, actual;
    wp_graphics_cull_result *r, *shadow;
    wp_graphics_cull_bounds_stats stats;
    wp_vec3f p = { 1.2f, 0, 0 };
    wp_quatf q = { 0.923879533f, 0, 0, 0.382683432f };
    wp_aabb3f authored = { { -10, -10, -3 }, { 10, 10, -1 } };
    wp_u64 generation;
    int i, j;
    wp_u32 nan_bits = 0x7fc00000u;
    double along, across;
    for( i = 0; i < 8; ++i )
    {
        along = ( i & 1 ) ? 2 : -2;
        across = ( i & 2 ) ? 0.05 : -0.05;
        vertices[i].position.x = (wp_f32)( ( along - across ) * 0.70710678118655 );
        vertices[i].position.y = (wp_f32)( ( along + across ) * 0.70710678118655 );
        vertices[i].position.z = ( i & 4 ) ? -1.95f : -2.05f;
    }
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    wp_graphics_mesh_compute_aabb( mesh );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &fitted ) );
    check_enclosure( &fitted, vertices, 8 );
    invalid = fitted;
    invalid.half_axes[1] = invalid.half_axes[0];
    generation = wp_graphics_mesh_get_content_generation( mesh );
    CHECK( !wp_graphics_mesh_set_local_obb( mesh, &invalid ) );
    CHECK( wp_graphics_mesh_get_content_generation( mesh ) == generation );
    CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
    CHECK( memcmp( &actual, &fitted, sizeof( actual ) ) == 0 );
    wp_graphics_object_set_mesh( object, mesh );
    wp_scenenode_attach_object( node, object );
    wp_scenenode_set_position( node, p );
    wp_scenenode_set_orientation( node, q );
    wp_camera_set_projection_type( camera, WORKPHONE_PROJECTION_ORTHOGRAPHIC );
    wp_camera_set_ortho_width( camera, 1 );
    wp_camera_set_aspect_ratio( camera, 1 );
    r = capture( s, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 0 );
    wp_graphics_cull_result_get_bounds_stats( r, &stats );
    CHECK( stats.sphere_tests == 1 && stats.sphere_rejections == 0 && stats.fitted_obb_tests == 1 &&
           stats.box_rejections == 1 );
    shadow = wp_graphics_cull_result_create_view( r, camera, WP_GRAPHICS_CULL_PASS_SHADOW );
    CHECK( shadow && wp_graphics_cull_result_get_visible_count( shadow ) == 1 );
    wp_graphics_cull_result_get_bounds_stats( shadow, &stats );
    CHECK( !stats.sphere_tests && !stats.box_tests );
    wp_graphics_cull_result_destroy( r );
    CHECK( wp_graphics_cull_result_get_visible_count( shadow ) == 1 );
    wp_graphics_mesh_clear_local_obb( mesh );
    CHECK( wp_graphics_cull_result_validate( shadow, s,
                                             wp_graphics_cull_result_get_scene_generation( shadow ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( shadow );
    r = capture( s, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 1 );
    wp_graphics_cull_result_destroy( r );
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &fitted ) );
    p.x = 100;
    wp_scenenode_set_position( node, p );
    r = capture( s, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( r, &stats );
    CHECK( stats.sphere_rejections == 1 && !stats.box_tests );
    wp_graphics_cull_result_destroy( r );
    wp_graphics_mesh_set_local_aabb( mesh, authored );
    CHECK( !wp_graphics_mesh_get_local_obb( mesh, &actual ) );
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    CHECK( wp_graphics_mesh_get_local_aabb( mesh ).max.x == 10 );
    /* Repeated eigenvalues, planar, linear, and point clouds remain enclosed. */
    for( j = 0; j < 4; ++j )
    {
        for( i = 0; i < 8; ++i )
        {
            vertices[i].position.x = ( j == 3 ) ? 4 : (wp_f32)( ( i & 1 ) ? 1 : -1 );
            vertices[i].position.y = ( j >= 2 ) ? 0 : (wp_f32)( ( i & 2 ) ? 1 : -1 );
            vertices[i].position.z = ( j >= 1 ) ? 0 : (wp_f32)( ( i & 4 ) ? 1 : -1 );
        }
        wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
        wp_graphics_mesh_compute_aabb( mesh );
        CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
        check_enclosure( &actual, vertices, 8 );
    }
    for( j = 0; j < 64; ++j )
    {
        for( i = 0; i < 8; ++i )
        {
            along = 3 * sin( i * 1.73 + j * 0.13 );
            across = 0.1 * cos( i * 0.9 + j );
            vertices[i].position.x = (wp_f32)( along * 0.7 - across * 0.6 );
            vertices[i].position.y = (wp_f32)( along * 0.4 + across * 0.8 );
            vertices[i].position.z = (wp_f32)( along * 0.591607978 + 0.02 * sin( i + j ) );
        }
        wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
        wp_graphics_mesh_compute_aabb( mesh );
        CHECK( wp_graphics_mesh_get_local_obb( mesh, &actual ) );
        check_enclosure( &actual, vertices, 8 );
    }
    wp_graphics_mesh_mark_dirty( mesh );
    CHECK( !wp_graphics_mesh_get_local_obb( mesh, &actual ) );
    CHECK( wp_graphics_mesh_get_local_aabb( mesh ).max.x == 0 );
    memcpy( &vertices[0].position.x, &nan_bits, sizeof( nan_bits ) );
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    wp_graphics_mesh_compute_aabb( mesh );
    CHECK( !wp_graphics_mesh_get_local_obb( mesh, &actual ) );
    p.x = 0;
    wp_scenenode_set_position( node, p );
    r = capture( s, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( r, &stats );
    CHECK( wp_graphics_cull_result_get_visible_count( r ) == 1 && stats.conservative_candidates == 1 );
    wp_graphics_cull_result_destroy( r );
    wp_graphics_scene_destroy( s );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    puts(
        "Fitted OBB: enclosure, tighter rejection, authored margins, shadow retention and invalidation "
        "passed." );
}

static double cube_box_gap( const wp_graphics_obb *box, wp_vec3d axis )
{
    wp_vec3d delta = box->center;
    double radius = 0;
    int i;
    delta.z += 2;
    for( i = 0; i < 3; ++i )
    {
        radius += fabs( box_dot( &box->half_axes[i], &axis ) );
    }
    return fabs( box_dot( &delta, &axis ) ) - radius - fabs( axis.x ) - fabs( axis.y ) - fabs( axis.z );
}

static void separating_axes( void )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( scene );
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_camera *camera = wp_camera_create();
    wp_graphics_cull_job *job = wp_graphics_cull_job_create();
    wp_graphics_cull_result *main, *off, *shadow, *derived;
    wp_graphics_cull_bounds_stats stats;
    wp_graphics_obb box = { { 1.3, 1.3, -2 },
                            { { 1.414213562373095, -1.414213562373095, 0 },
                              { 0.0353553390593274, 0.0353553390593274, 0 },
                              { 0, 0, 0.05 } } };
    wp_vec3f position = { 1.3f, 1.3f, 0 };
    wp_vec3d axis;
    wp_u64 policy_generation;
    wp_threadpool *policy_pool = wp_threadpool_create( 1 );
    int i;
    wp_camera_set_projection_type( camera, WORKPHONE_PROJECTION_ORTHOGRAPHIC );
    wp_camera_set_ortho_width( camera, 1 );
    wp_camera_set_aspect_ratio( camera, 1 );
    wp_camera_set_near_clip_distance( camera, 1 );
    wp_camera_set_far_clip_distance( camera, 3 );
    wp_graphics_scene_set_active_camera( scene, camera );
    wp_graphics_object_set_mesh( object, mesh );
    wp_graphics_object_set_render_func( object, record );
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    CHECK( wp_graphics_cull_job_set_sat_test( job, 0 ) == WP_GRAPHICS_CULL_OK );
    CHECK( wp_graphics_cull_job_configure( job, scene, camera, 0, 0 ) == WP_GRAPHICS_CULL_OK );
    CHECK( wp_graphics_cull_job_set_sat_test( job, 1 ) == WP_GRAPHICS_CULL_ERROR_INVALID_STATE );
    CHECK( wp_graphics_cull_job_execute( job ) == WP_GRAPHICS_CULL_OK );
    off = wp_graphics_cull_job_take_result( job );
    CHECK( wp_graphics_cull_result_get_visible_count( off ) == 1 );
    wp_graphics_cull_result_get_bounds_stats( off, &stats );
    CHECK( !stats.sat_tests && !stats.sat_axis_tests );
    derived = wp_graphics_cull_result_create_view( off, camera, 0 );
    CHECK( derived && wp_graphics_cull_result_get_visible_count( derived ) == 1 );
    wp_graphics_cull_result_destroy( derived );
    wp_graphics_cull_result_destroy( off );
    policy_generation = wp_graphics_scene_get_generation( scene );
    CHECK( wp_graphics_scene_get_cull_flags( scene ) ==
           ( WP_GRAPHICS_CULL_SPHERE_TEST | WP_GRAPHICS_CULL_SAT_TEST ) );
    CHECK( wp_graphics_scene_set_cull_flags( scene, 3 ) &&
           wp_graphics_scene_get_generation( scene ) == policy_generation );
    CHECK( !wp_graphics_scene_set_cull_flags( scene, 128 ) &&
           wp_graphics_scene_get_generation( scene ) == policy_generation );
    CHECK( wp_graphics_scene_set_cull_flags( scene, WP_GRAPHICS_CULL_SPHERE_TEST ) );
    off = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( off ) == 1 );
    wp_graphics_cull_result_get_bounds_stats( off, &stats );
    CHECK( stats.sphere_tests == 1 && !stats.sat_tests );
    draws = 0;
    wp_graphics_scene_render_with_submit( scene, (wp_renderer *)scene, NULL, NULL );
    CHECK( draws == 1 );
    wp_graphics_scene_set_transform_pool( scene, policy_pool );
    derived = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( derived ) == 1 &&
           wp_graphics_scene_get_cull_flags( scene ) == WP_GRAPHICS_CULL_SPHERE_TEST );
    wp_graphics_cull_result_destroy( derived );
    CHECK( wp_graphics_scene_set_cull_flags( scene, WP_GRAPHICS_CULL_SAT_TEST ) );
    CHECK( wp_graphics_cull_result_validate( off, scene,
                                             wp_graphics_cull_result_get_scene_generation( off ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( off );
    off = capture( scene, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( off, &stats );
    CHECK( !wp_graphics_cull_result_get_visible_count( off ) && !stats.sphere_tests &&
           stats.sat_rejections == 1 );
    wp_graphics_cull_result_destroy( off );
    wp_graphics_scene_set_transform_pool( scene, NULL );
    wp_threadpool_destroy( policy_pool );
    CHECK( wp_graphics_scene_set_cull_flags( scene, 3 ) );
    main = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 0 );
    wp_graphics_cull_result_get_bounds_stats( main, &stats );
    CHECK( stats.sat_tests == 1 && stats.sat_rejections == 1 && stats.sat_axis_tests > 0 );
    draws = 0;
    wp_graphics_scene_render_with_submit( scene, (wp_renderer *)scene, NULL, NULL );
    CHECK( draws == 0 );
    shadow = wp_graphics_cull_result_create_view( main, camera, WP_GRAPHICS_CULL_PASS_SHADOW );
    CHECK( shadow && wp_graphics_cull_result_get_visible_count( shadow ) == 1 );
    wp_graphics_cull_result_get_bounds_stats( shadow, &stats );
    CHECK( !stats.sat_tests && !stats.sphere_tests );
    wp_camera_set_position( camera, position );
    derived = wp_graphics_cull_result_create_view( main, camera, 0 );
    CHECK( derived && wp_graphics_cull_result_get_visible_count( derived ) == 1 );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_cull_result_destroy( shadow );
    CHECK( wp_graphics_cull_result_get_visible_count( derived ) == 1 );
    wp_graphics_cull_result_destroy( derived );
    position.x = position.y = 0;
    wp_camera_set_position( camera, position );
    /* Contact at the view corner, with no box vertex inside, must survive. */
    box.center.x = box.center.y = 1.0353553390593274;
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    main = capture( scene, camera, 0 );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 1 );
    wp_graphics_cull_result_destroy( main );
    /* Fixed edge-edge regression: all six cube and three OBB face axes overlap,
     * but the supplied cross-edge witness has a positive separating gap. */
    box.center.x = 1.2956478737760335;
    box.center.y = -0.5425630835816264;
    box.center.z = -0.6363588424865156;
    box.half_axes[0].x = 0.6986211111440349;
    box.half_axes[0].y = -0.421736194036587;
    box.half_axes[0].z = -1.2585972849579043;
    box.half_axes[1].x = 0.13126710687103646;
    box.half_axes[1].y = 0.11862733377251229;
    box.half_axes[1].z = 0.03311347664822984;
    box.half_axes[2].x = 0.06015061715330456;
    box.half_axes[2].y = -0.08370942140533064;
    box.half_axes[2].z = 0.061438066571640734;
    for( i = 0; i < 3; ++i )
    {
        axis.x = i == 0;
        axis.y = i == 1;
        axis.z = i == 2;
        CHECK( cube_box_gap( &box, axis ) < 0 );
        CHECK( cube_box_gap( &box, box.half_axes[i] ) < 0 );
    }
    axis.x = -0.839064856638603;
    axis.y = 0;
    axis.z = -0.4657474074293566;
    CHECK( cube_box_gap( &box, axis ) > 0.2 );
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    main = capture( scene, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( main, &stats );
    CHECK( !wp_graphics_cull_result_get_visible_count( main ) && stats.sat_rejections == 1 );
    wp_graphics_cull_result_destroy( main );
    /* A contained wide box skips SAT even when its sphere crosses the view planes. */
    memset( &box, 0, sizeof( box ) );
    box.center.z = -2;
    box.half_axes[0].x = box.half_axes[1].y = 0.95;
    box.half_axes[2].z = 0.05;
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    main = capture( scene, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( main, &stats );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 1 && stats.box_tests == 1 &&
           !stats.sphere_inside && !stats.sat_tests );
    wp_graphics_cull_result_destroy( main );
    /* Numerically collapsed finite frustum keeps its plane result. */
    memset( &box, 0, sizeof( box ) );
    box.center.z = -2;
    box.half_axes[0].x = box.half_axes[1].y = box.half_axes[2].z = 0.2;
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    wp_camera_set_aspect_ratio( camera, 1.0e20f );
    main = capture( scene, camera, 0 );
    wp_graphics_cull_result_get_bounds_stats( main, &stats );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 1 && stats.sat_fallbacks == 1 &&
           !stats.sat_tests );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_cull_job_destroy( job );
    CHECK( wp_graphics_scene_set_cull_flags( scene, 0 ) );
    wp_graphics_scene_clear( scene );
    CHECK( !wp_graphics_scene_get_cull_flags( scene ) );
    wp_graphics_scene_destroy( scene );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    puts(
        "Frustum/box SAT: corner and edge-edge rejection, corner contact, A/B inheritance, shadows and "
        "numeric fallback passed." );
}

static void sat_common_points( void )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( scene );
    wp_scenenode *parent = wp_graphics_scene_create_node( scene ),
                 *child = wp_graphics_scene_create_node( scene );
    wp_camera *camera = wp_camera_create();
    wp_graphics_obb box = { { 0, 0, 0 }, { { 2, 0, 0 }, { 0, 0.1, 0 }, { 0, 0, 0.2 } } };
    wp_graphics_cull_result *r;
    wp_graphics_cull_bounds_stats stats;
    wp_vec3f position = { 0.3f, 0.2f, -2 }, scale;
    wp_quatf orientation;
    wp_u32 tested = 0;
    int i;
    wp_graphics_object_set_mesh( object, mesh );
    wp_scenenode_attach_object( child, object );
    wp_scenenode_add_child( parent, child );
    wp_scenenode_set_position( parent, position );
    CHECK( wp_graphics_mesh_set_local_obb( mesh, &box ) );
    wp_camera_set_ortho_width( camera, 1 );
    wp_camera_set_aspect_ratio( camera, 1 );
    wp_camera_set_near_clip_distance( camera, 1 );
    wp_camera_set_far_clip_distance( camera, 3 );
    for( i = 0; i < 256; ++i )
    {
        wp_camera_set_projection_type(
            camera, ( i & 1 ) ? WORKPHONE_PROJECTION_PERSPECTIVE : WORKPHONE_PROJECTION_ORTHOGRAPHIC );
        scale.x = ( i % 13 ) ? ( ( i & 2 ) ? -3.0f : 0.2f ) : 0;
        scale.y = 1.7f;
        scale.z = 0.4f;
        wp_scenenode_set_scale( parent, scale );
        orientation.w = (wp_f32)cos( i * 0.077 );
        orientation.x = (wp_f32)( sin( i * 0.077 ) * 0.6 );
        orientation.y = (wp_f32)( sin( i * 0.077 ) * 0.8 );
        orientation.z = 0;
        wp_scenenode_set_orientation( child, orientation );
        /* The local origin is inside the box and maps to a strict interior point
         * of either view, independent of reflection, rank loss or parent shear. */
        r = capture( scene, camera, 0 );
        wp_graphics_cull_result_get_bounds_stats( r, &stats );
        tested += stats.sat_tests;
        CHECK( wp_graphics_cull_result_get_visible_count( r ) == 1 && !stats.sat_rejections );
        wp_graphics_cull_result_destroy( r );
    }
    CHECK( tested > 0 );
    wp_graphics_scene_destroy( scene );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    puts(
        "SAT common-point oracle: 256 perspective/orthographic reflected, singular and sheared boxes "
        "retained." );
}

static void sphere_parity( void )
{
    wp_graphics_scene *s = wp_graphics_scene_create();
    wp_camera *camera = wp_camera_create();
    wp_graphics_object *object = wp_graphics_scene_create_object( s );
    wp_scenenode *parent = wp_graphics_scene_create_node( s ),
                 *node = wp_graphics_scene_create_node( s );
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_mesh_vertex_p vertices[8];
    wp_graphics_cull_job *job = wp_graphics_cull_job_create();
    wp_graphics_cull_result *on, *off;
    wp_graphics_cull_bounds_stats stats;
    wp_vec3f p, scale;
    wp_quatf q;
    wp_mat4f world, view, projection, vp, clip;
    double v[4];
    int sample, i, j, k, inside, seen = 0;
    wp_u32 rejects = 0, accepts = 0, box_tests = 0;
    for( i = 0; i < 8; ++i )
    {
        vertices[i].position.x = ( i & 1 ) ? 0.4f : -0.4f;
        vertices[i].position.y = ( i & 2 ) ? 0.2f : -0.2f;
        vertices[i].position.z = ( i & 4 ) ? 0.1f : -0.1f;
    }
    wp_graphics_mesh_set_vertices( mesh, WORKPHONE_VERTEX_FORMAT_P, vertices, 8 );
    wp_graphics_mesh_compute_aabb( mesh );
    wp_graphics_object_set_mesh( object, mesh );
    wp_scenenode_attach_object( node, object );
    wp_scenenode_add_child( parent, node );
    wp_camera_set_projection_type( camera, WORKPHONE_PROJECTION_ORTHOGRAPHIC );
    wp_camera_set_ortho_width( camera, 2 );
    wp_camera_set_aspect_ratio( camera, 1 );
    for( sample = 0; sample < 256; ++sample )
    {
        /* Coprime sequences exercise reflected, singular and sheared hierarchies. */
        p.x = (wp_f32)( ( sample * 37 ) % 101 - 50 ) * 0.09f;
        p.y = (wp_f32)( ( sample * 19 ) % 79 - 39 ) * 0.09f;
        p.z = -5;
        scale.x = ( sample % 11 == 0 ) ? 0 : ( ( sample & 1 ) ? -3.0f : 0.3f );
        scale.y = 1.7f;
        scale.z = 0.5f;
        wp_scenenode_set_position( parent, p );
        wp_scenenode_set_scale( parent, scale );
        q.x = 0;
        q.y = 0;
        q.z = (wp_f32)sin( sample * 0.137 );
        q.w = (wp_f32)cos( sample * 0.137 );
        wp_scenenode_set_orientation( node, q );
        on = capture( s, camera, 0 );
        wp_graphics_cull_result_get_bounds_stats( on, &stats );
        rejects += stats.sphere_rejections;
        accepts += stats.sphere_inside;
        box_tests += stats.box_tests;
        CHECK( wp_graphics_cull_job_set_sphere_test( job, 0 ) == WP_GRAPHICS_CULL_OK );
        CHECK( wp_graphics_cull_job_configure( job, s, camera, 0, 0 ) == WP_GRAPHICS_CULL_OK );
        CHECK( wp_graphics_cull_job_set_sphere_test( job, 1 ) == WP_GRAPHICS_CULL_ERROR_INVALID_STATE );
        CHECK( wp_graphics_cull_job_execute( job ) == WP_GRAPHICS_CULL_OK );
        off = wp_graphics_cull_job_take_result( job );
        CHECK( wp_graphics_cull_result_get_visible_count( on ) ==
               wp_graphics_cull_result_get_visible_count( off ) );
        wp_graphics_cull_result_get_bounds_stats( off, &stats );
        CHECK( !stats.sphere_tests && !stats.sphere_rejections && !stats.sphere_inside );
        /* Independent clip-space vertex oracle: never discard a mesh containing a visible vertex. */
        wp_scenenode_get_world_matrix( node, &world );
        wp_graphics_cull_result_get_camera_matrices( on, &view, &projection );
        wp_mat4f_mul( &vp, &projection, &view );
        wp_mat4f_mul( &clip, &vp, &world );
        inside = 0;
        for( i = 0; i < 8; ++i )
        {
            for( j = 0; j < 4; ++j )
            {
                v[j] = clip.m[j][3];
                for( k = 0; k < 3; ++k )
                {
                    v[j] += clip.m[j][k] * ( (const wp_f32 *)&vertices[i].position )[k];
                }
            }
            if( v[0] > -v[3] + 0.0001 && v[0] < v[3] - 0.0001 && v[1] > -v[3] + 0.0001 &&
                v[1] < v[3] - 0.0001 && v[2] > -v[3] + 0.0001 && v[2] < v[3] - 0.0001 )
            {
                inside = 1;
            }
        }
        if( inside )
        {
            ++seen;
            CHECK( wp_graphics_cull_result_get_visible_count( on ) == 1 );
        }
        wp_graphics_cull_result_destroy( on );
        wp_graphics_cull_result_destroy( off );
        wp_graphics_cull_job_reset( job );
    }
    CHECK( seen > 0 && rejects > 0 && accepts > 0 && box_tests > 0 );
    printf(
        "Sphere parity: 256 affine hierarchies, %lu early rejects, %lu full accepts, %lu box tests.\n",
        (unsigned long)rejects, (unsigned long)accepts, (unsigned long)box_tests );
    wp_graphics_cull_job_destroy( job );
    wp_graphics_scene_destroy( s );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
}

static void shared_views( void )
{
    wp_graphics_scene *scene = wp_graphics_scene_create();
    wp_threadpool *pool = wp_threadpool_create( 1 );
    CHECK( pool != NULL );
    wp_camera *a = wp_camera_create(), *b = wp_camera_create();
    wp_graphics_mesh *mesh = wp_graphics_mesh_create();
    wp_graphics_object *objects[2];
    wp_scenenode *nodes[2];
    wp_aabb3f bounds = { { -.2f, -.2f, -2.2f }, { .2f, .2f, -1.8f } };
    wp_vec3f p = { 100, 0, 0 };
    wp_mat4f world, other_world;
    wp_graphics_cull_result *main, *shadow, *second, *third, *reference, *empty;
    wp_transform_job_stats original_stats, derived_stats;
    wp_u32 stamp;
    int i;
    for( i = 0; i < 2; ++i )
    {
        objects[i] = wp_graphics_scene_create_object( scene );
        nodes[i] = wp_graphics_scene_create_node( scene );
        wp_graphics_object_set_mesh( objects[i], mesh );
        wp_scenenode_attach_object( nodes[i], objects[i] );
    }
    wp_graphics_mesh_set_local_aabb( mesh, bounds );
    wp_scenenode_set_position( nodes[1], p );
    wp_camera_set_position( b, p );
    main = capture( scene, a, WP_GRAPHICS_CULL_PASS_MAIN );
    stamp = wp_graphics_cull_result_get_scene_generation( main );
    CHECK( wp_graphics_cull_result_get_visible_count( main ) == 1 &&
           wp_graphics_cull_result_get_object( main, 0 ) == objects[0] );
    shadow = wp_graphics_cull_result_create_view( main, a, WP_GRAPHICS_CULL_PASS_SHADOW );
    second = wp_graphics_cull_result_create_view( main, b, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( shadow && second );
    CHECK( wp_graphics_cull_result_get_visible_count( shadow ) == 2 );
    CHECK( wp_graphics_cull_result_get_visible_count( second ) == 1 &&
           wp_graphics_cull_result_get_object( second, 0 ) == objects[1] );
    CHECK( wp_graphics_cull_result_get_transform_dispatch_count( main ) == 1 &&
           !wp_graphics_cull_result_get_transform_dispatch_count( shadow ) &&
           !wp_graphics_cull_result_get_transform_dispatch_count( second ) );
    wp_graphics_cull_result_get_transform_stats( main, &original_stats );
    wp_graphics_cull_result_get_transform_stats( second, &derived_stats );
    CHECK( original_stats.output_count == 2 &&
           !memcmp( &original_stats, &derived_stats, sizeof( original_stats ) ) );
    reference = capture( scene, b, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( wp_graphics_cull_result_get_object( reference, 0 ) ==
           wp_graphics_cull_result_get_object( second, 0 ) );
    wp_graphics_cull_result_get_world_matrix( reference, 0, &world );
    wp_graphics_cull_result_get_world_matrix( second, 0, &other_world );
    CHECK( equal_matrix( &world, &other_world ) );
    wp_graphics_cull_result_destroy( reference );
    wp_camera_set_visibility_mask( b, 0 );
    empty = wp_graphics_cull_result_create_view( main, b, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( empty && wp_graphics_cull_result_get_visible_count( empty ) == 0 );
    CHECK( wp_graphics_cull_result_validate( main, scene, stamp ) == WP_GRAPHICS_CULL_OK );
    CHECK( wp_graphics_cull_result_validate( second, scene, stamp ) == WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( empty );
    wp_camera_set_visibility_mask( b, ~(wp_u32)0 );

    /* View-only edits permit a fresh view without recapturing valid geometry. */
    p.x = 10;
    wp_camera_set_position( a, p );
    CHECK( wp_graphics_cull_result_validate( main, scene, stamp ) == WP_GRAPHICS_CULL_ERROR_STALE );
    empty = wp_graphics_cull_result_create_view( main, a, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( empty && wp_graphics_cull_result_get_visible_count( empty ) == 0 &&
           !wp_graphics_cull_result_get_transform_dispatch_count( empty ) );
    wp_graphics_cull_result_destroy( empty );
    p.x = 0;
    wp_camera_set_position( a, p );
    third = wp_graphics_cull_result_create_view( second, a, WP_GRAPHICS_CULL_PASS_MAIN );
    CHECK( third && wp_graphics_cull_result_get_object( third, 0 ) == objects[0] );
    CHECK( !wp_graphics_cull_result_create_view( main, a, (wp_graphics_cull_pass)9 ) );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_cull_result_destroy( second );
    /* Neither source result nor the scene's persistent job owns derived entries. */
    wp_graphics_scene_set_transform_pool( scene, pool );
    CHECK( wp_graphics_cull_result_validate( shadow, scene, stamp ) == WP_GRAPHICS_CULL_OK );
    draws = 0;
    CHECK( wp_graphics_scene_render_cull_result( scene, (wp_renderer *)scene, third, consume, NULL ) ==
               WP_GRAPHICS_CULL_OK &&
           draws == 1 );
    wp_graphics_cull_result_destroy( third );
    wp_graphics_object_set_visibility_flags( objects[0], 2 );
    CHECK( !wp_graphics_cull_result_create_view( shadow, a, WP_GRAPHICS_CULL_PASS_MAIN ) );
    CHECK( wp_graphics_cull_result_validate( shadow, scene, stamp ) == WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( shadow );
    main = capture( scene, a, WP_GRAPHICS_CULL_PASS_MAIN );
    shadow = wp_graphics_cull_result_create_view( main, a, WP_GRAPHICS_CULL_PASS_SHADOW );
    p.x = 90;
    wp_scenenode_set_position( nodes[1], p );
    CHECK( !wp_graphics_cull_result_create_view( main, b, WP_GRAPHICS_CULL_PASS_MAIN ) );
    wp_graphics_scene_clear( scene );
    CHECK( wp_graphics_cull_result_validate( shadow, scene,
                                             wp_graphics_cull_result_get_scene_generation( shadow ) ) ==
           WP_GRAPHICS_CULL_ERROR_STALE );
    wp_graphics_cull_result_destroy( main );
    wp_graphics_cull_result_destroy( shadow );
    /* Empty publications also have independent, retainable view lifetimes. */
    main = capture( scene, NULL, 0 );
    shadow = wp_graphics_cull_result_create_view( main, NULL, WP_GRAPHICS_CULL_PASS_SHADOW );
    wp_graphics_cull_result_destroy( main );
    CHECK( shadow && !wp_graphics_cull_result_get_candidate_count( shadow ) );
    CHECK( wp_graphics_cull_result_validate( shadow, scene,
                                             wp_graphics_cull_result_get_scene_generation( shadow ) ) ==
           WP_GRAPHICS_CULL_OK );
    wp_graphics_cull_result_destroy( shadow );
    wp_graphics_scene_destroy( scene );
    wp_threadpool_destroy( pool );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( a );
    wp_camera_destroy( b );
    puts(
        "Shared candidate publication: one transform dispatch, independent colour/shadow/camera views "
        "and source retirement passed." );
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

static void compact_sat_axes( void )
{
    wp_camera *camera = wp_camera_create();
    wp_cull_frustum frustum;
    wp_cull_box box;
    wp_aabb3f bounds = { { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.5f, 0.5f } };
    wp_mat4f view, projection, vp, world, rotation, tilt, combined, scale;
    wp_vec3f position;
    wp_quatf orientation;
    double near_axis[3] = { 1, 1.0e-14, 0 }, exact_axis[3] = { 1, 0, 0 };
    wp_u32 full, compact, total_full = 0, total_compact = 0, rejected = 0, retained = 0;
    wp_s32 a, b, stage;
    int pass, sample;
    CHECK( !wp_cull_same_axis( near_axis, exact_axis ) );
    wp_camera_set_ortho_width( camera, 1 );
    wp_camera_set_aspect_ratio( camera, 1 );
    wp_camera_set_near_clip_distance( camera, 1 );
    wp_camera_set_far_clip_distance( camera, 4 );
    for( pass = 0; pass < 4; ++pass )
    {
        wp_camera_set_projection_type( camera, ( pass & 1 ) ? WORKPHONE_PROJECTION_PERSPECTIVE
                                                            : WORKPHONE_PROJECTION_ORTHOGRAPHIC );
        position.x = pass >= 2 ? 10.0f : 0;
        position.y = pass >= 2 ? -7.0f : 0;
        position.z = pass >= 2 ? 3.0f : 0;
        orientation.w = (wp_f32)cos( pass >= 2 ? 0.17 : 0 );
        orientation.x = (wp_f32)( sin( pass >= 2 ? 0.17 : 0 ) * 0.6 );
        orientation.y = (wp_f32)( sin( pass >= 2 ? 0.17 : 0 ) * 0.8 );
        orientation.z = 0;
        wp_camera_set_position( camera, position );
        wp_camera_set_orientation( camera, orientation );
        wp_camera_get_view_matrix( camera, &view );
        wp_camera_get_projection_matrix( camera, &projection );
        wp_mat4f_mul( &vp, &projection, &view );
        wp_cull_prepare_frustum( &frustum, &vp );
        wp_cull_prepare_sat_frustum( &frustum );
        CHECK( frustum.sat_valid );
        if( pass == 0 )
        {
            CHECK( frustum.face_count == 3 && frustum.edge_count == 3 );
        }
        if( pass == 1 )
        {
            CHECK( frustum.face_count == 5 && frustum.edge_count == 6 );
        }
        for( sample = 0; sample < 256; ++sample )
        {
            wp_mat4f_from_rotation_z( &rotation, (wp_f32)( sample * 0.137 ) );
            wp_mat4f_from_rotation_y( &tilt, (wp_f32)( sample * 0.071 ) );
            wp_mat4f_mul( &combined, &rotation, &tilt );
            wp_mat4f_from_scale( &scale, ( sample % 23 ) ? ( ( sample & 1 ) ? -3.0f : 0.3f ) : 0, 1.7f,
                                 0.6f );
            wp_mat4f_mul( &world, &scale, &combined );
            world.m[0][3] = position.x + (wp_f32)( ( sample * 37 ) % 101 - 50 ) * 0.06f;
            world.m[1][3] = position.y + (wp_f32)( ( sample * 19 ) % 79 - 39 ) * 0.06f;
            world.m[2][3] = position.z - 2 + (wp_f32)( ( sample * 11 ) % 31 - 15 ) * 0.1f;
            wp_cull_prepare_box( &box, &bounds, NULL, &world );
            CHECK( box.valid );
            a = wp_cull_sat_outside_mode( &box, &frustum, &full, 0 );
            b = wp_cull_sat_outside( &box, &frustum, &compact );
            CHECK( a == b && compact <= full );
            total_full += full;
            total_compact += compact;
            if( a )
            {
                ++rejected;
            }
            else
            {
                ++retained;
            }
        }
    }
    CHECK( rejected && retained && total_compact < total_full );
    /* A simple retained axis-aligned box gives an exact work-count comparison. */
    wp_mat4f_identity( &vp );
    wp_cull_prepare_frustum( &frustum, &vp );
    wp_cull_prepare_sat_frustum( &frustum );
    wp_mat4f_identity( &world );
    bounds.min.x = bounds.min.y = bounds.min.z = -2;
    bounds.max.x = bounds.max.y = bounds.max.z = 2;
    wp_cull_prepare_box( &box, &bounds, NULL, &world );
    CHECK( !wp_cull_box_outside( &box, &frustum, 1, &stage ) && stage == 4 );
    CHECK( !wp_cull_sat_outside_mode( &box, &frustum, &full, 0 ) );
    CHECK( !wp_cull_sat_outside( &box, &frustum, &compact ) );
    CHECK( full == 33 && compact == 12 );
    memset( &vp, 0, sizeof( vp ) );
    wp_cull_prepare_frustum( &frustum, &vp );
    wp_cull_prepare_sat_frustum( &frustum );
    CHECK( !wp_cull_sat_outside( &box, &frustum, &compact ) && !compact );
    wp_camera_destroy( camera );
    printf(
        "SAT compaction: 1024 full-axis classifications match; %lu -> %lu axis tests; boundary box 33 "
        "-> 12.\n",
        (unsigned long)total_full, (unsigned long)total_compact );
}

int main( void )
{
    matrix_cache();
    cache_depth_boundary();
    shared_transform_capture();
    shared_prefixes();
    retained_capture_scratch();
    retained_capture_paths();
    retained_capture_indices_and_batches();
    retained_capture_matrices( 0 );
    retained_capture_matrices( 2 );
    prefix_budget_fallback();
    transforms();
    hierarchy_limit();
    cull_order_fast_path();
    culling();
    conservative_bounds();
    oriented_bounds();
    refitted_bounds();
    adaptive_bounds();
    sphere_parity();
    separating_axes();
    sat_common_points();
    compact_sat_axes();
    retirement();
    shared_views();
#ifdef _WIN32
    own_completion();
#endif
    printf( "Native transform/cull contracts: %s\n", failures ? "FAIL" : "PASS" );
    return failures ? 1 : 0;
}
