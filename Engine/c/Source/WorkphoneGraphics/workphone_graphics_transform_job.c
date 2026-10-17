#include "workphone_graphics_transform_job.h"
#include <stdlib.h>
#include <string.h>
#define WP_TRANSFORM_CHAIN_LIMIT 257u
#define WP_TRANSFORM_BYTE_LIMIT ( 64u * 1024u * 1024u )
#ifndef WP_TRANSFORM_HASH_CHAIN_LIMIT
#    define WP_TRANSFORM_HASH_CHAIN_LIMIT 64u
#endif

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

static wp_u32 wp_transform_node_bucket( const wp_scenenode *node, wp_u32 buckets )
{
    wp_u64 address = (wp_u64)(uintptr_t)node;
    wp_u32 hash = (wp_u32)( address >> 4 ) ^ (wp_u32)( address >> 32 );
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    hash ^= hash >> 16;
    return hash % buckets;
}

/* The same array stores packed unique nodes/next links and bucket heads in
 * independent fields. There are no more unique nodes than path references.
 * After local capture its batch fields become the existing prefix lookup. */
static wp_s32 wp_transform_hash_references( const wp_scenenode *const *nodes,
                                            const wp_transform_path *paths, wp_u32 count,
                                            wp_u32 total, wp_transform_reference *references,
                                            wp_u32 *indices, wp_u32 *unique,
                                            wp_u64 *probes, wp_u64 *collisions )
{
    const wp_scenenode *node;
    wp_u32 i, j, bucket, slot, steps;
    *unique = 0;
    for( i = 0; i < total; ++i )
    {
        references[i].node = NULL;
        references[i].reference = 0;
        references[i].batch = (wp_u32)-1;
    }
    for( i = 0; i < count; ++i )
    {
        node = nodes[i];
        for( j = 0; j < paths[i].count; ++j )
        {
            bucket = wp_transform_node_bucket( node, total );
            slot = references[bucket].batch;
            steps = 0;
            while( slot != (wp_u32)-1 )
            {
                ++*probes;
                if( references[slot].node == node )
                {
                    break;
                }
                ++*collisions;
                if( ++steps >= WP_TRANSFORM_HASH_CHAIN_LIMIT )
                {
                    return 0;
                }
                slot = references[slot].reference;
            }
            if( slot == (wp_u32)-1 )
            {
                slot = ( *unique )++;
                references[slot].node = node;
                references[slot].reference = references[bucket].batch;
                references[bucket].batch = slot;
            }
            indices[paths[i].start + j] = slot;
            node = wp_scenenode_get_parent( node );
        }
    }
    return 1;
}

/* Bounded hash collisions may require the original comparison-sort path. Pack
 * unique nodes at the front only after mapping each sorted reference to a local
 * index. A separate previous pointer avoids reading overwritten packed slots. */
static wp_u32 wp_transform_sort_references( const wp_scenenode *const *nodes,
                                             const wp_transform_path *paths, wp_u32 count,
                                             wp_u32 total, wp_transform_reference *references,
                                             wp_u32 *indices )
{
    const wp_scenenode *node, *previous = NULL;
    wp_u32 i, j, start, unique = 0;
    for( i = 0; i < count; ++i )
    {
        node = nodes[i];
        for( j = 0; j < paths[i].count; ++j )
        {
            start = paths[i].start + j;
            references[start].node = node;
            references[start].reference = start;
            node = wp_scenenode_get_parent( node );
        }
    }
    if( total )
    {
        qsort( references, total, sizeof( *references ), wp_transform_reference_compare );
    }
    for( i = 0; i < total; ++i )
    {
        node = references[i].node;
        if( !i || node != previous )
        {
            references[unique++].node = node;
            previous = node;
        }
        indices[references[i].reference] = unique - 1u;
    }
    return unique;
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
    wp_transform_job_clock clock;
    void *clock_context;
    wp_u64 clock_last;
    wp_s32 timing_valid;
    wp_transform_job_timing_stats timing;
};

