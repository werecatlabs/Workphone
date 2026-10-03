/**
 * @file wp_jobqueue.c
 * @brief Implementation of the C job queue API.
 */

#include "workphone_jobqueue.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Internal structures */

typedef struct wp_job
{
    wp_job_callback callback;
    void *user_data;
    wp_job_type type;
    volatile wp_job_state state;
    volatile wp_s32 ref_count;
    struct wp_job *next;
} wp_job;

typedef struct wp_job_node
{
    wp_job *job;
    struct wp_job_node *next;
} wp_job_node;

typedef struct wp_job_list
{
    wp_job_node *head;
    wp_job_node *tail;
    wp_u32 count;
    CRITICAL_SECTION lock;
} wp_job_list;

typedef struct wp_jobqueue
{
    wp_job_list primary_jobs;
    wp_job_list regular_jobs;
    wp_job_list coroutine_jobs;
    volatile wp_s32 is_running;
    volatile wp_s32 is_loaded;
    wp_f32 update_rate;
    CRITICAL_SECTION state_lock;
} wp_jobqueue;

/* Forward declarations */
static void job_list_init( wp_job_list *list );
static void job_list_destroy( wp_job_list *list );
static void job_list_push( wp_job_list *list, wp_job *job );
static wp_job *job_list_pop( wp_job_list *list );
static wp_s32 job_list_remove( wp_job_list *list, wp_job *job );
static wp_s32 job_list_is_empty( wp_job_list *list );
static void job_addref( wp_job *job );
static void job_release( wp_job *job );

/* =========================================================================
 * Job list implementation
 * ====================================================================== */

static void job_list_init( wp_job_list *list )
{
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    InitializeCriticalSection( &list->lock );
}

