/* Windows QPC headless CPU benchmark. Production cull/transform algorithms;
 * recording renderer boundary from the scene-job contract, with no GPU work. */
#ifdef _MSC_VER
#    define _CRT_SECURE_NO_WARNINGS
#endif
#include "WorkphoneGraphicsCullBenchmark.h"
#include <stdio.h>
#ifdef _WIN32
#    include "workphone_graphics_scene.h"
#    include "workphone_graphics_cull_job.h"
#    include "workphone_graphics_transform_job.h"
#    include "workphone_graphics_mesh.h"
#    include "workphone_graphics_object.h"
#    include "workphone_graphics_scenenode.h"
#    include "workphone_graphics_camera.h"
#    include "workphone_threadpool.h"
#    include <windows.h>
#    include <stdlib.h>
#    include <string.h>
#    include <locale.h>
#    include <errno.h>
#    define WP_BENCH_SAMPLE_LIMIT 250000u

typedef struct wp_bench_sample
{
    double prepare_ns, retire_ns;
    wp_graphics_cull_bounds_stats bounds;
    wp_graphics_cull_storage_stats storage;
    wp_graphics_cull_timing_stats timing;
    wp_u32 visible, multiplies;
} wp_bench_sample;

static double wp_bench_frequency;
static int wp_bench_profile_phases;

static wp_u64 wp_bench_clock( void *context )
{
    LARGE_INTEGER value;
    (void)context;
    QueryPerformanceCounter( &value );
    /* Split seconds/remainder to avoid overflowing ticks * 1e9. */
    return (wp_u64)( value.QuadPart / (LONGLONG)wp_bench_frequency ) * 1000000000u +
           (wp_u64)( (double)( value.QuadPart % (LONGLONG)wp_bench_frequency ) *
                     1.0e9 / wp_bench_frequency );
}

static double wp_bench_now( void )
{
    LARGE_INTEGER value;
    QueryPerformanceCounter( &value );
    return (double)value.QuadPart / wp_bench_frequency;
}

static int wp_bench_number( const char *text, double minimum, double maximum, double *out )
{
    char *end;
    double value;
    errno = 0;
    value = strtod( text, &end );
    if( errno || end == text || *end || !( value >= minimum && value <= maximum ) )
    {
        return 0;
    }
    *out = value;
    return 1;
}

static int wp_bench_parity( wp_graphics_scene *scene, wp_camera *camera, wp_u32 expected )
{
    wp_graphics_cull_result *flat, *spatial;
    wp_mat4f a, b;
    wp_u32 i;
    int valid;
    wp_graphics_scene_set_cull_flags( scene, 3 );
    flat = wp_graphics_scene_prepare_cull( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN );
    wp_graphics_scene_set_cull_flags( scene, 11 );
    spatial = wp_graphics_scene_prepare_cull( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN );
    valid = flat && spatial && (wp_u32)wp_graphics_cull_result_get_visible_count( flat ) == expected &&
            (wp_u32)wp_graphics_cull_result_get_visible_count( spatial ) == expected;
    for( i = 0; valid && i < expected; ++i )
    {
        valid = wp_graphics_cull_result_get_object( flat, (wp_s32)i ) ==
                    wp_graphics_cull_result_get_object( spatial, (wp_s32)i ) &&
                wp_graphics_cull_result_get_world_matrix( flat, (wp_s32)i, &a ) &&
                wp_graphics_cull_result_get_world_matrix( spatial, (wp_s32)i, &b ) &&
                memcmp( &a, &b, sizeof( a ) ) == 0;
    }
    wp_graphics_cull_result_destroy( flat );
    wp_graphics_cull_result_destroy( spatial );
    return valid;
}

static int wp_bench_frame( wp_graphics_scene *scene, wp_camera *camera, wp_u32 expected,
                           wp_bench_sample *sample )
{
    wp_graphics_cull_result *result;
    wp_transform_job_stats transforms;
    double begin, captured, retire, end;
    begin = wp_bench_now();
    result = wp_graphics_scene_prepare_cull_profiled( scene, camera, WP_GRAPHICS_CULL_PASS_MAIN,
                 wp_bench_profile_phases ? wp_bench_clock : NULL, NULL );
    captured = wp_bench_now();
    if( !result )
    {
        return 0;
    }
    sample->visible = (wp_u32)wp_graphics_cull_result_get_visible_count( result );
    wp_graphics_cull_result_get_bounds_stats( result, &sample->bounds );
    wp_graphics_cull_result_get_storage_stats( result, &sample->storage );
    wp_graphics_cull_result_get_transform_stats( result, &transforms );
    sample->multiplies = transforms.matrix_multiply_count;
    if( !wp_graphics_cull_result_get_timing_stats( result, &sample->timing ) && wp_bench_profile_phases )
    {
        wp_graphics_cull_result_destroy( result );
        return 0;
    }
    retire = wp_bench_now();
    wp_graphics_cull_result_destroy( result );
    end = wp_bench_now();
    sample->prepare_ns = ( captured - begin ) * 1.0e9;
    sample->retire_ns = ( end - retire ) * 1.0e9;
    return sample->visible == expected;
}

