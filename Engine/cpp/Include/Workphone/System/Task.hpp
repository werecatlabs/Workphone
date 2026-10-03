#ifndef Task_h__
#define Task_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskLock.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/AtomicSmartPtr.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>
#include <Workphone/Thread/SpinRWMutex.hpp>

namespace workphone
{

    /**
     * @class Task
     * @brief Default concrete implementation of the ITask interface.
     *
     * The Task class models a schedulable unit of work managed by the
     * Workphone runtime. A Task owns a queue of jobs and exposes scheduling
     * parameters such as affinity, parallelism, target FPS and timestep
     * behavior. An optional finite-state-machine (IFSM) can be attached to
     * implement task-specific state transitions and event handling.
     *
     * Thread-safety: common operations that manipulate the job queue and
     * mutable task state are safe to call from multiple threads. External
     * systems should interact with Task through the ITask interface.
     */
    class WPCore_API Task : public ITask
    {
    public:
        class Flags
        {
        public:
            /**
             * @brief Thread-safe wrapper for an atomic Task smart pointer.
             *
             * Flags stores a SmartPtr<Task> inside an atomic smart-pointer
             * wrapper so that the reference can be read or replaced from
             * multiple threads without additional locking. This is useful
             * for small shared flags or scheduler-visible handles that are
             * frequently inspected.
             */
            explicit Flags( SmartPtr<Task> task );

            /** Destroy the Flags holder. */
            ~Flags();

            /** Get the currently stored Task pointer (thread-safe). */
            SmartPtr<Task> getTask() const;

            /** Replace the stored Task pointer (thread-safe). */
            void setTask( SmartPtr<Task> task );

        private:
            AtomicSmartPtr<Task> m_task;
        };

        /**
         * @brief Default constructor.
         *
         * Constructs an empty Task instance. The created task is disabled by
         * default and contains no attached FSM, owner or profile.
         */
        Task();

        /**
         * @brief Copy constructor.
         *
         * Performs a shallow copy of the task's visible state. Internal
         * runtime pointers are not deep-copied; use with care when sharing
         * between managers.
         * @param other Task to copy from.
         */
        Task( const Task &other );

        /**
         * @brief Destructor.
         *
         * Releases owned resources and detaches any references to manager
         * data. Destruction semantics assume the surrounding runtime will
         * not access the task after destruction.
         */
        ~Task() override;

        /**
         * @brief Load persistent or configuration data for the task.
         *
         * Implementations should read configuration values from the provided
         * @p data object and update task parameters accordingly. See
         * ISharedObject::load for the expected contract.
         * @param data Source object containing configuration values.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Reload configuration or runtime-updatable data.
         *
         * Called when dynamic configuration should be refreshed without
         * recreating the Task instance. Implementations should update
         * mutable state from @p data.
         * @param data Source object containing updated values.
         */
        void reload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload or clear data previously loaded with load().
         *
         * Implementations should release any resources or cached values that
         * were acquired during load(). The @p data parameter may be used as
         * context for selective unloading.
         * @param data Context object passed to unload.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Execute a single update step for this task.
         *
         * The update call will execute jobs queued on the task and drive the
         * attached FSM (if any). The behaviour of update is governed by the
         * task's scheduling parameters such as target FPS and fixed-time
         * settings. See ITask::update for the public contract.
         */
        void update() override;

        /**
         * @brief Reset runtime state to initial defaults.
         *
         * This resets internal counters and scheduling-related state while
         * leaving externally owned objects (owner, profile) intact.
         */
        void reset() override;

        /**
         * @brief Enqueue a job for execution by this task.
         *
         * The provided job will be stored in the task's concurrent queue
         * and executed during subsequent update/execute steps according to
         * the task's scheduling policy.
         * @param job Job instance to enqueue (may not be null).
         */
        void addJob( SmartPtr<IJob> job ) override;

        /** @copydoc ITask::clearEventJobs */
        void clearEventJobs() override;

        /**
         * @brief Mark or unmark this task as the manager's primary task.
         *
         * Primary tasks may be treated specially by the scheduler (for
         * example given higher priority or preferred placement). The exact
         * scheduler behaviour is implementation-defined.
         * @param usePrimary True to mark as primary, false to clear.
         */
        void setPrimary( bool usePrimary ) override;

        /**
         * @brief Query whether this task is marked as primary.
         * @return True when the task is primary.
         */
        bool isPrimary() const override;

        /**
         * @brief Control whether the manager recycles this task instance.
         *
         * When recycling is enabled the task will be returned to the
         * manager's free pool instead of being destroyed. This can reduce
         * allocation churn for short-lived tasks.
         * @param recycle True to enable recycling.
         */
        void setRecycle( bool recycle ) override;

