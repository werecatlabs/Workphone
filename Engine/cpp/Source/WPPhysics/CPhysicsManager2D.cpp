#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/CPhysicsManager2D.hpp"
#include "WPPhysics/CPhysicsBoxShape2.hpp"
#include "WPPhysics/CPhysicsScene2.hpp"
#include "WPPhysics/CPhysicsSphereShape2.hpp"
#include "WPPhysics/SphereShape2.hpp"
#include "WPPhysics/CBoxShape2.hpp"
#include <Workphone/Workphone.hpp>
#include "WPPhysics/PhysicsMaterial2.hpp"
#include "WPPhysics/RigidBodySortData.hpp"

namespace workphone::physics
{
    CPhysicsManager2D::CPhysicsManager2D()
    {
    }

    CPhysicsManager2D::~CPhysicsManager2D()
    {
        clear();
    }

    void CPhysicsManager2D::update( const s32 &task, const time_interval &t, const time_interval &dt )
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

    void CPhysicsManager2D::updateRigidBodies( const s32 &task, const time_interval &t,
                                               const time_interval &dt )
    {
        for( auto &world : CPhysicsManager2::getWorlds() )
        {
            if( auto scene = dynamic_cast<CPhysicsScene2 *>( world.get() ) )
            {
                scene->simulate( static_cast<real_Num>( dt ) );
            }
            else if( world )
            {
                world->updateRigidBodies();
            }
        }
    }

    void CPhysicsManager2D::updateParticles( const s32 &task, const time_interval &t,
                                             const time_interval &dt )
    {
        for( auto &world : CPhysicsManager2::getWorlds() )
        {
            if( world )
            {
                world->updateParticles();
            }
        }
    }

    SmartPtr<IPhysicsScene2> CPhysicsManager2D::addWorld( u32 id )
    {
        return CPhysicsManager2::addWorld( id );
    }

    void CPhysicsManager2D::removeWorld( SmartPtr<IPhysicsScene2> world )
    {
        CPhysicsManager2::removeWorld( world );
    }

    SmartPtr<IPhysicsScene2> CPhysicsManager2D::findWorld( u32 id ) const
    {
        return CPhysicsManager2::findWorld( id );
    }

    Array<SmartPtr<IPhysicsScene2>> CPhysicsManager2D::getWorlds() const
    {
        return CPhysicsManager2::getWorlds();
    }

    SmartPtr<IPhysicsShape2> CPhysicsManager2D::createCollisionShape( u8 type )
    {
        if( type == WORKPHONE_COLLISION_SHAPE_SPHERE )
        {
            return CPhysicsManager2::createCollisionShapeByType( ISphereShape2::typeInfo() );
        }
        if( type == WORKPHONE_COLLISION_SHAPE_BOX )
        {
            return CPhysicsManager2::createCollisionShapeByType( IBoxShape2::typeInfo() );
        }
        WP_LOG_WARNING( "CPhysicsManager2D::createCollisionShape: unsupported shape type." );
        return nullptr;
    }

    SmartPtr<IPhysicsShape2> CPhysicsManager2D::createCollisionShape( const Properties &properties )
    {
        auto shape = CPhysicsManager2::createCollisionShapeByType( IBoxShape2::typeInfo() );
        if( shape )
        {
            shape->setProperties( workphone::make_ptr<Properties>( properties ) );
        }
        return shape;
    }

    SmartPtr<IRigidBody2> CPhysicsManager2D::createRigidBody()
    {
        return CPhysicsManager2::createRigidBody();
    }

    SmartPtr<IRigidBody2> CPhysicsManager2D::createRigidBody( SmartPtr<IPhysicsShape2> collisionShape )
    {
        return CPhysicsManager2::createRigidBody( collisionShape );
    }

    SmartPtr<IRigidBody2> CPhysicsManager2D::getRigidBody( u32 id ) const
    {
        return CPhysicsManager2::getRigidBody( id );
    }

    SmartPtr<IPhysicsParticle2> CPhysicsManager2D::createParticle(
        u8 particleType, SmartPtr<IPhysicsShape2> collisionShape )
    {
        return CPhysicsManager2::createParticle( particleType, collisionShape );
    }

    SmartPtr<IPhysicsSoftBody2> CPhysicsManager2D::createSoftBody()
    {
        return CPhysicsManager2::createSoftBody();
    }

    void CPhysicsManager2D::clear()
    {
        CPhysicsManager2::clear();
    }

    bool CPhysicsManager2D::destroyCollisionShape( IPhysicsShape2 *collisionShape )
    {
        return removeCollisionShape( collisionShape );
    }

    bool CPhysicsManager2D::destroyPhysicsBody( CRigidBody2 *body )
    {
        return removeRigidBody( body );
    }

    SmartPtr<IPhysicsParticle2> CPhysicsManager2D::getParticle( u32 id ) const
    {
        return CPhysicsManager2::getParticle( id );
    }

    void CPhysicsManager2D::OnChangeFlags( IPhysicsBody2D *body )
    {
        CPhysicsManager2::OnChangeFlags( body );
    }

    u32 CPhysicsManager2D::getNumActiveRigidBodies() const
    {
        return getRigidBodyCount();
    }

    u32 CPhysicsManager2D::getNumActiveColRecords() const
    {
        return 0;
    }

    bool CPhysicsManager2D::isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const
    {
        return CPhysicsManager2::isColliding( bodyA, bodyB );
    }

    CPhysicsManager2D::ProcessWorld::ProcessWorld( SmartPtr<IPhysicsScene2> world, const s32 &task,
                                                   const time_interval &t, const time_interval &dt,
                                                   int state ) :
        m_world( world ),
        m_task( task ),
        m_t( t ),
        m_dt( dt ),
        m_state( state )
    {
    }

    CPhysicsManager2D::ProcessWorld::~ProcessWorld()
    {
    }

    void CPhysicsManager2D::ProcessWorld::execute()
    {
        if( !m_world )
        {
            return;
        }
        if( m_state == 0 )
        {
            if( auto scene = dynamic_cast<CPhysicsScene2 *>( m_world.get() ) )
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
