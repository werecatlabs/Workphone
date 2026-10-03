#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiVector2.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiVector2, IUIVector2 );

    ImGuiVector2::ImGuiVector2() = default;

    ImGuiVector2::~ImGuiVector2() = default;

    Vector2<real_Num> ImGuiVector2::getValue() const
    {
        return m_value;
    }

    void ImGuiVector2::setValue( const Vector2<real_Num> &value )
    {
        m_value = value;
    }

    String ImGuiVector2::getLabel() const
    {
        return m_label;
    }

    void ImGuiVector2::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
