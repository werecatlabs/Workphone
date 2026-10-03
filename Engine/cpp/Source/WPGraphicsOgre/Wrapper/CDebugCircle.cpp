#include <WPGraphicsOgre/WPGraphicsOgrePCH.hpp>
#include <WPGraphicsOgre/Wrapper/CDebugCircle.hpp>
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

    CDebugCircle::CDebugCircle() = default;

    CDebugCircle::CDebugCircle( hash_type id, const Vector3<real_Num> &position, real_Num radius,
                                u32 color )
    {
    }

    CDebugCircle::~CDebugCircle() = default;

    void CDebugCircle::load( SmartPtr<ISharedObject> data )
    {
        const auto &loadingState = getLoadingState();
        if( loadingState != LoadingState::Loaded )
        {
            setLoadingState( LoadingState::Loading );

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

                auto materialName = "UnlitColour" + StringUtil::getUUID();
                setMaterialName( materialName );

                auto iColour = getColor();

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

    void CDebugCircle::unload( SmartPtr<ISharedObject> data )
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

    void CDebugCircle::update()
    {
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
                        m_manualObject->clear();

                        auto materialName = getMaterialName();
                        m_manualObject->begin( materialName.c_str() );

                        auto angle = 0.0f;
                        const auto angleIncrement = 360.0f / segments;

                        auto count = 0;
                        for( int i = 0; i < segments; i++ )
                        {
                            auto x1 = MathF::Sin( MathF::deg_to_rad() * angle ) * m_radius;
                            auto z1 = MathF::Cos( MathF::deg_to_rad() * angle ) * m_radius;

                            angle += angleIncrement;

                            auto x2 = MathF::Sin( MathF::deg_to_rad() * angle ) * m_radius;
                            auto z2 = MathF::Cos( MathF::deg_to_rad() * angle ) * m_radius;

                            auto p1 = m_position + ( m_orientation * Vector3<real_Num>( x1, 0, z1 ) );
                            auto p2 = m_position + ( m_orientation * Vector3<real_Num>( x2, 0, z2 ) );

                            m_manualObject->position( Ogre::Vector3( p1.x, p1.y, p1.z ) );
                            m_manualObject->position( Ogre::Vector3( p2.x, p2.y, p2.z ) );
                            //m_manualObject->line( count, count + 1 );
                            count += 2;
                        }

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

    void CDebugCircle::setPosition( const Vector3<real_Num> &position )
    {
        m_position = position;
    }

    auto CDebugCircle::getLifeTime() const -> f64
    {
        return m_lifeTime;
    }

    void CDebugCircle::setLifeTime( f64 lifeTime )
    {
        m_lifeTime = lifeTime;
    }

    auto CDebugCircle::getMaxLifeTime() const -> f64
    {
        return m_maxLifeTime;
    }

    void CDebugCircle::setMaxLifeTime( f64 maxLifeTime )
    {
        m_maxLifeTime = maxLifeTime;
    }

    auto CDebugCircle::isVisible() const -> bool
    {
        return m_isVisible;
    }

    void CDebugCircle::setVisible( bool visible )
    {
        if( m_isVisible != visible )
        {
            m_isVisible = visible;
            setDirty( true );
        }
    }

    void CDebugCircle::setRadius( real_Num radius )
    {
        m_radius = radius;
    }

    void CDebugCircle::setColor( u32 colour )
    {
        if( m_colour != colour )
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            WP_ASSERT( graphicsSystem );

            ScopedLock lock( graphicsSystem );

            m_colour = colour;
        }
    }

    auto CDebugCircle::getPosition() const -> Vector3<real_Num>
    {
        return m_position;
    }

    auto CDebugCircle::getRadius() const -> real_Num
    {
        return m_radius;
    }

    auto CDebugCircle::getColor() const -> u32
    {
        return m_colour;
    }

    auto CDebugCircle::getMaterialName() const -> String
    {
        return m_materialName;
    }

    void CDebugCircle::setMaterialName( const String &materialName )
    {
        m_materialName = materialName;
    }

    auto CDebugCircle::isDirty() const -> bool
    {
        return m_dirty;
    }

    void CDebugCircle::setDirty( bool dirty )
    {
        m_dirty = dirty;
    }

    auto CDebugCircle::getOrientation() const -> workphone::Quaternion<workphone::real_Num>
    {
        return m_orientation;
    }

    void CDebugCircle::setOrientation( const Quaternion<real_Num> &orientation )
    {
        m_orientation = orientation;
    }

}  // namespace workphone::render
