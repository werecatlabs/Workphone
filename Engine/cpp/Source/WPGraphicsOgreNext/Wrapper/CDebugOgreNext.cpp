#include <WPGraphicsOgreNext/WPGraphicsOgreNextPCH.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugOgreNext.hpp>
#include <WPGraphicsOgreNext/DynamicLines.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugLine.hpp>
#include <WPGraphicsOgreNext/Wrapper/CDebugCircle.hpp>
#include <Workphone/Workphone.hpp>
#include <Ogre.h>
#include <OgreWireBoundingBox.h>
#include <OgreHlmsUnlit.h>
#include <OgreHlmsUnlitDatablock.h>
#include <OgreManualObject2.h>

namespace workphone::render
{

    CDebugOgreNext::CDebugOgreNext() = default;

    CDebugOgreNext::~CDebugOgreNext()
    {
        unload( nullptr );
    }

    void CDebugOgreNext::load( SmartPtr<ISharedObject> data )
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
            m_overlayElements.reserve( 256 );

            m_memoryManager = new Ogre::ObjectMemoryManager();

            WP_ASSERT( applicationManager->isValid() );
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CDebugOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                clear();

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                if( !graphicsSystem )
                {
                    return;
                }

                auto overlayManager = graphicsSystem->getOverlayManager();
                if( !overlayManager )
                {
                    return;
                }

                for( auto &element : m_overlayElements )
                {
                    overlayManager->removeElement( element );
                }

                m_overlayElements.clear();

                if( m_overlay )
                {
                    overlayManager->removeOverlay( m_overlay );
                    m_overlay = nullptr;
                }

