#ifndef __ITaskManager_H_
#define __ITaskManager_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/System/TaskLock.hpp>

namespace workphone
{

    /**
     * @brief Interface for managing tasks and their execution across multiple threads.
     *
     * The ITaskManager provides a centralized system for coordinating task execution with
     * different execution modes (parallel, sequential, lock-step, etc.). It manages task
     * lifecycles, synchronization, and job distribution across managed tasks.
     *
     * @remarks
     * This interface supports various execution strategies through its State enumeration,
     * allowing flexible control over how tasks are processed in the system.
     *
     * @see ITask
     * @see IJob
     * @see TaskLock
     */
    class WPCore_API ITaskManager : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration of task manager execution states.
         *
         * Defines the various operational modes that control how the task manager
         * schedules and executes tasks.
         */
        enum class State
        {
            None,        ///< No specific state; manager is idle or uninitialized
            FreeStep,    ///< Tasks execute freely without synchronization constraints
            Parallel,    ///< Tasks execute in parallel across available threads
            LockStep,    ///< Tasks execute in synchronized lock-step fashion
            FixedStep,   ///< Tasks execute at fixed time intervals
            Sequential,  ///< Tasks execute one after another in sequence
            Shutdown,    ///< Manager is shutting down; no new tasks accepted

            Count  ///< Total number of states (for iteration purposes)
        };

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived task manager implementations.
         */
        ~ITaskManager() override;

        /**
         * @brief Adds a job to all managed tasks for processing.
         *
         * Distributes the specified job to all tasks currently managed by this task manager.
         * Each task will queue and process the job according to its execution policy.
         *
         * @param job The job to be queued for processing across all tasks.
         *
         * @remarks
         * This is useful for broadcasting work that needs to be handled by multiple tasks
         * or for operations that should be processed by all available worker threads.
         *
         * @see IJob
         * @see ITask
         */
        virtual void addJobAllTasks( SmartPtr<IJob> job ) = 0;

        /**
         * @brief Clears pending EventJob instances from all managed tasks.
         */
        virtual void clearEventJobs() = 0;

        /**
         * @brief Retrieves a task by its unique identifier.
         *
         * @param taskId The unique ID of the task to retrieve.
         * @return A smart pointer to the ITask with the given task ID, or nullptr if not found.
         *
         * @see getTaskPtr
         * @see TaskId
         */
        virtual SmartPtr<ITask> getTask( TaskId taskId ) const = 0;

        /**
         * @brief Retrieves a raw pointer to a task by its unique identifier.
         *
         * Returns a raw pointer for performance-critical scenarios where smart pointer
         * overhead should be avoided. The caller must ensure the task manager remains
         * valid while using the returned pointer.
         *
         * @param taskId The unique ID of the task to retrieve.
         * @return A raw pointer to the ITask with the given task ID, or nullptr if not found.
         *
         * @warning
         * The returned pointer is not reference counted. Ensure the task manager and task
         * remain valid for the lifetime of the pointer usage.
         *
         * @see getTask
         */
        virtual ITask *getTaskPtr( TaskId taskId ) const = 0;

        /**
         * @brief Retrieves an array of all tasks managed by the task manager.
         *
         * @return An array containing smart pointers to all ITask objects currently managed.
         *
         * @remarks
         * The returned array is a snapshot of the current task collection. Modifications
         * to the array do not affect the task manager's internal task list.
         *
         * @see getNumTasks
         */
        virtual Array<SmartPtr<ITask>> getTasks() const = 0;

        /**
         * @brief Sets the execution state of the task manager.
         *
         * Changes the task manager's operational mode, affecting how tasks are scheduled
         * and executed. State transitions may trigger synchronization points.
         *
         * @param state The desired execution state for the task manager.
         *
         * @remarks
         * Changing states may block until all tasks reach a consistent state suitable
         * for the transition. For example, transitioning to Shutdown will typically
         * wait for pending work to complete.
         *
         * @see getState
         * @see State
         */
        virtual void setState( State state ) = 0;

        /**
         * @brief Retrieves the current execution state of the task manager.
         *
         * @return The current operational state of the task manager.
         *
         * @see setState
         * @see State
         */
        virtual State getState() const = 0;

        /**
         * @brief Retrieves the number of tasks currently managed.
         *
         * @return The total count of tasks managed by this task manager.
         *
         * @see getTasks
         */
        virtual u32 getNumTasks() const = 0;

        /**
         * @brief Blocks until all managed tasks have completed their current work.
         *
         * Synchronization point that ensures all tasks have finished processing their
         * queued jobs before returning. Useful for ensuring work completion before
         * proceeding with dependent operations.
         *
         * @remarks
         * This call will block the calling thread until all tasks signal completion.
         * Use with caution in time-critical code paths.
         *
         * @see stop
         */
        virtual void wait() = 0;

        /**
         * @brief Stops execution of all managed tasks.
         *
         * Signals all tasks to cease processing. Tasks will complete their current
         * job (if any) and then enter an idle state. This does not destroy the tasks.
         *
         * @remarks
         * After stopping, tasks can be restarted by changing the manager's state.
         * To permanently terminate, use shutdown() instead.
         *
         * @see wait
         * @see shutdown
         * @see reset
         */
        virtual void stop() = 0;

        /**
         * @brief Resets the task manager to its initial state.
         *
         * Clears internal state and prepares the task manager for reinitialization.
         * This typically stops all tasks and clears any pending work.
         *
         * @remarks
         * The exact behavior depends on the implementation, but generally this
         * provides a clean slate without destroying the task manager instance.
         *
         * @see stop
         * @see shutdown
         */
        virtual void reset() = 0;

        /**
         * @brief Initiates a complete shutdown of the task manager.
         *
         * Performs a graceful shutdown of all tasks and releases associated resources.
         * After shutdown, the task manager should not be reused.
         *
         * @remarks
         * This is typically called during application termination. Unlike stop(),
         * shutdown is intended to be final and non-reversible.
         *
         * @see stop
         * @see reset
         */
        virtual void shutdown() = 0;

        /**
         * @brief Acquires a lock on a specific task for thread-safe operations.
         *
         * Provides a RAII-style lock mechanism for synchronizing access to a task.
         * The lock is automatically released when the TaskLock object goes out of scope.
         *
         * @param taskId The unique ID of the task to lock.
         * @return A TaskLock object that manages the lock lifetime.
         *
         * @remarks
         * Use this when you need to perform operations on a task that require
         * exclusive access or synchronization with the task's execution thread.
         *
         * @code{.cpp}
         * {
         *     auto lock = taskManager->lockTask(myTaskId);
         *     // Perform thread-safe operations on the task
         *     // Lock automatically releases when 'lock' goes out of scope
         * }
         * @endcode
         *
         * @see TaskLock
         */
        virtual TaskLock lockTask( TaskId taskId ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
