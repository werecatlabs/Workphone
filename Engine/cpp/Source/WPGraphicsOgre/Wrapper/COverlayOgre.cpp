#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/COverlayOgre.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreOverlay.h>
#include <OgreOverlayContainer.h>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayOgre, IOverlay );

        // Definitions for static string constants declared in the header
        const String COverlayOgre::panelStr = "Panel";
        const String COverlayOgre::visibleStr = "visible";
        const String COverlayOgre::nameStr = "name";
        const String COverlayOgre::zorderStr = "zorder";
        const String COverlayOgre::originStr = "origin";
        const String COverlayOgre::zOrderStr = "zOrder";
        const String COverlayOgre::scrollXStr = "scrollX";
        const String COverlayOgre::scrollYStr = "scrollY";
        const String COverlayOgre::scaleXStr = "scaleX";
        const String COverlayOgre::scaleYStr = "scaleY";

        COverlayOgre::COverlayOgre()
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManager();
                WP_ASSERT( stateManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto stateContext = stateManager->addStateContext();

                auto stateListener = factoryManager->make_ptr<OverlayStateListener>();
                stateListener->setOwner( this );
                setStateListener( stateListener );
                stateContext->addStateListener( stateListener );

                auto state = factoryManager->make_ptr<State>();
                stateContext->addState( state );
                stateContext->setOwner( this );

                auto stateData = factoryManager->make_ptr<OverlayState>();
                state->setData( stateData );

                setStateContext( stateContext );

                m_state = stateData;

                auto stateTask = graphicsSystem->getStateTask();
                stateContext->setTaskId( stateTask );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        COverlayOgre::~COverlayOgre()
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                unload( nullptr );
            }
        }

        void COverlayOgre::load( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Loading );

                auto overlayManager = Ogre::OverlayManager::getSingletonPtr();
                WP_ASSERT( overlayManager );

                auto name = getName();
                m_overlay = overlayManager->create( name.c_str() );
                WP_ASSERT( m_overlay );

                auto panel = static_cast<Ogre::OverlayContainer *>(
                    overlayManager->createOverlayElement( panelStr.c_str(), name.c_str() ) );
                WP_ASSERT( panel );

                panel->setMetricsMode( Ogre::GuiMetricsMode::GMM_PIXELS );
                panel->initialise();

                m_overlay->add2D( panel );
                m_container = panel;

                m_overlay->initialise();
                m_overlay->show();

                setLoadingState( LoadingState::Loaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayOgre::unload( SmartPtr<ISharedObject> data )
        {
            try
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto stateManager = applicationManager->getStateManager();
                WP_ASSERT( stateManager );

                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );

                        stateListener->unload( nullptr );
                        setStateListener( nullptr );
                    }

                    stateManager->removeStateContext( stateContext );
                    setStateContext( nullptr );
                }

                if( m_overlay )
                {
                    auto overlayManager = Ogre::OverlayManager::getSingletonPtr();
                    WP_ASSERT( overlayManager );

                    if( m_container )
                    {
                        m_overlay->remove2D( m_container );

                        overlayManager->destroyOverlayElement( m_container );
                        m_container = nullptr;
                    }

                    overlayManager->destroy( m_overlay );
                    m_overlay = nullptr;
                }

                setLoadingState( LoadingState::Unloaded );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        void COverlayOgre::addElement( SmartPtr<IOverlayElement> element )
        {
            try
            {
                WP_ASSERT( element );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                if( isThreadSafe() )
                {
                    if( element->isLoaded() )
                    {
                        Ogre::OverlayElement *ogreElement;
                        element->_getObject( (void **)&ogreElement );

                        m_elements.push_back( element );

                        if( m_overlay )
                        {
                            if( m_container )
                            {
                                if( ogreElement->getParent() != m_container )
                                {
                                    m_container->addChild( ogreElement );
                                }
                            }
                        }
                    }
                    else
                    {
                        auto message = factoryManager->make_ptr<StateMessageObject>();
                        message->setType( IOverlayElement::STATE_MESSAGE_ATTACH_OBJECT );
                        message->setObject( element );
                        addMessage( message );
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( IOverlayElement::STATE_MESSAGE_ATTACH_OBJECT );
                    message->setObject( element );
                    addMessage( message );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool COverlayOgre::removeElement( SmartPtr<IOverlayElement> element )
        {
            try
            {
                WP_ASSERT( element );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                if( isThreadSafe() )
                {
                    Ogre::OverlayContainer *ogreElement;
                    element->_getObject( (void **)&ogreElement );

                    auto it = std::find( m_elements.begin(), m_elements.end(), element );
                    if( it != m_elements.end() )
                    {
                        m_elements.erase( it );
                    }

                    if( m_overlay )
                    {
                        if( m_container )
                        {
                            if( ogreElement->getParent() == m_container )
                            {
                                m_container->removeChild( ogreElement->getName() );
                            }
                        }
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setType( IOverlayElement::STATE_MESSAGE_DETACH_OBJECT );
                    message->setObject( element );
                    addMessage( message );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }

            return false;
        }

        void COverlayOgre::addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            WP_ASSERT( sceneNode );

            Ogre::SceneNode *ogreSceneNode;
            sceneNode->_getObject( (void **)&ogreSceneNode );

            if( m_overlay )
            {
                if( ogreSceneNode )
                {
                    // m_overlay->add3D(ogreSceneNode);
                }
            }
        }

        bool COverlayOgre::removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
        {
            WP_ASSERT( sceneNode );

            Ogre::SceneNode *ogreSceneNode;
            sceneNode->_getObject( (void **)&ogreSceneNode );

            if( m_overlay )
            {
                if( ogreSceneNode )
                {
                    // m_overlay->remove3D(ogreSceneNode);
                }
            }

            return false;
        }

        void COverlayOgre::setVisible( bool visible )
        {
            try
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto factoryManager = applicationManager->getFactoryManager();
                WP_ASSERT( factoryManager );

                auto renderTask = graphicsSystem->getRenderTask();
                auto stateTask = graphicsSystem->getStateTask();
                auto task = Thread::getCurrentTask();

                const auto &loadingState = getLoadingState();
                if( loadingState == LoadingState::Loaded && task == renderTask )
                {
                    if( m_overlay )
                    {
                        m_overlay->setVisible( visible );

                        if( visible )
                        {
                            m_overlay->show();
                        }
                        else
                        {
                            m_overlay->hide();
                        }
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageVisible>();
                    message->setVisible( visible );

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->addMessage( stateTask, message );
                    }
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        bool COverlayOgre::isVisible() const
        {
            if( m_overlay )
            {
                return m_overlay->isVisible();
            }

            return false;
        }

        void COverlayOgre::setZOrder( u32 zorder )
        {
            if( m_overlay )
            {
                m_overlay->setZOrder( static_cast<u16>( zorder ) );
            }
        }

        u32 COverlayOgre::getZOrder( void ) const
        {
            if( m_overlay )
            {
                return m_overlay->getZOrder();
            }

            return 0;
        }

        void COverlayOgre::updateZOrder()
        {
            if( m_overlay )
            {
                m_overlay->assignZOrders();
            }
        }

        void COverlayOgre::_getObject( void **ppObject ) const
        {
            *ppObject = m_overlay;
        }

        Array<SmartPtr<IOverlayElement>> COverlayOgre::getElements() const
        {
            return m_elements;
        }

        Vector2I COverlayOgre::getAbsoluteResolution() const
        {
            return Vector2I();
        }

        void COverlayOgre::setAbsoluteResolution( const Vector2I &absoluteResolution )
        {
        }

        bool COverlayOgre::isValid() const
        {
            auto stateContext = getStateContext();
            auto stateListener = getStateListener();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded )
            {
                if( m_overlay && stateContext && stateListener && m_state )
                {
                    if( !( stateContext->isValid() && stateListener->isValid() && m_state->isValid() ) )
                    {
                        return false;
                    }

                    for( auto element : m_elements )
                    {
                        if( !element->isValid() )
                        {
                            return false;
                        }
                    }

                    return true;
                }
            }
            else if( loadingState == LoadingState::Unloaded )
            {
                if( !( m_overlay && stateContext && stateListener && m_state ) )
                {
                    return false;
                }

                return true;
            }

            return false;
        }

        Array<SmartPtr<ISharedObject>> COverlayOgre::getChildObjects() const
        {
            auto objects = Array<SmartPtr<ISharedObject>>();
            objects.reserve( m_elements.size() + 12 );

            for( auto element : m_elements )
            {
                if( element )
                {
                    objects.push_back( element );
                }
            }

            return objects;
        }

        SmartPtr<Properties> COverlayOgre::getProperties() const
        {
            auto properties = workphone::make_ptr<Properties>();

            if( m_overlay )
            {
                auto visible = m_overlay->isVisible();
                properties->setProperty( visibleStr, visible );
                properties->setProperty( nameStr, m_overlay->getName().c_str() );
                properties->setProperty( zorderStr, m_overlay->getZOrder() );
                properties->setProperty( originStr, m_overlay->getOrigin().c_str() );
                properties->setProperty( zOrderStr, m_overlay->getZOrder() );
                properties->setProperty( scrollXStr, m_overlay->getScrollX() );
                properties->setProperty( scrollYStr, m_overlay->getScrollY() );
                properties->setProperty( scaleXStr, m_overlay->getScaleX() );
                properties->setProperty( scaleYStr, m_overlay->getScaleY() );
            }

            return properties;
        }

        void COverlayOgre::setProperties( SmartPtr<Properties> properties )
        {
            if( m_overlay )
            {
                auto visible = m_overlay->isVisible();
                auto zorder = (u32)m_overlay->getZOrder();

                properties->getPropertyValue( visibleStr, visible );
                properties->getPropertyValue( zorderStr, zorder );

                if( m_overlay->isVisible() != visible )
                {
                    m_overlay->setVisible( visible );
                }

                m_overlay->setZOrder( zorder );
            }
        }

        COverlayOgre::OverlayStateListener::OverlayStateListener() = default;

        COverlayOgre::OverlayStateListener::~OverlayStateListener()
        {
        }

        bool COverlayOgre::OverlayStateListener::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        bool COverlayOgre::OverlayStateListener::handleStateMessage(
            const SmartPtr<IStateMessage> &message )
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto factoryManager = applicationManager->getFactoryManager();
            auto renderTask = graphicsSystem->getRenderTask();
            auto currentTaskId = Thread::getCurrentTask();

            WP_ASSERT( currentTaskId == renderTask );

            if( currentTaskId == renderTask )
            {
                if( message->isExactly<StateMessageVisible>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageVisible>( message );
                    auto visible = objectMessage->isVisible();

                    m_owner->setVisible( visible );
                }
                else if( message->isExactly<StateMessageObject>() )
                {
                    auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                    WP_ASSERT( objectMessage );

                    auto type = objectMessage->getType();
                    auto object = objectMessage->getObject();

                    if( type == IOverlayElement::STATE_MESSAGE_ATTACH_OBJECT )
                    {
                        m_owner->addElement( object );
                    }
                    else if( type == IOverlayElement::STATE_MESSAGE_DETACH_OBJECT )
                    {
                        m_owner->removeElement( object );
                    }
                }
            }

            return false;
        }

        COverlayOgre *COverlayOgre::OverlayStateListener::getOwner() const
        {
            return m_owner;
        }

        void COverlayOgre::OverlayStateListener::setOwner( COverlayOgre *owner )
        {
            m_owner = owner;
        }
    }  // end namespace render
}  // namespace workphone
