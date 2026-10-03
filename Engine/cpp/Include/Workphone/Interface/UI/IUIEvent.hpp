#ifndef IUIEvent_h__
#define IUIEvent_h__

#include <Workphone/Interface/System/IEvent.hpp>

namespace workphone
{
    namespace ui
    {

        class WPCore_API IUIEvent : public IEvent
        {
        public:
            IUIEvent() : IEvent( IUIEvent::typeInfo() )
            {
            }

            IUIEvent( u32 poolTypeId ) : IEvent( poolTypeId )
            {
            }

            /** Destructor. */
            ~IUIEvent() override = default;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIEvent_h__
