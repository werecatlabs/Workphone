#ifndef __FSM_h__
#define __FSM_h__

#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>

namespace workphone
{
    /**
     * @brief Data-oriented finite state machine implementation.
     *
     * This class implements the IFSM interface and provides a lightweight,
     * data-oriented finite state machine used by the engine to manage
     * state transitions, timing and listeners. It is intended for use with
     * an external IFSMManager which coordinates multiple FSM instances.
     *
     * Key responsibilities:
     * - Track current, previous and pending (new) state ids.
     * - Track how long the FSM has been in the current state.
     * - Support automatic and manual state changes and completion signalling.
     * - Maintain and notify IFSMListener observers of state changes.
     *
     * Thread-safety:
     * - Internal atomic members are used for basic concurrent access to
     *   timing and flags. Complex coordination (e.g. listener modifications
     *   while firing events) should be performed from a single thread or
     *   externally synchronized by the caller.
     *
     * Lifecycle:
     * - Use `load` / `unload` (IObject API) to initialise and teardown
     *   associated data. `update` should be called periodically (per-frame)
     *   to advance timers and process pending state changes.
     */
    class WPCore_API FSM : public IFSM
    {
    public:
        /** Default constructor. Creates an FSM with default timings and flags. */
        FSM();

        /** Deleted copy constructor to prevent copying. */
        FSM( const FSM &other ) = delete;

        /** Destructor. Releases any allocated resources and unregisters listeners. */
        ~FSM() override;

        /**
         * @copydoc IObject::load
         *
         * Loads serialized or runtime data required by this FSM.
         * Typical use: restore state id, timings, listeners or configuration
         * supplied by the provided `data` object.
         *
         * @param data A shared object containing serialized or runtime data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IObject::unload
         *
         * Unloads or clears data previously provided via `load`.
         * After unload, the FSM should be safe for destruction or reuse.
         *
         * @param data Optional shared object context used when unloading.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IFSM::getFlags */
        u32 getFlags() const override;

        /** @copydoc IFSM::setFlags */
        void setFlags( u32 flags ) override;

        /**
         * @copydoc IObject::update
         *
         * Advance the FSM. This method should be called periodically (e.g.
         * once per frame). It will update internal timers and will trigger
         * state changes if configured to auto-change or if a pending state
         * has been scheduled with `setNewState(..., true)`.
         */
        void update() override;

        /**
         * @brief Process a pending state transition immediately.
         *
         * This method performs the logic required to leave the current state
         * and enter the new state (if any). It is separated from `update`
         * to allow callers to explicitly force state processing.
         */
        void updateState();

        /**
         * @copydoc IFSM::getFsmManager
         *
         * Returns the manager coordinating this FSM, if present.
         */
        IFSMManager *getFsmManagerPtr() const override;

        /**
         * @copydoc IFSM::getFsmManager
         *
         * Returns the manager coordinating this FSM, if present.
         */
        SmartPtr<IFSMManager> getFsmManager() const override;

        /**
         * @copydoc IFSM::setFsmManager
         *
         * Associates this FSM with an IFSMManager. The manager reference is
         * stored as an AtomicWeakPtr to avoid strong cyclic ownership.
         *
         * @param fsmManager Manager to associate with this FSM.
         */
        void setFsmManager( SmartPtr<IFSMManager> fsmManager ) override;

        /**
         * @copydoc IFSM::getStateTime
         *
         * Returns the configured duration for the current state, in engine
         * time units (time_interval).
         */
        time_interval getStateTime() const override;

        /**
         * @copydoc IFSM::setStateTime
         *
         * Sets the duration the FSM should remain in the current state.
         *
         * @param stateTime Duration in time_interval units.
         */
        void setStateTime( time_interval stateTime ) override;

        /**
         * @copydoc IFSM::getStateTimeElapsed
         *
         * Returns the elapsed time spent in the current state.
         */
        time_interval getStateTimeElapsed() const override;

        /**
         * @copydoc IFSM::getPreviousState
         *
         * Returns the previous state identifier (u8). If no previous state
         * exists, the value typically reflects the default initialized value.
         */
        u8 getPreviousState() const override;

        /**
         * @copydoc IFSM::getCurrentState
         *
         * Returns the current state identifier (u8).
         */
        u8 getCurrentState() const override;

        /**
         * @copydoc IFSM::getNewState
         *
         * Returns the pending state identifier (u8). If no state is pending,
         * this will typically match `getCurrentState()` or a sentinel value.
         */
        u8 getNewState() const override;

        /**
         * @copydoc IFSM::setNewState
         *
         * Schedule a state change. If `changeNow` is true, attempt to perform
         * the transition immediately (via `updateState`). Otherwise mark the
         * FSM as pending and handle transition during the next `update`.
         *
         * @param state New state id (s32). Converted/stored internally.
         * @param changeNow If true, perform transition immediately.
         */
        void setNewState( s32 state, bool changeNow = false ) override;

