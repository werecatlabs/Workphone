#ifndef JobGroupBase_h__
#define JobGroupBase_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IJobGroup.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

namespace workphone
{

    /**
     * @class JobGroupBase
     * @brief Base implementation class for job groups that manages collections of jobs with
     * dependencies.
     *
     * This class provides a thread-safe base implementation for managing groups of jobs.
     * It handles job state management, progress tracking, priority scheduling, and execution control.
     * Derived classes should implement specific job scheduling and dependency resolution logic.
     *
     * @par Thread Safety
     * This class uses atomic operations for state management, making it safe to query state
     * from multiple threads. However, state transitions should be carefully managed by derived classes.
     *
     * @see IJobGroup
     * @see IJob
     */
    class WPCore_API JobGroupBase : public IJobGroup
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the job group with default state (Ready), zero progress,
         * zero priority, and not marked as primary or finished.
         */
        JobGroupBase();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~JobGroupBase() override;

        /**
         * @brief Updates the job group's internal state.
         *
         * This method should be called periodically to update job states,
         * check for completion, and manage job execution flow.
         * Derived classes should override this to implement specific update logic.
         */
        void update() override;

        /**
         * @brief Gets the interrupt flag status.
         *
         * @return True if the job group has been requested to interrupt, false otherwise.
         */
        virtual bool getInterupt() const;

        /**
         * @brief Sets the interrupt flag to request early termination.
         *
         * Setting this flag signals that the job group should stop execution
         * as soon as possible. The actual interruption behavior is implementation-dependent.
         *
         * @param interupt True to request interrupt, false to clear the interrupt flag.
         */
        virtual void setInterupt( bool interupt );

        /**
         * @brief Gets the current execution state of the job group.
         *
         * @return The current state (Ready, Executing, Finished, etc.).
         * @see IJob::State
         */
        State getState() const override;

        /**
         * @brief Sets the execution state of the job group.
         *
         * This is thread-safe through atomic operations.
         *
         * @param state The new state to set.
         * @see IJob::State
         */
        void setState( State state ) override;

        /**
         * @brief Gets the current progress of the job group.
         *
         * Progress is typically represented as a value from 0 to 100,
         * but the exact range depends on the implementation.
         *
         * @return The current progress value (typically 0-100).
         */
        u32 getProgress() const override;

        /**
         * @brief Sets the progress of the job group.
         *
         * This is thread-safe through atomic operations.
         *
         * @param progress The progress value to set (typically 0-100).
         */
        void setProgress( u32 progress ) override;

        /**
         * @brief Gets the execution priority of the job group.
         *
         * Higher priority job groups should be scheduled before lower priority ones.
         *
         * @return The priority value (higher values = higher priority).
         */
        s32 getPriority() const override;

        /**
         * @brief Sets the execution priority of the job group.
         *
         * This is thread-safe through atomic operations.
         *
         * @param priority The priority value to set (higher values = higher priority).
         */
        void setPriority( s32 priority ) override;

        /**
         * @brief Checks if this job group is marked as primary.
         *
         * Primary job groups may receive special scheduling treatment or priority.
         *
         * @return True if this is a primary job group, false otherwise.
         */
        bool isPrimary() const override;

        /**
         * @brief Sets whether this job group is primary.
         *
         * This is thread-safe through atomic operations.
         *
         * @param primary True to mark as primary, false otherwise.
         */
        void setPrimary( bool primary ) override;

        /**
         * @brief Checks if the job group has finished execution.
         *
         * @return True if the job group has completed all work, false otherwise.
         */
        bool isFinished() const override;

        /**
         * @brief Blocks the calling thread until the job group finishes.
         *
         * This method will wait indefinitely until the job group completes.
         *
         * @return True if the wait completed successfully, false if interrupted or failed.
         */
        bool wait() override;

        /**
         * @brief Blocks the calling thread until the job group finishes or timeout occurs.
         *
         * @param maxWaitTime Maximum time to wait in seconds.
         * @return True if the job group finished within the timeout, false if timeout occurred.
         */
        bool wait( f64 maxWaitTime ) override;

        /**
         * @brief Gets the processor affinity mask for this job group.
         *
         * The affinity mask determines which CPU cores the job group can execute on.
         *
         * @return The affinity mask value (-1 typically means no affinity restriction).
         */
        s32 getAffinity() const override;

        /**
         * @brief Sets the processor affinity mask for this job group.
         *
         * This allows restricting job execution to specific CPU cores.
         *
         * @param affinity The affinity mask to set (-1 for no restriction).
         */
        void setAffinity( s32 affinity ) override;

        /**
         * @brief Executes the job group's work.
         *
         * This method should contain or orchestrate the main execution logic
         * for all jobs in the group. Derived classes must implement the specific
         * execution behavior.
         */
        void execute() override;

        /**
         * @brief Executes the job group as a coroutine.
         *
         * Allows the job group to yield execution and resume later.
         * Derived classes should implement coroutine-based execution logic.
         */
        void coroutine_execute() override;

        /**
         * @brief Executes a single step of the coroutine.
         *
         * This allows incremental execution of the job group with explicit
         * yield points for cooperative multitasking.
         *
         * @param rYield Reference to coroutine data for yielding and resuming.
         */
        void coroutine_execute_step( SmartPtr<ICoroutineData> &rYield ) override;

        /**
         * @brief Checks if this job group executes as a coroutine.
         *
         * @return True if coroutine execution is enabled, false for normal execution.
         */
        bool isCoroutine() const override;

        /**
         * @brief Sets whether this job group should execute as a coroutine.
         *
         * @param coroutine True to enable coroutine execution, false for normal execution.
         */
        void setCoroutine( bool coroutine ) override;

    protected:
        /**
         * @brief Current execution state of the job group.
         *
         * Thread-safe atomic value initialized to State::Ready.
         */
        AtomicValue<State> m_state = State::Ready;

        /**
         * @brief Current progress value (typically 0-100).
         *
         * Thread-safe atomic value for tracking execution progress.
         */
        atomic_u32 m_progress = 0;

        /**
         * @brief Priority value for scheduling (higher = higher priority).
         *
         * Thread-safe atomic value for priority management.
         */
        atomic_u32 m_priority = 0;

        /**
         * @brief Flag indicating if this is a primary job group.
         *
         * Thread-safe atomic boolean for primary status.
         */
        atomic_bool m_isPrimary = false;

        /**
         * @brief Flag indicating if the job group has finished execution.
         *
         * Thread-safe atomic boolean for completion tracking.
         */
        atomic_bool m_isFinished = false;
    };

}  // namespace workphone

#endif  // JobGroupBase_h__
