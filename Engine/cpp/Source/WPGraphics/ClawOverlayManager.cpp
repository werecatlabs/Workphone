#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawOverlayManager.hpp>
#include <WPGraphics/ClawOverlay.hpp>
#include <WPGraphics/ClawOverlayElement.hpp>
#include <WPGraphics/ClawOverlayElementContainer.hpp>
#include <WPGraphics/ClawOverlayElementText.hpp>
#include <WPGraphics/ClawOverlayElementVector.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawOverlayManager, IOverlayManager );

    ClawOverlayManager::ClawOverlayManager()
    {
        m_workphoneContext = std::make_unique<ui::ClawUIWorkphoneContext>();
    }

    ClawOverlayManager::~ClawOverlayManager() = default;

    void ClawOverlayManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            m_overlayElements.reserve( 32 );
            m_overlays.reserve( 32 );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawOverlayManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                for( auto overlayElement : m_overlayElements )
                {
                    WP_ASSERT( overlayElement );
                    if( overlayElement )
                    {
                        overlayElement->unload( nullptr );
                    }
                }
                m_overlayElements.clear();

                for( auto overlay : m_overlays )
                {
                    WP_ASSERT( overlay );
                    if( overlay )
                    {
                        overlay->unload( nullptr );
                    }
                }
                m_overlays.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ClawOverlayManager::update( struct wp_context *ctx )
    {
        if( !ctx && m_workphoneContext )
            ctx = m_workphoneContext->getContext();
        if( !ctx )
            return;

        auto overlays = getOverlays();
        for( auto overlay : overlays )
        {
            WP_ASSERT( overlay );
            if( overlay )
            {
                // overlay->update( ctx );
            }
        }
    }

    SmartPtr<IOverlay> ClawOverlayManager::addOverlay( const String &instanceName )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return nullptr;
        }

        if( auto existingOverlay = findOverlay( instanceName ) )
        {
            WP_ASSERT( false );
            return existingOverlay;
        }

        auto overlay = workphone::make_ptr<ClawOverlay>();
        WP_ASSERT( overlay );
        overlay->setName( instanceName );
        m_overlays.emplace_back( overlay );
        overlay->load( nullptr );

        WP_ASSERT( overlay->isLoaded() );
        if( !overlay->isLoaded() )
        {
            m_overlays.erase( std::remove( m_overlays.begin(), m_overlays.end(), overlay ),
                              m_overlays.end() );
            return nullptr;
        }

        return overlay;
    }

    bool ClawOverlayManager::removeOverlay( const String &instanceName )
    {
        auto overlay = findOverlay( instanceName );
        if( overlay )
        {
            return removeOverlay( overlay );
        }
        return false;
    }

    bool ClawOverlayManager::removeOverlay( SmartPtr<IOverlay> overlay )
    {
        WP_ASSERT( overlay );
        if( !overlay )
        {
            return false;
        }

        auto it = std::remove( m_overlays.begin(), m_overlays.end(), overlay );
        if( it != m_overlays.end() )
        {
            overlay->unload( nullptr );
            m_overlays.erase( it, m_overlays.end() );
            return true;
        }

        return false;
    }

    Array<SmartPtr<IOverlay>> ClawOverlayManager::getOverlays() const
    {
        return m_overlays.snapshot();
    }

    SmartPtr<IOverlay> ClawOverlayManager::findOverlay( const String &instanceName )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return nullptr;
        }

        for( auto overlay : m_overlays )
        {
            if( overlay && instanceName == overlay->getName() )
            {
                return overlay;
            }
        }

        return nullptr;
    }

    bool ClawOverlayManager::hasOverlay( const String &instanceName )
    {
        return findOverlay( instanceName ) != nullptr;
    }

    SmartPtr<IOverlayElement> ClawOverlayManager::addElement( const String &typeName,
                                                              const String &instanceName )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( typeName ) );
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( typeName ) || StringUtil::isNullOrEmpty( instanceName ) )
        {
            return nullptr;
        }

        if( auto existingElement = findElement( instanceName ) )
        {
            WP_ASSERT( false );
            return existingElement;
        }

        SmartPtr<IOverlayElement> element = nullptr;

        if( typeName == String( "Panel" ) )
        {
            element = workphone::make_ptr<ClawOverlayElementContainer>();
        }
        else if( typeName == String( "TextArea" ) )
        {
            element = workphone::make_ptr<ClawOverlayElementText>();
        }
        else if( typeName == String( "VectorImage" ) )
        {
            element = workphone::make_ptr<ClawOverlayElementVector>();
        }
        else
        {
            WP_LOG_ERROR( "COverlayManager::addElement - unknown element type." );
            return nullptr;
        }

        WP_ASSERT( element );
        element->setName( instanceName );
        m_overlayElements.emplace_back( element );
        element->load( nullptr );

        return element;
    }

    SmartPtr<IOverlayElement> ClawOverlayManager::findElement( const String &instanceName )
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return nullptr;
        }

        for( auto e : m_overlayElements )
        {
            if( e && instanceName == e->getName() )
            {
                return e;
            }
        }

        return nullptr;
    }

    bool ClawOverlayManager::removeElement( SmartPtr<IOverlayElement> element )
    {
        WP_ASSERT( element );
        if( !element )
        {
            return false;
        }

        auto it = std::remove( m_overlayElements.begin(), m_overlayElements.end(), element );
        if( it != m_overlayElements.end() )
        {
            element->unload( nullptr );
            m_overlayElements.erase( it, m_overlayElements.end() );
            return true;
        }

        return false;
    }

    void ClawOverlayManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }
}  // namespace workphone::render
