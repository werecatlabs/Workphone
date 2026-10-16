#include "workphone_graphics_transform_job.h"
#include <stdlib.h>
#include <string.h>
#define WP_TRANSFORM_CHAIN_LIMIT 257u
#define WP_TRANSFORM_BYTE_LIMIT ( 64u * 1024u * 1024u )

typedef struct wp_transform_path
{
    wp_u32 start, count;
} wp_transform_path;

typedef struct wp_transform_batch
{
    const wp_mat4f *locals;
    const wp_transform_path *paths;
    wp_mat4f *worlds;
    wp_u32 start, count;
} wp_transform_batch;

struct wp_transform_job_system
{
    wp_threadpool *pool;
    wp_task_group *completion;
    wp_u32 batch_size, count;
    wp_u64 generation;
    wp_mat4f *locals, *worlds, *destination;
    wp_transform_path *paths;
    wp_transform_batch *batches;
    wp_transform_job_status status;
};

static void wp_transform_batch_run( void *data )
{
    wp_transform_batch *b = (wp_transform_batch *)data;
    wp_u32 i, j, index;
    wp_mat4f world, combined;
    for( i = 0; i < b->count; ++i )
    {
        index = b->start + i;
        wp_mat4f_identity( &world );
        j = b->paths[index].count;
        while( j )
        {
            --j;
            wp_mat4f_mul( &combined, &world, &b->locals[b->paths[index].start + j] );
            world = combined;
        }
        b->worlds[index] = world;
    }
}

static void wp_transform_release( wp_transform_job_system *s )
{
    free( s->locals );
    free( s->worlds );
    free( s->paths );
    free( s->batches );
    s->locals = NULL;
    s->worlds = NULL;
    s->paths = NULL;
    s->batches = NULL;
    s->count = 0;
    s->destination = NULL;
}

wp_transform_job_system *wp_transform_job_system_create( wp_threadpool *pool, wp_u32 batch_size )
{
    wp_transform_job_system *s = (wp_transform_job_system *)calloc( 1, sizeof( *s ) );
    if( !s )
    {
        return NULL;
    }
    s->completion = wp_task_group_create();
    if( !s->completion )
    {
        free( s );
        return NULL;
    }
    s->pool = pool;
    s->batch_size = batch_size ? batch_size : 64u;
    return s;
}

void wp_transform_job_system_wait( wp_transform_job_system *s )
{
    if( !s || s->status != WP_TRANSFORM_RUNNING )
    {
        return;
    }
    wp_task_group_wait( s->completion );
    if( s->destination && s->count )
    {
        memcpy( s->destination, s->worlds, (size_t)s->count * sizeof( wp_mat4f ) );
    }
    s->status = WP_TRANSFORM_COMPLETE;
}

void wp_transform_job_system_cancel( wp_transform_job_system *s )
{
    if( !s || s->status != WP_TRANSFORM_RUNNING )
    {
        return;
    }
    wp_task_group_wait( s->completion );
    s->destination = NULL;
    s->status = WP_TRANSFORM_CANCELLED;
}

void wp_transform_job_system_destroy( wp_transform_job_system *s )
{
    if( !s )
    {
        return;
    }
    wp_transform_job_system_cancel( s );
    wp_task_group_destroy( s->completion );
    wp_transform_release( s );
    free( s );
}

