#ifndef IJob_h__
#define IJob_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Interface for a job class, providing support for managing job execution.
     */
    class WPCore_API IJob : public ISharedObject
    {
    public:
        /**
         * @brief Enum class representing the different states a job can be in.
         */
        enum class State
        {
            Ready,
            Queue,
            Executing,
            Finish,

            Count
        };

        /**
         * @brief Enum class representing the priority levels of a job.
         */
        enum class JobPriority
        {
            High,
            Medium,
            Low,

            Count
        };

        /**
         * @brief Destroy the IJob object.
         */
        ~IJob() override;

        /**
         * @brief Get the affinity mask for the job, determining on which cores it can execute.
         * @return s32 The affinity mask.
         */
        virtual s32 getAffinity() const = 0;

        /**
         * @brief Set the affinity mask for the job, determining on which cores it can execute.
         * @param affinity The desired affinity mask.
         */
        virtual void setAffinity( s32 affinity ) = 0;

        /**
         * @brief Execute the job. This function is used to execute from different threads.
         * The implementation is assumed to be thread-safe.
         */
        virtual void execute() = 0;

        /**
         * @brief Start the coroutine for the job.
         */
        virtual void coroutine_execute() = 0;

        /**
         * @brief Execute a single step of the coroutine.
         * @param yield A reference to the IObjectYield smart pointer for managing coroutine execution.
         */
        virtual void coroutine_execute_step( SmartPtr<ICoroutineData> &yield ) = 0;

        /**
         * @brief Get the current state of the job.
         * @return JobState The current state of the job.
         */
        virtual State getState() const = 0;

        /**
         * @brief Set the state of the job.
         * @param state The desired state for the job.
         */
        virtual void setState( State state ) = 0;

        /**
         * @brief Get the job progress.
         * @return u32 The current progress of the job.
         */
        virtual u32 getProgress() const = 0;

        /**
         * @brief Set the job progress.
         * @param progress The desired progress for the job.
         */
        virtual void setProgress( u32 progress ) = 0;

        /** Gets the job priority. */
        virtual s32 getPriority() const = 0;

        /** Sets the job priority. */
        virtual void setPriority( s32 priority ) = 0;

        /** Used to know if to run on the primary thread.
         * @return bool The value.
         */
        virtual bool isPrimary() const = 0;

        /** Used to know if to run on the primary thread.
         * @param primary The value to set.
         */
        virtual void setPrimary( bool primary ) = 0;

        /** To know if the job is finished.
         * @return bool The value.
         */
        virtual bool isFinished() const = 0;

        /** Sets if the job is interrupted.
         * @param interrupted The value to set.
         */
        virtual void setInterrupted( bool interrupted ) = 0;

        /** To know if the job is interrupted.
         * @return bool The value.
         */
        virtual bool isInterrupted() const = 0;

        /**
         * @brief Requests that the job stop as soon as possible.
         *
         * Implementations should treat this as a cooperative cancellation signal.
         * Long-running execute() implementations can call isInterrupted() and return early.
         */
        virtual void stop() = 0;

        /** Called to wait until the job is finished. */
        virtual bool wait() = 0;

        /** Called to wait until the job is finished. */
        virtual bool wait( f64 maxWaitTime ) = 0;

        /** Gets a value to know if this job should execute as a coroutine. */
        virtual bool isCoroutine() const = 0;

        /** Sets a value to know if this job should execute as a coroutine. */
        virtual void setCoroutine( bool coroutine ) = 0;

        /** Sets the callback function to be called when the job state hash changed.
         * @param callbackFunction The callback function to be called.
         */
        virtual void setCallbackFunction( std::function<void( int )> callbackFunction ) = 0;

        virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                               const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                               SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IJob_h__
