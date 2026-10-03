#ifndef __IFSM_h__
#define __IFSM_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * Interface for a state machine. Provides FSM functionality and the corresponding events.
     */
    class WPCore_API IFSM : public ISharedObject
    {
    public:
        static const u32 isStateChangeCompleteFlag;
        static const u32 autoChangeStateFlag;
        static const u32 isPendingFlag;
        static const u32 isReadyFlag;
        static const u32 isLockedFlag;
        static const u32 allowStateChangeFlag;

        /** Virtual destructor. */
        ~IFSM() override;

        /** Updates the state machine. */
        virtual void updateState() = 0;

        /** Gets the FSM flags. */
        virtual u32 getFlags() const = 0;

        /** Sets the FSM flags. */
        virtual void setFlags( u32 flags ) = 0;

        /**
         * If true, the FSM will automatically transition to `newState` when
         * the configured `stateTime` elapses.
         */
        virtual bool getAutoChangeState() const = 0;

        /**
         * Enable or disable automatic state changes when the state timer
         * expires.
         * @param autoChangeState True to enable automatic changes.
         */
        virtual void setAutoChangeState( bool autoChangeState ) = 0;

        /**
         * Gets the fsm manager.
         * @return A SmartPtr to the FSM manager.
         */
        virtual IFSMManager *getFsmManagerPtr() const = 0;

        /**
         * Gets the fsm manager.
         * @return A SmartPtr to the FSM manager.
         */
        virtual SmartPtr<IFSMManager> getFsmManager() const = 0;

        /**
         * Sets the fsm manager.
         * @param fsmManager The FSM manager to set.
         */
        virtual void setFsmManager( SmartPtr<IFSMManager> fsmManager ) = 0;

        /**
         * Gets the time of the state change.
         * @return A value containing the time of the state change.
         */
        virtual time_interval getStateTime() const = 0;

        /**
         * Sets the time of the state change.
         * @param stateTime The time of the state change.
         */
        virtual void setStateTime( time_interval stateTime ) = 0;

        /**
         * The time elapsed while in the current state.
         * @return A value containing the time elapsed in seconds.
         */
        virtual time_interval getStateTimeElapsed() const = 0;

        /**
         * Gets the previous state.
         * @return Returns an integer containing the previous state.
         */
        virtual u8 getPreviousState() const = 0;

        /**
         * Gets the current state.
         * @return Returns an integer containing the current state.
         */
        virtual u8 getCurrentState() const = 0;

        /**
         * Gets the new state.
         * @return Returns an integer containing the new state.
         */
        virtual u8 getNewState() const = 0;

        /**
         * Sets the new state.
         * @param newState An integer value containing the new state.
         * @param changeNow Used to know if the state should be changed immediately.
         */
        virtual void setNewState( s32 newState, bool changeNow = false ) = 0;

        /**
         * Used to know if a state change is pending. State changes are normally checked and updated per
         * frame.
         * @return true if a state change is pending, false otherwise.
         */
        virtual bool isPending() const = 0;

        /**
         * Adds a listener to the state machine.
         * @param listener The listener to add.
         */
        virtual void addListener( SmartPtr<IFSMListener> listener ) = 0;

        /**
         * Removes a listener from the state machine.
         * @param listener The listener to remove.
         */
        virtual void removeListener( SmartPtr<IFSMListener> listener ) = 0;

        /**
         * Gets the listeners of the state machine.
         * @return An array of SmartPtr to IFSMListener objects.
         */
        virtual Array<SmartPtr<IFSMListener>> getListeners() const = 0;

        /**
         * Gets the number of ticks while in the current state.
         * @param task The task for which to get the number of ticks.
         * @return The number of ticks while in the current state.
         */
        virtual s32 getStateTicks( TaskId task ) const = 0;

        /**
         * Gets the number of ticks while in the current state.
         * @return The number of ticks while in the current state.
         */
        virtual s32 getStateTicks() const = 0;

        /**
         * Set an integer tick counter used by logic that requires discrete
         * tick increments instead of or alongside time intervals.
         * @param ticks Tick count to set.
         */
        virtual void setStateTicks( s32 ticks ) = 0;

        /**
         * @brief Gets the event listener priority.
         * @return The priority value.
         */
        virtual s32 getPriority() const = 0;

        /**
         * @brief Sets the event listener priority.
         * @param priority The priority value.
         */
        virtual void setPriority( s32 priority ) = 0;

        /**
         * Sets the current state.
         * @param state The id of the new state.
         * @param changeNow Used to the know if the state should be change immediately.
         */
        template <class T>
        void setState( T state, bool changeNow = false );

        /**
         * Gets the current state id.
         * @return Returns an integer with the current state.
         */
        template <class T>
        T getState() const;

        WP_CLASS_REGISTER_DECL;
    };

    template <class T>
    void IFSM::setState( T state, bool changeNow )
    {
        const auto iState = static_cast<u8>( state );
        setNewState( iState, changeNow );
    }

    template <class T>
    T IFSM::getState() const
    {
        auto iState = getCurrentState();
        return static_cast<T>( iState );
    }

}  // namespace workphone

#endif  // __IFiniteStateMachine_h__
