#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiVector3.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiVector3, IUIVector3 );

    ImGuiVector3::ImGuiVector3() = default;

    ImGuiVector3::~ImGuiVector3() = default;

    Vector3<real_Num> ImGuiVector3::getValue() const
    {
        return m_value;
    }

    void ImGuiVector3::setValue( const Vector3<real_Num> &value )
    {
        m_value = value;
    }

    String ImGuiVector3::getLabel() const
    {
        return m_label;
    }

    void ImGuiVector3::setLabel( const String &label )
    {
        m_label = label;
    }
}  // namespace workphone::ui
