#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawOverlay.hpp>
#include <WPGraphics/ClawOverlayElement.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawOverlay, IOverlay );

    ClawOverlay::ClawOverlay() = default;
    ClawOverlay::~ClawOverlay() = default;

    void ClawOverlay::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            m_elements.reserve( 16 );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawOverlay::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                for( auto element : m_elements )
                {
                    WP_ASSERT( element );
                    if( element )
                    {
                        element->unload( nullptr );
                    }
                }
                m_elements.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawOverlay::update( struct wp_context *ctx )
    {
        for( auto element : m_elements )
        {
            WP_ASSERT( element );
            if( element )
            {
                // element->update( ctx );
            }
        }
    }

    void ClawOverlay::addElement( SmartPtr<IOverlayElement> element )
    {
        WP_ASSERT( element );
        if( !element )
        {
            return;
        }

        m_elements.emplace_back( element );
        element->setOverlay( this );
    }

    bool ClawOverlay::removeElement( SmartPtr<IOverlayElement> element )
    {
        WP_ASSERT( element );
        if( !element )
        {
            return false;
        }

        auto it = std::remove( m_elements.begin(), m_elements.end(), element );
        if( it != m_elements.end() )
        {
            element->setOverlay( nullptr );
            m_elements.erase( it, m_elements.end() );
            return true;
        }

        return false;
    }

    Array<SmartPtr<IOverlayElement>> ClawOverlay::getElements() const
    {
        return m_elements.snapshot();
    }

    void ClawOverlay::addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        WP_ASSERT( false );
    }

    bool ClawOverlay::removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        WP_ASSERT( false );
        return false;
    }

    void ClawOverlay::setVisible( bool visible )
    {
        m_visible = visible;
    }

    bool ClawOverlay::isVisible() const
    {
        return m_visible;
    }

    void ClawOverlay::setZOrder( u32 zorder )
    {
        m_zOrder = zorder;
    }

    u32 ClawOverlay::getZOrder() const
    {
        return m_zOrder;
    }

    void ClawOverlay::updateZOrder()
    {
        WP_ASSERT( false );
    }

    Vector2I ClawOverlay::getAbsoluteResolution() const
    {
        return m_absoluteResolution;
    }

    void ClawOverlay::setAbsoluteResolution( const Vector2I &absoluteResolution )
    {
        m_absoluteResolution = absoluteResolution;
    }

    void ClawOverlay::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }
}  // namespace workphone::render
