#ifndef __IFSMListener_h__
#define __IFSMListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Interface/System/IFSM.hpp>

namespace workphone
{

    /** Interface for a listener of a state machine. */
    class WPCore_API IFSMListener : public ISharedObject
    {
    public:
        /** Virtual destructor. */
        ~IFSMListener() override;

        /**
         * Handles a state machine event.
         * @param state The current state of the state machine.
         * @param eventType The type of event that occurred.
         * @return The return value of the state machine action, indicating whether the event was
         * accepted, rejected, or ignored.
         */
        virtual FSMReturnType handleEvent( u32 state, FSMEvent eventType ) = 0;

        /**
         * @brief Gets the state machine associated with the listener.
         * @return A SmartPtr to the IFSM object.
         */
        virtual SmartPtr<IFSM> getFSM() const = 0;

        /**
         * @brief Sets the state machine associated with the listener.
         * @param fsm A SmartPtr to the IFSM object.
         */
        virtual void setFSM( SmartPtr<IFSM> fsm ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // __IFSMListener_h__