        /**
         * @brief Query whether recycling is enabled for this task.
         * @return True when the task should be reused by the manager.
         */
        bool getRecycle() const override;

        /**
         * @brief Set the preferred thread affinity for this task.
         *
         * A value of -1 indicates no preference and allows the scheduler to
         * place the task on any worker thread. Affinity is advisory and the
         * scheduler may ignore it.
         * @param id Preferred thread index or -1 for no preference.
         */
        void setAffinity( s32 id ) override;

        /**
         * @brief Retrieve the preferred thread affinity.
         * @return Preferred thread index or -1 when no preference is set.
         */
        s32 getAffinity() const override;

        /**
         * @brief Query whether this task may execute jobs in parallel.
         * @return True when parallel execution is enabled.
         */
        bool isParallel() const override;

        /**
         * @brief Enable or disable parallel job execution.
         *
         * When enabled the task's scheduler may run multiple jobs from this
         * task concurrently across worker threads. Disable to force
         * single-threaded execution of the task's jobs.
         * @param parallel True to allow parallel execution.
         */
        void setParallel( bool parallel ) override;

        /**
         * @brief Check whether the task is enabled for scheduling.
         * @return True when the task is enabled and eligible to be scheduled.
         */
        bool isEnabled() const override;

        /**
         * @brief Enable or disable the task for scheduling.
         * @param enabled True to enable scheduling, false to disable.
         */
        void setEnabled( bool enabled ) override;

        /**
         * @brief Get the TaskId assigned to this task.
         * @return The task identifier.
         */
        TaskId getTask() const override;

        /**
         * @brief Set the TaskId for this task.
         * @param task Identifier to assign to the task.
         */
        void setTask( TaskId task ) override;

        /**
         * @brief Retrieve platform or thread specific scheduling flags.
         *
         * These flags may be used by the scheduler to adjust how the task
         * is executed on a particular platform or thread.
         * @return Bitmask of thread task flags.
         */
        u32 getThreadTaskFlags() const override;

        /**
         * @brief Set platform or thread specific scheduling flags.
         * @param threadTaskFlags Bitmask of flags understood by the scheduler.
         */
        void setThreadTaskFlags( u32 threadTaskFlags ) override;

        /**
         * @brief Stop scheduling this task.
         *
         * The task will be marked stopped; depending on configuration any
         * currently running jobs may be allowed to finish. Stopped tasks
         * will not be scheduled for new updates.
         */
        void stop() override;

        /**
         * @brief Start or resume scheduling this task.
         *
         * Clears the stopped flag so the scheduler may resume updates for
         * this task.
         */
        void start() override;

        /**
         * @brief Query whether the task is currently stopped.
         * @return True when the task will not be scheduled.
         */
        bool isStopped() const override;

        /**
         * @brief Check whether the task is currently in its update() call.
         * @return True while update() is being executed.
         */
        bool isUpdating() const override;

        /**
         * @brief Set or clear the internal updating flag.
         * @param updating True when the task is entering its update phase.
         */
        void setUpdating( bool updating ) override;

        /**
         * @brief Query whether the task is actively executing jobs.
         *
         * This may differ from isUpdating() when job execution is offloaded
         * to worker threads.
         * @return True while jobs are being executed for this task.
         */
        bool isExecuting() const override;

        /**
         * @brief Get the configured target frames-per-second for this task.
         * @return Target FPS value (may be 0 for unlimited).
         */
        f64 getTargetFPS() const override;

        /**
         * @brief Set the desired frames-per-second rate used by the scheduler.
         * @param targetFPS Target frames per second (0 to disable limiting).
         */
        void setTargetFPS( f64 targetFPS ) override;

        /**
         * @brief Check whether the task uses a fixed timestep for updates.
         * @return True when fixed timestep updates are enabled.
         */
        bool getUseFixedTime() const override;

        /**
         * @brief Enable or disable fixed timestep updates.
         * @param useFixedTime True to use a fixed timestep.
         */
        void setUseFixedTime( bool useFixedTime ) override;

        /**
         * @brief Get the absolute time scheduled for the next update.
         * @return Next update timestamp (time_interval representation).
         */
        time_interval getNextUpdateTime() const override;

        /**
         * @brief Set the scheduled time for the next update.
         * @param nextUpdateTime Absolute time when the next update should run.
         */
        void setNextUpdateTime( time_interval nextUpdateTime ) override;

        /**
         * @brief Set the current FSM state index for the attached FSM.
         * @param state State index to select in the FSM.
         */
        void setState( State state ) override;

