#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugOgre.hpp>
#include <WPGraphicsOgre/Addons/DynamicLines.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugLine.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugCircle.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreWireBoundingBox.h>

namespace workphone::render
{

    CDebugOgre::CDebugOgre() = default;

    CDebugOgre::~CDebugOgre()
    {
        unload( nullptr );
    }

    void CDebugOgre::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );
            WP_ASSERT( applicationManager->isValid() );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto stateContext = stateManager->addStateContext();
            WP_ASSERT( stateContext );

            setStateContext( stateContext );

            auto sceneNodeStateListener = factoryManager->make_ptr<StateListener>();
            sceneNodeStateListener->setOwner( this );
            m_stateListener = sceneNodeStateListener;
            stateContext->addStateListener( sceneNodeStateListener );

            m_debugLines.reserve( 256 );
            m_debugCircles.reserve( 256 );

            WP_ASSERT( applicationManager->isValid() );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CDebugOgre::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                clear();

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                auto overlayManager = graphicsSystem->getOverlayManager();

                if( m_overlay )
                {
                    overlayManager->removeOverlay( m_overlay );
                    m_overlay = nullptr;
                }

                for( auto &element : m_overlayElements )
                {
                    overlayManager->removeElement( element );
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CDebugOgre::preUpdate()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto stateTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task == stateTask )
        {
            for( auto debugLine : m_debugLines )
            {
                if( debugLine )
                {
                    debugLine->preUpdate();
                }
            }

            for( auto debugLine : m_debugCircles )
            {
                if( debugLine )
                {
                    debugLine->preUpdate();
                }
            }
        }
    }

    void CDebugOgre::update()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto stateTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task == stateTask )
        {
            for( auto &debugLine : m_debugLines )
            {
                if( debugLine )
                {
                    debugLine->update();
                }
            }

            for( auto debugCircle : m_debugCircles )
            {
                if( debugCircle )
                {
                    debugCircle->update();
                }
            }
        }
    }

    void CDebugOgre::postUpdate()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto timer = applicationManager->getTimer();
        WP_ASSERT( timer );

        auto stateTask = graphicsSystem->getRenderTask();
        auto task = Thread::getCurrentTask();

        if( task == stateTask )
        {
            auto debugLines = m_debugLines.snapshot();
            for( auto debugLine : debugLines )
            {
                if( debugLine )
                {
                    debugLine->postUpdate();
                }
            }

            for( auto &debugLine : debugLines )
            {
                if( debugLine->getLifeTime() > debugLine->getMaxLifeTime() )
                {
                    auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
                    if( it != m_debugLines.end() )
                    {
                        m_debugLines.erase( it );
                    }
                }
            }

            SmartPtr<IDebugLine> pQueuedDebugLine;
            while( m_removeQueue.try_pop( pQueuedDebugLine ) )
            {
                pQueuedDebugLine->unload( nullptr );
                pQueuedDebugLine = nullptr;
            }

            for( auto debugCircle : m_debugCircles )
            {
                if( debugCircle )
                {
                    debugCircle->postUpdate();
                }
            }

            for( auto &debugLine : debugLines )
            {
                if( debugLine->getLifeTime() > debugLine->getMaxLifeTime() )
                {
                    auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
                    if( it != m_debugLines.end() )
                    {
                        m_debugLines.erase( it );
                        m_removeQueue.push( debugLine );
                    }
                }
            }

            while( m_removeQueue.try_pop( pQueuedDebugLine ) )
            {
                pQueuedDebugLine->unload( nullptr );
                pQueuedDebugLine = nullptr;
            }
        }
    }

    void CDebugOgre::drawPoint( hash_type id, const Vector3<real_Num> &p, u32 color )
    {
        auto size = static_cast<real_Num>( 0.1 );
        auto offset0 = Vector3<real_Num>::forward() * size;
        auto offset1 = Vector3<real_Num>::up() * size;
        auto offset2 = Vector3<real_Num>::right() * size;

        drawLine( id, p + offset0, p - offset0, color );
        drawLine( id + 1, p + offset1, p - offset1, color );
        drawLine( id + 2, p + offset2, p - offset2, color );
    }

    auto CDebugOgre::drawLine( hash_type id, const Vector3<real_Num> &start,
                               const Vector3<real_Num> &end, u32 colour ) -> SmartPtr<IDebugLine>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto line = getLine( id );
        if( !line )
        {
            line = addLine( id );
        }

        if( line )
        {
            line->setColour( colour );
            line->setPosition( start );
            line->setVector( end - start );
            line->setDirty( true );
        }

        if( !line->isLoaded() )
        {
            graphicsSystem->loadObject( line );
        }

        return line;
    }

    auto CDebugOgre::drawCircle( hash_type id, const Vector3<real_Num> &position,
                                 const Quaternion<real_Num> &orientation, real_Num radius, u32 colour )
        -> SmartPtr<IDebugCircle>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto circle = getCircle( id );
        if( !circle )
        {
            circle = addCircle( id );
        }

        if( circle )
        {
            circle->setColor( colour );
            circle->setPosition( position );
            circle->setOrientation( orientation );
            circle->setRadius( radius );
            circle->setDirty( true );
        }

        if( !circle->isLoaded() )
        {
            graphicsSystem->loadObject( circle );
        }

        return circle;
    }

    void CDebugOgre::drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                               u32 color )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto overlayManager = graphicsSystem->getOverlayManager();
        WP_ASSERT( overlayManager );

        auto overlay = getOverlay();
        if( !overlay )
        {
            overlay = overlayManager->addOverlay( "debug_overlay" );
            setOverlay( overlay );
        }

        if( overlay )
        {
            auto textElement = getElementById( id );
            if( !textElement )
            {
                auto referenceScreenSize = Vector2F( 1920.0f, 1080.0f );

                auto name = StringUtil::toString( id );
                textElement = overlayManager->addElement( "TextArea", name );

                if( auto handle = textElement->getHandle() )
                {
                    handle->setId( (u32)id );
                }

                textElement->setPosition( position );
                textElement->setSize( Vector2F( 100, 40 ) / referenceScreenSize );
                textElement->setHorizontalAlignment( 0 );
                textElement->setVerticalAlignment( 0 );
                overlay->addElement( textElement );

                addOverlayElement( textElement );
            }

            if( textElement )
            {
                textElement->setCaption( text );
            }

            overlay->setVisible( true );
        }
    }

    void CDebugOgre::addOverlayElement( SmartPtr<IOverlayElement> element )
    {
        m_overlayElements.push_back( element );
    }

    auto CDebugOgre::getElementById( hash_type id ) const -> SmartPtr<IOverlayElement>
    {
        auto elements = m_overlayElements.snapshot();
        for( auto &element : elements )
        {
            if( auto handle = element->getHandle() )
            {
                if( handle->getId() == id )
                {
                    return element;
                }
            }
        }

        return nullptr;
    }

    auto CDebugOgre::getOverlay() const -> SmartPtr<IOverlay>
    {
        return m_overlay;
    }

    void CDebugOgre::setOverlay( SmartPtr<IOverlay> overlay )
    {
        m_overlay = overlay;
    }

    void CDebugOgre::createLineMaterial()
    {
    }

    auto CDebugOgre::getDebugLines() const -> ConcurrentArray<SmartPtr<IDebugLine>>
    {
        return m_debugLines;
    }

    void CDebugOgre::setDebugLines( ConcurrentArray<SmartPtr<IDebugLine>> debugLines )
    {
        m_debugLines = debugLines;
    }

    auto CDebugOgre::getDebugCircles() const -> ConcurrentArray<SmartPtr<IDebugCircle>>
    {
        return m_debugCircles;
    }

    void CDebugOgre::setDebugCircles( ConcurrentArray<SmartPtr<IDebugCircle>> debugCircles )
    {
        m_debugCircles = debugCircles;
    }

    auto CDebugOgre::addLine( hash_type id ) -> SmartPtr<IDebugLine>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        createLineMaterial();

        auto debugLine = factoryManager->make_ptr<CDebugLineOgreNext>();
        debugLine->setId( id );

        m_debugLines.push_back( debugLine );
        return debugLine;
    }

    void CDebugOgre::removeLine( hash_type id )
    {
        auto debugLines = m_debugLines.snapshot();
        for( auto &debugLine : debugLines )
        {
            if( debugLine->getId() == id )
            {
                auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
                if( it != m_debugLines.end() )
                {
                    m_debugLines.erase( it );
                }
            }
        }
    }

    void CDebugOgre::removeLine( SmartPtr<IDebugLine> debugLine )
    {
        auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
        if( it != m_debugLines.end() )
        {
            m_debugLines.erase( it );
        }
    }

    auto CDebugOgre::getLine( hash_type id ) const -> SmartPtr<IDebugLine>
    {
        auto debugLines = m_debugLines.snapshot();
        for( auto &debugLine : debugLines )
        {
            if( debugLine )
            {
                if( debugLine->getId() == id )
                {
                    return debugLine;
                }
            }
        }

        return nullptr;
    }

    auto CDebugOgre::addCircle( hash_type id ) -> SmartPtr<IDebugCircle>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        createLineMaterial();

        auto debugLine = factoryManager->make_ptr<CDebugCircle>();
        debugLine->setId( id );

        m_debugLines.push_back( debugLine );
        return debugLine;
    }

    void CDebugOgre::removeCircle( hash_type id )
    {
        auto debugLines = m_debugLines.snapshot();
        for( auto &debugLine : debugLines )
        {
            if( debugLine->getId() == id )
            {
                auto it = std::find( m_debugLines.begin(), m_debugLines.end(), debugLine );
                if( it != m_debugLines.end() )
                {
                    m_debugLines.erase( it );
                }
            }
        }
    }

    void CDebugOgre::removeCircle( SmartPtr<IDebugCircle> debugCircle )
    {
        auto it = std::find( m_debugCircles.begin(), m_debugCircles.end(), debugCircle );
        if( it != m_debugCircles.end() )
        {
            m_debugCircles.erase( it );
        }
    }

    auto CDebugOgre::getCircle( hash_type id ) const -> SmartPtr<IDebugCircle>
    {
        for( auto &debugLine : m_debugLines )
        {
            if( debugLine )
            {
                if( debugLine->getId() == id )
                {
                    return debugLine;
                }
            }
        }

        return nullptr;
    }

    void CDebugOgre::clear()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        if( graphicsSystem )
        {
            auto renderTask = graphicsSystem->getRenderTask();
            auto stateTask = graphicsSystem->getStateTask();
            auto task = Thread::getCurrentTask();

            const auto &loadingState = getLoadingState();
            if( loadingState == LoadingState::Loaded && task == renderTask )
            {
                for( auto &debugLine : m_debugLines )
                {
                    if( debugLine )
                    {
                        debugLine->unload( nullptr );
                    }
                }

                m_debugLines.clear();
            }
            else
            {
                for( auto &debugLine : m_debugLines )
                {
                    if( debugLine )
                    {
                        graphicsSystem->unloadObject( debugLine );
                    }
                }

                m_debugLines.clear();
            }
        }
    }

    CDebugOgre::StateListener::StateListener() = default;

    CDebugOgre::StateListener::~StateListener() = default;

    bool CDebugOgre::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool CDebugOgre::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( message->isExactly<StateMessageDrawLine>() )
        {
            auto stateMessageDrawLine = workphone::static_pointer_cast<StateMessageDrawLine>( message );

            auto colour = stateMessageDrawLine->getColour();
            auto lineId = stateMessageDrawLine->getLineId();
            auto start = stateMessageDrawLine->getStart();
            auto end = stateMessageDrawLine->getEnd();

            m_owner->drawLine( lineId, start, end, colour );
        }

        return false;
    }

    auto CDebugOgre::StateListener::getOwner() const -> SmartPtr<CDebugOgre>
    {
        auto p = m_owner.lock();
        return p;
    }

    void CDebugOgre::StateListener::setOwner( SmartPtr<CDebugOgre> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::render
