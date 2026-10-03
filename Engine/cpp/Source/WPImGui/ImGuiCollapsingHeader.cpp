#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiCollapsingHeader.hpp>
#include <WPImGui/ImGuiApplication.hpp>
#include <Workphone/Workphone.hpp>
#include <imgui.h>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiCollapsingHeader, ImGuiElement<IUICollapsingHeader> );

    ImGuiCollapsingHeader::ImGuiCollapsingHeader()
    {
        setRenderChildren( false );
    }

    ImGuiCollapsingHeader::~ImGuiCollapsingHeader()
    {
    }

    void ImGuiCollapsingHeader::update()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto ui = applicationManager->getUI();
        auto uiApplication = workphone::static_pointer_cast<ImGuiApplication>( ui->getApplication() );

        auto label = getLabel();
        if( ImGui::CollapsingHeader( label.c_str() ) )
        {
            auto children = getChildren();
            for( auto &child : children )
            {
                uiApplication->createElement( child );
            }
        }
    }

    String ImGuiCollapsingHeader::getLabel() const
    {
        return m_label;
    }

    void ImGuiCollapsingHeader::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