        /**
         * @brief Get the current FSM state index.
         * @return Current state index of the FSM instance.
         */
        State getState() const override;

        /**
         * @brief Get the owner object associated with this task.
         *
         * The owner typically represents a subsystem or resource holder
         * responsible for the task's lifecycle.
         * @return Smart pointer to the owner object (may be null).
         */
        SmartPtr<ISharedObject> getOwner() const override;

        /**
         * @brief Assign an owner object for this task.
         * @param owner Smart pointer to the owner or nullptr to clear.
         */
        void setOwner( SmartPtr<ISharedObject> owner ) override;

        /**
         * @brief Recompute the automatically estimated FPS from recent ticks.
         *
         * When auto-FPS is enabled the task can dynamically adjust its
         * scheduling. This method recalculates the estimate used by the
         * scheduler.
         */
        void calculateAutoFPS();

        /**
         * @brief Return the current automatically calculated FPS.
         * @return Estimated FPS value or 0 when auto-FPS is disabled.
         */
        f64 getAutoFPS() const;

        /**
         * @brief Enable or set the automatic FPS behaviour.
         * @param autoFPS Value to use for auto-FPS (0 to disable auto mode).
         */
        void setAutoFPS( f64 autoFPS );

        /**
         * @brief Get the number of update ticks executed by this task.
         * @return Tick count since creation or last reset.
         */
        u32 getTicks() const override;

        /**
         * @brief Get a raw pointer to the profiling object.
         *
         * Convenience helper equivalent to getProfile().get(). The returned
         * pointer is non-owning and may be null.
         * @return Raw IProfile pointer or nullptr.
         */
        IProfile *getProfilePtr() const;

        /**
         * @brief Retrieve the profiling object used to collect timing stats.
         * @return Smart pointer to the profile instance (may be null).
         */
        SmartPtr<IProfile> getProfile() const override;

        /**
         * @brief Assign a profiling object to this task.
         * @param profile Smart pointer to the profiling instance (may be null).
         */
        void setProfile( SmartPtr<IProfile> profile ) override;

        /**
         * @brief Return the finite-state-machine instance attached to this task.
         * @return Smart pointer to IFSM or nullptr if none is attached.
         */
        SmartPtr<IFSM> getFSM() const;

        /**
         * @brief Dispatch an event to the task's FSM for a specific state.
         * @param state State index to handle the event for.
         * @param eventType Event to dispatch to the FSM.
         * @return FSMReturnType describing the result of the event handling.
         */
        FSMReturnType handleEvent( u32 state, FSMEvent eventType );

        /**
         * @brief Acquire the task's internal recursive mutex.
         *
         * This mutex protects internal mutable state and may be used by
         * callers that need exclusive access to the task. The mutex is
         * recursive allowing the same thread to acquire it multiple times.
         */
        void lock() override;

        /**
         * @brief Try to acquire the task mutex without blocking.
         * @return True if the mutex was acquired, false otherwise.
         */
        bool try_lock() override;

        /**
         * @brief Release the task mutex previously acquired by lock().
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        AtomicSmartPtr<IFSM> m_fsm;            ///< Optional finite-state-machine
        AtomicWeakPtr<ISharedObject> m_owner;  ///< Non-owning owner or parent object
        AtomicSmartPtr<IProfile> m_profile;    ///< Profiling object (optional)

        TaskId m_taskId = TaskId::Primary;        ///< Identifier for this task
        atomic_u32 *m_threadTaskFlags = nullptr;  ///< Scheduler-specific flags
        atomic_f64 *m_targetfps = nullptr;        ///< Target FPS (atomic)
        atomic_f64 *m_autoFPS = nullptr;          ///< Auto-calculated FPS (atomic)
        atomic_f64 *m_nextUpdateTime = nullptr;   ///< Next scheduled update time
        atomic_u32 *m_tickCount = nullptr;        ///< Update tick counter (atomic)
        atomic_s32 *m_stopped = nullptr;          ///< Stopped flag (atomic)
        atomic_s32 *m_affinity = nullptr;         ///< Preferred thread affinity
        atomic_u32 *m_taskFlags = nullptr;        ///< Task flags (atomic)

        void *m_managerData = nullptr;  ///< Opaque pointer for manager use

        ConcurrentQueue<SmartPtr<IJob>> m_jobs;  ///< Thread-safe job queue

        mutable RecursiveSpinMutex m_mutex;  ///< Protects internal state
    };

    inline IProfile *Task::getProfilePtr() const
    {
        return m_profile.get();
    }

}  // namespace workphone

#endif  // Task_h__
