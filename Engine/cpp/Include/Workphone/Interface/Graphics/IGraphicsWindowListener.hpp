#ifndef _IGraphicsWindowListener_H
#define _IGraphicsWindowListener_H

#include <Workphone/Interface/System/IEventListener.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * Listens for render window events.
         */
        class WPCore_API IGraphicsWindowListener : public IEventListener
        {
        public:
            static const hash_type windowClosingHash;
            static const hash_type windowResizedHash;
            static const hash_type windowMovedHash;

            /** Virtual destructor. */
            ~IGraphicsWindowListener() override;

            /**
             * Handles the event.
             *
             * @param eventType The type of the event.
             * @param eventValue The value of the event.
             * @param arguments The arguments of the event.
             * @param sender The event sender.
             * @param event The event itself.
             *
             * @return The result of the event handling.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) override = 0;

            /**
             * Handles the window event.
             * @param event The window event.
             */
            virtual void handleEvent( SmartPtr<IGraphicsWindowEvent> event ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
