#include "workphone_graphics_transform_job.h"
#include <stdlib.h>
#include <string.h>
#define WP_TRANSFORM_CHAIN_LIMIT 257u
#define WP_TRANSFORM_BYTE_LIMIT ( 64u * 1024u * 1024u )

typedef struct wp_transform_path
{
    wp_u32 start, count;
} wp_transform_path;

typedef struct wp_transform_reference
{
    const wp_scenenode *node;
    wp_u32 reference, batch;
} wp_transform_reference;

static int wp_transform_reference_compare( const void *left, const void *right )
{
    uintptr_t a = (uintptr_t)( (const wp_transform_reference *)left )->node;
    uintptr_t b = (uintptr_t)( (const wp_transform_reference *)right )->node;
    return a < b ? -1 : a > b ? 1 : 0;
}

static int wp_transform_batch_reference_compare( const void *left, const void *right )
{
    const wp_transform_reference *a = (const wp_transform_reference *)left;
    const wp_transform_reference *b = (const wp_transform_reference *)right;
    if( a->batch != b->batch )
    {
        return a->batch < b->batch ? -1 : 1;
    }
    return wp_transform_reference_compare( left, right );
}

/* Each slot belongs to exactly one batch, including shared ancestors. No worker
 * waits on another worker, reads live nodes, or writes another batch's cache. */
typedef struct wp_transform_prefix
{
    wp_mat4f world;
    wp_u32 local, parent, ready;
} wp_transform_prefix;

typedef struct wp_transform_batch
{
    const wp_mat4f *locals;
    const wp_transform_path *paths;
    const wp_u32 *indices;
    wp_transform_prefix *prefixes;
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
    wp_transform_prefix *prefixes;
    wp_u32 *indices;
    wp_transform_job_stats stats;
    wp_transform_job_status status;
    wp_transform_reference *capture_scratch;
    wp_u32 capture_scratch_count;
};

