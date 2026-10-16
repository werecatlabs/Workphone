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

typedef enum wp_transform_job_status
{
    WP_TRANSFORM_IDLE,
    WP_TRANSFORM_RUNNING,
    WP_TRANSFORM_COMPLETE,
    WP_TRANSFORM_CANCELLED
} wp_transform_job_status;

/* Owner-thread capture requires a stable hierarchy during dispatch only.
 * Workers use owned local-matrix chains. Reject paths deeper than 257 and
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
