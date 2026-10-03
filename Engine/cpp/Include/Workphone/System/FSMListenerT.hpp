#ifndef FSMListenerT_h__
#define FSMListenerT_h__

#include <Workphone/Interface/System/IFSMListener.hpp>
#include <Workphone/Interface/System/IFSM.hpp>

/**
 * @file FSMListenerT.hpp
 * @brief Templated convenience implementation of @c IFSMListener.
 *
 * This header provides a small template class intended to be used as a
 * base for FSM listeners that are associated with a concrete owner type
 * `T`. It provides default (no-op) implementations for the listener
 * callbacks and stores weak references to both the owner and the FSM to
 * avoid ownership cycles.
 */

namespace workphone
{
    /**
     * @brief Generic FSM listener bound to an owner type `T`.
     *
     * The template holds weak references to the owner and the FSM and
     * provides default implementations for @c IFSMListener methods so
     * derived classes only need to override the callbacks they require.
     *
     * @tparam T The owner type this listener is bound to.
     */
    template <class T>
    class FSMListenerT : public IFSMListener
    {
    public:
        /**
         * @brief Construct a new listener.
         */
        FSMListenerT();

        /**
         * @brief Virtual default destructor.
         */
        ~FSMListenerT() override;

        /**
         * @brief Load listener state from a shared object.
         *
         * Default implementation does nothing. Derived classes may use
         * this to initialize state when the listener is attached.
         *
         * @param data Optional persisted state object provided by the caller.
         *
         * @copydoc IFSMListener::load
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload listener state and release internal references.
         *
         * The base implementation clears the stored FSM reference.
         * Derived implementations should call the base when overriding
         * to ensure references are released.
         *
         * @param data Optional object to store state before unloading.
         *
         * @copydoc IFSMListener::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Handle an event emitted by the FSM.
         *
         * Default implementation returns @c FSMReturnType::Ok and does
         * not react to the event. Override to implement custom behavior.
         *
         * @param state Current state identifier reported by the FSM.
         * @param eventType The event that occurred.
         * @return FSMReturnType Result that informs the FSM on the outcome.
         *
         * @copydoc IFSMListener::handleEvent
         */
        FSMReturnType handleEvent( u32 state, FSMEvent eventType ) override;

        /**
         * @brief Retrieve a strong pointer to the associated FSM.
         *
         * The stored FSM is kept as a weak pointer; this method returns a
         * strong pointer if the FSM is still alive or nullptr otherwise.
         *
         * @return SmartPtr<IFSM> Strong pointer to the FSM or nullptr.
         */
        SmartPtr<IFSM> getFSM() const override;

        /**
         * @brief Assign the FSM instance this listener observes.
         *
         * The provided pointer is stored as a weak reference.
         *
         * @param fsm Smart pointer to the FSM to observe.
         */
        void setFSM( SmartPtr<IFSM> fsm ) override;

        /**
         * @brief Get the weak pointer to the owner object.
         *
         * @return WeakPtr<T> Weak pointer referencing the owner.
         */
        WeakPtr<T> getOwner() const;

        /**
         * @brief Set the owner for this listener.
         *
         * The listener keeps only a weak reference to the owner to avoid
         * forming a reference cycle.
         *
         * @param owner Weak pointer to the owner instance.
         */
        void setOwner( WeakPtr<T> owner );

        WP_CLASS_REGISTER_TEMPLATE_DECL( FSMListenerT, T );

    protected:
        /**
         * @brief Weak reference to the owner of this listener.
         *
         * Stored as weak to avoid participating in lifetime management
         * of the owner object.
         */
        WeakPtr<T> m_owner;

        /**
         * @brief Weak reference to the associated FSM instance.
         *
         * Use getFSM() to obtain a strong reference before accessing the FSM.
         */
        WeakPtr<IFSM> m_fsm;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, FSMListenerT, T, IFSMListener );

    template <class T>
    FSMListenerT<T>::FSMListenerT() = default;

    template <class T>
    FSMListenerT<T>::~FSMListenerT() = default;

    template <class T>
    void FSMListenerT<T>::load( SmartPtr<ISharedObject> data )
    {
    }

    template <class T>
    void FSMListenerT<T>::unload( SmartPtr<ISharedObject> data )
    {
        m_fsm = nullptr;
        m_owner = nullptr;
    }

    template <class T>
    FSMReturnType FSMListenerT<T>::handleEvent( [[maybe_unused]] u32 state,
                                                [[maybe_unused]] FSMEvent eventType )
    {
        return FSMReturnType::Ok;
    }

    template <class T>
    SmartPtr<IFSM> FSMListenerT<T>::getFSM() const
    {
        auto fsm = m_fsm.lock();
        return fsm;
    }

    template <class T>
    void FSMListenerT<T>::setFSM( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

    template <class T>
    WeakPtr<T> FSMListenerT<T>::getOwner() const
    {
        return m_owner;
    }

    template <class T>
    void FSMListenerT<T>::setOwner( WeakPtr<T> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone

#endif  // FSMListenerT_h__
