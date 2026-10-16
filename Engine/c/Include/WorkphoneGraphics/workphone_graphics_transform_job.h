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

/* pool may be NULL (synchronous). batch_size 0 = default 64. Pool is not owned. */
wp_transform_job_system *wp_transform_job_system_create( wp_threadpool *pool, wp_u32 batch_size );
void wp_transform_job_system_destroy( wp_transform_job_system *system );

/* Starts computing world matrices; nodes/out_matrix must stay valid until wait returns. */
wp_s32 wp_transform_job_system_dispatch( wp_transform_job_system *system, const wp_scenenode *const *nodes, wp_mat4f *out_matrix, wp_u32 count );
void wp_transform_job_system_wait( wp_transform_job_system *system );
wp_s32 wp_transform_job_system_process( wp_transform_job_system *system, const wp_scenenode *const *nodes, wp_mat4f *out_matrix, wp_u32 count );

#ifdef __cplusplus
}
#endif

#endif