static void wp_transform_batch_run( void *data )
{
    wp_transform_batch *b = (wp_transform_batch *)data;
    wp_u32 i, j, index;
    wp_transform_prefix *prefix;
    wp_mat4f identity, world, combined;
    wp_mat4f_identity( &identity );
    for( i = 0; i < b->count; ++i )
    {
        index = b->start + i;
        world = identity;
        j = b->paths[index].count;
        while( j )
        {
            --j;
            if( !b->prefixes )
            {
                wp_mat4f_mul( &combined, &world, &b->locals[b->indices[b->paths[index].start + j]] );
                world = combined;
            }
            else
            {
                prefix = &b->prefixes[b->indices[b->paths[index].start + j]];
                if( !prefix->ready )
                {
                    wp_mat4f_mul(
                        &prefix->world,
                        prefix->parent == (wp_u32)-1 ? &identity : &b->prefixes[prefix->parent].world,
                        &b->locals[prefix->local] );
                    prefix->ready = 1;
                }
                world = prefix->world;
            }
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
    free( s->indices );
    free( s->prefixes );
    s->locals = NULL;
    s->worlds = NULL;
    s->paths = NULL;
    s->batches = NULL;
    s->indices = NULL;
    s->prefixes = NULL;
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
    free( s->capture_scratch );
    free( s );
}

static wp_s32 wp_transform_dispatch( wp_transform_job_system *s, const wp_scenenode *const *nodes,
                                     wp_mat4f *destination, wp_u32 count, wp_u64 generation )
{
    const wp_scenenode *node, *chain[WP_TRANSFORM_CHAIN_LIMIT];
    wp_mat4f *locals, *worlds;
    wp_transform_path *paths;
    wp_transform_batch *batches;
    wp_transform_reference *references;
    wp_transform_prefix *prefixes;
    wp_u32 *indices;
    wp_u32 i, j, depth, total = 0, unique = 0, batch_count, start, prefix_count = 0, reused;
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
    /* Before total path length is known, keep paths plus the prior scratch cache
     * within the capture limit. Prior completed result arrays are separately owned. */
    if( (size_t)s->capture_scratch_count * sizeof( *references ) >
        WP_TRANSFORM_BYTE_LIMIT - (size_t)count * sizeof( *paths ) )
    {
        free( s->capture_scratch );
        s->capture_scratch = NULL;
        s->capture_scratch_count = 0;
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
    /* Sort borrowed references on the owner to capture each unique node once.
     * Workers receive only owned matrices and integer path indices. Scratch and
     * retained arrays together must fit the snapshot limit. */
    bytes = (size_t)total * ( sizeof( wp_transform_reference ) + sizeof( wp_u32 ) ) +
            (size_t)count * ( sizeof( *paths ) + sizeof( wp_mat4f ) ) +
            (size_t)batch_count * sizeof( *batches );
    if( bytes > WP_TRANSFORM_BYTE_LIMIT )
    {
        free( paths );
        return 0;
    }
    reused = total && s->capture_scratch_count == total;
    if( s->capture_scratch_count != total )
    {
        /* Exact-size reuse avoids oversized retained capacity and staging two
         * scratch buffers. Losing this cache cannot invalidate a completed result. */
        free( s->capture_scratch );
        s->capture_scratch = NULL;
        s->capture_scratch_count = 0;
        if( total )
        {
            s->capture_scratch =
                (wp_transform_reference *)malloc( (size_t)total * sizeof( *references ) );
            if( s->capture_scratch )
            {
                s->capture_scratch_count = total;
            }
        }
    }
    references = s->capture_scratch;
    indices = total ? (wp_u32 *)malloc( (size_t)total * sizeof( *indices ) ) : NULL;
    if( total && ( !references || !indices ) )
    {
        free( paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        free( indices );
        return 0;
    }
    for( i = 0; i < count; ++i )
    {
        node = nodes[i];
        for( j = 0; j < paths[i].count; ++j )
        {
            start = paths[i].start + j;
            references[start].node = node;
            references[start].reference = start;
            references[start].batch = i / s->batch_size;
            node = wp_scenenode_get_parent( node );
        }
    }
    if( total )
    {
        qsort( references, total, sizeof( *references ), wp_transform_reference_compare );
    }
    for( i = 0; i < total; ++i )
    {
        if( !i || references[i].node != references[i - 1].node )
        {
            ++unique;
        }
    }
    bytes += (size_t)unique * sizeof( wp_mat4f );
    if( bytes > WP_TRANSFORM_BYTE_LIMIT )
    {
        free( paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        free( indices );
        return 0;
    }
    locals = unique ? (wp_mat4f *)malloc( (size_t)unique * sizeof( *locals ) ) : NULL;
    worlds = count ? (wp_mat4f *)malloc( (size_t)count * sizeof( *worlds ) ) : NULL;
    batches = batch_count ? (wp_transform_batch *)calloc( batch_count, sizeof( *batches ) ) : NULL;
    if( ( unique && !locals ) || ( count && !worlds ) || ( batch_count && !batches ) )
    {
        free( paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        free( indices );
        free( locals );
        free( worlds );
        free( batches );
        return 0;
    }
    unique = 0;
    for( i = 0; i < total; ++i )
    {
        if( !i || references[i].node != references[i - 1].node )
        {
            wp_scenenode_get_local_matrix( references[i].node, &locals[unique] );
            ++unique;
        }
        indices[references[i].reference] = unique - 1u;
    }
    if( total && s->batch_size > 1u && count > 1u )
    {
        qsort( references, total, sizeof( *references ), wp_transform_batch_reference_compare );
        for( i = 0; i < total; ++i )
        {
            if( !i || references[i].batch != references[i - 1].batch ||
                references[i].node != references[i - 1].node )
            {
                ++prefix_count;
            }
        }
    }
    else
    {
        prefix_count = total;
    }
    /* Keep the original algorithm when a cache removes no work, exceeds the
     * capture budget, or cannot be allocated. Caching is an optional optimization. */
    prefixes = NULL;
    if( prefix_count < total &&
        (size_t)prefix_count <= ( WP_TRANSFORM_BYTE_LIMIT - bytes ) / sizeof( *prefixes ) )
    {
        prefixes = (wp_transform_prefix *)calloc( prefix_count, sizeof( *prefixes ) );
    }
    if( prefixes )
    {
        bytes += (size_t)prefix_count * sizeof( *prefixes );
        prefix_count = 0;
        for( i = 0; i < total; ++i )
        {
            if( !i || references[i].batch != references[i - 1].batch ||
                references[i].node != references[i - 1].node )
            {
                prefixes[prefix_count].local = indices[references[i].reference];
                ++prefix_count;
            }
            indices[references[i].reference] = prefix_count - 1u;
        }
        /* The owner resolves parent slots while paths are still available. Repeated
         * slots have the same parent; all pointers are discarded before submission. */
        for( i = 0; i < count; ++i )
        {
            for( j = 0; j < paths[i].count; ++j )
            {
                start = paths[i].start + j;
                prefixes[indices[start]].parent =
                    j + 1u < paths[i].count ? indices[start + 1u] : (wp_u32)-1;
            }
        }
    }
    else
    {
        prefix_count = total;
    }
    /* Scratch remains owner-only and contains no borrowed scene pointers while
     * workers run, including after source-node retirement or cancellation. */
    if( references )
    {
        memset( references, 0, (size_t)total * sizeof( *references ) );
    }
    wp_transform_release( s );
    s->indices = indices;
    s->stats.output_count = count;
    s->stats.local_matrix_count = unique;
    s->stats.path_reference_count = total;
    s->stats.matrix_multiply_count = prefix_count;
    s->stats.batch_count = batch_count;
    s->stats.capture_scratch_allocations = total && !reused;
    s->stats.capture_scratch_reuses = reused;
    s->stats.capture_scratch_bytes = (size_t)total * sizeof( *references );
    s->stats.capture_peak_bytes = bytes;
    s->stats.retained_snapshot_bytes = bytes - (size_t)total * sizeof( wp_transform_reference );
    s->locals = locals;
    s->worlds = worlds;
    s->paths = paths;
    s->batches = batches;
    s->prefixes = prefixes;
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
        batches[i].indices = indices;
        batches[i].prefixes = prefixes;
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

void wp_transform_job_system_get_stats( const wp_transform_job_system *s, wp_transform_job_stats *out )
{
    if( !out )
    {
        return;
    }
    if( s )
    {
        *out = s->stats;
    }
    else
    {
        memset( out, 0, sizeof( *out ) );
    }
}