        /**
         * @copydoc IFSM::stateOverride
         *
         * Force the FSM into the specified state immediately, bypassing
         * normal transition logic. Use with caution — listeners will still
         * be notified of the change but any "exit" semantics for the previous
         * state may be skipped.
         *
         * @param state State id to override to.
         */
        void stateOverride( s32 state );

        /**
         * @copydoc IFSM::isPending
         *
         * Returns true when a state change has been scheduled but not yet
         * processed.
         */
        bool isPending() const override;

        /**
         * @copydoc IFSM::isStateChangeComplete
         *
         * Returns true if the FSM has finished any post-transition processing
         * and is not waiting for completion callbacks.
         */
        bool isStateChangeComplete() const;

        /**
         * @copydoc IFSM::setStateChangeComplete
         *
         * Mark the state change completion flag. When set to true any
         * pending "enter complete" behaviour will be skipped/acknowledged.
         *
         * @param stateChangeComplete Boolean indicating whether state change
         *                            processing is complete.
         */
        void setStateChangeComplete( bool stateChangeComplete );

        /**
         * @copydoc IFSM::addListener
         *
         * Add an IFSMListener that will receive notifications for state
         * transitions and related events. Duplicate listeners should be
         * avoided by the caller.
         *
         * @param listener Listener to add.
         */
        void addListener( SmartPtr<IFSMListener> listener ) override;

        /**
         * @copydoc IFSM::removeListener
         * Remove a previously added IFSMListener.
         * @param listener Listener to remove.
         */
        void removeListener( SmartPtr<IFSMListener> listener ) override;

        /** @copydoc IFSM::getListeners
         * Returns a copy of the internal array of listeners.
         */
        Array<SmartPtr<IFSMListener>> getListeners() const;

        /**
         * @copydoc IFSM::getAutoChangeState
         * If true, the FSM will automatically transition to `newState` when
         * the configured `stateTime` elapses.
         */
        bool getAutoChangeState() const;

        /**
         * @copydoc IFSM::setAutoChangeState
         * Enable or disable automatic state changes when the state timer
         * expires.
         * @param autoChangeState True to enable automatic changes.
         */
        void setAutoChangeState( bool autoChangeState );

        /**
         * @copydoc IFSM::getAllowStateChange
         * Returns whether state changes are allowed. When false any attempt
         * to schedule a new state will be ignored.
         */
        bool getAllowStateChange() const;

        /**
         * @copydoc IFSM::setAllowStateChange
         * Enable or disable the ability to change state.
         * @param allowStateChange True to allow state changes.
         */
        void setAllowStateChange( bool allowStateChange );

        /**
         * @copydoc IFSM::getAutoTriggerEnterStateComplete
         * Returns whether the FSM automatically triggers the "enter state
         * complete" sequence immediately after entering a new state.
         */
        bool getAutoTriggerEnterStateComplete() const;

        /**
         * @copydoc IFSM::setAutoTriggerEnterStateComplete
         * When enabled the FSM will automatically mark enter-state processing
         * as complete (and notify listeners) immediately after a state enter.
         * @param autoTriggerEnterStateComplete True to auto-trigger completion.
         */
        void setAutoTriggerEnterStateComplete( bool autoTriggerEnterStateComplete );

        /**
         * @copydoc IFSM::getStateTicks
         * Returns the current tick count associated with the FSM for the
         * specified task or the default thread task.
         * @param task TaskId enum value identifying a task tick counter.
         */
        s32 getStateTicks( TaskId task ) const override;

        /**
         * @copydoc IFSM::getStateTicks
         * Returns the default state tick count.
         */
        s32 getStateTicks() const override;

        /**
         * @copydoc IFSM::setStateTicks
         *
         * Set an integer tick counter used by logic that requires discrete
         * tick increments instead of or alongside time intervals.
         * @param ticks Tick count to set.
         */
        void setStateTicks( s32 ticks );

        /** Gets the event listener priority.
         * @return The priority value.
         */
        virtual s32 getPriority() const;

        /** Sets the event listener priority.
         * @param priority The priority value.
         */
        virtual void setPriority( s32 priority );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Notify and finalize a state change completion.
         *
         * Called internally to signal that any pending change has completed
         * and to notify listeners. This centralises completion semantics so
         * that callers do not need to repeat notification logic.
         */
        void triggerStateChangeComplete();

        /** Weak pointer to the owning FSM manager (if any). */
        AtomicWeakPtr<IFSMManager> m_fsmManager;

        /** Identifier of the thread owning this FSM's main operations. */
        atomic_u32 m_threadId = 0;

        /** Integer tick counter used by some FSM logic. */
        atomic_u32 m_stateTicks = 0;

        /// The event listener priority.
        s32 m_eventListenerPriority = 0;

        /**
         * Pointer to a small flags array (bitfield) used to store boolean
         * configuration and runtime flags such as pending state, auto-change,
         * allow-change etc. Stored as a pointer to preserve POD layout and
         * allow compact copying where required.
         */
        u32 *m_flags = nullptr;
    };

    inline IFSMManager *FSM::getFsmManagerPtr() const
    {
        return m_fsmManager.get();
    }

}  // namespace workphone

#endif  // __FSM_h__
