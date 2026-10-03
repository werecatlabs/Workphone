#ifndef JobQueueTBB_h__
#define JobQueueTBB_h__

#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <boost/thread.hpp>

namespace workphone
{

    /**
     * @brief Multi-threaded job queue implementation.
     *
     * This class implements a job queue that dispatches work to a pool of
     * worker threads while also supporting coroutine-style jobs that must be
     * executed on the primary (main) thread. It provides facilities to add
     * jobs for specific tasks, run jobs across all tasks, and control the
     * processing rate of primary-thread coroutine execution.
     */
    class WPCore_API JobQueueMT : public IJobQueue
    {
    public:
        struct WorkerThread
        {
            /**
             * @brief Create a worker thread object bound to a job queue.
             * @param jobQueue Pointer to the owning JobQueueMT instance.
             */
            WorkerThread( JobQueueMT *jobQueue, Thread::ThreadId threadId );

            /**
             * @brief Thread entry point that continuously processes jobs from
             * the worker queue until shutdown.
             */
            void operator()();

            /// Shared pointer to the owning JobQueueMT.
            SmartPtr<JobQueueMT> m_jobQueue;

            /// Thread id assigned to this worker.
            Thread::ThreadId m_threadId = Thread::ThreadId::WorkerThread;
        };

        /**
         * @brief Construct a new JobQueueMT.
         */
        JobQueueMT();

        /**
         * @brief Destroy the JobQueueMT and release resources.
         */
        ~JobQueueMT() override;

        /**
         * @brief Execute pending primary-thread coroutine jobs. Should be
         * called from the main thread at the frequency described by
         * getRate()/setRate().
         */
        void update() override;

        /**
         * @brief Check whether there are any jobs waiting to be processed.
         * @return true if any queue (worker or primary) contains jobs.
         */
        bool hasJobs() const override;

        /**
         * @brief Add a job to the queue for processing by worker threads.
         * @param job Job to enqueue.
         */
        void addJob( SmartPtr<IJob> job ) override;

        /**
         * @brief Add a job to the queue that belongs to a specific task.
         * @param job Job to enqueue.
         * @param task Task identifier to associate with the job.
         */
        void addJob( SmartPtr<IJob> job, TaskId task ) override;

        /**
         * @brief Add a job that should be executed for all tasks.
         * @param job Job to enqueue for all tasks.
         */
        void addJobAllTasks( SmartPtr<IJob> job ) override;

        /** @copydoc IJobQueue::clearEventJobs */
        void clearEventJobs() override;

        /**
         * @brief Whether the worker threads are currently running.
         * @return true if job processing is enabled.
         */
        bool isRunning() const override;

        /**
         * @brief Enable or disable processing by worker threads.
         * @param running true to start processing, false to stop.
         */
        void setRunning( bool running ) override;

        /**
         * @brief Get the update rate used by primary-thread coroutine
         * processing.
         * @return Seconds per tick.
         */
        f32 getRate() const override;

        /**
         * @brief Set the update rate used by primary-thread coroutine
         * processing.
         * @param rate Seconds per tick.
         */
        void setRate( f32 rate ) override;

        /**
         * @brief Query whether worker threads use CPU affinity.
         * @return true if affinity is enabled.
         */
        bool getUseAffinity() const override;

        /**
         * @brief Enable or disable CPU affinity for worker threads.
         * @param affinity true to enable affinity.
         */
        void setUseAffinity( bool affinity ) override;

        /**
         * @brief Request shutdown of the job queue and join worker threads.
         */
        void shutdown() override;

        /**
         * @brief Start a coroutine that will be driven by the primary thread.
         * @param func Function which receives a pull-type coroutine object.
         */
        void startCoroutine( std::function<void( ICoroutineData::PullType & )> func ) override;

        /**
         * @brief Creates and starts a function-based job.
         * @param func The function to execute asynchronously.
         * @return Smart pointer to the created job, or nullptr if the job could not be started.
         */
        SmartPtr<IJob> startJob( std::function<void()> func ) override;

        /**
         * @brief Get the configured number of worker threads.
         * @return Worker thread count used when creating the MT worker pool.
         */
        u32 getNumWorkerThreads() const;

