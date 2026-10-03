#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiLabelTogglePair.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiLabelTogglePair, ImGuiElement<IUILabelTogglePair> );

    ImGuiLabelTogglePair::ImGuiLabelTogglePair() = default;

    ImGuiLabelTogglePair::~ImGuiLabelTogglePair()
    {
        unload( nullptr );
    }

    void ImGuiLabelTogglePair::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setLoadingState( LoadingState::Loaded );
    }

    void ImGuiLabelTogglePair::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        setLoadingState( LoadingState::Unloaded );
    }

    String ImGuiLabelTogglePair::getLabel() const
    {
        return m_label;
    }

    void ImGuiLabelTogglePair::setLabel( const String &label )
    {
        m_label = label;
    }

    bool ImGuiLabelTogglePair::getValue() const
    {
        return m_value;
    }

    void ImGuiLabelTogglePair::setValue( bool value )
    {
        m_value = value;
    }

    bool ImGuiLabelTogglePair::getShowLabel() const
    {
        return false;
    }

    void ImGuiLabelTogglePair::setShowLabel( bool showLabel )
    {
    }
}  // namespace workphone::ui