int wp_graphics_cull_benchmark_main( int argc, char **argv )
{
    wp_graphics_scene *scene = NULL;
    wp_graphics_mesh *mesh = NULL;
    wp_camera *camera = NULL;
    wp_threadpool *pool = NULL;
    wp_scenenode *parent, *node;
    wp_graphics_object *object;
    wp_bench_sample *samples = NULL, warm;
    wp_aabb3f bounds = { { -.2f, -.2f, -2.2f }, { .2f, .2f, -1.8f } };
    wp_vec3f position;
    LARGE_INTEGER frequency;
    FILE *file = NULL;
    double values[6], begin, warmup, seconds;
    wp_u32 objects, pairs, seed, workers, pair, order, mode, i, count, cluster, expected;
    int result = 1;
    setlocale( LC_NUMERIC, "C" );
    if( ( argc != 9 && argc != 10 ) ||
        ( argc == 10 && strcmp( argv[9], "0" ) && strcmp( argv[9], "1" ) ) ||
        !wp_bench_number( argv[3], 64, 50000, &values[0] ) ||
        !wp_bench_number( argv[4], 0, 60, &values[1] ) ||
        !wp_bench_number( argv[5], .01, 120, &values[2] ) ||
        !wp_bench_number( argv[6], 1, 10, &values[3] ) ||
        !wp_bench_number( argv[7], 1, 4294967295.0, &values[4] ) ||
        !wp_bench_number( argv[8], 0, 16, &values[5] ) )
    {
        fprintf( stderr,
                 "Usage: --cull-benchmark csv objects[64..50000] warmup-s measurement-s pairs seed "
                 "workers [profile-phases=0|1]\n" );
        return 2;
    }
    objects = (wp_u32)values[0];
    pairs = (wp_u32)values[3];
    seed = (wp_u32)values[4];
    workers = (wp_u32)values[5];
    if( values[0] != objects || values[3] != pairs || values[4] != seed || values[5] != workers )
    {
        return 2;
    }
    warmup = values[1];
    wp_bench_profile_phases = argc == 10 && !strcmp( argv[9], "1" );
    seconds = values[2];
    expected = ( objects + 9u ) / 10u;
    if( !QueryPerformanceFrequency( &frequency ) || frequency.QuadPart <= 0 )
    {
        return 1;
    }
    wp_bench_frequency = (double)frequency.QuadPart;
    samples = (wp_bench_sample *)malloc( (size_t)WP_BENCH_SAMPLE_LIMIT * sizeof( *samples ) );
    scene = wp_graphics_scene_create();
    mesh = wp_graphics_mesh_create();
    camera = wp_camera_create();
    if( !samples || !scene || !mesh || !camera )
    {
        goto cleanup;
    }
    memset( samples, 0, (size_t)WP_BENCH_SAMPLE_LIMIT * sizeof( *samples ) );
    if( workers )
    {
        pool = wp_threadpool_create( workers );
        if( !pool )
        {
            goto cleanup;
        }
    }
    wp_graphics_scene_set_transform_pool( scene, pool );
    wp_graphics_mesh_set_local_aabb( mesh, bounds );
    parent = wp_graphics_scene_create_node( scene );
    if( !parent )
    {
        goto cleanup;
    }
    for( i = 0; i < objects; ++i )
    {
        object = wp_graphics_scene_create_object( scene );
        node = wp_graphics_scene_create_node( scene );
        if( !object || !node )
        {
            goto cleanup;
        }
        wp_graphics_object_set_mesh( object, mesh );
        wp_scenenode_add_child( parent, node );
        wp_scenenode_attach_object( node, object );
        cluster = i % 10u;
        seed = seed * 1664525u + 1013904223u;
        position.x = ( cluster ? (wp_f32)( 5u + ( cluster % 4u ) * 110u ) : 0.0f ) +
                     (wp_f32)( seed & 255u ) * .0001f;
        seed = seed * 1664525u + 1013904223u;
        position.y = ( cluster ? (wp_f32)( 3u + ( cluster / 4u ) * 95u ) : 0.0f ) +
                     (wp_f32)( seed & 255u ) * .0001f;
        position.z = 0;
        wp_scenenode_set_position( node, position );
        wp_graphics_object_set_render_queue_group( object, i % 3u );
        wp_graphics_object_set_z_order( object, i % 5u );
    }
    file = fopen( argv[2], "w" );
    if( !file )
    {
        goto cleanup;
    }
    fprintf( file,
             "pair,order,mode,frame,prepare_ns,retire_ns,lifecycle_ns,candidates,visible,multiplies,"
             "sphere_tests,box_tests,sat_tests,group_tests,group_rejections,group_inside,publication_"
             "bytes,view_peak_bytes,map_bytes,class_bytes,phases_available,capture_ns,transform_ns,"
             "bounds_ns,groups_ns,view_ns,visibility_ns,sort_ns,publish_ns\n" );
    fprintf( stderr, "Headless CPU only; QPC %.0f Hz; sample-buffer %lu bytes; no GPU/readback.\n",
             wp_bench_frequency, (unsigned long)( WP_BENCH_SAMPLE_LIMIT * sizeof( *samples ) ) );
    for( pair = 0; pair < pairs; ++pair )
    {
        if( !wp_bench_parity( scene, camera, expected ) )
        {
            goto cleanup;
        }
        for( order = 0; order < 2; ++order )
        {
            mode = ( pair + order ) % 2u;
            wp_graphics_scene_set_cull_flags( scene, mode ? 11u : 3u );
            begin = wp_bench_now();
            do
            {
                if( !wp_bench_frame( scene, camera, expected, &warm ) )
                {
                    goto cleanup;
                }
            } while( wp_bench_now() - begin < warmup );
            count = 0;
            begin = wp_bench_now();
            do
            {
                if( count == WP_BENCH_SAMPLE_LIMIT )
                {
                    fprintf( stderr, "Sample capacity exceeded; no truncated run accepted.\n" );
                    goto cleanup;
                }
                if( !wp_bench_frame( scene, camera, expected, &samples[count] ) )
                {
                    goto cleanup;
                }
                ++count;
            } while( wp_bench_now() - begin < seconds );
            /* Raw output and expensive parity validation stay outside measurement. */
            for( i = 0; i < count; ++i )
            {
                wp_bench_sample *s = &samples[i];
                fprintf( file,
                         "%lu,%lu,%s,%lu,%.0f,%.0f,%.0f,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.0f,%.0f,%."
                         "0f,%.0f,%d,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f\n",
                         (unsigned long)( pair + 1u ), (unsigned long)order, mode ? "spatial" : "flat",
                         (unsigned long)i, s->prepare_ns, s->retire_ns, s->prepare_ns + s->retire_ns,
                         (unsigned long)objects, (unsigned long)s->visible, (unsigned long)s->multiplies,
                         (unsigned long)s->bounds.sphere_tests, (unsigned long)s->bounds.box_tests,
                         (unsigned long)s->bounds.sat_tests, (unsigned long)s->bounds.group_tests,
                         (unsigned long)s->bounds.group_rejections,
                         (unsigned long)s->bounds.group_inside, (double)s->storage.publication_bytes,
                         (double)s->storage.view_array_peak_bytes, (double)s->storage.map_bytes,
                         (double)s->storage.classification_scratch_bytes, wp_bench_profile_phases,
                         (double)s->timing.capture_ns, (double)s->timing.transform_ns,
                         (double)s->timing.bounds_ns, (double)s->timing.groups_ns,
                         (double)s->timing.view_ns, (double)s->timing.visibility_ns,
                         (double)s->timing.sort_ns, (double)s->timing.publish_ns );
            }
            if( ferror( file ) || fflush( file ) )
            {
                goto cleanup;
            }
            fprintf( stderr,
                     "pair %lu order %lu %s: %lu frames; parity verified outside timed interval.\n",
                     (unsigned long)( pair + 1u ), (unsigned long)order, mode ? "spatial" : "flat",
                     (unsigned long)count );
            if( !wp_bench_parity( scene, camera, expected ) )
            {
                goto cleanup;
            }
        }
    }
    result = 0;
cleanup:
    if( file && fclose( file ) )
    {
        result = 1;
    }
    wp_graphics_scene_destroy( scene );
    wp_threadpool_destroy( pool );
    wp_graphics_mesh_destroy( mesh );
    wp_camera_destroy( camera );
    free( samples );
    if( result )
    {
        fprintf( stderr, "Benchmark failed; partial output is not valid evidence.\n" );
    }
    return result;
}
#else
int wp_graphics_cull_benchmark_main( int argc, char **argv )
{
    (void)argc;
    (void)argv;
    fprintf( stderr, "The cull benchmark requires Windows QPC.\n" );
    return 2;
}
#endif
