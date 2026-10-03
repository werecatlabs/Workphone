/**
 * @file wp_jobqueue.h
 * @brief C API for job queue management.
 *
 * Provides a thread-safe job queue for asynchronous task execution.
 * Jobs can be executed on the primary thread, worker threads, or as coroutines.
 */

#ifndef WORKPHONE_JOBQUEUE_H
#define WORKPHONE_JOBQUEUE_H

#include <stddef.h>
#include <stdint.h>
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wp_jobqueue wp_jobqueue;
typedef struct wp_job wp_job;
typedef void ( *wp_job_callback )( void *user_data );

/**
 * @brief Job execution states.
 */
typedef enum wp_job_state
{
    WORKPHONE_JOB_STATE_IDLE = 0,      /* Job is idle and ready to queue */
    WORKPHONE_JOB_STATE_QUEUE = 1,     /* Job is queued for execution */
    WORKPHONE_JOB_STATE_EXECUTING = 2, /* Job is currently executing */
    WORKPHONE_JOB_STATE_FINISH = 3     /* Job has finished execution */
} wp_job_state;

/**
 * @brief Job types.
 */
typedef enum wp_job_type
{
    WORKPHONE_JOB_TYPE_REGULAR = 0,  /* Regular job for worker threads */
    WORKPHONE_JOB_TYPE_PRIMARY = 1,  /* Job that must run on primary thread */
    WORKPHONE_JOB_TYPE_COROUTINE = 2 /* Coroutine job (incremental execution) */
} wp_job_type;

/**
 * @brief Creates a new job queue.
 * @return Pointer to the created job queue, or NULL on failure.
 */
wp_jobqueue *wp_jobqueue_create( void );

/**
 * @brief Destroys a job queue and waits for all jobs to complete.
 * @param queue Pointer to the job queue to destroy.
 */
void wp_jobqueue_destroy( wp_jobqueue *queue );

/**
 * @brief Loads/initializes the job queue resources.
 * @param queue Pointer to the job queue.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_jobqueue_load( wp_jobqueue *queue );

/**
 * @brief Unloads/releases the job queue resources.
 * @param queue Pointer to the job queue.
 */
void wp_jobqueue_unload( wp_jobqueue *queue );

/**
 * @brief Creates a new job with the specified callback.
 * @param callback Function to execute for this job.
 * @param user_data User data to pass to the callback.
 * @param type Type of job (regular, primary, or coroutine).
 * @return Pointer to the created job, or NULL on failure.
 */
wp_job *wp_job_create( wp_job_callback callback, void *user_data, wp_job_type type );

/**
 * @brief Destroys a job.
 * @param job Pointer to the job to destroy.
 */
void wp_job_destroy( wp_job *job );

/**
 * @brief Gets the current state of a job.
 * @param job Pointer to the job.
 * @return The current state of the job.
 */
wp_job_state wp_job_get_state( wp_job *job );

/**
 * @brief Sets the state of a job.
 * @param job Pointer to the job.
 * @param state The new state.
 */
void wp_job_set_state( wp_job *job, wp_job_state state );

/**
 * @brief Gets the type of a job.
 * @param job Pointer to the job.
 * @return The type of the job.
 */
wp_job_type wp_job_get_type( wp_job *job );

/* -------------------------------------------------------------------------
 * Job Queue Operations
 * ---------------------------------------------------------------------- */

/**
 * @brief Adds a job to the queue for execution.
 * @param queue Pointer to the job queue.
 * @param job Pointer to the job to add.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_jobqueue_add_job( wp_jobqueue *queue, wp_job *job );

/**
 * @brief Updates the job queue and processes pending jobs.
 * @param queue Pointer to the job queue.
 * @param is_primary_thread 1 if called from primary thread, 0 otherwise.
 *
 * When called from the primary thread, processes one primary job, one coroutine step,
 * and optionally one regular job if no thread pool exists.
 * When called from worker threads, processes one regular job.
 */
void wp_jobqueue_update( wp_jobqueue *queue, wp_s32 is_primary_thread );

/**
 * @brief Checks if there are any pending jobs in the queue.
 * @param queue Pointer to the job queue.
 * @return 1 if there are jobs, 0 otherwise.
 */
wp_s32 wp_jobqueue_has_jobs( wp_jobqueue *queue );

/**
 * @brief Gets whether the job queue is running.
 * @param queue Pointer to the job queue.
 * @return 1 if running, 0 otherwise.
 */
wp_s32 wp_jobqueue_is_running( wp_jobqueue *queue );

/**
 * @brief Sets whether the job queue is running.
 * @param queue Pointer to the job queue.
 * @param running 1 to start running, 0 to stop.
 */
void wp_jobqueue_set_running( wp_jobqueue *queue, wp_s32 running );

/**
 * @brief Gets the update rate of the job queue.
 * @param queue Pointer to the job queue.
 * @return The update rate in seconds.
 */
wp_f32 wp_jobqueue_get_rate( wp_jobqueue *queue );

/**
 * @brief Sets the update rate of the job queue.
 * @param queue Pointer to the job queue.
 * @param rate The update rate in seconds.
 */
void wp_jobqueue_set_rate( wp_jobqueue *queue, wp_f32 rate );

/**
 * @brief Shuts down the job queue.
 * @param queue Pointer to the job queue.
 */
void wp_jobqueue_shutdown( wp_jobqueue *queue );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_JOBQUEUE_H */
