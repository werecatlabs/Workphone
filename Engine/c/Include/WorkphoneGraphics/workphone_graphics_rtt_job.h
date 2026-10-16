#ifndef WORKPHONE_GRAPHICS_RTT_JOB_H
#define WORKPHONE_GRAPHICS_RTT_JOB_H

#include "workphone_types.h"
#include "workphone_graphics_viewport.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_graphics_rtt_job wp_graphics_rtt_job;
typedef enum wp_rtt_status {
    WP_RTT_IDLE, WP_RTT_READY, WP_RTT_RUNNING, WP_RTT_SUCCEEDED,
    WP_RTT_FAILED, WP_RTT_CANCELLED
} wp_rtt_status;
typedef enum wp_rtt_result {
    WP_RTT_OK = 1, WP_RTT_INVALID = -1, WP_RTT_BUSY = -2,
    WP_RTT_NOT_READY = -3, WP_RTT_STALE = -4, WP_RTT_CANCEL = -5,
    WP_RTT_BACKEND_FAILED = -6, WP_RTT_HAZARD = -7,
    WP_RTT_UNSUPPORTED = -8, WP_RTT_WRONG_THREAD = -9
} wp_rtt_result;

/* First execution tier: synchronous render-owner work. No worker GPU dispatch.
 * All descriptors are copied. Contexts are held until reconfigure/destroy through
 * the optional paired retain/release callbacks; otherwise the caller keeps them
 * alive. A dependency callback reports OK only after immutable result publication,
 * NOT_READY while pending, STALE/other failure when invalid. No cull job is implied.
 * Generations are supplied/checked by the resource owner, never inferred from
 * raw addresses. validate runs before binding and again before publication. */
typedef enum wp_rtt_format { WP_RTT_RGBA8_UNORM = 0, WP_RTT_RGBA16_FLOAT = 1 } wp_rtt_format;
typedef enum wp_rtt_colour_space { WP_RTT_LEGACY_LDR = 0, WP_RTT_SCENE_LINEAR = 1 } wp_rtt_colour_space;
typedef struct wp_graphics_rtt_desc {
    wp_rtt_format format;
    wp_rtt_colour_space colour_space;
    wp_u64 renderer_generation, target_generation, scene_generation, view_generation;
    wp_s32 width, height;
    wp_viewport_i viewport;
    wp_s32 clear_colour, clear_depth;
    wp_f32 colour[4], depth;
} wp_graphics_rtt_desc;

typedef wp_rtt_result (*wp_rtt_check_func)(void *, const wp_graphics_rtt_desc *);
typedef wp_rtt_result (*wp_rtt_finish_func)(void *, wp_s32 publish);
typedef struct wp_graphics_rtt_ops {
    /* Paired optional context ownership. retain must be transactional on failure. */
    wp_s32 (*retain)(void *);
    void (*release)(void *);
    wp_rtt_check_func dependency;
    wp_rtt_check_func validate;
    /* begin may fail partially; finish(0) must still release/restore its state. */
    wp_rtt_check_func begin;
    wp_rtt_check_func draw;
    /* Restores state on every path; publishes staged output only when requested.
     * A failed finish must leave the last complete output intact. */
    wp_rtt_finish_func finish;
} wp_graphics_rtt_ops;

wp_graphics_rtt_job *wp_graphics_rtt_job_create(void);
/* Single render-owner API. Reentry while RUNNING rejects mutation/destruction.
 * Cancellation before finish is cooperative; finish(0) follows the callback.
 * finish is the non-cancellable publication/restore boundary (cancel returns BUSY).
 * No methods are thread safe; drain synchronous execution before owner teardown. */
wp_rtt_result wp_graphics_rtt_job_configure(wp_graphics_rtt_job *,
    const wp_graphics_rtt_desc *, const wp_graphics_rtt_ops *, void *context);
wp_rtt_result wp_graphics_rtt_job_execute(wp_graphics_rtt_job *);
wp_rtt_result wp_graphics_rtt_job_cancel(wp_graphics_rtt_job *);
wp_rtt_status wp_graphics_rtt_job_status(const wp_graphics_rtt_job *);
wp_rtt_result wp_graphics_rtt_job_result(const wp_graphics_rtt_job *);
wp_rtt_result wp_graphics_rtt_job_destroy(wp_graphics_rtt_job *);

#ifdef __cplusplus
}
#endif
#endif
