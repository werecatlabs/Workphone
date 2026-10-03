#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiLabelTextInputPair.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiLabelTextInputPair,
                               ImGuiElement<IUILabelTextInputPair> );

    ImGuiLabelTextInputPair::ImGuiLabelTextInputPair() = default;

    ImGuiLabelTextInputPair::~ImGuiLabelTextInputPair() = default;

    String ImGuiLabelTextInputPair::getLabel() const
    {
        return m_label;
    }

    void ImGuiLabelTextInputPair::setLabel( const String &label )
    {
        m_label = label;
    }

    String ImGuiLabelTextInputPair::getValue() const
    {
        return m_value;
    }

    void ImGuiLabelTextInputPair::setValue( const String &value )
    {
        m_value = value;
    }
}  // namespace workphone::ui
