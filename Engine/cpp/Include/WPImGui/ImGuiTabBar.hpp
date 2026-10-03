#ifndef ImGuiTabBar_h__
#define ImGuiTabBar_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUITabBar.hpp>

namespace workphone
{
    namespace ui
    {
        class ImGuiTabBar : public ImGuiElement<IUITabBar>
        {
        public:
            ImGuiTabBar();
            ~ImGuiTabBar() override;

            void update() override;

            SmartPtr<IUITabItem> addTabItem() override;
            void removeTabItem( SmartPtr<IUITabItem> tabItem ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<IUITabItem>> m_tabItems;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // ImGuiTabBar_h__
