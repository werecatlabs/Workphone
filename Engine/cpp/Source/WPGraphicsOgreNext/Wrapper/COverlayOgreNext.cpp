#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/COverlayOgreNext.hpp>
#include <Workphone/Workphone.hpp>
#include <OgreOverlaySystem.h>
#include <OgreOverlayManager.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
#include <OgreOverlayContainer.h>

namespace workphone::render
{

    WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayOgreNext, IOverlay );
    WP_CLASS_REGISTER_DERIVED( workphone::render, COverlayOgreNext::OverlayStateListener,
                               IStateListener );

    COverlayOgreNext::COverlayOgreNext()
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
            setName( "COverlayOgreNext" );

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

            auto stateTask = graphicsSystem->getStateTask();
            stateContext->setTaskId( stateTask );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    COverlayOgreNext::~COverlayOgreNext()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto stateManager = applicationManager->getStateManager();
        if( stateManager )
        {
            if( auto stateContext = getStateContext() )
            {
                stateManager->removeStateContext( stateContext );
                setStateContext( nullptr );
            }
        }
    }

    void COverlayOgreNext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
            WP_ASSERT( overlayManager );

            auto name = getName();

            m_overlay = overlayManager->create( name.c_str() );
            WP_ASSERT( m_overlay );

            auto panel = static_cast<Ogre::v1::OverlayContainer *>(
                overlayManager->createOverlayElement( "Panel", name.c_str() ) );
            WP_ASSERT( panel );

            panel->setMetricsMode( Ogre::v1::GuiMetricsMode::GMM_PIXELS );
            panel->initialise();

            m_overlay->add2D( panel );
            m_container = panel;

            m_overlay->hide();

            if( auto stateContext = getStateContext() )
            {
                stateContext->setDirty( true );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void COverlayOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();
            if( stateManager )
            {
                if( auto stateContext = getStateContext() )
                {
                    if( auto stateListener = getStateListener() )
                    {
                        stateContext->removeStateListener( stateListener );
                        setStateListener( nullptr );
                    }

                    stateManager->removeStateContext( stateContext );

                    stateContext->unload( nullptr );
                    setStateContext( nullptr );
                }
            }

            for( auto element : m_elements )
            {
                Ogre::v1::OverlayContainer *ogreElement;
                element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

                if( ogreElement )
                {
                    m_container->_notifyParent( m_container->getParent(), nullptr );
                    m_container->removeChild( ogreElement->getName() );
                }
            }

            m_elements.clear();

            if( m_overlay )
            {
                auto overlayManager = Ogre::v1::OverlayManager::getSingletonPtr();
                WP_ASSERT( overlayManager );

                if( m_container )
                {
                    m_overlay->remove2D( m_container );

                    auto it = m_container->getChildIterator();
                    while( it.hasMoreElements() )
                    {
                        auto element = it.getNext();
                        m_container->removeChild( element->getName() );
                    }

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

    void COverlayOgreNext::update()
    {
        auto container = m_container;
        auto overlay = m_overlay;

        if( container )
        {
            container->_notifyViewport();
            container->_notifyParent( nullptr, overlay );
        }
    }

    void COverlayOgreNext::addElement( SmartPtr<IOverlayElement> element )
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

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            if( isThreadSafe() )
            {
                if( !element->isLoaded() )
                {
                    element->load( nullptr );
                }

                if( element->isLoaded() )
                {
                    Ogre::v1::OverlayContainer *ogreElement;
                    element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

                    element->setOverlay( this );

                    m_elements.push_back( element );

                    if( m_overlay )
                    {
                        //m_overlay->add2D( ogreElement );

                        if( m_container )
                        {
                            if( ogreElement->getParent() != m_container )
                            {
                                m_container->addChild( ogreElement );
                            }
                        }
                    }

                    if( auto stateContext = getStateContext() )
                    {
                        stateContext->setDirty( true );
                    }
                }
                else
                {
                    auto message = factoryManager->make_ptr<StateMessageObject>();
                    message->setSender( this );
                    message->setType( IOverlayElement::STATE_MESSAGE_ATTACH_OBJECT );
                    message->setObject( element );
                    addMessage( message );
                }
            }
            else
            {
                auto message = factoryManager->make_ptr<StateMessageObject>();
                message->setSender( this );
                message->setType( STATE_MESSAGE_ATTACH_OBJECT );
                message->setObject( element );
                addMessage( message );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto COverlayOgreNext::removeElement( SmartPtr<IOverlayElement> element ) -> bool
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

            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            if( isThreadSafe() )
            {
                Ogre::v1::OverlayContainer *ogreElement;
                element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

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
                message->setType( STATE_MESSAGE_DETACH_OBJECT );
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

    void COverlayOgreNext::addSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode )
    {
        WP_ASSERT( sceneNode );

        Ogre::SceneNode *ogreSceneNode;
        sceneNode->_getObject( reinterpret_cast<void **>( &ogreSceneNode ) );

        if( m_overlay )
        {
            if( ogreSceneNode )
            {
                // m_overlay->add3D(ogreSceneNode);
            }
        }
    }

    auto COverlayOgreNext::removeSceneNode( SmartPtr<IGraphicsSceneNode> sceneNode ) -> bool
    {
        WP_ASSERT( sceneNode );

        Ogre::SceneNode *ogreSceneNode;
        sceneNode->_getObject( reinterpret_cast<void **>( &ogreSceneNode ) );

        if( m_overlay )
        {
            if( ogreSceneNode )
            {
                // m_overlay->remove3D(ogreSceneNode);
            }
        }

        return false;
    }

    void COverlayOgreNext::setVisible( bool visible )
    {
        try
        {
            if( auto stateContext = getStateContext() )
            {
                if( auto state = stateContext->invalidateStateData<OverlayState>() )
                {
                    state->setVisible( visible );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto COverlayOgreNext::isVisible() const -> bool
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<OverlayState>() )
            {
                return state->isVisible();
            }
        }

        return false;
    }

    void COverlayOgreNext::setZOrder( u32 zorder )
    {
        if( m_overlay )
        {
            m_overlay->setZOrder( static_cast<u16>( zorder ) );
        }
    }

    auto COverlayOgreNext::getZOrder() const -> u32
    {
        if( m_overlay )
        {
            return static_cast<u32>( m_overlay->getZOrder() );
        }

        return 0;
    }

    void COverlayOgreNext::updateZOrder()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();

        ScopedLock lock( graphicsSystem );

        for( auto element : m_elements )
        {
            Ogre::v1::OverlayContainer *ogreElement;
            element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

            if( ogreElement )
            {
                if( m_container )
                {
                    m_container->removeChild( ogreElement->getName() );
                }
            }
        }

        std::sort( m_elements.begin(), m_elements.end(),
                   []( auto a, auto b ) { return a->getZOrder() < b->getZOrder(); } );

        for( auto element : m_elements )
        {
            Ogre::v1::OverlayContainer *ogreElement;
            element->_getObject( reinterpret_cast<void **>( &ogreElement ) );

            if( ogreElement )
            {
                if( m_container )
                {
                    m_container->addChild( ogreElement );
                }
            }
        }
    }

    void COverlayOgreNext::_getObject( void **ppObject ) const
    {
        *ppObject = m_overlay;
    }

    auto COverlayOgreNext::getElements() const -> Array<SmartPtr<IOverlayElement>>
    {
        return m_elements.snapshot();
    }

    auto COverlayOgreNext::getAbsoluteResolution() const -> Vector2I
    {
        return {};
    }

    void COverlayOgreNext::setAbsoluteResolution( const Vector2I &absoluteResolution )
    {
    }

    auto COverlayOgreNext::isValid() const -> bool
    {
        auto stateContext = getStateContext();
        auto stateListener = getStateListener();

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
            if( m_overlay && stateContext && stateListener )
            {
                if( !( stateContext->isValid() && stateListener->isValid() ) )
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
            if( !( m_overlay && stateContext && stateListener ) )
            {
                return false;
            }

            return true;
        }

        return false;
    }

    auto COverlayOgreNext::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( m_elements.size() + 12 );

        for( auto element : m_elements )
        {
            if( element )
            {
                //objects.push_back( element );
            }
        }

        return objects;
    }

    auto COverlayOgreNext::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();

        if( m_overlay )
        {
            auto visible = m_overlay->isVisible();
            properties->setProperty( "visible", visible );

            properties->setProperty( "name", m_overlay->getName().c_str() );
            properties->setProperty( "zorder", m_overlay->getZOrder() );
            properties->setProperty( "origin", m_overlay->getOrigin().c_str() );
            properties->setProperty( "zOrder", m_overlay->getZOrder() );
            properties->setProperty( "scrollX", m_overlay->getScrollX() );
            properties->setProperty( "scrollY", m_overlay->getScrollY() );
            properties->setProperty( "scaleX", m_overlay->getScaleX() );
            properties->setProperty( "scaleY", m_overlay->getScaleY() );
        }

        return properties;
    }

    void COverlayOgreNext::setProperties( SmartPtr<Properties> properties )
    {
        auto visible = m_overlay->isVisible();
        auto zorder = static_cast<u32>( m_overlay->getZOrder() );

        properties->getPropertyValue( "visible", visible );
        properties->getPropertyValue( "zorder", zorder );

        if( m_overlay->isVisible() != visible )
        {
            m_overlay->setVisible( visible );
        }

        m_overlay->setZOrder( zorder );
    }

    COverlayOgreNext::OverlayStateListener::OverlayStateListener() = default;

    COverlayOgreNext::OverlayStateListener::~OverlayStateListener() = default;

    bool COverlayOgreNext::OverlayStateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
            {
                auto applicationManager = core::IApplicationManager::instance();
                auto graphicsSystem = applicationManager->getGraphicsSystem();
                auto fontManager = graphicsSystem->getFontManager();

                if( fontManager->isLoaded() )
                {
                    auto stateData = state->getData();
                    if( stateData->isDerived<OverlayState>() )
                    {
                        auto overlayState = workphone::static_pointer_cast<OverlayState>( stateData );

                        auto container = owner->m_container;
                        auto overlay = owner->m_overlay;

                        //container->_notifyViewport();
                        //container->_notifyParent( nullptr, overlay );

                        auto visible = overlayState->isVisible();
                        if( visible != overlay->isVisible() )
                        {
                            overlay->setVisible( visible );
                        }
                    }
                }
            }
        }

        return false;
    }

    bool COverlayOgreNext::OverlayStateListener::handleStateMessage(
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
                m_owner->setVisible( objectMessage->isVisible() );
            }
            else if( message->isExactly<StateMessageObject>() )
            {
                auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                WP_ASSERT( objectMessage );

                auto type = objectMessage->getType();
                auto object = objectMessage->getObject();

                if( type == STATE_MESSAGE_ATTACH_OBJECT )
                {
                    m_owner->addElement( object );
                }
                else if( type == STATE_MESSAGE_DETACH_OBJECT )
                {
                    m_owner->removeElement( object );
                }
            }
        }

        return false;
    }

    auto COverlayOgreNext::OverlayStateListener::getOwner() const -> COverlayOgreNext *
    {
        return m_owner;
    }

    void COverlayOgreNext::OverlayStateListener::setOwner( COverlayOgreNext *owner )
    {
        m_owner = owner;
    }

    void COverlayOgreNext::OverlayStateListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

}  // namespace workphone::render
