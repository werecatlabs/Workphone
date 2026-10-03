#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiTabItem.hpp>
#include "WPImGui/ImGuiApplication.hpp"
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiTabItem, ImGuiElement<IUITabItem> );

    ImGuiTabItem::ImGuiTabItem() = default;

    ImGuiTabItem::~ImGuiTabItem() = default;

    void ImGuiTabItem::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto ui = applicationManager->getUI();
        auto uiApplication = workphone::static_pointer_cast<ImGuiApplication>( ui->getApplication() );

        auto name = getLabel();
        if( ImGui::BeginTabItem( name.c_str() ) )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                uiApplication->createElement( child );
            }

            ImGui::EndTabItem();
        }
    }

    String ImGuiTabItem::getLabel() const
    {
        return m_label;
    }

    void ImGuiTabItem::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
