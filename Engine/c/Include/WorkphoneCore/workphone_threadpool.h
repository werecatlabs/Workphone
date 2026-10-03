/**
 * @file wp_threadpool.h
 * @brief C API for thread pool management.
 */

#ifndef WORKPHONE_THREADPOOL_H
#define WORKPHONE_THREADPOOL_H

#include <stddef.h>
#include "workphone_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------
 * Forward declarations and types
 * ---------------------------------------------------------------------- */

typedef struct wp_threadpool wp_threadpool;
typedef void ( *wp_task_func )( void *user_data );

/* -------------------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------------- */

/**
 * @brief Creates a new thread pool with the specified number of worker threads.
 * @param num_threads Number of worker threads to create (0 = auto-detect from CPU cores).
 * @return Pointer to the created thread pool, or NULL on failure.
 */
wp_threadpool *wp_threadpool_create( wp_u32 num_threads );

/**
 * @brief Destroys a thread pool and waits for all tasks to complete.
 * @param pool Pointer to the thread pool to destroy.
 */
void wp_threadpool_destroy( wp_threadpool *pool );

/* -------------------------------------------------------------------------
 * Task submission
 * ---------------------------------------------------------------------- */

/**
 * @brief Submits a task to the thread pool for execution.
 * @param pool Pointer to the thread pool.
 * @param task_func Function pointer to the task to execute.
 * @param user_data User data to pass to the task function.
 * @return 1 on success, 0 on failure.
 */
wp_s32 wp_threadpool_submit( wp_threadpool *pool, wp_task_func task_func, void *user_data );

/* -------------------------------------------------------------------------
 * Control
 * ---------------------------------------------------------------------- */

/**
 * @brief Waits for all pending tasks in the thread pool to complete.
 * @param pool Pointer to the thread pool.
 */
void wp_threadpool_wait( wp_threadpool *pool );

/**
 * @brief Gets the number of worker threads in the pool.
 * @param pool Pointer to the thread pool.
 * @return Number of worker threads.
 */
wp_u32 wp_threadpool_get_num_threads( const wp_threadpool *pool );

/**
 * @brief Gets the number of pending tasks in the queue.
 * @param pool Pointer to the thread pool.
 * @return Number of pending tasks.
 */
wp_u32 wp_threadpool_get_pending_tasks( const wp_threadpool *pool );

#ifdef __cplusplus
}
#endif

#endif /* WORKPHONE_THREADPOOL_H */
