#include "workphone_graphics_transform_job.h"
#include <stdlib.h>

#define WP_TRANSFORM_JOB_DEFAULT_BATCH 64u

typedef struct wp_transform_batch
{
    const wp_scenenode *const *nodes;
    wp_mat4f *out_matrix;
    wp_u32 count;
} wp_transform_batch;

struct wp_transform_job_system
{
    wp_threadpool *pool;
    wp_u32 batch_size;
    wp_transform_batch *batches;
    wp_u32 batch_capacity;
    wp_s32 pending;
};

static void wp_transform_batch_run( void *user_data )
{
    wp_transform_batch *batch = (wp_transform_batch *)user_data;
    wp_u32 i;
    for( i = 0; i < batch->count; ++i )
    {
        wp_scenenode_get_world_matrix( batch->nodes[i], &batch->out_matrix[i] );
    }
}

wp_transform_job_system *wp_transform_job_system_create( wp_threadpool *pool, wp_u32 batch_size )
{
    wp_transform_job_system *system = (wp_transform_job_system *)calloc( 1, sizeof( wp_transform_job_system ) );
    if( !system )
    {
        return NULL;
    }
    system->pool = pool;
    system->batch_size = batch_size ? batch_size : WP_TRANSFORM_JOB_DEFAULT_BATCH;
    return system;
}

void wp_transform_job_system_destroy( wp_transform_job_system *system )
{
    if( !system )
    {
        return;
    }
    wp_transform_job_system_wait( system );
    free( system->batches );
    free( system );
}

wp_s32 wp_transform_job_system_dispatch( wp_transform_job_system *system, const wp_scenenode *const *nodes, wp_mat4f *out_matrix, wp_u32 count )
{
    wp_u32 batch_count;
    wp_u32 i;

    if( !system || system->pending || ( count > 0 && ( !nodes || !out_matrix ) ) )
    {
        return 0;
    }
    if( count == 0 )
    {
        return 1;
    }

    batch_count = ( count + system->batch_size - 1 ) / system->batch_size;

    if( !system->pool || batch_count == 1 )
    {
        wp_transform_batch single;
        single.nodes = nodes;
        single.out_matrix = out_matrix;
        single.count = count;
        wp_transform_batch_run( &single );
        return 1;
    }

    if( batch_count > system->batch_capacity )
    {
        wp_transform_batch *grown = (wp_transform_batch *)realloc( system->batches, batch_count * sizeof( wp_transform_batch ) );
        if( !grown )
        {
            return 0;
        }
        system->batches = grown;
        system->batch_capacity = batch_count;
    }

    for( i = 0; i < batch_count; ++i )
    {
        wp_u32 start = i * system->batch_size;
        wp_u32 remaining = count - start;
        wp_transform_batch *batch = &system->batches[i];
        batch->nodes = nodes + start;
        batch->out_matrix = out_matrix + start;
        batch->count = remaining < system->batch_size ? remaining : system->batch_size;
    }

    system->pending = 1;

    for( i = 0; i < batch_count; ++i )
    {
        if( !wp_threadpool_submit( system->pool, wp_transform_batch_run, &system->batches[i] ) )
        {
            wp_transform_batch_run( &system->batches[i] );
        }
    }
    return 1;
}

void wp_transform_job_system_wait( wp_transform_job_system *system )
{
    if( !system || !system->pending )
    {
        return;
    }
    wp_threadpool_wait( system->pool );
    system->pending = 0;
}

wp_s32 wp_transform_job_system_process( wp_transform_job_system *system, const wp_scenenode *const *nodes, wp_mat4f *out_matrix, wp_u32 count )
{
    if( !wp_transform_job_system_dispatch( system, nodes, out_matrix, count ) )
    {
        return 0;
    }
    wp_transform_job_system_wait( system );
    return 1;
}
