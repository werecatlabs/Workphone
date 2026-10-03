#ifndef StateMessageObject_h__
#define StateMessageObject_h__

#include <Workphone/State/Messages/StateMessage.hpp>

namespace workphone
{
    /**
     * @file StateMessageObject.hpp
     * @brief Message type that carries a (weak) reference to a shared object.
     *
     * This message is used by the Workphone state system to transport a reference
     * to an `ISharedObject` between systems or states without taking strong
     * ownership. The underlying object is stored as a `WeakPtr<ISharedObject>`
     * to avoid reference cycles; callers should be prepared to receive a null
     * `SmartPtr` when the object has expired.
     */

    /**
     * @class StateMessageObject
     * @brief State message that contains a reference to an `ISharedObject`.
     *
     * `StateMessageObject` extends `StateMessage` and provides accessors to set
     * and retrieve a shared object associated with the message. The stored
     * object is kept as a weak reference internally; `getObject()` returns a
     * `SmartPtr<ISharedObject>` which may be null if the object has been
     * destroyed elsewhere.
     */
    class WPCore_API StateMessageObject : public StateMessage
    {
    public:
        /**
         * @brief Construct an empty `StateMessageObject`.
         *
         * The internal weak reference is initially empty.
         */
        StateMessageObject();

        /**
         * @brief Virtual destructor.
         *
         * Declared `override` to match the polymorphic base destructor in
         * `StateMessage`.
         */
        ~StateMessageObject() override;

        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Retrieve the referenced shared object.
         *
         * Attempts to lock the internal weak pointer and return a `SmartPtr`
         * to the underlying `ISharedObject`.
         *
         * @return `SmartPtr<ISharedObject>` that is null if the underlying
         *         object has expired.
         */
        SmartPtr<ISharedObject> getObject() const;

        /**
         * @brief Set the message's shared object.
         *
         * Stores a weak reference to `object`. The message does not assume
         * ownership; callers retain ownership via the provided `SmartPtr`.
         *
         * @param object Strong pointer to the `ISharedObject` to attach to the message.
         */
        void setObject( SmartPtr<ISharedObject> object );

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Weak reference to the attached shared object.
         *
         * Used to avoid creating ownership cycles when messages are passed
         * between systems. This member may expire at any time; callers should
         * use `getObject()` to obtain a safe `SmartPtr`.
         */
        AtomicWeakPtr<ISharedObject> m_object;
    };
}  // namespace workphone

#endif  // StateMessageObject_h__
