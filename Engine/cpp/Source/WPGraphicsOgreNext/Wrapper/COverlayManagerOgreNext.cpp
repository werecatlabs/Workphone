#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayManagerOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementOgreNext.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayElementText.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlay.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayManagerOgreNext, IOverlayManager );

    u32 COverlayManagerOgreNext::m_nameExt = 0;

    COverlayManagerOgreNext::COverlayManagerOgreNext()
    {
        setName( "COverlayManagerOgreNext" );
    }

    COverlayManagerOgreNext::~COverlayManagerOgreNext()
    {
        unload( nullptr );
    }

    void COverlayManagerOgreNext::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_overlayElements.reserve( 32 );
        m_overlays.reserve( 32 );
        setLoadingState( LoadingState::Loaded );
    }

    void COverlayManagerOgreNext::unload( SmartPtr<ISharedObject> data )
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

                if( auto overlayMgr = Ogre::v1::OverlayManager::getSingletonPtr() )
                {
                    overlayMgr->destroyAll();
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void COverlayManagerOgreNext::update()
    {
        auto overlays = getOverlays();
        for( auto overlay : overlays )
        {
            WP_ASSERT( overlay );
            if( overlay )
            {
                overlay->update();
            }
        }
    }

    auto COverlayManagerOgreNext::addOverlay( const String &instanceName ) -> SmartPtr<IOverlay>
    {
        try
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

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return nullptr;
            }

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );
            if( !graphicsSystem )
            {
                return nullptr;
            }

            auto overlay = workphone::make_ptr<COverlayOgreNext>();
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
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    auto COverlayManagerOgreNext::removeOverlay( const String &instanceName ) -> bool
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return false;
        }

        auto it = std::find_if( m_overlays.begin(), m_overlays.end(),
                                [&instanceName]( const auto &overlay ) {
                                    return overlay && instanceName == overlay->getName();
                                } );

        if( it != m_overlays.end() )
        {
            auto overlay = *it;
            WP_ASSERT( overlay );
            if( overlay )
            {
                overlay->unload( nullptr );
            }

            m_overlays.erase( it );
            return true;
        }

        return false;
    }

    auto COverlayManagerOgreNext::removeOverlay( SmartPtr<IOverlay> overlay ) -> bool
    {
        WP_ASSERT( overlay );
        if( !overlay )
        {
            return false;
        }

        auto it = std::find( m_overlays.begin(), m_overlays.end(), overlay );
        if( it != m_overlays.end() )
        {
            overlay->unload( nullptr );

            m_overlays.erase( it );
            return true;
        }

        return false;
    }

    auto COverlayManagerOgreNext::findOverlay( const String &instanceName ) -> SmartPtr<IOverlay>
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return nullptr;
        }

        for( auto e : m_overlays )
        {
            if( e && instanceName == e->getName() )
            {
                return e;
            }
        }

        return nullptr;
    }

    auto COverlayManagerOgreNext::hasOverlay( const String &instanceName ) -> bool
    {
        WP_ASSERT( !StringUtil::isNullOrEmpty( instanceName ) );
        if( StringUtil::isNullOrEmpty( instanceName ) )
        {
            return false;
        }

        for( auto e : m_overlays )
        {
            if( e && instanceName == e->getName() )
            {
                return true;
            }
        }

        return false;
    }

    auto COverlayManagerOgreNext::addElement( const String &typeName, const String &instanceName )
        -> SmartPtr<IOverlayElement>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return nullptr;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );
        if( !graphicsSystem )
        {
            return nullptr;
        }

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

        ScopedLock lock( this );

        if( typeName == String( "Panel" ) )
        {
            auto container = workphone::make_ptr<COverlayElementContainer>();
            WP_ASSERT( container );
            container->setName( instanceName );
            m_overlayElements.emplace_back( container );
            container->load( nullptr );

            return container;
        }
        else if( typeName == String( "TextArea" ) )
        {
            auto container = workphone::make_ptr<COverlayElementText>();
            WP_ASSERT( container );
            container->setName( instanceName );
            m_overlayElements.emplace_back( container );
            container->load( nullptr );

            return container;
        }
        else if( typeName == String( "VectorImage" ) )
        {
            WP_ASSERT( false );
            return nullptr;
        }
        else
        {
            WP_ASSERT( false );
            WP_LOG_ERROR( "COverlayManager::addElement - unknown element type." );
        }

        return nullptr;
    }

    auto COverlayManagerOgreNext::findElement( const String &instanceName ) -> SmartPtr<IOverlayElement>
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

    auto COverlayManagerOgreNext::removeElement( SmartPtr<IOverlayElement> element ) -> bool
    {
        WP_ASSERT( element );
        if( !element )
        {
            return false;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return false;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );
        if( !graphicsSystem )
        {
            return false;
        }

        auto it = std::remove( m_overlayElements.begin(), m_overlayElements.end(), element );
        if( it != m_overlayElements.end() )
        {
            graphicsSystem->unloadObject( element );
            m_overlayElements.erase( it, m_overlayElements.end() );
            return true;
        }

        return false;
    }

    void COverlayManagerOgreNext::_getObject( void **ppObject ) const
    {
        WP_ASSERT( ppObject );
        if( !ppObject )
        {
            return;
        }

        *ppObject = nullptr;

        if( auto overlayMgr = Ogre::v1::OverlayManager::getSingletonPtr() )
        {
            *ppObject = overlayMgr;
        }

        WP_ASSERT( *ppObject );
    }

    auto COverlayManagerOgreNext::getOverlays() const -> Array<SmartPtr<IOverlay>>
    {
        return m_overlays.snapshot();
    }

    void COverlayManagerOgreNext::unlock()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );
        if( !graphicsSystem )
        {
            return;
        }

        graphicsSystem->unlock();
    }

    void COverlayManagerOgreNext::lock()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );
        if( !applicationManager )
        {
            return;
        }

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );
        if( !graphicsSystem )
        {
            return;
        }

        graphicsSystem->lock();
    }

}  // namespace workphone::render
