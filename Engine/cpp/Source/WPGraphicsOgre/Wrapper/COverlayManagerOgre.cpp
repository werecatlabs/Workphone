#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayManagerOgre.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayOgre.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementOgre.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementContainer.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayElementText.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlay.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayManagerOgre, IOverlayManager );

        u32 COverlayManagerOgre::m_nameExt = 0;

        COverlayManagerOgre::COverlayManagerOgre()
        {
        }

        COverlayManagerOgre::~COverlayManagerOgre()
        {
            unload( nullptr );
        }

        void COverlayManagerOgre::load( SmartPtr<ISharedObject> data )
        {
        }

        void COverlayManagerOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                const auto &loadingState = getLoadingState();
                if( loadingState != LoadingState::Unloaded )
                {
                    setLoadingState( LoadingState::Unloading );

                    for( auto overlayElement : m_overlayElements )
                    {
                        overlayElement->unload( nullptr );
                    }

                    m_overlayElements.clear();

                    for( auto overlay : m_overlays )
                    {
                        overlay->unload( nullptr );
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

        SmartPtr<IOverlay> COverlayManagerOgre::addOverlay( const String &instanceName )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto overlay = workphone::make_ptr<COverlayOgre>();
                overlay->setName( instanceName );
                m_overlays.push_back( overlay );
                graphicsSystem->loadObject( overlay );
                return overlay;
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return nullptr;
        }

        bool COverlayManagerOgre::removeOverlay( const String &instanceName )
        {
            try
            {
                auto count = 0;
                for( auto e : m_overlays )
                {
                    if( instanceName == e->getName() )
                    {
                        auto it = m_overlays.begin();
                        std::advance( it, count );
                        m_overlays.erase( it );
                        return true;
                    }

                    count++;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        bool COverlayManagerOgre::removeOverlay( SmartPtr<IOverlay> overlay )
        {
            try
            {
                auto it = std::find( m_overlays.begin(), m_overlays.end(), overlay );
                if( it != m_overlays.end() )
                {
                    m_overlays.erase( it );
                    return true;
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        SmartPtr<IOverlay> COverlayManagerOgre::findOverlay( const String &instanceName )
        {
            for( auto e : m_overlays )
            {
                if( instanceName == e->getName() )
                {
                    return e;
                }
            }

            return nullptr;
        }

        bool COverlayManagerOgre::hasOverlay( const String &instanceName )
        {
            for( auto e : m_overlays )
            {
                if( instanceName == e->getName() )
                {
                    return true;
                }
            }

            return false;
        }

        SmartPtr<IOverlayElement> COverlayManagerOgre::addElement( const String &typeName,
                                                                   const String &instanceName )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            if( typeName == ( String( "Panel" ) ) )
            {
                auto container = workphone::make_ptr<COverlayElementContainer>();
                container->setName( instanceName );
                m_overlayElements.push_back( container );
                graphicsSystem->loadObject( container );
                return container;
            }
            if( typeName == ( String( "TextArea" ) ) )
            {
                auto container = workphone::make_ptr<COverlayElementText>();
                container->setName( instanceName );
                m_overlayElements.push_back( container );
                graphicsSystem->loadObject( container );
                return container;
            }
            if( typeName == ( String( "VectorImage" ) ) )
            {
                return nullptr;
            }

            WP_LOG_ERROR( "COverlayManager::addElement - unknown element type." );

            return nullptr;
        }

        SmartPtr<IOverlayElement> COverlayManagerOgre::findElement( const String &instanceName )
        {
            for( auto e : m_overlayElements )
            {
                if( instanceName == e->getName() )
                {
                    return e;
                }
            }

            return nullptr;
        }

        bool COverlayManagerOgre::removeElement( SmartPtr<IOverlayElement> element )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            graphicsSystem->unloadObject( element );

            auto it = std::find( m_overlayElements.begin(), m_overlayElements.end(), element );
            if( it != m_overlayElements.end() )
            {
                m_overlayElements.erase( it );
                return true;
            }

            return false;
        }

        void COverlayManagerOgre::_getObject( void **ppObject ) const
        {
            auto overlayMgr = Ogre::OverlayManager::getSingletonPtr();
            *ppObject = overlayMgr;
        }

        Array<SmartPtr<IOverlay>> COverlayManagerOgre::getOverlays() const
        {
            return m_overlays.snapshot();
        }

        void COverlayManagerOgre::lock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            graphicsSystem->lock();
        }

        void COverlayManagerOgre::unlock()
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            graphicsSystem->unlock();
        }

    }  // end namespace render
}  // namespace workphone
