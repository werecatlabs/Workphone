/**
 * @file wp_taskmanager.c
 * @brief Implementation of the C task manager API.
 */

#include "workphone_taskmanager.h"
#include "workphone_types.h"
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* Internal structures */

typedef struct wp_task
{
    wp_task_callback callback;
    void *user_data;
    wp_task_priority priority;
    volatile wp_task_state state;
    volatile wp_s32 ref_count;
    HANDLE completion_event;
    struct wp_task *next;
} wp_task;

typedef struct wp_task_queue
{
    wp_task *head;
    wp_task *tail;
    wp_u32 count;
    CRITICAL_SECTION lock;
} wp_task_queue;

typedef struct wp_worker_thread
{
    HANDLE thread;
    DWORD thread_id;
    volatile wp_s32 should_stop;
    struct wp_taskmanager *manager;
} wp_worker_thread;

typedef struct wp_taskmanager
{
    wp_worker_thread *workers;
    wp_u32 num_threads;
    wp_taskmanager_mode mode;
    wp_task_queue queues[4];
    volatile wp_u32 active_count;
    volatile wp_s32 shutdown;
    CRITICAL_SECTION mode_lock;
    HANDLE work_available;
    HANDLE all_complete;
} wp_taskmanager;

/* Forward declarations */
static void task_queue_init( wp_task_queue *queue );
static void task_queue_destroy( wp_task_queue *queue );
static void task_queue_push( wp_task_queue *queue, wp_task *task );
static wp_task *task_queue_pop( wp_task_queue *queue );
static wp_s32 task_queue_remove( wp_task_queue *queue, wp_task *task );
static wp_task *taskmanager_get_next_task( wp_taskmanager *manager );
static DWORD WINAPI worker_thread_proc( LPVOID param );
static void task_addref( wp_task *task );
static void task_release( wp_task *task );

/* =========================================================================
 * Task queue implementation
 * ====================================================================== */

static void task_queue_init( wp_task_queue *queue )
{
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    InitializeCriticalSection( &queue->lock );
}

static void task_queue_destroy( wp_task_queue *queue )
{
    EnterCriticalSection( &queue->lock );

    wp_task *current = queue->head;
    while( current )
    {
        wp_task *next = current->next;
        task_release( current );
        current = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;

    LeaveCriticalSection( &queue->lock );
    DeleteCriticalSection( &queue->lock );
}

static void task_queue_push( wp_task_queue *queue, wp_task *task )
{
    EnterCriticalSection( &queue->lock );

    task->next = NULL;

    if( queue->tail )
    {
        queue->tail->next = task;
        queue->tail = task;
    }
    else
    {
        queue->head = task;
        queue->tail = task;
    }

    queue->count++;
    task_addref( task );

    LeaveCriticalSection( &queue->lock );
}

static wp_task *task_queue_pop( wp_task_queue *queue )
{
    EnterCriticalSection( &queue->lock );

    wp_task *task = queue->head;

    if( task )
    {
        queue->head = task->next;
        if( !queue->head )
        {
            queue->tail = NULL;
        }
        task->next = NULL;
        queue->count--;
    }

    LeaveCriticalSection( &queue->lock );

    return task;
}

static wp_s32 task_queue_remove( wp_task_queue *queue, wp_task *task )
{
    EnterCriticalSection( &queue->lock );

    wp_task *current = queue->head;
    wp_task *prev = NULL;
    wp_s32 found = 0;

    while( current )
    {
        if( current == task )
        {
            if( prev )
            {
                prev->next = current->next;
            }
            else
            {
                queue->head = current->next;
            }

            if( queue->tail == current )
            {
                queue->tail = prev;
            }

            current->next = NULL;
            queue->count--;
            task_release( current );
            found = 1;
            break;
        }

        prev = current;
        current = current->next;
    }

    LeaveCriticalSection( &queue->lock );

    return found;
}

/* =========================================================================
 * Task reference counting
 * ====================================================================== */

static void task_addref( wp_task *task )
{
    if( task )
    {
        InterlockedIncrement( (LONG *)&task->ref_count );
    }
}

static void task_release( wp_task *task )
{
    if( task )
    {
        if( InterlockedDecrement( (LONG *)&task->ref_count ) == 0 )
        {
            if( task->completion_event )
            {
                CloseHandle( task->completion_event );
            }
            free( task );
        }
    }
}

/* =========================================================================
 * Worker thread implementation
 * ====================================================================== */

static wp_task *taskmanager_get_next_task( wp_taskmanager *manager )
{
    for( wp_s32 priority = WORKPHONE_TASK_PRIORITY_CRITICAL; priority >= WORKPHONE_TASK_PRIORITY_LOW;
         priority-- )
    {
        wp_task *task = task_queue_pop( &manager->queues[priority] );
        if( task )
        {
            return task;
        }
    }
    return NULL;
}

static DWORD WINAPI worker_thread_proc( LPVOID param )
{
    wp_worker_thread *worker = (wp_worker_thread *)param;
    wp_taskmanager *manager = worker->manager;

    while( !worker->should_stop && !manager->shutdown )
    {
        DWORD wait_result = WaitForSingleObject( manager->work_available, 100 );

        if( worker->should_stop || manager->shutdown )
        {
            break;
        }

        if( wait_result != WAIT_OBJECT_0 && wait_result != WAIT_TIMEOUT )
        {
            continue;
        }

        wp_task *task = taskmanager_get_next_task( manager );

        if( task )
        {
            if( task->state == WORKPHONE_TASK_STATE_IDLE )
            {
                InterlockedExchange( (LONG *)&task->state, WORKPHONE_TASK_STATE_RUNNING );
                InterlockedIncrement( (LONG *)&manager->active_count );

                EnterCriticalSection( &manager->mode_lock );
                wp_taskmanager_mode current_mode = manager->mode;
                LeaveCriticalSection( &manager->mode_lock );

                if( task->callback )
                {
                    task->callback( task->user_data );
                }

                InterlockedExchange( (LONG *)&task->state, WORKPHONE_TASK_STATE_COMPLETED );
                InterlockedDecrement( (LONG *)&manager->active_count );

                if( task->completion_event )
                {
                    SetEvent( task->completion_event );
                }

                if( manager->active_count == 0 )
                {
                    wp_u32 total_pending = 0;
                    for( wp_s32 i = 0; i < 4; i++ )
                    {
                        EnterCriticalSection( &manager->queues[i].lock );
                        total_pending += manager->queues[i].count;
                        LeaveCriticalSection( &manager->queues[i].lock );
                    }

                    if( total_pending == 0 )
                    {
                        SetEvent( manager->all_complete );
                    }
                }

                if( current_mode == WORKPHONE_TASKMANAGER_MODE_SEQUENTIAL )
                {
                    Sleep( 1 );
                }
            }

            task_release( task );
        }
    }

    return 0;
}

/* =========================================================================
 * Task Manager Lifecycle
 * ====================================================================== */

wp_taskmanager *wp_taskmanager_create( wp_u32 num_threads, wp_taskmanager_mode mode )
{
    if( num_threads == 0 )
    {
        SYSTEM_INFO sysinfo;
        GetSystemInfo( &sysinfo );
        num_threads = sysinfo.dwNumberOfProcessors;
    }

    if( num_threads < 1 )
    {
        num_threads = 1;
    }

    wp_taskmanager *manager = (wp_taskmanager *)calloc( 1, sizeof( wp_taskmanager ) );
    if( !manager )
    {
        return NULL;
    }

    manager->num_threads = num_threads;
    manager->mode = mode;
    manager->shutdown = 0;
    manager->active_count = 0;

    InitializeCriticalSection( &manager->mode_lock );

    for( wp_s32 i = 0; i < 4; i++ )
    {
        task_queue_init( &manager->queues[i] );
    }

    manager->work_available = CreateEvent( NULL, TRUE, FALSE, NULL );
    manager->all_complete = CreateEvent( NULL, TRUE, TRUE, NULL );

    if( !manager->work_available || !manager->all_complete )
    {
        wp_taskmanager_destroy( manager );
        return NULL;
    }

    manager->workers = (wp_worker_thread *)calloc( num_threads, sizeof( wp_worker_thread ) );
    if( !manager->workers )
    {
        wp_taskmanager_destroy( manager );
        return NULL;
    }

    for( wp_u32 i = 0; i < num_threads; i++ )
    {
        manager->workers[i].manager = manager;
        manager->workers[i].should_stop = 0;
        manager->workers[i].thread = CreateThread( NULL, 0, worker_thread_proc, &manager->workers[i], 0,
                                                   &manager->workers[i].thread_id );

        if( !manager->workers[i].thread )
        {
            for( wp_u32 j = 0; j < i; j++ )
            {
                manager->workers[j].should_stop = 1;
                WaitForSingleObject( manager->workers[j].thread, INFINITE );
                CloseHandle( manager->workers[j].thread );
            }
            wp_taskmanager_destroy( manager );
            return NULL;
        }
    }

    return manager;
}

void wp_taskmanager_destroy( wp_taskmanager *manager )
{
    if( !manager )
    {
        return;
    }

    manager->shutdown = 1;

    if( manager->work_available )
    {
        SetEvent( manager->work_available );
    }

    if( manager->workers )
    {
        for( wp_u32 i = 0; i < manager->num_threads; i++ )
        {
            if( manager->workers[i].thread )
            {
                manager->workers[i].should_stop = 1;
                WaitForSingleObject( manager->workers[i].thread, INFINITE );
                CloseHandle( manager->workers[i].thread );
            }
        }
        free( manager->workers );
    }

    for( wp_s32 i = 0; i < 4; i++ )
    {
        task_queue_destroy( &manager->queues[i] );
    }

    if( manager->work_available )
    {
        CloseHandle( manager->work_available );
    }

    if( manager->all_complete )
    {
        CloseHandle( manager->all_complete );
    }

    DeleteCriticalSection( &manager->mode_lock );

    free( manager );
}

/* =========================================================================
 * Task Creation and Management
 * ====================================================================== */

wp_task *wp_taskmanager_create_task( wp_taskmanager *manager, wp_task_callback callback, void *user_data,
                                     wp_task_priority priority )
{
    if( !manager || !callback )
    {
        return NULL;
    }

    if( priority < WORKPHONE_TASK_PRIORITY_LOW || priority > WORKPHONE_TASK_PRIORITY_CRITICAL )
    {
        priority = WORKPHONE_TASK_PRIORITY_NORMAL;
    }

    wp_task *task = (wp_task *)calloc( 1, sizeof( wp_task ) );
    if( !task )
    {
        return NULL;
    }

    task->callback = callback;
    task->user_data = user_data;
    task->priority = priority;
    task->state = WORKPHONE_TASK_STATE_IDLE;
    task->ref_count = 1;
    task->next = NULL;
    task->completion_event = CreateEvent( NULL, TRUE, FALSE, NULL );

    if( !task->completion_event )
    {
        free( task );
        return NULL;
    }

    return task;
}

wp_s32 wp_taskmanager_submit_task( wp_taskmanager *manager, wp_task *task )
{
    if( !manager || !task || manager->shutdown )
    {
        return 0;
    }

    if( task->state != WORKPHONE_TASK_STATE_IDLE )
    {
        return 0;
    }

    ResetEvent( task->completion_event );
    ResetEvent( manager->all_complete );

    task_queue_push( &manager->queues[task->priority], task );

    SetEvent( manager->work_available );

    return 1;
}

wp_s32 wp_taskmanager_cancel_task( wp_taskmanager *manager, wp_task *task )
{
    if( !manager || !task )
    {
        return 0;
    }

    if( task->state != WORKPHONE_TASK_STATE_IDLE )
    {
        return 0;
    }

    wp_s32 removed = task_queue_remove( &manager->queues[task->priority], task );

    if( removed )
    {
        InterlockedExchange( (LONG *)&task->state, WORKPHONE_TASK_STATE_CANCELLED );
        SetEvent( task->completion_event );
    }

    return removed;
}

void wp_taskmanager_destroy_task( wp_task *task )
{
    task_release( task );
}

/* =========================================================================
 * Task State Queries
 * ====================================================================== */

wp_task_state wp_task_get_state( const wp_task *task )
{
    if( !task )
    {
        return WORKPHONE_TASK_STATE_IDLE;
    }
    return task->state;
}

wp_s32 wp_task_is_complete( const wp_task *task )
{
    if( !task )
    {
        return 1;
    }
    return ( task->state == WORKPHONE_TASK_STATE_COMPLETED ||
             task->state == WORKPHONE_TASK_STATE_CANCELLED );
}

/* =========================================================================
 * Task Manager Control
 * ====================================================================== */

void wp_taskmanager_set_mode( wp_taskmanager *manager, wp_taskmanager_mode mode )
{
    if( !manager )
    {
        return;
    }

    EnterCriticalSection( &manager->mode_lock );
    manager->mode = mode;
    LeaveCriticalSection( &manager->mode_lock );
}

wp_taskmanager_mode wp_taskmanager_get_mode( const wp_taskmanager *manager )
{
    if( !manager )
    {
        return WORKPHONE_TASKMANAGER_MODE_PARALLEL;
    }

    wp_taskmanager_mode mode;
    EnterCriticalSection( (CRITICAL_SECTION *)&manager->mode_lock );
    mode = manager->mode;
    LeaveCriticalSection( (CRITICAL_SECTION *)&manager->mode_lock );

    return mode;
}

void wp_taskmanager_wait( wp_taskmanager *manager )
{
    if( !manager )
    {
        return;
    }

    WaitForSingleObject( manager->all_complete, INFINITE );
}

void wp_taskmanager_wait_task( wp_taskmanager *manager, wp_task *task )
{
    if( !manager || !task )
    {
        return;
    }

    if( task->completion_event )
    {
        WaitForSingleObject( task->completion_event, INFINITE );
    }
}

wp_u32 wp_taskmanager_get_pending_count( const wp_taskmanager *manager )
{
    if( !manager )
    {
        return 0;
    }

    wp_u32 total = 0;

    for( wp_s32 i = 0; i < 4; i++ )
    {
        EnterCriticalSection( (CRITICAL_SECTION *)&manager->queues[i].lock );
        total += manager->queues[i].count;
        LeaveCriticalSection( (CRITICAL_SECTION *)&manager->queues[i].lock );
    }

    return total;
}

wp_u32 wp_taskmanager_get_active_count( const wp_taskmanager *manager )
{
    if( !manager )
    {
        return 0;
    }

    return manager->active_count;
}

wp_u32 wp_taskmanager_get_num_threads( const wp_taskmanager *manager )
{
    if( !manager )
    {
        return 0;
    }

    return manager->num_threads;
}
