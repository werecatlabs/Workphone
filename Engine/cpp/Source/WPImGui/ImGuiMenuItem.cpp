#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiMenuItem.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiMenuItem, ImGuiElement<IUIMenuItem> );

    ImGuiMenuItem::ImGuiMenuItem() = default;

    ImGuiMenuItem::~ImGuiMenuItem() = default;

    IUIMenuItem::Type ImGuiMenuItem::getMenuItemType() const
    {
        return m_type;
    }

    void ImGuiMenuItem::setMenuItemType( Type type )
    {
        m_type = type;
    }

    String ImGuiMenuItem::getText() const
    {
        return m_text;
    }

    void ImGuiMenuItem::setText( const String &text )
    {
        m_text = text;
    }

    String ImGuiMenuItem::getHelp() const
    {
        return m_help;
    }

    void ImGuiMenuItem::setHelp( const String &help )
    {
        m_help = help;
    }
}  // namespace workphone::ui