static void job_list_destroy( wp_job_list *list )
{
    EnterCriticalSection( &list->lock );

    wp_job_node *current = list->head;
    while( current )
    {
        wp_job_node *next = current->next;
        if( current->job )
        {
            job_release( current->job );
        }
        free( current );
        current = next;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;

    LeaveCriticalSection( &list->lock );
    DeleteCriticalSection( &list->lock );
}

static void job_list_push( wp_job_list *list, wp_job *job )
{
    if( !job )
    {
        return;
    }

    wp_job_node *node = (wp_job_node *)malloc( sizeof( wp_job_node ) );
    if( !node )
    {
        return;
    }

    job_addref( job );
    node->job = job;
    node->next = NULL;

    EnterCriticalSection( &list->lock );

    if( list->tail )
    {
        list->tail->next = node;
        list->tail = node;
    }
    else
    {
        list->head = node;
        list->tail = node;
    }
    list->count++;

    LeaveCriticalSection( &list->lock );
}

static wp_job *job_list_pop( wp_job_list *list )
{
    wp_job *job = NULL;

    EnterCriticalSection( &list->lock );

    if( list->head )
    {
        wp_job_node *node = list->head;
        job = node->job;
        list->head = node->next;

        if( !list->head )
        {
            list->tail = NULL;
        }

        list->count--;
        free( node );
    }

    LeaveCriticalSection( &list->lock );

    return job;
}

static wp_s32 job_list_remove( wp_job_list *list, wp_job *job )
{
    wp_s32 found = 0;

    EnterCriticalSection( &list->lock );

    wp_job_node *prev = NULL;
    wp_job_node *current = list->head;

    while( current )
    {
        if( current->job == job )
        {
            if( prev )
            {
                prev->next = current->next;
            }
            else
            {
                list->head = current->next;
            }

            if( current == list->tail )
            {
                list->tail = prev;
            }

            job_release( current->job );
            free( current );
            list->count--;
            found = 1;
            break;
        }

        prev = current;
        current = current->next;
    }

    LeaveCriticalSection( &list->lock );

    return found;
}

static wp_s32 job_list_is_empty( wp_job_list *list )
{
    wp_s32 empty;

    EnterCriticalSection( &list->lock );
    empty = ( list->count == 0 );
    LeaveCriticalSection( &list->lock );

    return empty;
}

/* =========================================================================
 * Job reference counting
 * ====================================================================== */

static void job_addref( wp_job *job )
{
    if( job )
    {
        InterlockedIncrement( (LONG *)&job->ref_count );
    }
}

static void job_release( wp_job *job )
{
    if( job )
    {
        if( InterlockedDecrement( (LONG *)&job->ref_count ) == 0 )
        {
            free( job );
        }
    }
}

/* =========================================================================
 * Job Queue Lifecycle
 * ====================================================================== */

wp_jobqueue *wp_jobqueue_create( void )
{
    wp_jobqueue *queue = (wp_jobqueue *)malloc( sizeof( wp_jobqueue ) );
    if( !queue )
    {
        return NULL;
    }

    memset( queue, 0, sizeof( wp_jobqueue ) );

    job_list_init( &queue->primary_jobs );
    job_list_init( &queue->regular_jobs );
    job_list_init( &queue->coroutine_jobs );

    InitializeCriticalSection( &queue->state_lock );

    queue->is_running = 1;
    queue->is_loaded = 0;
    queue->update_rate = 1.0f / 15.0f;

    return queue;
}

void wp_jobqueue_destroy( wp_jobqueue *queue )
{
    if( !queue )
    {
        return;
    }

    wp_jobqueue_unload( queue );

    job_list_destroy( &queue->primary_jobs );
    job_list_destroy( &queue->regular_jobs );
    job_list_destroy( &queue->coroutine_jobs );

    DeleteCriticalSection( &queue->state_lock );

    free( queue );
}

wp_s32 wp_jobqueue_load( wp_jobqueue *queue )
{
    if( !queue )
    {
        return 0;
    }

    EnterCriticalSection( &queue->state_lock );
    queue->is_loaded = 1;
    LeaveCriticalSection( &queue->state_lock );

    return 1;
}

void wp_jobqueue_unload( wp_jobqueue *queue )
{
    if( !queue )
    {
        return;
    }

    EnterCriticalSection( &queue->state_lock );

    /* Wait for all jobs to finish executing */
    wp_job *job;

    /* Wait for coroutine jobs */
    while( ( job = job_list_pop( &queue->coroutine_jobs ) ) != NULL )
    {
        while( job->state == WORKPHONE_JOB_STATE_EXECUTING )
        {
            Sleep( 1 );
        }
        job_release( job );
    }

    /* Wait for primary jobs */
    while( ( job = job_list_pop( &queue->primary_jobs ) ) != NULL )
    {
        while( job->state == WORKPHONE_JOB_STATE_EXECUTING )
        {
            Sleep( 1 );
        }
        job_release( job );
    }

    /* Wait for regular jobs */
    while( ( job = job_list_pop( &queue->regular_jobs ) ) != NULL )
    {
        while( job->state == WORKPHONE_JOB_STATE_EXECUTING )
        {
            Sleep( 1 );
        }
        job_release( job );
    }

    queue->is_loaded = 0;

    LeaveCriticalSection( &queue->state_lock );
}

/* =========================================================================
 * Job Creation and Management
 * ====================================================================== */

wp_job *wp_job_create( wp_job_callback callback, void *user_data, wp_job_type type )
{
    if( !callback )
    {
        return NULL;
    }

    wp_job *job = (wp_job *)malloc( sizeof( wp_job ) );
    if( !job )
    {
        return NULL;
    }

    job->callback = callback;
    job->user_data = user_data;
    job->type = type;
    job->state = WORKPHONE_JOB_STATE_IDLE;
    job->ref_count = 1;
    job->next = NULL;

    return job;
}

void wp_job_destroy( wp_job *job )
{
    if( job )
    {
        job_release( job );
    }
}

wp_job_state wp_job_get_state( wp_job *job )
{
    if( !job )
    {
        return WORKPHONE_JOB_STATE_IDLE;
    }

    return job->state;
}

void wp_job_set_state( wp_job *job, wp_job_state state )
{
    if( job )
    {
        job->state = state;
    }
}

wp_job_type wp_job_get_type( wp_job *job )
{
    if( !job )
    {
        return WORKPHONE_JOB_TYPE_REGULAR;
    }

    return job->type;
}

/* =========================================================================
 * Job Queue Operations
 * ====================================================================== */

wp_s32 wp_jobqueue_add_job( wp_jobqueue *queue, wp_job *job )
{
    if( !queue || !job )
    {
        return 0;
    }

    if( !queue->is_running )
    {
        return 0;
    }

    /* Set job state to queued */
    job->state = WORKPHONE_JOB_STATE_QUEUE;

    /* Add to appropriate queue based on job type */
    switch( job->type )
    {
    case WORKPHONE_JOB_TYPE_PRIMARY:
        job_list_push( &queue->primary_jobs, job );
        break;

    case WORKPHONE_JOB_TYPE_COROUTINE:
        job_list_push( &queue->coroutine_jobs, job );
        break;

    case WORKPHONE_JOB_TYPE_REGULAR:
    default:
        job_list_push( &queue->regular_jobs, job );
        break;
    }

    return 1;
}

void wp_jobqueue_update( wp_jobqueue *queue, wp_s32 is_primary_thread )
{
    if( !queue )
    {
        return;
    }

    if( is_primary_thread )
    {
        /* Process one coroutine job step */
        wp_job_node *coroutine_node = NULL;

        EnterCriticalSection( &queue->coroutine_jobs.lock );
        coroutine_node = queue->coroutine_jobs.head;
        LeaveCriticalSection( &queue->coroutine_jobs.lock );

        if( coroutine_node && coroutine_node->job )
        {
            wp_job *job = coroutine_node->job;

            if( job->state == WORKPHONE_JOB_STATE_QUEUE )
            {
                job->state = WORKPHONE_JOB_STATE_EXECUTING;
            }

            if( job->state == WORKPHONE_JOB_STATE_EXECUTING )
            {
                /* Execute one step of the coroutine */
                if( job->callback )
                {
                    job->callback( job->user_data );
                }

                /* For simplicity, mark as finished after one step */
                /* A full implementation would support incremental execution */
                job->state = WORKPHONE_JOB_STATE_FINISH;

                /* Remove from coroutine jobs list */
                job_list_remove( &queue->coroutine_jobs, job );
            }
        }

        /* Process one primary job */
        wp_job *job = job_list_pop( &queue->primary_jobs );
        if( job )
        {
            job->state = WORKPHONE_JOB_STATE_EXECUTING;

            if( job->callback )
            {
                job->callback( job->user_data );
            }

            job->state = WORKPHONE_JOB_STATE_FINISH;
            job_release( job );
        }

        /* If no thread pool, also process regular jobs on primary thread */
        job = job_list_pop( &queue->regular_jobs );
        if( job )
        {
            job->state = WORKPHONE_JOB_STATE_EXECUTING;

            if( job->callback )
            {
                job->callback( job->user_data );
            }

            job->state = WORKPHONE_JOB_STATE_FINISH;
            job_release( job );
        }
    }
    else
    {
        /* Worker thread: process regular jobs */
        wp_job *job = job_list_pop( &queue->regular_jobs );
        if( job )
        {
            job->state = WORKPHONE_JOB_STATE_EXECUTING;

            if( job->callback )
            {
                job->callback( job->user_data );
            }

            job->state = WORKPHONE_JOB_STATE_FINISH;
            job_release( job );
        }
    }
}

wp_s32 wp_jobqueue_has_jobs( wp_jobqueue *queue )
{
    if( !queue )
    {
        return 0;
    }

    return !job_list_is_empty( &queue->primary_jobs ) || !job_list_is_empty( &queue->regular_jobs ) ||
           !job_list_is_empty( &queue->coroutine_jobs );
}

wp_s32 wp_jobqueue_is_running( wp_jobqueue *queue )
{
    if( !queue )
    {
        return 0;
    }

    wp_s32 running;
    EnterCriticalSection( &queue->state_lock );
    running = queue->is_running;
    LeaveCriticalSection( &queue->state_lock );

    return running;
}

void wp_jobqueue_set_running( wp_jobqueue *queue, wp_s32 running )
{
    if( !queue )
    {
        return;
    }

    EnterCriticalSection( &queue->state_lock );
    queue->is_running = running;
    LeaveCriticalSection( &queue->state_lock );
}

wp_f32 wp_jobqueue_get_rate( wp_jobqueue *queue )
{
    if( !queue )
    {
        return 0.0f;
    }

    wp_f32 rate;
    EnterCriticalSection( &queue->state_lock );
    rate = queue->update_rate;
    LeaveCriticalSection( &queue->state_lock );

    return rate;
}

void wp_jobqueue_set_rate( wp_jobqueue *queue, wp_f32 rate )
{
    if( !queue )
    {
        return;
    }

    EnterCriticalSection( &queue->state_lock );
    queue->update_rate = rate;
    LeaveCriticalSection( &queue->state_lock );
}

void wp_jobqueue_shutdown( wp_jobqueue *queue )
{
    if( !queue )
    {
        return;
    }

    wp_jobqueue_set_running( queue, 0 );
    wp_jobqueue_unload( queue );
}
