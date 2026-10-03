#ifndef IFSMManager_h__
#define IFSMManager_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for a finite state machine (FSM) manager.
     *
     * This interface provides creation, destruction and management of FSM
     * instances and exposes accessors for state information and listener
     * management. Implementations are responsible for tracking FSM state
     * transitions, notifying listeners and maintaining any timing/flag
     * information associated with FSMs.
     */
    class WPCore_API IFSMManager : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures derived implementations are properly destroyed through
         * the IFSMManager interface.
         */
        ~IFSMManager() override;

        /**
         * @brief Create a new FSM instance.
         *
         * @return A smart pointer to the newly created IFSM instance.
         */
        virtual SmartPtr<IFSM> createFSM() = 0;

        /**
         * @brief Destroy an FSM instance previously created by this manager.
         *
         * After this call the provided smart pointer should no longer be
         * considered valid for use by callers.
         *
         * @param fsm Smart pointer to the FSM to destroy.
         */
        virtual void destroyFSM( SmartPtr<IFSM> fsm ) = 0;

        /**
         * @brief Get all FSM instances managed by this manager.
         *
         * @return An array of smart pointers to the managed FSMs.
         */
        virtual Array<SmartPtr<IFSM>> getFsms() const = 0;

        /**
         * @brief Get the number of FSM instances currently managed.
         *
         * @return The number of FSMs.
         */
        virtual u32 getNumFsms() const = 0;

        /**
         * @brief Get the timestamp for when the state last changed for a
         * specific FSM identified by id.
         *
         * @param id Identifier of the FSM.
         * @return The time interval representing when the state last changed.
         */
        virtual time_interval getStateTime( u32 id ) const = 0;

        /**
         * @brief Set the state timestamp for a specific FSM.
         *
         * This updates the stored time representing when the FSM's current
         * state became active.
         *
         * @param id Identifier of the FSM.
         * @param stateTime Time interval to set for the state change.
         */
        virtual void setStateTime( u32 id, time_interval stateTime ) = 0;

        /**
         * @brief Get the last state-change time for the FSM identified by
         * id as a floating point value.
         *
         * @param id Identifier of the FSM.
         * @return The last state change time as a double (f64).
         */
        virtual f64 getStateChangeTime( u32 id ) const = 0;

        /**
         * @brief Set the last state-change time for the FSM identified by
         * id.
         *
         * @param id Identifier of the FSM.
         * @param stateChangeTime The time value to set.
         */
        virtual void setStateChangeTime( u32 id, const f64 &stateChangeTime ) = 0;

        /**
         * @brief Retrieve the previous state value for the FSM identified by
         * id.
         *
         * @param id Identifier of the FSM.
         * @return The previous state as an unsigned 8-bit integer.
         */
        virtual u8 getPreviousState( u32 id ) const = 0;

        /**
         * @brief Retrieve the current state value for the FSM identified by
         * id.
         *
         * @param id Identifier of the FSM.
         * @return The current state as an unsigned 8-bit integer.
         */
        virtual u8 getCurrentState( u32 id ) const = 0;

        /**
         * @brief Get the pending (next) state for the FSM identified by id.
         *
         * This is the state that will be applied on the next state-change
         * processing step (unless applied immediately).
         *
         * @param id Identifier of the FSM.
         * @return The pending new state as an unsigned 8-bit integer.
         */
        virtual u8 getNewState( u32 id ) const = 0;

        /**
         * @brief Set a new state for the FSM identified by id.
         *
         * The new state may be applied immediately if @p changeNow is true,
         * otherwise it will be queued for the next change processing step.
         *
         * @param id Identifier of the FSM.
         * @param state New state value to set (signed 32-bit).
         * @param changeNow If true the state change is applied immediately.
         */
        virtual void setNewState( u32 id, s32 state, bool changeNow = false ) = 0;

        /**
         * @brief Add a listener for events related to the FSM identified by
         * id.
         *
         * The listener will be notified of state changes and other FSM
         * events according to the manager's notification policy.
         *
         * @param id Identifier of the FSM.
         * @param listener Smart pointer to the listener to add.
         */
        virtual void addListener( u32 id, SmartPtr<IFSMListener> listener ) = 0;

        /**
         * @brief Remove a previously added listener from the FSM identified
         * by id.
         *
         * @param id Identifier of the FSM.
         * @param listener Smart pointer to the listener to remove.
         */
        virtual void removeListener( u32 id, SmartPtr<IFSMListener> listener ) = 0;

        /**
         * @brief Remove all listeners associated with the FSM identified
         * by id.
         *
         * After this call no listeners registered for the specified FSM
         * will receive further notifications.
         *
         * @param id Identifier of the FSM.
         */
        virtual void removeListeners( u32 id ) = 0;

        /**
         * @brief Get the listener priority for the specified FSM.
         *
         * Listener priority can be used by the manager to determine the
         * order in which listeners are invoked.
         *
         * @param id Identifier of the FSM.
         * @return Priority value for listeners of the FSM.
         */
        virtual u32 getListenerPriority( u32 id ) = 0;

        /**
         * @brief Set the listener priority for the specified FSM.
         *
         * @param id Identifier of the FSM.
         * @param priority Priority value to assign to the FSM's listeners.
         */
        virtual void setListenerPriority( u32 id, u32 priority ) = 0;

        /**
         * @brief Get a pointer to the flags storage for the FSM identified
         * by id.
         *
         * Flags are implementation-defined bitfields associated with an
         * FSM instance and may be used to store small pieces of state or
         * configuration.
         *
         * @param id Identifier of the FSM.
         * @return Pointer to the flags storage (u32) or nullptr if not
         *         available.
         */
        virtual u32 *getFlagsPtr( u32 id ) const = 0;

        /**
         * @brief Retrieve the collection of listeners registered for the
         * specified FSM.
         *
         * @param id Identifier of the FSM.
         * @return Shared pointer to an array of smart pointers to the
         *         listeners.
         */
        virtual Array<SmartPtr<IFSMListener>> getListeners( u32 id ) const = 0;

        /**
         * @brief Process and apply pending state changes for all managed
         * FSMs.
         *
         * Implementations should iterate pending changes, apply them and
         * notify any affected listeners.
         */
        virtual void changeState() = 0;

        /**
         * @brief Process and apply pending state changes for a single FSM.
         *
         * @param id Identifier of the FSM to process.
         */
        virtual void changeState( u32 id ) = 0;

        /**
         * @brief Mark an FSM as dirty so it will be processed on the next
         * state-change pass.
         *
         * This is useful for deferring state transitions until the manager
         * runs its change processing.
         *
         * @param fsm Smart pointer to the FSM to queue.
         */
        virtual void queueDirtyFSM( SmartPtr<IFSM> fsm ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // IFSMManager_h__
