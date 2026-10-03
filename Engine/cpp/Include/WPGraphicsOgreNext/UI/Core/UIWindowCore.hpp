#ifndef __UIWindowCore_h__
#define __UIWindowCore_h__

#include <WPGraphicsOgreNext/UI/Core/UIElementCore.hpp>
#include <Workphone/UI/UIWindow.hpp>

namespace workphone
{
    namespace ui
    {
        class UIWindowCore : public UIElementCore<UIWindow>
        {
        public:
            UIWindowCore();
            ~UIWindowCore() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;
            void update() override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __UIWindowCore_h__
