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
            IUIEvent();

            IUIEvent( u32 poolTypeId );

            /** Destructor. */
            ~IUIEvent() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // IUIEvent_h__