        /**
         * @brief Set the configured number of worker threads.
         * @param numWorkerThreads Worker thread count to use.
         */
        void setNumWorkerThreads( u32 numWorkerThreads );

        /**
         * @brief Get the number of shutdown polling attempts.
         * @return Maximum number of attempts used while waiting for executing jobs.
         */
        u32 getShutdownWaitCount() const;

        /**
         * @brief Set the number of shutdown polling attempts.
         * @param shutdownWaitCount Maximum number of attempts used while waiting for executing jobs.
         */
        void setShutdownWaitCount( u32 shutdownWaitCount );

        /**
         * @brief Get the delay between shutdown polling attempts.
         * @return Delay in seconds between attempts.
         */
        f64 getShutdownWaitInterval() const;

        /**
         * @brief Set the delay between shutdown polling attempts.
         * @param shutdownWaitInterval Delay in seconds between attempts.
         */
        void setShutdownWaitInterval( f64 shutdownWaitInterval );

    protected:
        void executeJob( SmartPtr<IJob> job );

        void stopAllJobs();

        bool hasExecutingJobs() const;

        bool isShutdownRequested() const;

        void setShutdownRequested( bool shutdownRequested );

        void createWorkerThreads();

        void destroyWorkerThreads();

        Thread::ThreadId getWorkerThreadId( u32 index ) const;

        /**
         * @brief Determine whether a given task id is currently being processed
         * by any worker thread.
         * @param id Task identifier.
         * @return true if the task is being processed.
         */
        bool isProcessing( s32 id ) const;

        /**
         * @brief Get a snapshot of coroutine jobs that should be executed on
         * the primary thread.
         * @return Array of coroutine job pointers.
         */
        Array<SmartPtr<IJob>> getCoroutineJobs() const;

        /**
         * @brief Add a job to the coroutine list to be processed on the primary thread.
         * @param job The coroutine job to add.
         */
        void addCoroutineJob( SmartPtr<IJob> job );

        /**
         * @brief Remove a job from the coroutine list.
         * @param job The coroutine job to remove.
         */
        void removeCoroutineJob( SmartPtr<IJob> job );

        /**
         * @brief Get the shared pointer that holds the coroutine jobs array.
         * @return SharedPtr to the coroutine jobs array.
         */
        SharedPtr<Array<SmartPtr<IJob>>> getCoroutineJobsPtr() const;

        /**
         * @brief Atomically set the pointer to the coroutine jobs array.
         * @param coroutineJobs New shared pointer to the coroutine jobs array.
         */
        void setCoroutineJobsPtr( SharedPtr<Array<SmartPtr<IJob>>> coroutineJobs );

        /// Atomic shared pointer to the array of coroutine jobs for the primary thread.
        AtomicSharedPtr<Array<SmartPtr<IJob>>> m_coroutineJobs;

        /// Jobs currently being executed by this queue.
        ConcurrentArray<SmartPtr<IJob>> m_executingJobs;

        /// Worker threads managed by this job queue.
        Array<boost::thread *> m_threads;

        /// Alias for the concurrent job queue type used by workers.
        using Jobs = ConcurrentQueue<SmartPtr<IJob>>;

        /// Queue for jobs processed by worker threads.
        Jobs m_jobQueue;

        /// Queue for jobs that must be executed on the primary thread.
        Jobs m_primaryJobQueue;

        /// Update rate (seconds per tick) for worker-thread processing. Default: 1/15s
        atomic_f64 m_updateRate = 1.0 / 15.0;

        /// Configured number of worker threads.
        atomic_u32 m_numWorkerThreads = 0;

        /// Number of shutdown polling attempts while waiting for executing jobs.
        atomic_u32 m_shutdownWaitCount = 100;

        /// Delay between shutdown polling attempts, in seconds.
        atomic_f64 m_shutdownWaitInterval = 0.1;

        /// Flag indicating whether thread affinity should be used.
        atomic_bool m_useAffinity = false;

        /// Flag indicating whether the queue is actively processing jobs.
        atomic_bool m_isRunning = true;

        /// Flag indicating worker threads should exit.
        atomic_bool m_shutdownRequested = false;
    };
}  // namespace workphone

#endif  // JobQueueTBB_h__
