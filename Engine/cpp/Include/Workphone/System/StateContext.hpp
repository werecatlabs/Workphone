#ifndef StateObjectStandard_h__
#define StateObjectStandard_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{

    /**
     * @brief Object-oriented implementation of the IStateContext interface.
     *
     * StateContextOO provides a thread-safe, object-oriented approach to state management
     * within the workphone framework. It manages state objects, message queues, event listeners,
     * and state listeners, enabling complex state-driven behavior in multi-threaded environments.
     *
     * This class acts as a central hub for state management, allowing objects to:
     * - Maintain multiple states concurrently
     * - Queue and process state messages across different threads
     * - Listen to state changes and events
     * - Manage dirty state tracking for efficient updates
     *
     * @par Thread Safety
     * This class is designed to be thread-safe and can be accessed from multiple threads
     * concurrently. State queues are maintained per thread task to ensure proper isolation.
     *
     * @par Usage Example
     * @code
     * auto stateContext = make_ptr<StateContextOO>();
     * stateContext->load(nullptr);
     * stateContext->addState(myState);
     * stateContext->addStateListener(myListener);
     * @endcode
     *
     * @see IStateContext
     * @see IState
     * @see IStateListener
     * @author Workphone Framework Team
     */
    class WPCore_API StateContext : public IStateContext
    {
    public:
        /**
         * @brief Internal event listener for handling shared object lifecycle events.
         *
         * This nested class monitors the owner object's lifecycle events, particularly
         * loading state changes, to ensure proper state context synchronization.
         * It automatically registers/unregisters itself with the state manager when
         * the owner object's loading state changes.
         */
        class SharedObjectListener : public IEventListener
        {
        public:
            /**
             * @brief Default constructor.
             */
            SharedObjectListener();

            /**
             * @brief Destructor.
             */
            ~SharedObjectListener() override;

            /**
             * @brief Unloads the listener and clears its owner reference.
             * @param data Shared object data (unused)
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Handles events from the owner object.
             *
             * Specifically monitors loading state changes to trigger state context
             * updates when the owner becomes loaded.
             *
             * @param eventType The type of event
             * @param eventValue The event identifier
             * @param arguments Event arguments
             * @param sender The object that sent the event
             * @param object The target object
             * @param event The event object
             * @return Event handling result
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @brief Gets the owner state context.
             * @return Weak reference to the owning StateContextOO, or nullptr if invalid
             */
            SmartPtr<StateContext> getOwner() const;

            /**
             * @brief Sets the owner state context.
             * @param owner The StateContextOO that owns this listener
             */
            void setOwner( SmartPtr<StateContext> owner );

            WP_CLASS_REGISTER_DECL;

        private:
            /// Weak reference to the owning state context to prevent circular dependencies
            AtomicWeakPtr<StateContext> m_owner;
        };

        /**
         * @brief Default constructor.
         *
         * Initializes the state context with default values. The context will need to be
         * loaded before use.
         */
        StateContext();

        /**
         * @brief Constructor with identifier.
         * @param id Unique identifier for this state context
         */
        explicit StateContext( u32 id );

        /**
         * @brief Destructor.
         *
         * Automatically unloads the context and cleans up all resources.
         */
        ~StateContext() override;

        /** @copydoc IStateContext::load */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IStateContext::unload */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Updates the state context by processing queued messages and dirty states.
         *
         * This method should be called regularly to process state changes and messages.
         * It processes messages from the current thread's state queue and notifies
         * listeners of any dirty states that need updating.
         *
         * @note This method should only be called from the thread that matches getTaskId()
         *
         * @par Complexity
         * O(n + m) where n is the number of states and m is the number of queued messages
         */
        void update() override;

        /** @copydoc IStateContext::addMessage */
        void addMessage( TaskId taskId, SmartPtr<IStateMessage> message ) override;

        /** @copydoc IStateContext::setDirty */
        void setDirty( bool dirty, bool cascade = true ) override;

        /** @copydoc IStateContext::isDirty */
        bool isDirty() const override;

        /**
         * @brief Checks if the state context itself is dirty (internal state changed).
         *
         * This is different from isDirty() which checks if any managed states are dirty.
         * This method specifically tracks changes to the state context's internal state.
         *
         * @return true if the state context's internal state has changed since last update
         */
        bool isStateDirty() const;

        /**
         * @brief Marks the state context's internal state as dirty or clean.
         *
         * @param dirty true to mark as dirty (increment change counter),
         *              false to mark as clean (sync update counter)
         */
        void setStateDirty( bool dirty );

        /**
         * @brief Adds a state listener to receive state change notifications.
         *
         * State listeners are notified when states become dirty or when state messages
         * are processed. Multiple listeners can be added and will be called in the order
         * they were added.
         *
         * @param stateListener The listener to add (must not be null)
         *
         * @par Thread Safety
         * This method is thread-safe and can be called from any thread.
         */
        void addStateListener( SmartPtr<IStateListener> stateListener ) override;

        /**
         * @brief Removes a previously added state listener.
         *
         * @param stateListener The listener to remove
         * @return true if the listener was found and removed, false otherwise
         *
         * @par Thread Safety
         * This method is thread-safe and can be called from any thread.
         */
        bool removeStateListener( SmartPtr<IStateListener> stateListener ) override;

        /** @copydoc IStateContext::getStateListeners */
        Array<SmartPtr<IStateListener>> getStateListeners() const override;

        /** @copydoc IStateContext::addEventListener */
        void addEventListener( SmartPtr<IEventListener> eventListener ) override;

        /** @copydoc IStateContext::removeEventListener */
        bool removeEventListener( SmartPtr<IEventListener> eventListener ) override;

        /** @copydoc IStateContext::getEventListeners */
        Array<SmartPtr<IEventListener>> getEventListeners() const override;

        /**
         * @brief Sets the owner object for this state context.
         *
         * The owner object is typically the object whose state this context manages.
         * When an owner is set, the context automatically listens for the owner's
         * lifecycle events and manages state synchronization accordingly.
         *
         * @param owner The object that owns this state context
         */
        void setOwner( SmartPtr<ISharedObject> owner ) override;

        /** @copydoc IStateContext::getOwner */
        SmartPtr<ISharedObject> getOwner() const override;

        /**
         * @brief Get the raw owner pointer (no smart pointer dereference).
         *
         * Returns the raw pointer stored in the owner's atomic weak pointer
         * without locking the weak reference or touching the pointed-to object.
         * This is safe to call even when the owner object has already been
         * destroyed: the returned pointer may be dangling, but no read of the
         * pointee is performed.
         *
         * Useful for owner identity comparisons during teardown when the
         * owner may already be gone and constructing a SmartPtr would crash.
         */
        ISharedObject *getOwnerPtr() const;

        /**
         * @brief Gets the state queue for a specific thread task.
         *
         * Each thread task has its own message queue to ensure thread isolation.
         * Messages added via addMessage() are queued in the appropriate task's queue.
         *
         * @param taskId The thread task identifier
         * @return State queue for the specified task, or nullptr if none exists
         *
         * @warning Returns nullptr if taskId is invalid or queues are not initialized
         */
        SmartPtr<IStateQueue> getStateQueue( u32 taskId );

        /**
         * @brief Gets the state queue for a specific thread task (const version).
         * @param taskId The thread task identifier
         * @return State queue for the specified task, or nullptr if none exists
         */
        SmartPtr<IStateQueue> getStateQueue( u32 taskId ) const;

        /**
         * @brief Immediately sends a message to all state listeners.
         *
         * Unlike addMessage(), this bypasses the message queue system and immediately
         * delivers the message to all registered state listeners.
         *
         * @param message The message to send
         *
         * @par Thread Safety
         * This method should be called from the appropriate thread context.
         */
        void sendMessage( SmartPtr<IStateMessage> message ) override;

        /**
         * @brief Processes a state update by notifying all listeners.
         *
         * This is an internal method used during the update cycle to notify listeners
         * when a state has changed.
         *
         * @param state The state that has been updated
         *
         * @warning This is an internal method and should not be called directly by users
         */
        void _processStateUpdate( SmartPtr<IState> &state );

        /** @copydoc IStateContext::addState */
        void addState( SmartPtr<IState> state ) override;

        /** @copydoc IStateContext::removeState */
        void removeState( SmartPtr<IState> state ) override;

        void removeStatesById( hash_type id ) override;

        void clear() override;

        /** @copydoc IStateContext::getStateById */
        SmartPtr<IState> getStateById( hash_type id ) const override;

        /** @copydoc IStateContext::getStateById */
        SmartPtr<IState> getStateById( hash_type id, u32 type ) const override;

        /** @copydoc IStateContext::getStateByTypeId */
        SmartPtr<IState> getStateByTypeId( u32 typeId ) const override;

        /** @copydoc IStateContext::getStateDataPtrById */
        void *getStateDataPtrById( hash_type id, hash_type typeinfo ) const override;

        /** @copydoc IStateContext::getStates */
        Array<SmartPtr<IState>> getStates() const override;

        /**
         * @brief Gets the current update state flag.
         * @return true if state updates are enabled, false otherwise
         */
        bool getUpdateState() const;

        /**
         * @brief Enables or disables state updates.
         *
         * When disabled, the update() method will skip processing.
         *
         * @param updateState true to enable updates, false to disable
         */
        void setUpdateState( bool updateState );

        /** @copydoc IStateContext::getProperties */
        SmartPtr<Properties> getProperties() const override;

        /** @copydoc IStateContext::setProperties */
        void setProperties( SmartPtr<Properties> properties ) override;

        /** @copydoc IStateContext::getTaskId */
        TaskId getTaskId() const override;

        /** @copydoc IStateContext::setTaskId */
        void setTaskId( TaskId task ) override;

        /** @copydoc IStateContext::invalidateState */
        void invalidateState() override;

        /** @copydoc IStateContext::triggerEvent */
        Parameter triggerEvent( EventType eventType, hash_type eventValue,
                                const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override;

        /** @copydoc ISharedObject::isValid */
        bool isValid() const override;

        WP_CLASS_REGISTER_DECL;

    protected:
        Array<SmartPtr<IState>> snapshotStates() const;
        void clearStateNodes();
        Array<SmartPtr<IStateListener>> snapshotStateListeners() const;
        void clearStateListenerNodes();
        Array<SmartPtr<IStateQueue>> snapshotStateQueues() const;
        void clearStateQueueNodes();
        void appendStateQueue( SmartPtr<IStateQueue> stateQueue );
        Array<SmartPtr<IEventListener>> snapshotEventListeners() const;
        void clearEventListenerNodes();

        /**
         * @brief Checks if a specific bit is set in a flags value.
         * @param flags The flags value to check
         * @param bitIdx The bit index to test (0-based)
         * @return true if the bit is set, false otherwise
         */
        bool isBitSet( u32 flags, s32 bitIdx ) const;

        /**
         * @brief Gets the current dirty flags.
         * @return Bitmask representing which aspects are dirty
         */
        u32 getDirtyFlags() const;

        /**
         * @brief Sets the dirty flags.
         * @param dirtyFlags Bitmask of dirty flags to set
         */
        void setDirtyFlags( u32 dirtyFlags );

        /**
         * @brief Sets or clears a specific dirty flag.
         * @param flag The flag bitmask to modify
         * @param value true to set the flag, false to clear it
         */
        void setDirtyFlag( u32 flag, bool value );

        /**
         * @brief Sets the state listeners collection.
         * @param listeners New collection of state listeners
         */
        void setStateListeners( Array<SmartPtr<IStateListener>> listeners );

        /**
         * @brief Sets the event listeners collection.
         * @param eventListeners New collection of event listeners
         */
        void setEventListeners( Array<SmartPtr<IEventListener>> eventListeners );

        /**
         * @brief Checks if message queuing is enabled.
         * @return true if messages are queued, false if sent immediately
         */
        bool getEnableMessageQueues() const;

        /// The thread task this context is associated with
        AtomicValue<TaskId> m_taskId = TaskId::Primary;

        /// Event listener for monitoring owner object lifecycle
        AtomicSmartPtr<IEventListener> m_sharedObjectListener;

        /// Weak reference to the owner object to prevent circular dependencies
        AtomicWeakPtr<ISharedObject> m_owner;

        /// Single state object (legacy support)
        AtomicSmartPtr<IState> m_state;

        /// Flag indicating if this context is registered with the state manager
        atomic_bool m_isAdded = false;

        /// Flag controlling whether messages are queued or sent immediately
        atomic_bool m_enableMessageQueues = true;

        /// Atomic flag for general dirty state tracking
        atomic_bool m_isDirty = false;

        /// Atomic flag controlling update processing
        atomic_bool m_bUpdateState = false;

        /// Counter for state changes (used for dirty tracking)
        atomic_u32 m_stateChangeCount;

        /// Counter for state updates (used for dirty tracking)
        atomic_u32 m_stateUpdateCount;

        /// Counter for removal operations
        atomic_u32 m_removeCount;

        /// Null-terminated, C-style linked list of event listeners
        IEventListener *m_eventListenersHead = nullptr;
        IEventListener *m_eventListenersTail = nullptr;
        mutable RecursiveSpinMutex m_eventListenersMutex;

        /// Null-terminated, C-style linked list of managed states
        IState *m_statesHead = nullptr;
        IState *m_statesTail = nullptr;
        mutable RecursiveSpinMutex m_statesMutex;

        /// Null-terminated, C-style linked list of state change listeners
        IStateListener *m_listenersHead = nullptr;
        IStateListener *m_listenersTail = nullptr;
        mutable RecursiveSpinMutex m_listenersMutex;

        /// Null-terminated, C-style linked list of per-thread message queues
        IStateQueue *m_stateQueuesHead = nullptr;
        IStateQueue *m_stateQueuesTail = nullptr;
        mutable RecursiveSpinMutex m_stateQueuesMutex;

        /// Static counter for generating unique name extensions
        static u32 m_nextGeneratedNameExt;
    };

}  // namespace workphone

#endif  // StateObjectStandard_h__