static wp_s32 wp_transform_dispatch( wp_transform_job_system *s, const wp_scenenode *const *nodes,
                                     wp_mat4f *destination, wp_u32 count, wp_u64 generation )
{
    const wp_scenenode *node, *chain[WP_TRANSFORM_CHAIN_LIMIT];
    wp_mat4f *locals, *worlds;
    wp_transform_path *paths;
    wp_transform_batch *batches;
    wp_u32 i, j, depth, total = 0, batch_count, start;
    size_t bytes;
    if( !s || s->status == WP_TRANSFORM_RUNNING || ( count && !nodes ) )
    {
        return 0;
    }
    if( count > WP_TRANSFORM_BYTE_LIMIT /
                    ( sizeof( wp_transform_path ) + sizeof( wp_mat4f ) + sizeof( wp_transform_batch ) ) )
    {
        return 0;
    }
    paths = count ? (wp_transform_path *)calloc( count, sizeof( *paths ) ) : NULL;
    if( count && !paths )
    {
        return 0;
    }
    for( i = 0; i < count; ++i )
    {
        depth = 0;
        node = nodes[i];
        while( node )
        {
            if( depth == WP_TRANSFORM_CHAIN_LIMIT )
            {
                free( paths );
                return 0;
            }
            for( j = 0; j < depth; ++j )
            {
                if( chain[j] == node )
                {
                    free( paths );
                    return 0;
                }
            }
            chain[depth++] = node;
            node = wp_scenenode_get_parent( node );
        }
        if( depth > WP_TRANSFORM_BYTE_LIMIT / sizeof( wp_mat4f ) - total )
        {
            free( paths );
            return 0;
        }
        paths[i].start = total;
        paths[i].count = depth;
        total += depth;
    }
    batch_count = count / s->batch_size + ( count % s->batch_size != 0 );
    bytes = (size_t)total * sizeof( wp_mat4f ) +
            (size_t)count * ( sizeof( *paths ) + sizeof( wp_mat4f ) ) +
            (size_t)batch_count * sizeof( *batches );
    if( bytes > WP_TRANSFORM_BYTE_LIMIT )
    {
        free( paths );
        return 0;
    }
    locals = total ? (wp_mat4f *)malloc( (size_t)total * sizeof( *locals ) ) : NULL;
    worlds = count ? (wp_mat4f *)malloc( (size_t)count * sizeof( *worlds ) ) : NULL;
    batches = batch_count ? (wp_transform_batch *)calloc( batch_count, sizeof( *batches ) ) : NULL;
    if( ( total && !locals ) || ( count && !worlds ) || ( batch_count && !batches ) )
    {
        free( paths );
        free( locals );
        free( worlds );
        free( batches );
        return 0;
    }
    /* All live hierarchy reads finish before the first task is submitted. */
    for( i = 0; i < count; ++i )
    {
        node = nodes[i];
        for( j = 0; j < paths[i].count; ++j )
        {
            wp_scenenode_get_local_matrix( node, &locals[paths[i].start + j] );
            node = wp_scenenode_get_parent( node );
        }
    }
    wp_transform_release( s );
    s->locals = locals;
    s->worlds = worlds;
    s->paths = paths;
    s->batches = batches;
    s->destination = destination;
    s->count = count;
    s->generation = generation;
    s->status = WP_TRANSFORM_RUNNING;
    for( i = 0; i < batch_count; ++i )
    {
        start = i * s->batch_size;
        batches[i].locals = locals;
        batches[i].worlds = worlds;
        batches[i].paths = paths;
        batches[i].start = start;
        batches[i].count = count - start < s->batch_size ? count - start : s->batch_size;
        if( !wp_task_group_submit( s->completion, s->pool, wp_transform_batch_run, &batches[i] ) )
        {
            wp_transform_batch_run( &batches[i] );
        }
    }
    return 1;
}

wp_s32 wp_transform_job_system_dispatch( wp_transform_job_system *s, const wp_scenenode *const *nodes,
                                         wp_mat4f *out, wp_u32 count )
{
    if( count && !out )
    {
        return 0;
    }
    return wp_transform_dispatch( s, nodes, out, count, 0 );
}

wp_s32 wp_transform_job_system_dispatch_snapshot( wp_transform_job_system *s,
                                                  const wp_scenenode *const *nodes, wp_u32 count,
                                                  wp_u64 generation )
{
    return wp_transform_dispatch( s, nodes, NULL, count, generation );
}

wp_s32 wp_transform_job_system_process( wp_transform_job_system *s, const wp_scenenode *const *nodes,
                                        wp_mat4f *out, wp_u32 count )
{
    if( !wp_transform_job_system_dispatch( s, nodes, out, count ) )
    {
        return 0;
    }
    wp_transform_job_system_wait( s );
    return 1;
}

wp_transform_job_status wp_transform_job_system_get_status( const wp_transform_job_system *s )
{
    return s ? s->status : WP_TRANSFORM_IDLE;
}

const wp_mat4f *wp_transform_job_system_get_result( const wp_transform_job_system *s, wp_u32 *count,
                                                    wp_u64 *generation )
{
    if( count )
    {
        *count = 0;
    }
    if( generation )
    {
        *generation = 0;
    }
    if( !s || s->status != WP_TRANSFORM_COMPLETE )
    {
        return NULL;
    }
    if( count )
    {
        *count = s->count;
    }
    if( generation )
    {
        *generation = s->generation;
    }
    return s->worlds;
}
