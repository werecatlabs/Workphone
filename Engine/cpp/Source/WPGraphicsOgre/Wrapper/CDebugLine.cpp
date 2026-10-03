#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugLine.hpp>
#include <WPGraphicsOgre/Addons/DynamicLines.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <OgreSceneManager.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>
#include <Ogre.h>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, CDebugLineOgreNext, IDebugLine );

    u32 CDebugLineOgreNext::m_nameExt = 0;

    CDebugLineOgreNext::CDebugLineOgreNext() = default;

    CDebugLineOgreNext::~CDebugLineOgreNext()
    {
        unload( nullptr );
    }

    void CDebugLineOgreNext::load( SmartPtr<ISharedObject> data )
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Loaded )
        {
            setLoadingState( LoadingState::Loading );

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto smgr = graphicsSystem->getGraphicsScene();
            WP_ASSERT( smgr );

            Ogre::SceneManager *sceneManager = nullptr;
            smgr->_getObject( reinterpret_cast<void **>( &sceneManager ) );

            if( sceneManager )
            {
                auto root = Ogre::Root::getSingletonPtr();
                auto rootSceneNode = sceneManager->getRootSceneNode();
                auto sceneNode = rootSceneNode->createChildSceneNode();

                auto iColour = getColour();

                Ogre::ColourValue colourValue;
                colourValue.setAsRGBA( iColour );
                colourValue.a = 1.0f;

                auto manualObject = sceneManager->createManualObject();

                //manualObject->colour( 1.0, 0, 0 );

                sceneNode->attachObject( manualObject );

                m_manualObject = manualObject;
                m_sceneNode = sceneNode;
            }

            setLoadingState( LoadingState::Loaded );
        }
    }

    void CDebugLineOgreNext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto graphicsSystem = applicationManager->getGraphicsSystem();
                WP_ASSERT( graphicsSystem );

                ScopedLock lock( graphicsSystem );

                if( m_sceneNode )
                {
                    if( m_manualObject )
                    {
                        m_sceneNode->detachObject( m_manualObject );
                    }

                    auto sceneManager = m_sceneNode->getCreator();
                    if( sceneManager )
                    {
                        sceneManager->destroySceneNode( m_sceneNode );
                        m_sceneNode = nullptr;

                        if( m_manualObject )
                        {
                            sceneManager->destroyManualObject( m_manualObject );
                            m_manualObject = nullptr;
                        }
                    }
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void CDebugLineOgreNext::update()
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            auto timer = applicationManager->getTimer();
            WP_ASSERT( timer );

            auto t = timer->getTime();
            auto dt = timer->getDeltaTime();

            auto stateTask = graphicsSystem->getRenderTask();
            auto task = Thread::getCurrentTask();

            if( task == stateTask )
            {
                if( isDirty() )
                {
                    if( auto visible = isVisible() )
                    {
                        if( m_manualObject )
                        {
                            auto s = getPosition();
                            auto e = s + getVector();

                            auto p1 = Ogre::Vector3( s.X(), s.Y(), s.Z() );
                            auto p2 = Ogre::Vector3( e.X(), e.Y(), e.Z() );

                            auto materialName = getMaterialName();

                            m_manualObject->clear();

                            m_manualObject->begin( materialName.c_str() );  // Datablock DbRed works

                            m_manualObject->position( p1 );
                            m_manualObject->position( p2 );
                            m_manualObject->end();
                        }

                        m_sceneNode->setVisible( true );
                    }
                    else
                    {
                        m_sceneNode->setVisible( false );
                    }

                    m_lifeTime = 0.0;
                    setDirty( false );
                }

                m_lifeTime += dt;
            }
        }
    }

    auto CDebugLineOgreNext::getVector() const -> Vector3F
    {
        return m_vector;
    }

    void CDebugLineOgreNext::setVector( const Vector3F &vector )
    {
        m_vector = vector;
    }

    auto CDebugLineOgreNext::getPosition() const -> Vector3F
    {
        return m_position;
    }

    void CDebugLineOgreNext::setPosition( const Vector3F &position )
    {
        m_position = position;
    }

    auto CDebugLineOgreNext::getLifeTime() const -> f64
    {
        return m_lifeTime;
    }

    void CDebugLineOgreNext::setLifeTime( f64 lifeTime )
    {
        m_lifeTime = lifeTime;
    }

    auto CDebugLineOgreNext::getMaxLifeTime() const -> f64
    {
        return m_maxLifeTime;
    }

    void CDebugLineOgreNext::setMaxLifeTime( f64 maxLifeTime )
    {
        m_maxLifeTime = maxLifeTime;
    }

    auto CDebugLineOgreNext::isVisible() const -> bool
    {
        return m_isVisible;
    }

    void CDebugLineOgreNext::setVisible( bool visible )
    {
        if( m_isVisible != visible )
        {
            m_isVisible = visible;
            setDirty( true );
        }
    }

    auto CDebugLineOgreNext::getMemoryManager() const -> Ogre::ObjectMemoryManager *
    {
        return m_memoryManager;
    }

    void CDebugLineOgreNext::setMemoryManager( Ogre::ObjectMemoryManager *memoryManager )
    {
        m_memoryManager = memoryManager;
    }

    auto CDebugLineOgreNext::getDatablock() const -> Ogre::HlmsUnlitDatablock *
    {
        return m_datablock;
    }

    void CDebugLineOgreNext::setDatablock( Ogre::HlmsUnlitDatablock *datablock )
    {
        m_datablock = datablock;
    }

    auto CDebugLineOgreNext::getMaterialName() const -> String
    {
        ScopedLock lock( this );
        return m_materialName;
    }

    void CDebugLineOgreNext::setMaterialName( const String &materialName )
    {
        ScopedLock lock( this );
        m_materialName = materialName;
    }

    auto CDebugLineOgreNext::getColour() const -> u32
    {
        return m_colour;
    }

    void CDebugLineOgreNext::setColour( u32 colour )
    {
        if( m_colour != colour )
        {
            ScopedLock lock( this );

            m_colour = colour;

            if( auto datablock = getDatablock() )
            {
                auto iColour = getColour();
                Ogre::ColourValue colourValue;
                colourValue.setAsRGBA( iColour );
                colourValue.a = 1.0f;
            }
        }
    }

    auto CDebugLineOgreNext::isDirty() const -> bool
    {
        return m_isDirty;
    }

    void CDebugLineOgreNext::setDirty( bool dirty )
    {
        m_isDirty = dirty;
    }

    void CDebugLineOgreNext::lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        graphicsSystem->lock();
    }

    bool CDebugLineOgreNext::try_lock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        return graphicsSystem->try_lock();
    }

    void CDebugLineOgreNext::unlock()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        graphicsSystem->unlock();
    }

}  // namespace workphone::render