static wp_u64 wp_transform_time_read( wp_transform_job_system *s, wp_u64 *last, wp_s32 *valid )
{
    wp_u64 now;
    if( !s->clock )
    {
        return 0;
    }
    now = s->clock( s->clock_context );
    if( now < *last )
    {
        *valid = 0;
    }
    *last = now;
    return now;
}

static void wp_transform_time_phase( wp_transform_job_system *s, wp_u64 *last,
                                      wp_s32 *valid, wp_u64 *phase )
{
    wp_u64 previous = *last;
    wp_u64 now = wp_transform_time_read( s, last, valid );
    *phase = now >= previous ? now - previous : 0;
}

wp_s32 wp_transform_job_system_set_clock( wp_transform_job_system *s,
                                           wp_transform_job_clock clock, void *context )
{
    if( !s || s->status == WP_TRANSFORM_RUNNING )
    {
        return 0;
    }
    s->clock = clock;
    s->clock_context = clock ? context : NULL;
    return 1;
}

wp_s32 wp_transform_job_system_get_timing_stats( const wp_transform_job_system *s,
                                                wp_transform_job_timing_stats *out )
{
    if( !out )
    {
        return 0;
    }
    memset( out, 0, sizeof( *out ) );
    if( !s || s->status != WP_TRANSFORM_COMPLETE || !s->timing_valid )
    {
        return 0;
    }
    *out = s->timing;
    return 1;
}

