#ifndef _WP_ITask_H_
#define _WP_ITask_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Thread/Thread.hpp>

namespace workphone
{

    /**
     * @brief An interface for a task.
     *
     * @details This class provides an interface for a task. It includes a set of methods for managing
     * and controlling the task execution.
     */
    class WPCore_API ITask : public ISharedObject
    {
    public:
        /**
         * @brief The possible states of a task.
         *
         * @details This enumeration defines the possible states of a task.
         */
        enum class State
        {
            None,       //!< No state.
            Idle,       //!< The task is idle.
            Stopped,    //!< The task is stopped.
            Executing,  //!< The task is currently executing.
            Shutdown,   //!< The task is shutting down.
            Count       //!< The total number of states.
        };

        ///< A flag indicating whether the task can be recycled.
        static const u32 recycle_flag;

        ///< A flag indicating whether to use accurate time for task execution.
        static const u32 use_accurate_time_flag;

        ///< A flag indicating whether the task is enabled.
        static const u32 enabled_flag;

        ///< A flag indicating whether to automatically set the target FPS.
        static const u32 auto_target_fps_flag;

        ///< A flag indicating whether to use fixed time for task execution.
        static const u32 fixed_time_flag;

        ///< A flag indicating whether the task is being updated.
        static const u32 updating_flag;

        ///< A flag indicating whether to queue messages for the task.
        static const u32 queue_messages_flag;

        ///< A flag indicating whether to use a separate thread for task execution.
        static const u32 use_thread_flag;

        ///< A flag indicating whether to smooth the time for task execution.
        static const u32 smooth_time_flag;

        ///< A flag indicating whether to run the task on the primary thread.
        static const u32 primary_flag;

        ///< A flag indicating whether the task can be executed in parallel.
        static const u32 parallel_flag;

        ///< A flag indicating whether the task is currently executing.
        static const u32 executing_flag;

        ///< A flag indicating whether the task is paused.
        static const u32 paused_flag;

        /** @brief Virtual destructor. */
        ~ITask() override;

        /**
         * @brief Resets the task to its initial state.
         */
        virtual void reset() = 0;

        /**
         * @brief Queues a job to run on this task.
         *
         * @param job The job to queue.
         */
        virtual void addJob( SmartPtr<IJob> job ) = 0;

        /**
         * @brief Clears pending EventJob instances queued on this task.
         */
        virtual void clearEventJobs() = 0;

        /**
         * @brief Sets whether to run the task on the primary thread or not.
         *
         * @param usePrimary Whether to run the task on the primary thread or not.
         */
        virtual void setPrimary( bool usePrimary ) = 0;

        /**
         * @brief Gets whether the task runs on the primary thread or not.
         *
         * @return Whether the task runs on the primary thread or not.
         */
        virtual bool isPrimary() const = 0;

        /**
         * @brief Sets whether to recycle the task or not.
         *
         * @param recycle Whether to recycle the task or not.
         */
        virtual void setRecycle( bool recycle ) = 0;

        /**
         * @brief Gets whether the task is recycled or not.
         *
         * @return Whether the task is recycled or not.
         */
        virtual bool getRecycle() const = 0;

        /**
         * Sets the ID of the thread that should execute this task.
         * @param id The ID of the thread that should execute this task.
         */
        virtual void setAffinity( s32 id ) = 0;

        /**
         * Gets the ID of the thread that should execute this task.
         * @return The ID of the thread that should execute this task.
         */
        virtual s32 getAffinity() const = 0;

        /**
         * Gets whether the task is parallel or not.
         * @return Whether the task is parallel or not.
         */
        virtual bool isParallel() const = 0;

        /**
         * Sets whether the task is parallel or not.
         * @param parallel Whether the task is parallel or not.
         */
        virtual void setParallel( bool parallel ) = 0;

        /**
         * Gets whether the task is enabled or not.
         * @return Whether the task is enabled or not.
         */
        virtual bool isEnabled() const = 0;

        /**
         * Sets whether the task is enabled or not.
         * @param enabled Whether the task is enabled or not.
         */
        virtual void setEnabled( bool enabled ) = 0;

        /**
         * @brief Gets the task ID of this task.
         *
         * @return The task ID of this task.
         */
        virtual TaskId getTask() const = 0;

        /**
         * @brief Sets the task ID of this task.
         *
         * @param task The task ID to set.
         */
        virtual void setTask( TaskId task ) = 0;

        /**
         * @brief Gets the thread task flags.
         *
         * @return The thread task flags.
         */
        virtual u32 getThreadTaskFlags() const = 0;

        /**
         * @brief Sets the thread task flags.
         *
         * @param threadTaskFlags The thread task flags to set.
         */
        virtual void setThreadTaskFlags( u32 threadTaskFlags ) = 0;

        /**
         * @brief Stops the task from running.
         */
        virtual void stop() = 0;

        /**
         * @brief Starts the task.
         */
        virtual void start() = 0;

        /**
         * @brief Determines whether the task is stopped.
         *
         * @return True if the task is stopped, false otherwise.
         */
        virtual bool isStopped() const = 0;

        /**
         * @brief Determines whether the task is being updated.
         *
         * @return True if the task is being updated, false otherwise.
         */
        virtual bool isUpdating() const = 0;

        /**
         * @brief Sets whether the task is being updated.
         *
         * @param updating Whether the task is being updated or not.
         */
        virtual void setUpdating( bool updating ) = 0;

        /**
         * @brief Determines whether the task is executing.
         *
         * @return True if the task is executing, false otherwise.
         */
        virtual bool isExecuting() const = 0;

        /** Gets the target fps. */
        virtual f64 getTargetFPS() const = 0;

        /** Sets the target fps. */
        virtual void setTargetFPS( f64 targetFPS ) = 0;

        /** Gets the task state. */
        virtual bool getUseFixedTime() const = 0;

        /** Sets the task state. */
        virtual void setUseFixedTime( bool useFixedTime ) = 0;

        /** Gets next update time. */
        virtual time_interval getNextUpdateTime() const = 0;

        /** Sets next update time. */
        virtual void setNextUpdateTime( time_interval nextUpdateTime ) = 0;

        /** Gets the ticks. */
        virtual u32 getTicks() const = 0;

        /** Sets the task state. */
        virtual void setState( State state ) = 0;

        /** Gets the task state. */
        virtual State getState() const = 0;

        /** Gets the owner.
        @return A pointer to the owner.
        */
        virtual SmartPtr<ISharedObject> getOwner() const = 0;

        /** Gets the owner.
        @param owner The owner of the task.
        */
        virtual void setOwner( SmartPtr<ISharedObject> owner ) = 0;

        /**
         * @brief Gets the profiling data for this task.
         *
         * @return A smart pointer to the profiling data for this task.
         */
        virtual SmartPtr<IProfile> getProfile() const = 0;

        /**
         * @brief Sets the profiling data for this task.
         *
         * @param profile A smart pointer to the profiling data to set for this task.
         */
        virtual void setProfile( SmartPtr<IProfile> profile ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
