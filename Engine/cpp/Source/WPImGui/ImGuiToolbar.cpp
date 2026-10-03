#include <WPImGui/WPImGuiPCH.hpp>
#include <WPImGui/ImGuiToolbar.hpp>

namespace workphone::ui
{
    WP_CLASS_REGISTER_DERIVED( workphone::ui, ImGuiToolbar, ImGuiElement<IUIToolbar> );

    ImGuiToolbar::ImGuiToolbar()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
    }

    void ImGuiToolbar::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        ImGuiElement<IUIToolbar>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ImGuiToolbar::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        ImGuiElement<IUIToolbar>::load( data );
        m_children.reserve( 32 );

        setLoadingState( LoadingState::Loaded );
    }

    ImGuiToolbar::~ImGuiToolbar() = default;
}  // namespace workphone::ui
