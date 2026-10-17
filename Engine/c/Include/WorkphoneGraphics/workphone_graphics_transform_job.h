#ifndef WORKPHONE_GRAPHICS_TRANSFORM_JOB_H
#define WORKPHONE_GRAPHICS_TRANSFORM_JOB_H

#include "workphone_types.h"
#include "workphone_matrix.h"
#include "workphone_threadpool.h"
#include "workphone_graphics_scenenode.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_transform_job_system wp_transform_job_system;

/* Owner elapsed phases. Submit includes inline composition with a NULL pool;
 * with a pool it measures submission, overlapping worker work. Wait measures
 * remaining task-group wait and destination publication, not worker CPU.
 * Inter-call gaps are excluded. Only completed accepted dispatches are readable. */
typedef struct wp_transform_job_timing_stats
{
    wp_u64 hierarchy_ns, unique_ns, locals_ns, prefixes_ns, submit_ns, wait_ns;
} wp_transform_job_timing_stats;
typedef wp_u64 ( *wp_transform_job_clock )( void *context );
/* Borrowed monotonic nanosecond clock, invoked only by the owner; never workers.
 * Reject changes while RUNNING. NULL disables calls. Survives wait/cancel until
 * replaced. Callback/context must survive dispatch and wait/cancel. */
wp_s32 wp_transform_job_system_set_clock( wp_transform_job_system *, wp_transform_job_clock, void * );
/* False/zero for unprofiled, running, cancelled or backwards-clock samples.
 * Rejected dispatch preserves prior accepted result and timing metadata. */
wp_s32 wp_transform_job_system_get_timing_stats( const wp_transform_job_system *,
                                                wp_transform_job_timing_stats * );

/* Metadata for the last accepted dispatch; reading it does not publish results.
 * Byte counts cover job-owned arrays and capture scratch, excluding allocator,
 * task-group/pool overhead and any previous result retained during capture. */
typedef struct wp_transform_job_stats
{
    wp_u32 output_count, local_matrix_count, path_reference_count;
    /* Planned/executed after draining: one multiplication per unique ancestor
     * within each cached batch; uncached fallback uses path_reference_count. */
    wp_u32 matrix_multiply_count, batch_count;
    /* Per accepted capture, for reference/hash/prefix scratch (not all allocations).
     * Exact-size reuse retains storage after scrubbing every borrowed pointer. */
    wp_u32 capture_scratch_allocations, capture_scratch_reuses;
    /* Private integer path records, separate from the published matrix output.
     * Reused when output count is unchanged and the previous tasks have drained. */
    wp_u32 capture_path_allocations, capture_path_reuses;
    /* Exact-size reuse of private path indices and drained worker descriptors. */
    wp_u32 capture_index_allocations, capture_index_reuses;
    wp_u32 capture_batch_allocations, capture_batch_reuses;
    /* Private local matrices and optional per-batch composed prefixes. Reused
     * at unchanged unique-local / cached-prefix counts, then fully rebuilt. */
    wp_u32 capture_local_allocations, capture_local_reuses;
    wp_u32 capture_prefix_allocations, capture_prefix_reuses;
    wp_u64 capture_scratch_bytes;
    wp_u64 retained_snapshot_bytes, capture_peak_bytes;
    /* Pointer comparisons in hash chains; unequal comparisons are collisions.
     * Buckets share the existing reference array (path_reference_count slots).
     * A bounded chain falls back to comparison sorting; counters then describe
     * the attempted hash portion, not the fallback sort. */
    wp_u64 capture_hash_probes, capture_hash_collisions;
    wp_u32 capture_hash_fallbacks;
} wp_transform_job_stats;

void wp_transform_job_system_get_stats( const wp_transform_job_system *, wp_transform_job_stats * );

typedef enum wp_transform_job_status
{
    WP_TRANSFORM_IDLE,
    WP_TRANSFORM_RUNNING,
    WP_TRANSFORM_COMPLETE,
    WP_TRANSFORM_CANCELLED
} wp_transform_job_status;

/* Owner-thread capture requires a stable hierarchy during dispatch only.
 * Workers use owned, deduplicated local matrices and integer ancestor paths,
 * caching composed prefixes privately within each batch. Reject paths deeper than 257 and
 * snapshots above 64 MiB. APIs are single-owner; pool is borrowed. */
wp_s32 wp_transform_job_system_dispatch_snapshot( wp_transform_job_system *, const wp_scenenode *const *,
                                                  wp_u32 count, wp_u64 generation );
wp_transform_job_status wp_transform_job_system_get_status( const wp_transform_job_system * );
/* Borrowed immutable result until next accepted dispatch or destruction. */
const wp_mat4f *wp_transform_job_system_get_result( const wp_transform_job_system *, wp_u32 *,
                                                    wp_u64 * );
/* Drains this job's tasks without publication. Destroy also cancels. */
void wp_transform_job_system_cancel( wp_transform_job_system * );

/* pool may be NULL (synchronous). batch_size 0 = default 64. Pool is not owned. */
wp_transform_job_system *wp_transform_job_system_create( wp_threadpool *pool, wp_u32 batch_size );
void wp_transform_job_system_destroy( wp_transform_job_system *system );

/* Nodes are copied before return and may then be edited/destroyed. Caller-owned
 * output stays valid until wait/cancel. Only wait publishes; cancellation and
 * destruction leave it untouched. Result availability never depends on polling. */
wp_s32 wp_transform_job_system_dispatch( wp_transform_job_system *system,
                                         const wp_scenenode *const *nodes, wp_mat4f *out_matrix,
                                         wp_u32 count );
void wp_transform_job_system_wait( wp_transform_job_system *system );
wp_s32 wp_transform_job_system_process( wp_transform_job_system *system,
                                        const wp_scenenode *const *nodes, wp_mat4f *out_matrix,
                                        wp_u32 count );

#ifdef __cplusplus
}
#endif

#endif
