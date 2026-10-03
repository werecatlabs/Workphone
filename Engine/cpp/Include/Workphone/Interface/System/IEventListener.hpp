#ifndef _WP_IEventListener_h__
#define _WP_IEventListener_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{

    /** Interface for an event listener. */
    class WPCore_API IEventListener : public ISharedObject
    {
    public:
        /** Constructor. */
        IEventListener();

        /** Virtual destructor. */
        ~IEventListener() override;

        /** Handles an event.
         @param eventType The event type.
         @param eventValue The event value.
         @param sender The object triggering the event. This can be null.
         @param event The event data. This can be null.
         @return Contains a return parameter.
         */
        virtual Parameter handleEvent( EventType eventType, hash_type eventValue,
                                       const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                       SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) = 0;

        /** Gets the event listener priority.
         * @return The priority value.
         */
        virtual s32 getPriority() const;

        /** Sets the event listener priority.
         * @param priority The priority value.
         */
        virtual void setPriority( s32 priority );

        // 'c' style linked list of states for message queueing. This is used to avoid dynamic memory
        // allocation when queuing messages.
        AtomicRawPtr<IEventListener> m_next;

        WP_CLASS_REGISTER_DECL;

    protected:
        /// The event listener priority.
        s32 m_eventListenerPriority = 0;
    };
}  // namespace workphone

#endif  // _WP_IEventListener_h__
