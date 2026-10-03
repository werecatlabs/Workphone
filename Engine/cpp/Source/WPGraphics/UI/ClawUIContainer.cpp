#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/UI/ClawUIContainer.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::ui
{
    ClawUIContainer::ClawUIContainer()
    {
        m_type = "Container";
    }

    ClawUIContainer::~ClawUIContainer()
    {
        unload( nullptr );
    }

    void ClawUIContainer::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        setContainer( this );
        setLoadingState( LoadingState::Loaded );
    }

    void ClawUIContainer::unload( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Unloaded )
        {
            return;
        }

        setLoadingState( LoadingState::Unloading );
        ClawUIElement<IUILayoutContainer>::unload( data );
        setLoadingState( LoadingState::Unloaded );
    }

    void ClawUIContainer::setPosition( const Vector2F &position )
    {
        ClawUIElement::setPosition( position );
    }

    void ClawUIContainer::draw( struct wp_context *ctx )
    {
        if( ctx && isVisible() && isEnabled() )
        {
            drawWorkphoneChildren( ctx );
        }
    }

    SmartPtr<render::IOverlayElementContainer> ClawUIContainer::getOverlayContainer() const
    {
        return nullptr;
    }

    void ClawUIContainer::setOverlayContainer(
        SmartPtr<render::IOverlayElementContainer> overlayContainer )
    {
        // Retained for source compatibility; WorkphoneCore has no retained container object.
    }

    void ClawUIContainer::_getObject( void **ppObject ) const
    {
        if( ppObject )
        {
            *ppObject = nullptr;
        }
    }
}  // namespace workphone::ui
