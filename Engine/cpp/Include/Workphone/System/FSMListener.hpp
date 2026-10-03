#ifndef __FiniteStateMachineListener_h__
#define __FiniteStateMachineListener_h__

#include <Workphone/Interface/System/IFSMListener.hpp>

namespace workphone
{
    /**
     * @brief Base implementation of an FSM listener.
     *
     * This class provides a default implementation of the @c IFSMListener
     * interface. It stores a weak reference to the associated finite state
     * machine and provides overridable hooks for load, unload and event
     * handling. Derive from this class to implement custom listeners that
     * observe or react to FSM events.
     */
    class WPCore_API FSMListener : public IFSMListener
    {
    public:
        /**
         * @brief Construct a new FSMListener.
         *
         * The listener is created without an associated FSM. Use
         * @c setFSM to attach an FSM instance.
         */
        FSMListener();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup in derived classes.
         */
        ~FSMListener() override;

        /**
         * @copydoc IFSMListener::load
         *
         * Called when the listener should load or initialize resources.
         * The optional @p data parameter can carry initialization details
         * in a shared object.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IFSMListener::unload
         *
         * Called when the listener should release resources. The optional
         * @p data parameter can carry cleanup details in a shared object.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc IFSMListener::handleEvent
         *
         * Handle a single event for the given @p state. Returns an
         * @c FSMReturnType value that indicates how the state machine
         * should continue processing (for example whether the event was
         * handled or should be propagated).
         *
         * @param state The current FSM state identifier.
         * @param eventType The event being delivered to the listener.
         * @return FSMReturnType Indicates how the FSM should proceed.
         */
        FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

        /**
         * @copydoc IFSMListener::getFSM
         *
         * Retrieve a strong reference to the associated FSM. If the weak
         * reference has expired this will return a null smart pointer.
         *
         * @return SmartPtr<IFSM> Strong pointer to the FSM or null.
         */
        SmartPtr<IFSM> getFSM() const override;

        /**
         * @copydoc IFSMListener::setFSM
         *
         * Attach or replace the associated FSM. The listener stores a weak
         * reference so attaching a smart pointer does not change the FSM's
         * ownership semantics.
         *
         * @param fsm Strong pointer to the FSM to associate with this
         * listener. Can be null to detach.
         */
        void setFSM( SmartPtr<IFSM> fsm ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Weak reference to the associated FSM.
         *
         * A weak pointer is used to avoid extending the lifetime of the
         * finite state machine. Callers should use @c getFSM to obtain a
         * strong reference before performing operations on the FSM.
         */
        WeakPtr<IFSM> m_fsm;
    };
}  // namespace workphone

#endif  // FiniteStateMachineListener_h__
