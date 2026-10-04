#ifndef _WorkerThread_H_
#define _WorkerThread_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Interface/System/IWorkerThread.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>

#if WP_ENABLE_ASSERTS
#    include <Workphone/Math/Math.hpp>
#endif

namespace workphone
{

    /**
     * @class WorkerThread
     * @brief Implements a worker thread for executing tasks in parallel.
     *
     * This class provides an implementation of the IWorkerThread interface, managing
     * the lifecycle and execution of a worker thread, including thread state, target FPS,
     * and update control. It is designed for use in systems requiring concurrent task execution.
     */
    class WPCore_API WorkerThread : public IWorkerThread
    {
    public:
        /**
         * @brief Default constructor. Initializes the worker thread object.
         */
        WorkerThread();

        /**
         * @brief Copy constructor (deleted).
         *
         * Copying WorkerThread instances is not allowed.
         * @param other The WorkerThread instance to copy from.
         */
        WorkerThread( const WorkerThread &other ) = delete;

        /**
         * @brief Destructor. Cleans up resources used by the worker thread.
         */
        ~WorkerThread() override;

        /**
         * @brief Loads data or resources required by the worker thread.
         * @param data Shared object containing initialization data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads data or resources used by the worker thread.
         * @param data Shared object containing data to be released.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Starts or runs the worker thread's main execution loop.
         */
        void run() override;

        /**
         * @brief Gets the unique identifier of the worker thread.
         * @return The thread ID.
         */
        Thread::ThreadId getThreadId() const override;

        /**
         * @brief Sets the unique identifier for the worker thread.
         * @param threadId The thread ID to set.
         */
        void setThreadId( Thread::ThreadId threadId ) override;

        /**
         * @brief Gets the target frames per second (FPS) for the worker thread.
         * @return The target FPS as a time interval.
         */
        time_interval getTargetFPS() const override;

        /**
         * @brief Sets the target frames per second (FPS) for the worker thread.
         * @param framesPerSecond The desired FPS as a time interval.
         */
        void setTargetFPS( time_interval framesPerSecond ) override;

        /**
         * @brief Gets the current state of the worker thread.
         * @return The current state.
         */
        State getState() const override;

        /**
         * @brief Sets the current state of the worker thread.
         * @param state The state to set.
         */
        void setState( State state ) override;

        /**
         * @brief Requests the worker thread to stop execution.
         */
        void stop() override;

        /**
         * @brief Checks if the worker thread is currently updating.
         * @return True if updating, false otherwise.
         */
        bool isUpdating() const override;

        /**
         * @brief Sets whether the worker thread should be updating.
         * @param updating True to enable updating, false to disable.
         */
        void setUpdating( bool updating ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Gets the underlying std::thread pointer for this worker thread.
         * @return Pointer to the std::thread object.
         */
        std::thread *getThread() const;

        /**
         * @brief Sets the underlying std::thread pointer for this worker thread.
         * @param thread Pointer to the std::thread object to set.
         */
        void setThread( std::thread *thread );

        /// Atomic pointer to the underlying std::thread object.
        AtomicValue<std::thread *> m_thread;

        /// Target frames per second for the worker thread (default: 5000.0).
        atomic_f64 m_targetFPS = 5000.0;

        /// The worker's index in its owning pool (default: the first worker).
        AtomicValue<Thread::ThreadId> m_workerThreadId = Thread::ThreadId::WorkerThread;

        /// The current state of the worker thread (default: State::None).
        AtomicValue<State> m_state = State::None;

        /// Indicates whether the worker thread is currently updating (default: false).
        atomic_bool m_isUpdating = false;

    };

}  // namespace workphone

#endif
