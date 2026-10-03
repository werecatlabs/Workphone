#ifndef __IThreadPool_H_
#define __IThreadPool_H_

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class IThreadPool
     * @brief Interface for managing a pool of worker threads.
     *
     * This interface provides methods to add, retrieve, and manage worker threads in a thread pool.
     * It allows for dynamic adjustment of the number of threads, querying and setting the pool's state,
     * and stopping all threads. Inherits from ISharedObject for shared ownership semantics.
     */
    class WPCore_API IThreadPool : public ISharedObject
    {
    public:
        /**
         * @brief Enumeration representing the possible states of the thread pool.
         */
        enum class State
        {
            None,  /**< The thread pool is uninitialized or in a neutral state. */
            Start, /**< The thread pool is running and processing tasks. */
            Stop,  /**< The thread pool is stopped and not processing tasks. */

            Count /**< Number of possible states (not a valid state). */
        };

        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IThreadPool() override;

        /**
         * @brief Adds a new worker thread to the pool.
         *
         * @return SmartPtr<IWorkerThread> A smart pointer to the newly added worker thread.
         */
        virtual SmartPtr<IWorkerThread> addWorkerThread() = 0;

        /**
         * @brief Retrieves a worker thread by its index.
         *
         * @param index The zero-based index of the worker thread to retrieve.
         * @return SmartPtr<IWorkerThread> A smart pointer to the worker thread at the specified index.
         */
        virtual SmartPtr<IWorkerThread> getThread( u32 index ) = 0;

        /**
         * @brief Gets the current number of worker threads in the pool.
         *
         * @return u32 The number of worker threads in the pool.
         */
        virtual u32 getNumThreads() const = 0;

        /**
         * @brief Sets the desired number of worker threads in the pool.
         *
         * If the new number is greater than the current, new threads may be created.
         * If less, existing threads may be stopped or removed.
         *
         * @param numThreads The desired number of worker threads in the pool.
         */
        virtual void setNumThreads( u32 numThreads ) = 0;

        /**
         * @brief Gets the current state of the thread pool.
         *
         * @return State The current state of the thread pool.
         */
        virtual State getState() const = 0;

        /**
         * @brief Sets the state of the thread pool.
         *
         * @param state The new state to set for the thread pool.
         */
        virtual void setState( State state ) = 0;

        /**
         * @brief Stops the execution of all worker threads in the thread pool.
         *
         * This method should ensure that all threads are safely stopped and any resources are released.
         */
        virtual void stop() = 0;

        /**
         * @brief Macro for class registration (implementation-specific).
         */
        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif
