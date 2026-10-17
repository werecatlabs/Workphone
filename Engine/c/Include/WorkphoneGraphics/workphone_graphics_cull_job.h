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
/* Disjoint elapsed owner intervals, not worker CPU/GPU time. Transform includes
 * dispatch/wait and node-array setup; visibility includes execute allocations.
 * Capture includes old-job retirement and entry/ancestor allocation; publication
 * includes result ownership transfer/cleanup. Scene wrapper setup is excluded.
 * Derived views are unprofiled; they do not inherit source phase durations. */
typedef struct wp_graphics_cull_timing_stats
{
    wp_u64 capture_ns, transform_ns, bounds_ns, groups_ns;
    wp_u64 view_ns, visibility_ns, sort_ns, publish_ns;
} wp_graphics_cull_timing_stats;
/* Reject while READY. Borrowed callback/context must survive configure/execute,
 * including worker execution if used there; never retained by results. Reset
 * preserves this option. NULL disables clock reads. A backwards clock invalidates
 * the timing sample without affecting visibility or result publication. */
wp_s32 wp_graphics_cull_job_set_clock( wp_graphics_cull_job *, wp_graphics_cull_clock, void *context );
/* False and zeroed for null/unprofiled/invalid-clock samples. */
wp_s32 wp_graphics_cull_result_get_timing_stats( const wp_graphics_cull_result *,
                                               wp_graphics_cull_timing_stats * );
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

/* Per-view stages; unknown/nonfinite bounds and invalid views are retained.
 * Shadow views bypass camera geometry tests entirely. */
typedef struct wp_graphics_cull_bounds_stats
{
    wp_u32 sphere_tests, sphere_rejections, sphere_inside;
    wp_u32 box_tests, box_rejections, fitted_obb_tests, aabb_box_tests, conservative_candidates;
    wp_u32 sat_tests, sat_rejections, sat_axis_tests, sat_fallbacks;
    wp_u32 group_tests, group_rejections, group_skipped_candidates, group_fallbacks;
    /* Entire padded bounds inside the view: candidate counts precede member
     * visibility/mask/callback filtering, not an exact count of tests avoided. */
    wp_u32 group_inside, group_inside_candidates;
} wp_graphics_cull_bounds_stats;

void wp_graphics_cull_result_get_bounds_stats( const wp_graphics_cull_result *,
                                               wp_graphics_cull_bounds_stats * );

/* Per-view ordering work, independent of shared candidate/transform capture.
 * Ordered survivors (including zero/one object) need no merge scratch. */
typedef struct wp_graphics_cull_sort_stats
{
    wp_u32 order_checks, merge_passes, scratch_allocations;
    wp_u64 scratch_bytes;
} wp_graphics_cull_sort_stats;

void wp_graphics_cull_result_get_sort_stats( const wp_graphics_cull_result *,
                                             wp_graphics_cull_sort_stats * );

/* Actual cull array storage for this result. Publication bytes cover shared
 * entries, ancestor revisions, group bounds and spatial map; count once across
 * derived views. View bytes cover owned indices and temporary array peaks only.
 * Excludes wrappers/allocator overhead, native transform buffers and pools.
 * Classification and merge scratch do not overlap in time. Mode is actual
 * capture storage: 0=flat, 1=blocks, 2=spatial; optional view-cache failure can
 * still use flat geometry tests in mode 2. Mapped/individual counts describe
 * spatial capture membership before per-view visibility/mask filtering. */
typedef struct wp_graphics_cull_storage_stats
{
    wp_u64 publication_bytes, group_bytes, map_bytes;
    wp_u64 view_index_bytes, view_array_peak_bytes, classification_scratch_bytes;
    wp_u32 group_mode, group_count, valid_group_count;
    wp_u32 mapped_candidates, individual_candidates;
} wp_graphics_cull_storage_stats;

void wp_graphics_cull_result_get_storage_stats( const wp_graphics_cull_result *,
                                                wp_graphics_cull_storage_stats * );
/* Default enabled. Owner-only A/B control; reject while READY. Derived views
 * inherit this policy. A disabled sphere stage leaves OBB classification intact. */
wp_s32 wp_graphics_cull_job_set_sphere_test( wp_graphics_cull_job *, wp_s32 enabled );
/* Default enabled; owner-only A/B switch with the same READY restriction.
 * Refines plane-test survivors using finite-frustum/box separating axes.
 * Invalid/unbounded frusta fall back to the conservative plane result. */
wp_s32 wp_graphics_cull_job_set_sat_test( wp_graphics_cull_job *, wp_s32 enabled );

/* Default disabled. Optional padded world bounds over 32 captured candidates,
 * built for >=64 candidates and shared by derived views. Fully contained blocks
 * bypass individual geometry tests; member filtering remains active. Invalid bounds or
 * unavailable storage use individual tests. Shadow views bypass camera rejection.
 * Owner-only; rejects changes while READY. */
wp_s32 wp_graphics_cull_job_set_group_test( wp_graphics_cull_job *, wp_s32 enabled );
/* Default disabled. Prefer widest-cell-axis spatial bins over capture-order
 * blocks, without reordering candidates. Unknown and cross-cell bounds remain
 * individual, preventing long bounds from loosening neighbouring bins.
 * Optional capture storage failure falls back to blocks if enabled, otherwise
 * flat tests. Per-view classification allocation failure also uses flat tests.
 * Independent derived-view classifications; same READY restriction. */
wp_s32 wp_graphics_cull_job_set_spatial_test( wp_graphics_cull_job *, wp_s32 enabled );

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

/* Derive an independently owned view from the complete immutable candidate
 * publication, including candidates excluded by the source view. Captures only
 * camera/masks and sorting; never dispatches transforms. Rejects stale scene,
 * object, mesh or ancestor revisions (source camera changes alone are allowed).
 * NULL camera selects active/default. Source may be destroyed after success.
 * Scene/resources remain borrowed. Derive/destroy/validate share the owner thread.
 * Returns NULL on stale inputs, invalid pass or allocation failure. */
wp_graphics_cull_result *wp_graphics_cull_result_create_view( const wp_graphics_cull_result *,
                                                              const wp_camera *, wp_graphics_cull_pass );
/* Transform work performed specifically for this view: zero for derived views.
 * get_transform_stats describes their shared publication and must not be summed
 * as fresh work for each derived view. */
wp_u32 wp_graphics_cull_result_get_transform_dispatch_count( const wp_graphics_cull_result * );

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
