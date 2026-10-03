#ifndef IStateListener_h__
#define IStateListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @class IStateListener
     * @brief Interface for objects that listen to state-related events and messages.
     *
     * This interface should be implemented by classes that need to respond to state changes or
     * receive state messages within the system. It inherits from ISharedObject to support
     * reference counting and shared ownership semantics.
     */
    class WPCore_API IStateListener : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor for safe polymorphic destruction.
         */
        ~IStateListener() override;

        /**
         * @brief Handles a state message sent to this listener.
         *
         * This method is called when a state message is dispatched to the listener. Implementations
         * should process the message as appropriate for the application.
         *
         * @param message A smart pointer to the IStateMessage object containing the message data.
         * @return True if the message was handled successfully, false otherwise.
         */
        virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message ) = 0;

        /**
         * @brief Handles notification of a state change.
         *
         * This method is called when the state changes and the listener needs to react to the new state.
         * Implementations can use the provided state object to query details about the new state.
         *
         * @param state A smart pointer to the IState object representing the new state.
         * @return True if the state change was handled successfully, false otherwise.
         */
        virtual bool handleStateChanged( SmartPtr<IState> &state ) = 0;

        // 'c' style linked list of states for message queueing. This is used to avoid dynamic memory
        // allocation when queuing messages.
        AtomicRawPtr<IStateListener> m_next;

        /**
         * @brief Macro for class registration and reflection support.
         */
        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IStateListener_h__
