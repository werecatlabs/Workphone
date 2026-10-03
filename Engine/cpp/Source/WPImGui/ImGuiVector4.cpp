#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiVector4.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiVector4, IUIVector4 );

    ImGuiVector4::ImGuiVector4() = default;

    ImGuiVector4::~ImGuiVector4() = default;

    Vector4<real_Num> ImGuiVector4::getValue() const
    {
        return m_value;
    }

    void ImGuiVector4::setValue( const Vector4<real_Num> &value )
    {
        m_value = value;
    }

    String ImGuiVector4::getLabel() const
    {
        return m_label;
    }

    void ImGuiVector4::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
