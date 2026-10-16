/**
 * @file workphone_graphics_cull_job.h
 * @brief C API for a per-view cull job.
 *
 * A cull job copies what it needs from a scene and camera into a private
 * snapshot (configure), then computes visibility and draw order against that
 * snapshot only (execute).  The outcome is a separately owned, immutable
 * result that the caller takes and destroys.
 *
 * Rules match wp_graphics_scene_render_with_submit: visible flag, visibility
 * mask, render callback present, conservative frustum test (unknown or
 * invalid bounds stay visible) and a stable sort by render queue group then
 * z-order.  No shared object state is modified.
 *
 * Scene, camera, object and mesh lifetimes are borrowed until result consumption. Destroy results before
 * destroying the scene. Mutation/capture/validation/submission share an owner;
 * execute alone may run on a worker. Camera and world matrices are frozen.
 * Lifecycle: IDLE -> READY (configure) -> COMPLETE | FAILED (execute).
 * Configure is rejected while READY; cancel or reset make the job
 * reconfigurable.  Execute reads only the snapshot, so it may run off the
 * owning thread if the caller does not touch the job concurrently.
 */

#ifndef WORKPHONE_GRAPHICS_CULL_JOB_H
#define WORKPHONE_GRAPHICS_CULL_JOB_H

#include <stdint.h>
#include "workphone_graphics_scene.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_graphics_cull_job wp_graphics_cull_job;
typedef struct wp_graphics_cull_result wp_graphics_cull_result;
typedef struct wp_transform_job_stats wp_transform_job_stats;
/* Copied transform preparation counters; include transform_job.h for the fields. */
void wp_graphics_cull_result_get_transform_stats( const wp_graphics_cull_result *,
                                                  wp_transform_job_stats * );

typedef enum wp_graphics_cull_status
{
    WP_GRAPHICS_CULL_STATUS_IDLE = 0,
    WP_GRAPHICS_CULL_STATUS_READY = 1,
    WP_GRAPHICS_CULL_STATUS_COMPLETE = 2,
    WP_GRAPHICS_CULL_STATUS_FAILED = 3,
    WP_GRAPHICS_CULL_STATUS_CANCELLED = 4
} wp_graphics_cull_status;

typedef enum wp_graphics_cull_pass
{
    /** Camera view: visibility mask plus frustum test. */
    WP_GRAPHICS_CULL_PASS_MAIN = 0,
    /** Shadow casters: casting objects survive camera frustum culling. */
    WP_GRAPHICS_CULL_PASS_SHADOW = 1
} wp_graphics_cull_pass;

typedef enum wp_graphics_cull_result_code
{
    WP_GRAPHICS_CULL_OK = 1,
    WP_GRAPHICS_CULL_ERROR = 0,
    WP_GRAPHICS_CULL_ERROR_INVALID_ARGUMENT = -1,
    WP_GRAPHICS_CULL_ERROR_INVALID_STATE = -2,
    WP_GRAPHICS_CULL_ERROR_OUT_OF_MEMORY = -3,
    WP_GRAPHICS_CULL_ERROR_STALE = -4
} wp_graphics_cull_result_code;

wp_graphics_cull_job *wp_graphics_cull_job_create( void );
/* Optional borrowed pool; owner-thread capture/wait precedes culling. */
wp_s32 wp_graphics_cull_job_set_threadpool( wp_graphics_cull_job *, wp_threadpool * );
wp_s32 wp_graphics_cull_result_validate_object( const wp_graphics_cull_result *,
                                                const wp_graphics_scene *, wp_s32 );
wp_s32 wp_graphics_cull_result_get_world_matrix( const wp_graphics_cull_result *, wp_s32, wp_mat4f * );
wp_s32 wp_graphics_cull_result_get_camera_matrices( const wp_graphics_cull_result *, wp_mat4f *,
                                                    wp_mat4f * );

/** @brief Destroys the job and its snapshot.  Results already taken stay valid. */
void wp_graphics_cull_job_destroy( wp_graphics_cull_job *job );

/**
 * @brief Snapshots the scene and view.  Rejected while READY; any prior
 *        unconsumed result is released.
 * @param camera NULL selects the scene active camera, then default camera;
 *               if none exists no frustum test is applied.
 * @param scene_generation Caller-defined generation stored in the result.
 * @return WP_GRAPHICS_CULL_OK or a negative error code.
 */
wp_s32 wp_graphics_cull_job_configure( wp_graphics_cull_job *job, const wp_graphics_scene *scene,
                                       const wp_camera *camera, wp_graphics_cull_pass pass,
                                       wp_u32 scene_generation );

/** @brief Runs the cull against the snapshot.  Requires READY status. */
wp_s32 wp_graphics_cull_job_execute( wp_graphics_cull_job *job );

/** @brief Cancels a READY job, discarding its snapshot. */
void wp_graphics_cull_job_cancel( wp_graphics_cull_job *job );

/** @brief Returns the job to idle, discarding snapshot and unconsumed result. */
void wp_graphics_cull_job_reset( wp_graphics_cull_job *job );

wp_graphics_cull_status wp_graphics_cull_job_get_status( const wp_graphics_cull_job *job );

/**
 * @brief Transfers ownership of the completed result to the caller.
 * @return The result, or NULL unless the job is COMPLETE.  Destroy it with
 *         wp_graphics_cull_result_destroy.  The job returns to idle.
 */
wp_graphics_cull_result *wp_graphics_cull_job_take_result( wp_graphics_cull_job *job );

void wp_graphics_cull_result_destroy( wp_graphics_cull_result *result );
wp_s32 wp_graphics_cull_result_get_candidate_count( const wp_graphics_cull_result *result );
wp_s32 wp_graphics_cull_result_get_visible_count( const wp_graphics_cull_result *result );

/** @brief Returns the i-th visible object in draw order, or NULL if out of range. */
wp_graphics_object *wp_graphics_cull_result_get_object( const wp_graphics_cull_result *result,
                                                        wp_s32 index );
wp_u32 wp_graphics_cull_result_get_scene_generation( const wp_graphics_cull_result *result );
wp_graphics_cull_pass wp_graphics_cull_result_get_pass( const wp_graphics_cull_result *result );

/**
 * @brief Checks a result at consumption time.
 * @return WP_GRAPHICS_CULL_OK if the generation matches and every object is
 *         still owned by the scene with unchanged object, mesh and ancestor revisions, otherwise
 * WP_GRAPHICS_CULL_ERROR_STALE.
 */
wp_s32 wp_graphics_cull_result_validate( const wp_graphics_cull_result *result,
                                         const wp_graphics_scene *scene, wp_u32 scene_generation );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_GRAPHICS_CULL_JOB_H */
