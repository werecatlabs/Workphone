#ifndef __JobQueue_h__
#define __JobQueue_h__

#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /**
     * @class JobQueue
     * @brief Thread-safe job queue implementation for asynchronous task execution.
     *
     * The JobQueue class manages and executes jobs across multiple threads, coordinating
     * with the thread pool for concurrent execution. It supports three types of job execution:
     * - Primary jobs: Execute on the primary/main thread
     * - Regular jobs: Execute on worker threads from the thread pool
     * - Coroutine jobs: Execute incrementally using cooperative multitasking
     *
     * Jobs are processed during the update() call, which should be invoked regularly
     * (typically once per frame on the primary thread or continuously on worker threads).
     *
     * @note This class is thread-safe and can be accessed from multiple threads concurrently.
     * @see IJobQueue, IJob, IThreadPool, ITaskManager
     */
    class WPCore_API JobQueue : public IJobQueue
    {
    public:
        class EventListener : public IEventListener
        {
        public:
            EventListener();

            ~EventListener() override;

            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

            SmartPtr<JobQueue> getOwner() const;

            void setOwner( SmartPtr<JobQueue> owner );

        protected:
            AtomicWeakPtr<JobQueue> m_owner;
        };

        /**
         * @brief Constructor.
         * Initializes the job queue with default settings (running=true, rate=1/15 seconds).
         */
        JobQueue();

        /**
         * @brief Destructor.
         * Cleans up the job queue resources.
         */
        ~JobQueue() override;

        /**
         * @brief Loads the job queue and initializes its resources.
         * @param data Optional shared object containing initialization data.
         *
         * Sets the loading state to Loading, performs initialization, then sets
         * the loading state to Loaded.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads the job queue and releases its resources.
         * @param data Optional shared object containing unload parameters.
         *
         * Waits for all executing jobs (coroutines, primary, and regular jobs) to
         * complete before clearing the queues. Sets the loading state to Unloading,
         * then Unloaded when complete.
         *
         * @note This is a blocking operation that yields until jobs finish executing.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Processes queued jobs on the current thread.
         *
         * Behavior depends on the calling thread:
         * - Primary thread: Executes one coroutine step per job, one primary job,
         *   and if no thread pool exists, one regular job.
         * - Worker threads: Executes one regular job from the queue.
         *
         * Jobs are marked as Executing during execution and Finish when complete.
         * Completed coroutine jobs are automatically removed from the queue.
         *
         * @note Should be called regularly to keep the job queue processing.
         * @note Temporarily sets current task to None during execution to prevent
         *       task context contamination.
         */
        void update() override;

        /**
         * @brief Checks if there are any pending jobs in any queue.
         * @return True if there are jobs in the primary, regular, or coroutine queues;
         *         false if all queues are empty.
         */
        bool hasJobs() const override;

        /**
         * @brief Adds a job to the appropriate queue for execution.
         * @param job The job to add. Must not be null or already queued.
         *
         * Jobs marked as primary (via isPrimary()) are added to the primary queue
         * and will execute on the main thread. Other jobs are added to the regular
         * queue for execution on worker threads.
         *
         * @note Does nothing if the application is quitting, not running, or if
         *       the job queue is not running.
         * @note Asserts that the job is not already in the Queue state.
         */
        void addJob( SmartPtr<IJob> job ) override;

        /**
         * @brief Adds a job to a specific task's queue.
         * @param job The job to add. Must not be null.
         * @param task The task ID that should process this job.
         *
         * Routes the job to the specified task's job queue via the task manager.
         * This allows jobs to be processed by specific system tasks (e.g., Physics,
         * Render, Audio).
         *
         * @note Does nothing if the application is quitting, not running, or if
         *       the job queue is not running.
         * @see ITaskManager, TaskId
         */
        void addJob( SmartPtr<IJob> job, TaskId task ) override;

        /**
         * @brief Adds a job to all task queues.
         * @param job The job to add. Must not be null.
         *
         * Adds the job to every registered task's queue (up to TaskId::Count).
         * Useful for jobs that need to be executed once per task/subsystem.
         *
         * @note Does nothing if the application is quitting, not running, or if
         *       the job queue is not running.
         * @see ITaskManager, TaskId
         */
        void addJobAllTasks( SmartPtr<IJob> job ) override;

        /** @copydoc IJobQueue::clearEventJobs */
        void clearEventJobs() override;

        /**
         * @brief Gets whether the job queue is currently processing jobs.
         * @return True if the queue is running and accepting/processing jobs;
         *         false otherwise.
         */
        bool isRunning() const override;

        /**
         * @brief Sets whether the job queue should process jobs.
         * @param running True to enable job processing; false to pause it.
         *
         * When set to false, new jobs will not be accepted and existing jobs
         * will not be processed until set back to true.
         */
        void setRunning( bool running ) override;

        /**
         * @brief Gets the update rate for worker threads.
         * @return The time interval (in seconds) between worker thread updates.
         *         Default is 1/15 (approximately 66.67ms).
         *
         * This rate determines how frequently worker threads wake up to check
         * for new jobs when idle.
         */
        f32 getRate() const override;

        /**
         * @brief Sets the update rate for worker threads.
         * @param rate The time interval (in seconds) between worker thread updates.
         *             Lower values increase responsiveness but use more CPU.
         *             Higher values reduce CPU usage but increase job latency.
         *
         * @note This affects worker thread sleep intervals, not the execution
         *       frequency of jobs themselves.
         */
        void setRate( f32 rate ) override;

        /**
         * @brief Gets whether thread affinity should be used for job execution.
         * @return True if thread affinity is enabled; false otherwise.
         *
         * When enabled, jobs may be pinned to specific CPU cores based on their
         * affinity settings for improved cache locality and performance.
         */
        bool getUseAffinity() const override;

        /**
         * @brief Sets whether thread affinity should be used for job execution.
         * @param affinity True to enable thread affinity; false to disable it.
         *
         * @see IJob::getAffinity(), IJob::setAffinity()
         */
        void setUseAffinity( bool affinity ) override;

        /**
         * @brief Initiates graceful shutdown of the job queue.
         *
         * Stops accepting new jobs and waits up to 10 seconds (100 iterations
         * of 100ms sleep) for existing jobs to complete. After timeout or when
         * all jobs finish, the queue is fully shut down.
         *
         * @note This is a blocking operation.
         * @note Remaining jobs after timeout may be terminated.
         */
        void shutdown() override;

        /**
         * @brief Starts a coroutine job that executes incrementally over multiple frames.
         * @param func The coroutine function that yields execution control periodically.
         *             The function receives a PullType reference for yielding.
         *
         * Creates a JobCoroutine internally and adds it to the coroutine job queue.
         * The coroutine executes one step per update() call on the primary thread
         * until completion.
         *
         * @note Does nothing if the application is quitting, not running, or if
         *       the job queue is not running.
         * @see JobCoroutine, ICoroutineData
         */
        void startCoroutine( std::function<void( ICoroutineData::PullType & )> func ) override;

        /**
         * @brief Creates and starts a function-based job.
         * @param func The function to execute asynchronously.
         * @return Smart pointer to the created job, or nullptr if the job could not be started.
         *
         * Creates a JobFunction internally, adds it to the job queue, and returns
         * a reference allowing the caller to track completion or cancel the job.
         *
         * @note Returns nullptr if the application is quitting, not running, or if
         *       the job queue is not running.
         * @see JobFunction
         */
        SmartPtr<IJob> startJob( std::function<void()> func ) override;

        /**
         * @brief Locks the job queue's internal mutex.
         *
         * Provides manual synchronization control. Must be paired with unlock().
         *
         * @warning Failing to unlock may cause deadlocks.
         * @see unlock()
         */
        void lock() override;

        /**
         * @copydoc JobQueue::try_lock
         */
        bool try_lock() override;

        /**
         * @brief Unlocks the job queue's internal mutex.
         *
         * Releases the lock acquired by lock().
         *
         * @see lock()
         */
        void unlock() override;

    protected:
        void executeJob( SmartPtr<IJob> job );

        void stopAllJobs();

        bool hasExecutingJobs() const;

        /**
         * @brief Gets a thread-safe snapshot of all coroutine jobs.
         * @return Array containing all current coroutine jobs.
         *
         * Used internally to iterate over coroutine jobs safely during updates.
         */
        Array<SmartPtr<IJob>> getCoroutineJobs() const;

        /**
         * @brief Adds a job to the coroutine job collection.
         * @param job The coroutine job to add.
         *
         * @note Thread-safe due to ConcurrentArray.
         */
        void addCoroutineJob( SmartPtr<IJob> job );

        /**
         * @brief Removes a job from the coroutine job collection.
         * @param job The coroutine job to remove.
         *
         * @note Thread-safe due to ConcurrentArray.
         */
        void removeCoroutineJob( SmartPtr<IJob> job );

        /// Thread-safe array of coroutine jobs that execute incrementally
        ConcurrentArray<SmartPtr<IJob>> m_coroutineJobs;

        /// Thread-safe array of jobs currently executing.
        ConcurrentArray<SmartPtr<IJob>> m_executingJobs;

        /// Thread-safe queue of jobs that must execute on the primary thread
        ConcurrentQueue<SmartPtr<IJob>> m_primaryJobs;

        /// Thread-safe queue of regular jobs for worker thread execution
        ConcurrentQueue<SmartPtr<IJob>> m_jobs;

        /// Update rate for worker threads in seconds (default: 1/15)
        atomic_f64 m_rate = 1.0 / 15.0;

        /// Flag indicating whether thread affinity should be used
        atomic_bool m_useAffinity = false;

        /// Flag indicating whether the queue is actively processing jobs
        atomic_bool m_isRunning = true;

        /// Recursive mutex for manual synchronization via lock()/unlock()
        mutable RecursiveMutex m_mutex;
    };
}  // namespace workphone

#endif  // __JobQueue_h__