static void wp_transform_discard_capture_paths( const wp_transform_job_system *s,
                                                wp_transform_path *paths )
{
    /* Rejected capture may overwrite private inputs, but must not free reused
     * storage owned by the last completed job or touch its published worlds. */
    if( paths != s->paths )
    {
        free( paths );
    }
}

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
    wp_u64 stamp;
    if( !s || s->status != WP_TRANSFORM_RUNNING )
    {
        return;
    }
    stamp = wp_transform_time_read( s, &s->clock_last, &s->timing_valid );
    wp_task_group_wait( s->completion );
    if( s->destination && s->count )
    {
        memcpy( s->destination, s->worlds, (size_t)s->count * sizeof( wp_mat4f ) );
    }
    s->status = WP_TRANSFORM_COMPLETE;
    wp_transform_time_phase( s, &stamp, &s->timing_valid, &s->timing.wait_ns );
    s->clock_last = stamp;
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
    s->timing_valid = 0;
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
    wp_u32 i, j, depth, total = 0, unique = 0, batch_count, start, prefix_count = 0, reused,
                        reused_paths, reused_indices, reused_batches, reused_locals, reused_prefixes = 0,
                        batch_index;
    size_t bytes;
    wp_transform_job_timing_stats timing;
    wp_u64 stamp = 0;
    wp_s32 timing_valid;
    wp_u64 hash_probes = 0, hash_collisions = 0;
    wp_u32 hash_fallbacks = 0;
    if( !s || s->status == WP_TRANSFORM_RUNNING || ( count && !nodes ) )
    {
        return 0;
    }
    if( count > WP_TRANSFORM_BYTE_LIMIT /
                    ( sizeof( wp_transform_path ) + sizeof( wp_mat4f ) + sizeof( wp_transform_batch ) ) )
    {
        return 0;
    }
    memset( &timing, 0, sizeof( timing ) );
    timing_valid = s->clock != NULL;
    wp_transform_time_read( s, &stamp, &timing_valid );
    /* Before total path length is known, keep paths plus the prior scratch cache
     * within the capture limit. Prior completed result arrays are separately owned. */
    if( (size_t)s->capture_scratch_count * sizeof( *references ) >
        WP_TRANSFORM_BYTE_LIMIT - (size_t)count * sizeof( *paths ) )
    {
        free( s->capture_scratch );
        s->capture_scratch = NULL;
        s->capture_scratch_count = 0;
    }
    reused_paths = count && count == s->count && s->paths != NULL;
    paths = reused_paths ? s->paths
                         : ( count ? (wp_transform_path *)calloc( count, sizeof( *paths ) ) : NULL );
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
                wp_transform_discard_capture_paths( s, paths );
                return 0;
            }
            for( j = 0; j < depth; ++j )
            {
                if( chain[j] == node )
                {
                    wp_transform_discard_capture_paths( s, paths );
                    return 0;
                }
            }
            chain[depth++] = node;
            node = wp_scenenode_get_parent( node );
        }
        if( depth > WP_TRANSFORM_BYTE_LIMIT / sizeof( wp_mat4f ) - total )
        {
            wp_transform_discard_capture_paths( s, paths );
            return 0;
        }
        paths[i].start = total;
        paths[i].count = depth;
        total += depth;
    }
    batch_count = count / s->batch_size + ( count % s->batch_size != 0 );
    wp_transform_time_phase( s, &stamp, &timing_valid, &timing.hierarchy_ns );
    /* Deduplicate borrowed references on the owner to capture each unique node once.
     * Workers receive only owned matrices and integer path indices. Scratch and
     * retained arrays together must fit the snapshot limit. */
    bytes = (size_t)total * ( sizeof( wp_transform_reference ) + sizeof( wp_u32 ) ) +
            (size_t)count * ( sizeof( *paths ) + sizeof( wp_mat4f ) ) +
            (size_t)batch_count * sizeof( *batches );
    if( bytes > WP_TRANSFORM_BYTE_LIMIT )
    {
        wp_transform_discard_capture_paths( s, paths );
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
    reused_indices = total && total == s->stats.path_reference_count && s->indices != NULL;
    indices = reused_indices ? s->indices
                             : ( total ? (wp_u32 *)malloc( (size_t)total * sizeof( *indices ) ) : NULL );
    if( total && ( !references || !indices ) )
    {
        wp_transform_discard_capture_paths( s, paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        if( indices != s->indices )
        {
            free( indices );
        }
        return 0;
    }
    if( !wp_transform_hash_references( nodes, paths, count, total, references, indices,
                                       &unique, &hash_probes, &hash_collisions ) )
    {
        unique = wp_transform_sort_references( nodes, paths, count, total, references, indices );
        hash_fallbacks = 1;
    }
    bytes += (size_t)unique * sizeof( wp_mat4f );
    wp_transform_time_phase( s, &stamp, &timing_valid, &timing.unique_ns );
    if( bytes > WP_TRANSFORM_BYTE_LIMIT )
    {
        wp_transform_discard_capture_paths( s, paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        if( indices != s->indices )
        {
            free( indices );
        }
        return 0;
    }
    reused_locals = unique && unique == s->stats.local_matrix_count && s->locals != NULL;
    locals = reused_locals
                 ? s->locals
                 : ( unique ? (wp_mat4f *)malloc( (size_t)unique * sizeof( *locals ) ) : NULL );
    worlds = count ? (wp_mat4f *)malloc( (size_t)count * sizeof( *worlds ) ) : NULL;
    reused_batches = batch_count && batch_count == s->stats.batch_count && s->batches != NULL;
    batches =
        reused_batches
            ? s->batches
            : ( batch_count ? (wp_transform_batch *)malloc( (size_t)batch_count * sizeof( *batches ) )
                            : NULL );
    if( ( unique && !locals ) || ( count && !worlds ) || ( batch_count && !batches ) )
    {
        wp_transform_discard_capture_paths( s, paths );
        if( references )
        {
            memset( references, 0, (size_t)total * sizeof( *references ) );
        }
        if( indices != s->indices )
        {
            free( indices );
        }
        if( locals != s->locals )
        {
            free( locals );
        }
        free( worlds );
        if( batches != s->batches )
        {
            free( batches );
        }
        return 0;
    }
    for( i = 0; i < unique; ++i )
    {
        wp_scenenode_get_local_matrix( references[i].node, &locals[i] );
    }
    wp_transform_time_phase( s, &stamp, &timing_valid, &timing.locals_ns );
    if( total && s->batch_size > 1u && count > 1u )
    {
        /* Global deduplication already gave every node a dense local index.
         * Reuse reference scratch as an owner-only local-to-batch lookup instead
         * of sorting the same path references again by (batch,node). Each batch
         * visits a node once for its private prefix; no extra storage is needed. */
        for( i = 0; i < unique; ++i )
        {
            references[i].batch = (wp_u32)-1;
        }
        for( i = 0; i < count; ++i )
        {
            batch_index = i / s->batch_size;
            for( j = 0; j < paths[i].count; ++j )
            {
                start = indices[paths[i].start + j];
                if( references[start].batch != batch_index )
                {
                    references[start].batch = batch_index;
                    ++prefix_count;
                }
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
        reused_prefixes = s->prefixes != NULL && prefix_count == s->stats.matrix_multiply_count;
        if( reused_prefixes )
        {
            prefixes = s->prefixes;
            /* Drained workers left ready flags and worlds from the old hierarchy.
             * Rebuild the entire private cache before any new task can read it. */
            memset( prefixes, 0, (size_t)prefix_count * sizeof( *prefixes ) );
        }
        else
        {
            prefixes = (wp_transform_prefix *)calloc( prefix_count, sizeof( *prefixes ) );
        }
    }
    if( prefixes )
    {
        bytes += (size_t)prefix_count * sizeof( *prefixes );
        prefix_count = 0;
        for( i = 0; i < unique; ++i )
        {
            references[i].batch = (wp_u32)-1;
        }
        for( i = 0; i < count; ++i )
        {
            batch_index = i / s->batch_size;
            for( j = 0; j < paths[i].count; ++j )
            {
                start = paths[i].start + j;
                depth = indices[start];
                if( references[depth].batch != batch_index )
                {
                    references[depth].batch = batch_index;
                    references[depth].reference = prefix_count;
                    prefixes[prefix_count].local = depth;
                    ++prefix_count;
                }
                indices[start] = references[depth].reference;
            }
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
    if( reused_paths )
    {
        s->paths = NULL;
    }
    /* Prior workers are drained. Every private index and batch field is rebuilt;
     * only the public world matrices require separate transactional storage. */
    if( reused_indices )
    {
        s->indices = NULL;
    }
    if( reused_batches )
    {
        s->batches = NULL;
    }
    if( reused_locals )
    {
        s->locals = NULL;
    }
    if( reused_prefixes )
    {
        s->prefixes = NULL;
    }
    wp_transform_release( s );
    s->indices = indices;
    s->stats.output_count = count;
    s->stats.local_matrix_count = unique;
    s->stats.path_reference_count = total;
    s->stats.matrix_multiply_count = prefix_count;
    s->stats.batch_count = batch_count;
    s->stats.capture_hash_probes = hash_probes;
    s->stats.capture_hash_collisions = hash_collisions;
    s->stats.capture_hash_fallbacks = hash_fallbacks;
    s->stats.capture_scratch_allocations = total && !reused;
    s->stats.capture_scratch_reuses = reused;
    s->stats.capture_path_allocations = count && !reused_paths;
    s->stats.capture_path_reuses = reused_paths;
    s->stats.capture_index_allocations = total && !reused_indices;
    s->stats.capture_index_reuses = reused_indices;
    s->stats.capture_batch_allocations = batch_count && !reused_batches;
    s->stats.capture_batch_reuses = reused_batches;
    s->stats.capture_local_allocations = unique && !reused_locals;
    s->stats.capture_local_reuses = reused_locals;
    s->stats.capture_prefix_allocations = prefixes != NULL && !reused_prefixes;
    s->stats.capture_prefix_reuses = reused_prefixes;
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
    wp_transform_time_phase( s, &stamp, &timing_valid, &timing.prefixes_ns );
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
    wp_transform_time_phase( s, &stamp, &timing_valid, &timing.submit_ns );
    s->timing = timing;
    s->clock_last = stamp;
    s->timing_valid = timing_valid;
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
