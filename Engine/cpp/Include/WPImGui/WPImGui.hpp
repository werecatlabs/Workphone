#ifndef __WPImGui_h__
#define __WPImGui_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/WPImGuiStyle.hpp>
#include <Workphone/Memory/SmartPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/UI/IconsFontAwesome6.h>

namespace workphone
{
    namespace ui
    {
        class WPImGui_API WPImGui : public ISharedObject
        {
        public:
            WPImGui();
            ~WPImGui() override;

            void load( SmartPtr<ISharedObject> data ) override;
            void unload( SmartPtr<ISharedObject> data ) override;

            static SmartPtr<IUIManager> createUI();

            static SmartPtr<WPImGui> instance();
            static void setInstance( SmartPtr<WPImGui> plugin );

        protected:
            static SmartPtr<WPImGui> m_sPlugin;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // __WPImGui_h__
