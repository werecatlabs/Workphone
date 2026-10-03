#ifndef __IWorkerThread_H_
#define __IWorkerThread_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Thread/Thread.hpp>

namespace workphone
{

    /**
     * @class IWorkerThread
     * @brief Interface representing a worker thread that belongs to a thread pool.
     *
     * This interface provides methods for managing and interacting with a worker thread,
     * including thread state, thread ID, target frame rate, and update status. It inherits
     * from ISharedObject to support shared ownership semantics.
     */
    class WPCore_API IWorkerThread : public ISharedObject
    {
    public:
        /**
         * @enum State
         * @brief Enumeration representing the possible states of a worker thread.
         */
        enum class State
        {
            None,  /**< The thread is not started or in an undefined state. */
            Start, /**< The thread is running. */
            Stop,  /**< The thread has been requested to stop or is stopped. */
            Count  /**< Number of states (for internal use). */
        };

        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IWorkerThread() override;

        /**
         * @brief Executes the main functionality of the worker thread.
         *
         * This method should be implemented to define the thread's main loop or task.
         * It is typically called when the thread is started.
         */
        virtual void run() = 0;

        /**
         * @brief Retrieves the ID of the thread.
         *
         * @return Thread::ThreadId The unique identifier of the thread.
         */
        virtual Thread::ThreadId getThreadId() const = 0;

        /**
         * @brief Sets the thread ID for this worker thread.
         *
         * @param threadId The unique identifier to assign to the thread.
         */
        virtual void setThreadId( Thread::ThreadId threadId ) = 0;

        /**
         * @brief Retrieves the target frame rate of the worker thread.
         *
         * @return time_interval The target frame rate as the number of frames per second.
         */
        virtual time_interval getTargetFPS() const = 0;

        /**
         * @brief Sets the target frame rate for the worker thread.
         *
         * @param framesPerSecond The number of frames per second to target.
         */
        virtual void setTargetFPS( time_interval framesPerSecond ) = 0;

        /**
         * @brief Gets the current state of the worker thread.
         *
         * @return State The current state of the thread (None, Start, Stop).
         */
        virtual State getState() const = 0;

        /**
         * @brief Sets the state of the worker thread.
         *
         * @param state The new state to set for the thread.
         */
        virtual void setState( State state ) = 0;

        /**
         * @brief Requests the worker thread to stop execution.
         *
         * This method should signal the thread to terminate its main loop and exit cleanly.
         */
        virtual void stop() = 0;

        /**
         * @brief Checks if the worker thread is currently updating.
         *
         * @return true if the worker thread is performing an update; false otherwise.
         */
        virtual bool isUpdating() const = 0;

        /**
         * @brief Sets the updating status of the worker thread.
         *
         * @param updating Set to true if the thread is updating, false otherwise.
         */
        virtual void setUpdating( bool updating ) = 0;

        /**
         * @brief Macro for class registration (implementation-specific).
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif
