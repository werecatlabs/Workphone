#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTabBar.hpp>
#include <WPImGui/ImGuiTabItem.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTabBar, ImGuiElement<IUITabBar> );

    ImGuiTabBar::ImGuiTabBar() = default;

    ImGuiTabBar::~ImGuiTabBar() = default;

    void ImGuiTabBar::update()
    {
        auto name = getName();
        if( ImGui::BeginTabBar( name.c_str() ) )
        {
            for( auto &tabItem : m_tabItems )
            {
                tabItem->update();
            }

            ImGui::EndTabBar();
        }
    }

    SmartPtr<IUITabItem> ImGuiTabBar::addTabItem()
    {
        auto tabItem = workphone::make_ptr<ImGuiTabItem>();
        m_tabItems.emplace_back( tabItem );
        return tabItem;
    }

    void ImGuiTabBar::removeTabItem( SmartPtr<IUITabItem> tabItem )
    {
        auto it = std::find( m_tabItems.begin(), m_tabItems.end(), tabItem );
        if( it != m_tabItems.end() )
        {
            m_tabItems.erase( it );
        }
    }
}  // namespace workphone::ui