                SharedGraphicsObject<IDebug>::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CDebugOgreNext::preUpdate()
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
                    debugLine->preUpdate();
                }
            }

            for( auto &debugCircle : m_debugCircles )
            {
                if( debugCircle )
                {
                    debugCircle->preUpdate();
                }
            }
        }
    }

    void CDebugOgreNext::update()
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

            for( auto &debugCircle : m_debugCircles )
            {
                if( debugCircle )
                {
                    debugCircle->update();
                }
            }
        }
    }

    void CDebugOgreNext::postUpdate()
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
                    debugLine->postUpdate();
                }
            }

            auto debugLines = m_debugLines.snapshot();
            for( auto &debugLine : debugLines )
            {
                if( debugLine->getLifeTime() > debugLine->getMaxLifeTime() )
                {
                    debugLine->unload( nullptr );
                    m_debugLines.erase( debugLine );
                }
            }

            for( auto &debugCircle : m_debugCircles )
            {
                if( debugCircle )
                {
                    debugCircle->postUpdate();
                }
            }

            auto debugCircles = m_debugCircles.snapshot();
            for( auto &debugCircle : debugCircles )
            {
                if( debugCircle->getLifeTime() > debugCircle->getMaxLifeTime() )
                {
                    debugCircle->unload( nullptr );
                    m_debugCircles.erase( debugCircle );
                }
            }
        }
    }

    void CDebugOgreNext::drawPoint( hash_type id, const Vector3<real_Num> &p, u32 color )
    {
        auto size = static_cast<real_Num>( 0.1 );
        auto offset0 = Vector3<real_Num>::forward() * size;
        auto offset1 = Vector3<real_Num>::up() * size;
        auto offset2 = Vector3<real_Num>::right() * size;

        drawLine( id, p + offset0, p - offset0, color );
        drawLine( id + 1, p + offset1, p - offset1, color );
        drawLine( id + 2, p + offset2, p - offset2, color );
    }

    auto CDebugOgreNext::drawLine( hash_type id, const Vector3<real_Num> &start,
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

    auto CDebugOgreNext::drawCircle( hash_type id, const Vector3<real_Num> &position,
                                     const Quaternion<real_Num> &orientation, real_Num radius,
                                     u32 colour ) -> SmartPtr<IDebugCircle>
    {
        ScopedLock lock( this );

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

    auto CDebugOgreNext::getDatablock() const -> Ogre::HlmsUnlitDatablock *
    {
        return m_datablock;
    }

    void CDebugOgreNext::setDatablock( Ogre::HlmsUnlitDatablock *datablock )
    {
        m_datablock = datablock;
    }

    void CDebugOgreNext::drawText( hash_type id, const Vector2<real_Num> &position, const String &text,
                                   u32 color )
    {
        ScopedLock lock( this );

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

    void CDebugOgreNext::addOverlayElement( SmartPtr<IOverlayElement> element )
    {
        m_overlayElements.push_back( element );
    }

    auto CDebugOgreNext::getElementById( hash_type id ) const -> SmartPtr<IOverlayElement>
    {
        for( auto &element : m_overlayElements )
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

    auto CDebugOgreNext::getOverlay() const -> SmartPtr<IOverlay>
    {
        return m_overlay;
    }

    void CDebugOgreNext::setOverlay( SmartPtr<IOverlay> overlay )
    {
        m_overlay = overlay;
    }

    void CDebugOgreNext::createLineMaterial()
    {
        ScopedLock lock( this );

        auto root = Ogre::Root::getSingletonPtr();

        auto hlmsManager = root->getHlmsManager();

        auto hlms = hlmsManager->getHlms( Ogre::HLMS_UNLIT );
        auto hlmsUnlitMaterials = static_cast<Ogre::HlmsUnlit *>( hlms );

        static const auto datablockUnlitName = Ogre::String( "UnlitColour" );

        auto macroBlock = Ogre::HlmsMacroblock();
        auto blendBlock = Ogre::HlmsBlendblock();
        auto paramVec = Ogre::HlmsParamVec();

        auto unlitDatablock =
            static_cast<Ogre::HlmsUnlitDatablock *>( hlmsUnlitMaterials->createDatablock(
                datablockUnlitName, datablockUnlitName, macroBlock, blendBlock, paramVec ) );

        unlitDatablock->setUseColour( true );

        auto colour = Ogre::ColourValue( 1, 1, 1 );
        unlitDatablock->setColour( colour );

        m_datablock = unlitDatablock;
    }

    auto CDebugOgreNext::addLine( hash_type id ) -> SmartPtr<IDebugLine>
    {
        ScopedLock lock( this );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        if( m_datablock == nullptr )
        {
            createLineMaterial();
        }

        auto debugLine = factoryManager->make_ptr<CDebugLineOgreNext>();
        debugLine->setMemoryManager( m_memoryManager );
        debugLine->setId( id );
        debugLine->setDatablock( m_datablock );

        m_debugLines.push_back( debugLine );

        return debugLine;
    }

    void CDebugOgreNext::removeLine( hash_type id )
    {
        auto debugLines = m_debugLines.snapshot();
        for( auto &debugLine : debugLines )
        {
            if( debugLine->getId() == id )
            {
                m_debugLines.erase( debugLine );
            }
        }
    }

    void CDebugOgreNext::removeLine( SmartPtr<IDebugLine> debugLine )
    {
        m_debugLines.erase( debugLine );
    }

    auto CDebugOgreNext::getLine( hash_type id ) const -> SmartPtr<IDebugLine>
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

    auto CDebugOgreNext::addCircle( hash_type id ) -> SmartPtr<IDebugCircle>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        if( m_datablock == nullptr )
        {
            createLineMaterial();
        }

        auto debugLine = factoryManager->make_ptr<CDebugCircle>();
        debugLine->setMemoryManager( m_memoryManager );
        debugLine->setId( id );
        debugLine->setDatablock( m_datablock );

        m_debugCircles.push_back( debugLine );

        return debugLine;
    }

    void CDebugOgreNext::removeCircle( hash_type id )
    {
        auto debugLines = m_debugLines.snapshot();
        for( auto &debugLine : debugLines )
        {
            if( debugLine->getId() == id )
            {
                m_debugLines.erase( debugLine );
            }
        }
    }

    void CDebugOgreNext::removeCircle( SmartPtr<IDebugCircle> debugCircle )
    {
        m_debugCircles.erase( debugCircle );
    }

    auto CDebugOgreNext::getCircle( hash_type id ) const -> SmartPtr<IDebugCircle>
    {
        for( auto &debugLine : m_debugCircles )
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

    void CDebugOgreNext::clear()
    {
        for( auto &debugLine : m_debugLines )
        {
            if( debugLine )
            {
                debugLine->unload( nullptr );
            }
        }

        for( auto &debugCircle : m_debugCircles )
        {
            if( debugCircle )
            {
                debugCircle->unload( nullptr );
            }
        }

        for( auto &overlayElement : m_overlayElements )
        {
            if( overlayElement )
            {
                overlayElement->unload( nullptr );
            }
        }

        m_debugLines.clear();
        m_debugCircles.clear();
        m_overlayElements.clear();
    }

    CDebugOgreNext::StateListener::StateListener() = default;

    CDebugOgreNext::StateListener::~StateListener() = default;

    bool CDebugOgreNext::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

    bool CDebugOgreNext::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            if( message->isExactly<StateMessageDrawLine>() )
            {
                auto stateMessageDrawLine =
                    workphone::static_pointer_cast<StateMessageDrawLine>( message );

                auto colour = stateMessageDrawLine->getColour();
                auto lineId = stateMessageDrawLine->getLineId();
                auto start = stateMessageDrawLine->getStart();
                auto end = stateMessageDrawLine->getEnd();

                owner->drawLine( lineId, start, end, colour );
            }
        }

        return false;
    }

    auto CDebugOgreNext::StateListener::getOwner() const -> SmartPtr<CDebugOgreNext>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void CDebugOgreNext::StateListener::setOwner( SmartPtr<CDebugOgreNext> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone::render
