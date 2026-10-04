#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/WPPhysicsManager2D.hpp"
#include "WPPhysics/WPPhysicsNativeBoxShape2.hpp"
#include "WPPhysics/WPPhysicsScene2.hpp"
#include "WPPhysics/WPPhysicsNativeSphereShape2.hpp"
#include "WPPhysics/WPPhysicsSphereShape2.hpp"
#include "WPPhysics/WPPhysicsBoxShape2.hpp"
#include <Workphone/Workphone.hpp>
#include "WPPhysics/WPPhysicsMaterial2.hpp"
#include "WPPhysics/WPPhysicsRigidBodySortData.hpp"

namespace workphone::physics
{
    WPPhysicsManager2D::WPPhysicsManager2D()
    {
    }

    WPPhysicsManager2D::~WPPhysicsManager2D()
    {
        clear();
    }

    void WPPhysicsManager2D::update( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        if( dt <= static_cast<time_interval>( 0.0 ) )
            return;

        auto engine = core::IApplicationManager::instance();
        auto profiler = engine->getProfiler();

        WP_PROFILE_START( "updateRigidBodies" );
        updateRigidBodies( task, t, dt );
        WP_PROFILE_END( "updateRigidBodies" );

        WP_PROFILE_START( "updateParticles" );
        updateParticles( task, t, dt );
        WP_PROFILE_END( "updateParticles" );
    }

    void WPPhysicsManager2D::updateRigidBodies( const s32 &task, const time_interval &t,
                                               const time_interval &dt )
    {
        for( auto &world : WPPhysicsManager2::getWorlds() )
        {
            if( auto scene = dynamic_cast<WPPhysicsScene2 *>( world.get() ) )
            {
                scene->simulate( static_cast<real_Num>( dt ) );
            }
            else if( world )
            {
                world->updateRigidBodies();
            }
        }
    }

    void WPPhysicsManager2D::updateParticles( const s32 &task, const time_interval &t,
                                             const time_interval &dt )
    {
        for( auto &world : WPPhysicsManager2::getWorlds() )
        {
            if( world )
            {
                world->updateParticles();
            }
        }
    }

    SmartPtr<IPhysicsScene2> WPPhysicsManager2D::addWorld( u32 id )
    {
        return WPPhysicsManager2::addWorld( id );
    }

    void WPPhysicsManager2D::removeWorld( SmartPtr<IPhysicsScene2> world )
    {
        WPPhysicsManager2::removeWorld( world );
    }

    SmartPtr<IPhysicsScene2> WPPhysicsManager2D::findWorld( u32 id ) const
    {
        return WPPhysicsManager2::findWorld( id );
    }

    Array<SmartPtr<IPhysicsScene2>> WPPhysicsManager2D::getWorlds() const
    {
        return WPPhysicsManager2::getWorlds();
    }

    SmartPtr<IPhysicsShape2> WPPhysicsManager2D::createCollisionShape( u8 type )
    {
        if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        {
            return WPPhysicsManager2::createCollisionShapeByType( ISphereShape2::typeInfo() );
        }
        if( type == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            return WPPhysicsManager2::createCollisionShapeByType( IBoxShape2::typeInfo() );
        }
        WP_LOG_WARNING( "WPPhysicsManager2D::createCollisionShape: unsupported shape type." );
        return nullptr;
    }

    SmartPtr<IPhysicsShape2> WPPhysicsManager2D::createCollisionShape( const Properties &properties )
    {
        auto shape = WPPhysicsManager2::createCollisionShapeByType( IBoxShape2::typeInfo() );
        if( shape )
        {
            shape->setProperties( workphone::make_ptr<Properties>( properties ) );
        }
        return shape;
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2D::createRigidBody()
    {
        return WPPhysicsManager2::createRigidBody();
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2D::createRigidBody( SmartPtr<IPhysicsShape2> collisionShape )
    {
        return WPPhysicsManager2::createRigidBody( collisionShape );
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2D::getRigidBody( u32 id ) const
    {
        return WPPhysicsManager2::getRigidBody( id );
    }

    SmartPtr<IPhysicsParticle2> WPPhysicsManager2D::createParticle(
        u8 particleType, SmartPtr<IPhysicsShape2> collisionShape )
    {
        return WPPhysicsManager2::createParticle( particleType, collisionShape );
    }

    SmartPtr<IPhysicsSoftBody2> WPPhysicsManager2D::createSoftBody()
    {
        return WPPhysicsManager2::createSoftBody();
    }

    void WPPhysicsManager2D::clear()
    {
        WPPhysicsManager2::clear();
    }

    bool WPPhysicsManager2D::destroyCollisionShape( IPhysicsShape2 *collisionShape )
    {
        return removeCollisionShape( collisionShape );
    }

    bool WPPhysicsManager2D::destroyPhysicsBody( WPPhysicsRigidBody2 *body )
    {
        return removeRigidBody( body );
    }

    SmartPtr<IPhysicsParticle2> WPPhysicsManager2D::getParticle( u32 id ) const
    {
        return WPPhysicsManager2::getParticle( id );
    }

    void WPPhysicsManager2D::OnChangeFlags( IPhysicsBody2D *body )
    {
        WPPhysicsManager2::OnChangeFlags( body );
    }

    u32 WPPhysicsManager2D::getNumActiveRigidBodies() const
    {
        return getRigidBodyCount();
    }

    u32 WPPhysicsManager2D::getNumActiveColRecords() const
    {
        return 0;
    }

    bool WPPhysicsManager2D::isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const
    {
        return WPPhysicsManager2::isColliding( bodyA, bodyB );
    }

    WPPhysicsManager2D::ProcessWorld::ProcessWorld( SmartPtr<IPhysicsScene2> world, const s32 &task,
                                                   const time_interval &t, const time_interval &dt,
                                                   int state ) :
        m_world( world ),
        m_task( task ),
        m_t( t ),
        m_dt( dt ),
        m_state( state )
    {
    }

    WPPhysicsManager2D::ProcessWorld::~ProcessWorld()
    {
    }

    void WPPhysicsManager2D::ProcessWorld::execute()
    {
        if( !m_world )
        {
            return;
        }
        if( m_state == 0 )
        {
            if( auto scene = dynamic_cast<WPPhysicsScene2 *>( m_world.get() ) )
            {
                scene->simulate( static_cast<real_Num>( m_dt ) );
            }
            else
            {
                m_world->updateRigidBodies();
            }
        }
        else
        {
            m_world->updateParticles();
        }
    }
} // namespace workphone::physics
