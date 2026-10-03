#ifndef FSMManager_h__
#define FSMManager_h__

#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /**
     * @class FSMManager
     * @brief Data-oriented manager for many small finite state machines (FSMs).
     *
     * FSMManager stores runtime state for a large number of lightweight FSM
     * objects using parallel, contiguous arrays. This layout reduces per-FSM
     * allocation overhead and improves cache locality for bulk operations such
     * as ticking, committing pending state transitions and notifying listeners.
     *
     * Responsibilities
     * - Create and destroy IFSM instances and allocate compact integer ids.
     * - Maintain per-FSM bytes for previous/current/pending states using atomic
     *   containers to permit lightweight concurrent access.
     * - Track per-FSM timing (enter time, elapsed time, tick counters) and
     *   boolean flags (ready, allow-change, auto-change, etc.).
     * - Manage per-FSM listener lists and dispatch transition events.
     * - Provide a thread-safe dirty queue to enqueue FSMs for deferred
     *   processing from other threads.
     *
     * Thread-safety
     * - Simple per-FSM operations are safe via atomic containers.
     * - Mutating operations that resize or reassign internal arrays are guarded
     *   by a recursive mutex (`m_mutex`).
     * - `ConcurrentArray` and `ConcurrentQueue` are used for thread-safe
     *   ownership and deferred processing respectively.
     *
     * Growth and performance
     * - The manager keeps a capacity (`m_size`) and grows internal arrays when
     *   new ids are requested. The `m_growSize` controls the allocation
     *   granularity.
     * - For performance, pre-sizing via `setSize` is recommended when creating
     *   many FSMs at startup to avoid repeated reallocations.
     */
    class WPCore_API FSMManager : public IFSMManager
    {
    public:
        /** Constructor. Initializes an empty manager. */
        FSMManager();

        /** Destructor. Releases resources. */
        ~FSMManager() override;

        /**
         * @copydoc ISharedObject::load
         * @param data Configuration or serialized state (optional).
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc ISharedObject::unload
         * @param data Optional data to use during unload.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IObject::update
         *
         * Performs per-frame processing:
         * - processes the dirty FSM queue
         * - applies pending state changes
         * - updates internal time counters
         */
        void update() override;

        /**
         * @copydoc IFSMManager::createFSM
         * @return SmartPtr to a newly created IFSM instance managed by this manager.
         */
        SmartPtr<IFSM> createFSM() override;

        /**
         * @copydoc IFSMManager::destroyFSM
         * Remove and free resources associated with the provided FSM.
         * @param fsm FSM to destroy.
         */
        void destroyFSM( SmartPtr<IFSM> fsm ) override;

        Array<SmartPtr<IFSM>> getFsms() const override;

        u32 getNumFsms() const override;

        /**
         * @copydoc IFSMManager::getStateChangeTime
         * @param id FSM identifier.
         * @return Time since last state change for the FSM.
         */
        f64 getStateChangeTime( u32 id ) const override;

        /**
         * @copydoc IFSMManager::setStateChangeTime
         * @param id FSM identifier.
         * @param stateChangeTime New state change time value.
         */
        void setStateChangeTime( u32 id, const f64 &stateChangeTime ) override;

        /**
         * @copydoc IFSMManager::getPreviousState
         * @param id FSM identifier.
         * @return The previous state byte for the FSM.
         */
        u8 getPreviousState( u32 id ) const override;

        /**
         * @brief Set the previous state for an FSM.
         * @param id FSM identifier.
         * @param state New previous state (signed int converted to u8).
         */
        void setPreviousState( u32 id, s32 state );

        /**
         * @copydoc IFSMManager::getCurrentState
         * @param id FSM identifier.
         * @return The current state byte for the FSM.
         */
        u8 getCurrentState( u32 id ) const override;

        /**
         * @brief Set the current state for an FSM.
         * @param id FSM identifier.
         * @param state New current state (signed int converted to u8).
         */
        void setCurrentState( u32 id, s32 state );

        /**
         * @copydoc IFSMManager::getNewState
         * @param id FSM identifier.
         * @return The pending/new state byte for the FSM.
         */
        u8 getNewState( u32 id ) const override;

        /**
         * @copydoc IFSMManager::setNewState
         * @param id FSM identifier.
         * @param state The state to transition to.
         * @param changeNow If true, apply the state change immediately; otherwise mark pending.
         */
        void setNewState( u32 id, s32 state, bool changeNow = false ) override;

        /**
         * @brief Forcefully override the state for an FSM without following the normal transition flow.
         * @param id FSM identifier.
         * @param state State to set.
         */
        void stateOverride( u32 id, s32 state );

        /**
         * @brief Check if an FSM has a pending state change.
         * @param id FSM identifier.
         * @return True if there is a pending state change.
         */
        bool isPending( u32 id ) const;

        /**
         * @brief Check whether a pending state change has completed for the FSM.
         * @param id FSM identifier.
         * @return True if the state change is complete.
         */
        bool isStateChangeComplete( u32 id ) const;

        /**
         * @brief Mark a state change as complete or not.
         * @param id FSM identifier.
         * @param value New completion value.
         */
        void setStateChangeComplete( u32 id, bool value );

        /**
         * @copydoc IFSMManager::addListener
         * @param id FSM identifier.
         * @param value Listener to add.
         */
        void addListener( u32 id, SmartPtr<IFSMListener> listener ) override;

        /**
         * @copydoc IFSMManager::removeListener
         * @param id FSM identifier.
         * @param value Listener to remove.
         */
        void removeListener( u32 id, SmartPtr<IFSMListener> listener ) override;

        /**
         * @brief Remove all listeners for the specified FSM.
         * @param id FSM identifier.
         */
        void removeListeners( u32 id ) override;

        /**
         * @brief Get whether this FSM automatically changes state when a new state is set.
         * @param id FSM identifier.
         * @return true if auto-change is enabled.
         */
        bool getAutoChangeState( u32 id ) const;

        /**
         * @brief Set whether the FSM should automatically apply new states.
         * @param id FSM identifier.
         * @param value New auto-change value.
         */
        void setAutoChangeState( u32 id, bool value );

        /**
         * @brief Get whether state changes are currently allowed for this FSM.
         * @param id FSM identifier.
         * @return True if state changes are allowed.
         */
        bool getAllowStateChange( u32 id ) const;

        /**
         * @brief Set whether state changes are allowed for this FSM.
         * @param id FSM identifier.
         * @param value Allow/disallow state changes.
         */
        void setAllowStateChange( u32 id, bool value );

        /**
         * @brief Query whether the FSM has been initialised and is ready.
         * @param id FSM identifier.
         * @return True if ready.
         */
        bool isReady( u32 id ) const;

        /**
         * @brief Set the ready flag for an FSM.
         * @param id FSM identifier.
         * @param ready Ready state.
         */
        void setReady( u32 id, bool ready );

        /**
         * @brief Get whether the manager auto-triggers the EnterStateComplete event.
         * @param id FSM identifier.
         * @return True if auto-triggering is enabled.
         */
        bool getAutoTriggerEnterStateComplete( u32 id ) const;

        /**
         * @brief Set auto-trigger behaviour for EnterStateComplete.
         * @param id FSM identifier.
         * @param value New value.
         */
        void setAutoTriggerEnterStateComplete( u32 id, bool value );

        /**
         * @brief Get tick counter for a particular task on the FSM.
         * @param id FSM identifier.
         * @param task Task index.
         * @return Tick count.
         */
        s32 getStateTicks( u32 id, s32 task ) const;

        /**
         * @brief Get the primary tick counter for the FSM.
         * @param id FSM identifier.
         * @return Tick count.
         */
        s32 getStateTicks( u32 id ) const;

        /**
         * @brief Set the primary tick counter for the FSM.
         * @param id FSM identifier.
         * @param value Tick count to set.
         */
        void setStateTicks( u32 id, s32 ticks );

        /**
         * @copydoc IFSMManager::getListenerPriority
         * @param id FSM identifier.
         * @return Listener priority for the FSM.
         */
        u32 getListenerPriority( u32 id ) override;

        /**
         * @copydoc IFSMManager::setListenerPriority
         * @param id FSM identifier.
         * @param priority New priority value.
         */
        void setListenerPriority( u32 id, u32 priority ) override;

        /**
         * @copydoc IFSMManager::getFlagsPtr
         * @param id FSM identifier.
         * @return Pointer to the flags (u32) for direct manipulation.
         */
        u32 *getFlagsPtr( u32 id ) const override;

        /**
         * @brief Get a copy of the flags for an FSM.
         * @param id FSM identifier.
         * @return Flags bitmask.
         */
        u32 getFlags( u32 id );

        /**
         * @brief Set flags for an FSM.
         * @param id FSM identifier.
         * @param flags New flags bitmask.
         */
        void setFlags( u32 id, u32 flags );

        /**
         * @brief Replace listener array for an FSM.
         * @param id FSM identifier.
         * @param listeners New list container (SharedPtr to Array).
         */
        void setListeners( u32 id, const Array<SmartPtr<IFSMListener>> &listeners );

        /**
         * @copydoc IFSMManager::getListeners
         * @param id FSM identifier.
         * @return Shared pointer to the listeners array (may be null).
         */
        Array<SmartPtr<IFSMListener>> getListeners( u32 id ) const override;

        /**
         * @copydoc IFSMManager::getStateTime
         * @param id FSM identifier.
         * @return Time elapsed since entering the current state.
         */
        time_interval getStateTime( u32 id ) const override;

        /**
         * @copydoc IFSMManager::setStateTime
         * @param id FSM identifier.
         * @param stateTime New state time value.
         */
        void setStateTime( u32 id, time_interval stateTime ) override;

        /**
         * @brief Add to the state time for an FSM.
         * @param id FSM identifier.
         * @param stateTime Amount to add.
         */
        void addStateTime( u32 id, time_interval stateTime );

        /**
         * @copydoc ISharedObject::isValid
         * @return True if the internal storage is valid and initialised.
         */
        bool isValid() const override;

        /**
         * @brief Get the current capacity (number of FSM slots allocated).
         * @return Capacity size.
         */
        size_t getSize() const;

        /**
         * @brief Set the capacity for FSM storage. Resizes internal arrays.
         * @param size New capacity.
         */
        void setSize( size_t size );

        /**
         * @brief Get the grow size used when expanding internal arrays.
         * @return Grow size.
         */
        size_t getGrowSize() const;

        /**
         * @brief Set the grow size used when expanding internal arrays.
         * @param growSize New grow size.
         */
        void setGrowSize( size_t growSize );

        /**
         * @brief Accessor for the array of previous states (atomic bytes).
         * @return Shared pointer to the previous states array.
         */
        Array<atomic_u8> getPreviousStates() const;

        /**
         * @brief Set the array used to store previous states.
         * @param previousStates Shared pointer to an atomic byte array.
         */
        void setPreviousStates( const Array<atomic_u8> &previousStates );

        /**
         * @brief Accessor for the array of current states (atomic bytes).
         * @return Shared pointer to the current states array.
         */
        Array<atomic_u8> getCurrentStates() const;

        /**
         * @brief Set the array used to store current states.
         * @param currentStates Shared pointer to an atomic byte array.
         */
        void setCurrentStates( const Array<atomic_u8> &currentStates );

        /**
         * @brief Accessor for the array of new/pending states (atomic bytes).
         * @return Shared pointer to the new states array.
         */
        Array<atomic_u8> getNewStates() const;

        /**
         * @brief Set the array used to store new/pending states.
         * @param newStates Shared pointer to an atomic byte array.
         */
        void setNewStates( const Array<atomic_u8> &newStates );

        /**
         * @brief Perform a state change on all pending FSMs.
         * This will iterate internal arrays and trigger listeners for changed FSMs.
         */
        void changeState() override;

        /**
         * @brief Apply a state change for a single FSM by id.
         * @param id FSM identifier.
         */
        void changeState( u32 id ) override;

        /**
         * @brief Enqueue an FSM for deferred processing.
         *
         * Add the given FSM to the internal dirty queue so it will be
         * processed by a later call to `update` or `changeState`. This is
         * thread-safe and intended for use when an FSM needs evaluation from a
         * non-owner thread.
         *
         * Notes:
         * - The manager stores `SmartPtr<IFSM>` in a `ConcurrentQueue`, so the
         *   FSM object will be kept alive until processed or explicitly
         *         destroyed.
         * - Processing order is unspecified; callers should not rely on FIFO
         *   semantics unless provided by the underlying queue implementation.
         *
         * @param fsm SmartPtr to the IFSM instance to enqueue. Passing a null
         *            pointer has no effect.
         */
        void queueDirtyFSM( SmartPtr<IFSM> fsm ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Resize internal storage arrays to the requested size.
         * @param size New capacity for arrays.
         */
        void resize( size_t size );

        /**
         * @brief Allocate and return a new unique FSM id.
         * @return Newly created id.
         */
        u32 createNewId();

        /// Counter used to hand out unique ids.
        u32 m_idCount = 0;

        /// Number of FSM slots currently allocated.
        size_t m_size = 0;

        /// Grow size used during resize operations (allocation granularity).
        size_t m_growSize = 12;

        /// Per-FSM auto-change state flags
        Array<atomic_bool> m_autoChangeState;

        /// Per-FSM allow state change flags
        Array<atomic_bool> m_allowStateChange;

        /// Per-FSM state change complete flags
        Array<atomic_bool> m_stateChangeComplete;

        /// Per-FSM auto-trigger enter state complete flags
        Array<atomic_bool> m_autoTriggerEnterStateComplete;

        /// Per-FSM flags stored as bitmask values.
        Array<u32> m_flags;

        /// Per-FSM state tick counters
        Array<atomic_s32> m_stateTicks;

        /// Per-FSM listener priorities
        Array<atomic_u32> m_listenerPriority;

        /// Time (timestamp) of the last state change for each FSM.
        Array<time_interval> m_stateChangeTimes;

        /// Elapsed time spent in the current state for each FSM.
        Array<time_interval> m_stateTimes;

        /// Ready flags indicating whether each FSM is initialised and ready.
        Array<atomic_bool> m_ready;

        /// Listeners per-FSM: outer array indexed by FSM id, each entry may be null.
        ConcurrentArray<ConcurrentArray<SmartPtr<IFSMListener>>> m_listeners;

        /// Owned IFSM objects for bookkeeping (may be null entries for free slots).
        ConcurrentArray<SmartPtr<IFSM>> m_fsms;

        /// The previous state (atomic byte) for each FSM.
        ConcurrentArray<atomic_u8> m_previousStates;

        /// The current state (atomic byte) for each FSM.
        ConcurrentArray<atomic_u8> m_currentStates;

        /// The requested/new state (atomic byte) for each FSM.
        ConcurrentArray<atomic_u8> m_newStates;

        /// Queue of FSMs marked dirty for deferred processing (thread-safe).
        ConcurrentQueue<SmartPtr<IFSM>> m_dirtyQueue;

        /// Recursive mutex protecting complex operations and array mutation.
        mutable RecursiveMutex m_mutex;

        /// External id offset (static extension).
        static u32 m_idExt;
    };
}  // namespace workphone

#endif  // FSMManager_h__
