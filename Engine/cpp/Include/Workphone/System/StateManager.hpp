#ifndef StateManagerStandard_h__
#define StateManagerStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/HashTable.hpp>
#include <Workphone/Core/HashMap.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /**
     * @class StateManagerOO
     * @brief Object-oriented implementation of the IStateManager interface for managing state contexts
     * and state objects.
     *
     * StateManagerOO provides a comprehensive state management system that coordinates state contexts,
     * state objects, and their updates across multiple threads. It maintains separate state queues for
     * each thread task, handles dirty state tracking and updating, and provides thread-safe operations
     * for state lifecycle management.
     *
     * Key features:
     * - Thread-safe state context and state object management
     * - Per-task state queues for efficient message passing
     * - Dirty state tracking and batched updates
     * - Support for 2D state hierarchies for complex object relationships
     * - Automatic state queue initialization and cleanup
     *
     * The state manager is designed to handle high-frequency state updates efficiently by batching
     * dirty state processing and maintaining separate queues per thread to minimize contention.
     *
     * @see IStateManager
     * @see IStateContext
     * @see IState
     * @see IStateQueue
     * @ingroup System
     */
    class WPCore_API StateManager : public IStateManager
    {
    public:
        static const u32 maxDirtyQueueSize;
        static const String nameStr;

        /**
         * @brief Default constructor.
         *
         * Initializes the state manager with state queues for each thread task and sets up
         * the internal data structures for managing state contexts and dirty state tracking.
         */
        StateManager();

        /**
         * @brief Virtual destructor.
         *
         * Performs cleanup by unloading all managed state contexts and releasing resources.
         */
        ~StateManager() override;

        /**
         * @brief Loads and initializes the state manager.
         * @param data Optional shared object data (currently unused)
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads and cleans up all managed resources.
         *
         * This method performs a complete cleanup of the state manager, including:
         * - Unloading all state contexts
         * - Clearing state queues
         * - Releasing all internal data structures
         *
         * @param data Optional shared object data (currently unused)
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Updates all dirty state contexts for the current thread task.
         *
         * This method processes the dirty queue for the current thread, extraing dirty
         * state contexts and updating them. It performs the following operations:
         * - Extracts dirty contexts from the thread-specific queue
         * - Sorts contexts for consistent processing order
         * - Updates contexts that belong to the current thread task
         * - Removes non-dirty contexts and duplicates from the processing list
         *
         * The update process is designed to be efficient with batched processing and
         * a configurable maximum batch size to prevent frame rate issues.
         */
        void update() override;

        /**
         * @brief Sends a message to the state queue for a specific thread task.
         *
         * Messages are queued in the appropriate task-specific state queue and will be
         * processed by the corresponding thread when it processes its queue.
         *
         * @param taskId The thread task ID that should receive the message
         * @param message The state message to send
         */
        void sendMessage( TaskId taskId, SmartPtr<IStateMessage> message ) override;

        /**
         * @brief Creates and adds a new state context to the manager.
         *
         * Creates a new StateContextOO instance, loads it, and adds it to the managed
         * state contexts collection. The new context is immediately available for use.
         *
         * @return A smart pointer to the newly created and loaded state context
         */
        SmartPtr<IStateContext> addStateContext() override;

        /**
         * @brief Removes a state context from the manager.
         *
         * Unloads the specified state context and removes it from the managed collection.
         * This operation is thread-safe and creates a new internal collection without
         * the removed context.
         *
         * @param stateContext The state context to remove
         * @return True if the context was found and removed, false otherwise
         */
        bool removeStateContext( SmartPtr<IStateContext> stateContext ) override;

        /**
         * @brief Removes a state context by its unique identifier.
         * @param id The unique identifier of the state context to remove.
         * @return True if the state context was found and removed, false otherwise.
         */
        auto removeStateContext( u32 id ) -> bool;

        /**
         * @brief Finds a state context by its unique identifier.
         *
         * Searches through all managed state contexts to find one with the specified ID.
         *
         * @param id The unique identifier of the state context to find
         * @return A smart pointer to the found context, or nullptr if not found
         */
        SmartPtr<IStateContext> findStateContext( u32 id ) const override;

        /**
         * @brief Retrieves all currently managed state contexts.
         *
         * Returns a snapshot of all state contexts currently managed by this state manager.
         * The returned array is a copy and can be safely iterated without affecting the
         * internal state.
         *
         * @return An array containing all managed state contexts
         */
        Array<SmartPtr<IStateContext>> getStateContexts() const override;

        /**
         * @brief Gets the state queue for a specific thread task.
         *
         * Each thread task has its own state queue for processing messages and states.
         * This method provides access to the queue for the specified task.
         *
         * @param taskId The thread task for which to retrieve the queue
         * @return A smart pointer to the state queue for the specified task
         */
        SmartPtr<IStateQueue> getQueue( TaskId taskId ) override;

        /**
         * @brief Marks a state context as dirty and queues it for update.
         *
         * This method marks all states within the context as dirty and adds the context
         * to the appropriate dirty queue based on the context's task ID.
         *
         * @param context The state context to mark as dirty
         */
        void makeDirty( SmartPtr<IStateContext> context ) override;

        /**
         * @brief Marks all managed state contexts as dirty.
         *
         * Iterates through all managed state contexts, marks their states as dirty,
         * and adds them to the appropriate task-specific dirty queues.
         */
        void makeAllDirty() override;

        /**
         * @brief Adds a state context to all dirty queues.
         *
         * This method adds the context to the dirty queue for every thread task,
         * ensuring it will be processed regardless of which thread performs the update.
         *
         * @param context The state context to add to all dirty queues
         */
        void addDirty( SmartPtr<IStateContext> context ) override;

        /**
         * @brief Adds a state context to the dirty queue for a specific task.
         *
         * Adds the context to the dirty queue for the specified thread task only.
         * This is more efficient when the context should only be processed by a
         * specific thread.
         *
         * @param context The state context to add to the dirty queue
         * @param task The specific thread task for which to queue the context
         */
        void addDirty( SmartPtr<IStateContext> context, TaskId task ) override;

        /**
         * @brief Acquires an exclusive lock on the state manager.
         *
         * Provides thread-safe access to the state manager's internal structures.
         * Should be paired with a corresponding unlock() call.
         */
        void lock() override;

        /**
         * @brief Attempts to acquire an exclusive lock without blocking.
         *
         * @return True if the lock was successfully acquired, false otherwise
         */
        bool try_lock() override;

        /**
         * @brief Releases the exclusive lock on the state manager.
         *
         * Should be called after a successful lock() or try_lock() call.
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Gets the number of state queues.
         *
         * @return The total number of state queues (typically equal to TaskId::Count)
         */
        u32 getNumStateQueues() const;

        /**
         * @brief Gets a state queue by index.
         *
         * @param index The index of the state queue to retrieve
         * @return A smart pointer to the state queue at the specified index
         */
        SmartPtr<IStateQueue> getStateQueue( u32 index ) const;

        /**
         * @brief Destroys a state queue and releases its resources.
         *
         * @param queue The state queue to destroy
         */
        void destroyQueue( SmartPtr<IStateQueue> queue );

        /** @brief Concurrent queues for dirty state contexts, one per thread task. */
        ConcurrentArray<ConcurrentQueue<SmartPtr<IStateContext>>> m_dirtyQueue;

        /** @brief Atomic shared pointer to the array of state queues, one per thread task. */
        ConcurrentArray<SmartPtr<IStateQueue>> m_stateQueues;

        /** @brief Atomic shared pointer to the array of managed state contexts. */
        ConcurrentArray<SmartPtr<IStateContext>> m_stateContexts;

        /** @brief Working array for processing dirty state contexts during updates. */
        FixedArray<Array<SmartPtr<IStateContext>>, (u32)TaskId::Count> m_dirtyArray;

        /** @brief Recursive mutex for thread-safe access to internal data structures. */
        mutable RecursiveMutex m_mutex;
    };
}  // namespace workphone

#endif  // StateManagerStandard_h__
