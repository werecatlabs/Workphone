#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsManager2.hpp>
#include <WPPhysics/WPPhysicsScene2.hpp>
#include <WPPhysics/WPPhysicsRigidBody2.hpp>
#include <WPPhysics/WPPhysicsBoxShape2.hpp>
#include <WPPhysics/WPPhysicsSphereShape2.hpp>
#include <WPPhysics/WPPhysicsParticle2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone::physics
{
    namespace
    {
        class PhysicsSoftBody2 final : public IPhysicsSoftBody2
        {
        public:
            void setPosition( const Vector2<real_Num> &position ) override
            {
                m_position = position;
            }

            const Vector2<real_Num> &getPosition() const override
            {
                return m_position;
            }

        private:
            Vector2<real_Num> m_position = Vector2<real_Num>::ZERO;
        };

        SmartPtr<IPhysicsShape2> getBodyShape( IPhysicsBody2D *body )
        {
            if( auto rigidBody = dynamic_cast<IRigidBody2 *>( body ) )
            {
                return rigidBody->getCollisionShape();
            }
            if( auto particle = dynamic_cast<IPhysicsParticle2 *>( body ) )
            {
                return particle->getCollisionShape();
            }
            return nullptr;
        }

        Vector2<real_Num> rotate( const Vector2<real_Num> &value, real_Num angle )
        {
            const auto cosine = static_cast<real_Num>( std::cos( angle ) );
            const auto sine = static_cast<real_Num>( std::sin( angle ) );
            return Vector2<real_Num>( value.X() * cosine - value.Y() * sine,
                                      value.X() * sine + value.Y() * cosine );
        }

        Vector2<real_Num> worldShapeCenter( IPhysicsBody2D *body, const Vector2<real_Num> &localCenter )
        {
            return body->getPosition() + rotate( localCenter, body->getOrientation() );
        }

        bool sphereIntersectsOrientedBox( const Vector2<real_Num> &sphereCenter, real_Num radius,
                                          IPhysicsBody2D *boxBody, const IBoxShape2 &box )
        {
            const auto bounds = box.getAABB();
            const auto boxCenter = worldShapeCenter( boxBody, bounds.getCenter() );
            const auto localSphere = rotate( sphereCenter - boxCenter, -boxBody->getOrientation() );
            const auto halfSize = bounds.getHalfSize();
            const auto closest =
                Vector2<real_Num>( std::clamp( localSphere.X(), -halfSize.X(), halfSize.X() ),
                                   std::clamp( localSphere.Y(), -halfSize.Y(), halfSize.Y() ) );
            return ( localSphere - closest ).lengthSquared() <= radius * radius;
        }

        bool orientedBoxesIntersect( IPhysicsBody2D *bodyA, const IBoxShape2 &boxA,
                                     IPhysicsBody2D *bodyB, const IBoxShape2 &boxB )
        {
            const auto              boundsA = boxA.getAABB();
            const auto              boundsB = boxB.getAABB();
            const auto              centerA = worldShapeCenter( bodyA, boundsA.getCenter() );
            const auto              centerB = worldShapeCenter( bodyB, boundsB.getCenter() );
            const auto              halfA = boundsA.getHalfSize();
            const auto              halfB = boundsB.getHalfSize();
            const Vector2<real_Num> axesA[2] = {
                rotate( Vector2<real_Num>::UNIT_X, bodyA->getOrientation() ),
                rotate( Vector2<real_Num>::UNIT_Y, bodyA->getOrientation() )
            };
            const Vector2<real_Num> axesB[2] = {
                rotate( Vector2<real_Num>::UNIT_X, bodyB->getOrientation() ),
                rotate( Vector2<real_Num>::UNIT_Y, bodyB->getOrientation() )
            };
            const auto centerDelta = centerB - centerA;

            const auto overlapsOnAxis = [&]( const Vector2<real_Num> &axis )
            {
                const auto distance = Math<real_Num>::Abs( centerDelta.dotProduct( axis ) );
                const auto radiusA = Math<real_Num>::Abs( axesA[0].dotProduct( axis ) ) * halfA.X() +
                                     Math<real_Num>::Abs( axesA[1].dotProduct( axis ) ) * halfA.Y();
                const auto radiusB = Math<real_Num>::Abs( axesB[0].dotProduct( axis ) ) * halfB.X() +
                                     Math<real_Num>::Abs( axesB[1].dotProduct( axis ) ) * halfB.Y();
                return distance <= radiusA + radiusB;
            };

            return overlapsOnAxis( axesA[0] ) && overlapsOnAxis( axesA[1] ) &&
                   overlapsOnAxis( axesB[0] ) && overlapsOnAxis( axesB[1] );
        }

        bool collisionFilterPasses( u32 typeA, u32 maskA, u32 typeB, u32 maskB )
        {
            return ( typeA == 0u && typeB == 0u ) || ( typeA & maskB ) != 0u || ( typeB & maskA ) != 0u;
        }
    } // namespace

    WPPhysicsManager2::WPPhysicsManager2() = default;
    WPPhysicsManager2::~WPPhysicsManager2()
    {
        clear();
    }

    void *WPPhysicsManager2::getNativeObject() const
    {
        return nullptr;
    }

    void WPPhysicsManager2::updateRigidBodies()
    {
        Array<SmartPtr<IPhysicsScene2>> worlds;
        {
            ScopedLock lock( this );
            worlds = m_worlds;
        }
        for( auto &world : worlds )
        {
            WP_ASSERT( world );
            if( world )
            {
                world->updateRigidBodies();
            }
        }
    }

    void WPPhysicsManager2::updateParticles()
    {
        Array<SmartPtr<IPhysicsScene2>> worlds;
        {
            ScopedLock lock( this );
            worlds = m_worlds;
        }
        for( auto &world : worlds )
        {
            WP_ASSERT( world );
            if( world )
            {
                world->updateParticles();
            }
        }
    }

    SmartPtr<IPhysicsScene2> WPPhysicsManager2::addWorld( u32 id )
    {
        if( auto existingWorld = findWorld( id ) )
        {
            return existingWorld;
        }

        auto world = workphone::make_ptr<WPPhysicsScene2>();
        WP_ASSERT( world );
        {
            ScopedLock lock( this );
            for( u32 i = 0; i < m_worldIds.size(); ++i )
            {
                if( m_worldIds[i] == id )
                {
                    return m_worlds[i];
                }
            }
            m_worlds.push_back( world );
            m_worldIds.push_back( id );
        }
        WP_ASSERT( findWorld( id ) == world );
        return world;
    }

    void WPPhysicsManager2::removeWorld( SmartPtr<IPhysicsScene2> world )
    {
        if( !world )
        {
            return;
        }
        ScopedLock lock( this );
        for( u32 i = 0; i < m_worlds.size(); ++i )
        {
            if( m_worlds[i] == world )
            {
                m_worlds.erase( m_worlds.begin() + i );
                m_worldIds.erase( m_worldIds.begin() + i );
                return;
            }
        }
    }

    SmartPtr<IPhysicsScene2> WPPhysicsManager2::findWorld( u32 id ) const
    {
        ScopedLock lock( this );
        for( u32 i = 0; i < m_worldIds.size(); ++i )
        {
            if( m_worldIds[i] == id )
            {
                WP_ASSERT( i < m_worlds.size() );
                return m_worlds[i];
            }
        }
        return nullptr;
    }

    Array<SmartPtr<IPhysicsScene2>> WPPhysicsManager2::getWorlds() const
    {
        ScopedLock lock( this );
        return m_worlds;
    }

    SmartPtr<IPhysicsShape2> WPPhysicsManager2::createCollisionShapeByType( hash32 type )
    {
        SmartPtr<IPhysicsShape2> shape;
        if( type == IBoxShape2::typeInfo() )
        {
            shape = SmartPtr<IPhysicsShape2>( new WPPhysicsBoxShape2 );
        }
        else if( type == ISphereShape2::typeInfo() )
        {
            shape = SmartPtr<IPhysicsShape2>( new WPPhysicsSphereShape2 );
        }
        else
        {
            WP_LOG_WARNING( "WPPhysicsManager2::createCollisionShapeByType: unsupported shape type." );
            return nullptr;
        }

        WP_ASSERT( shape );
        {
            ScopedLock lock( this );
            m_shapes.push_back( shape );
        }
        return shape;
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2::createRigidBody()
    {
        auto body = SmartPtr<IRigidBody2>( new WPPhysicsRigidBody2 );
        WP_ASSERT( body );
        {
            ScopedLock lock( this );
            m_bodies.push_back( body );
        }
        return body;
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2::createRigidBody( SmartPtr<IPhysicsShape2> collisionShape )
    {
        auto body = createRigidBody();
        if( collisionShape )
        {
            body->setCollisionShape( collisionShape );
            WP_ASSERT( body->getCollisionShape() == collisionShape );
        }
        return body;
    }

    SmartPtr<IPhysicsParticle2> WPPhysicsManager2::createParticle(
        u8 particleType, SmartPtr<IPhysicsShape2> collisionShape )
    {
        auto particle = workphone::make_ptr<WPPhysicsParticle2>( this );
        particle->setObjectType( particleType );
        particle->setCollisionShape( collisionShape );
        {
            ScopedLock lock( this );
            m_particles.push_back( particle );
        }
        return particle;
    }

    SmartPtr<IPhysicsSoftBody2> WPPhysicsManager2::createSoftBody()
    {
        auto       softBody = workphone::make_ptr<PhysicsSoftBody2>();
        ScopedLock lock( this );
        m_softBodies.push_back( softBody );
        return softBody;
    }

    void WPPhysicsManager2::clear()
    {
        ScopedLock lock( this );
        m_softBodies.clear();
        m_particles.clear();
        m_bodies.clear();
        m_shapes.clear();
        m_worldIds.clear();
        m_worlds.clear();
    }

    SmartPtr<IPhysicsParticle2> WPPhysicsManager2::getParticle( u32 id ) const
    {
        ScopedLock lock( this );
        for( const auto &particle : m_particles )
        {
            if( auto backendParticle = dynamic_cast<WPPhysicsParticle2 *>( particle.get() ) )
            {
                if( backendParticle->getId() == id )
                {
                    return particle;
                }
            }
        }
        return nullptr;
    }

    SmartPtr<IRigidBody2> WPPhysicsManager2::getRigidBody( u32 id ) const
    {
        ScopedLock lock( this );
        for( const auto &body : m_bodies )
        {
            if( body && static_cast<u32>( body->getId() ) == id )
            {
                return body;
            }
        }
        return nullptr;
    }

    bool WPPhysicsManager2::removeRigidBody( IRigidBody2 *body )
    {
        if( !body )
        {
            return false;
        }

        SmartPtr<IRigidBody2>           ownedBody;
        Array<SmartPtr<IPhysicsScene2>> worlds;
        {
            ScopedLock lock( this );
            const auto it = std::find_if( m_bodies.begin(), m_bodies.end(),
                                          [body]( const SmartPtr<IRigidBody2> &candidate )
                                          { return candidate.get() == body; } );
            if( it == m_bodies.end() )
            {
                return false;
            }
            ownedBody = *it;
            m_bodies.erase( it );
            worlds = m_worlds;
        }

        for( auto &world : worlds )
        {
            if( world )
            {
                world->removeRigidBody( ownedBody );
            }
        }
        ownedBody->setCollisionShape( nullptr );
        ownedBody->setEnabled( false );
        ownedBody->setLoadingState( LoadingState::Unloaded );
        return true;
    }

    bool WPPhysicsManager2::removeCollisionShape( IPhysicsShape2 *shape )
    {
        if( !shape )
        {
            return false;
        }

        ScopedLock lock( this );
        for( const auto &body : m_bodies )
        {
            if( body && body->getCollisionShape().get() == shape )
            {
                return false;
            }
        }
        for( const auto &particle : m_particles )
        {
            if( particle && particle->getCollisionShape().get() == shape )
            {
                return false;
            }
        }

        const auto it = std::find_if( m_shapes.begin(), m_shapes.end(),
                                      [shape]( const SmartPtr<IPhysicsShape2> &candidate )
                                      { return candidate.get() == shape; } );
        if( it == m_shapes.end() )
        {
            return false;
        }
        m_shapes.erase( it );
        return true;
    }

    u32 WPPhysicsManager2::getRigidBodyCount() const
    {
        ScopedLock lock( this );
        return static_cast<u32>( m_bodies.size() );
    }

    void WPPhysicsManager2::OnChangeFlags( IPhysicsBody2D *body )
    {
        if( !body )
        {
            WP_LOG_WARNING( "WPPhysicsManager2::OnChangeFlags: body is null." );
        }
    }

    bool WPPhysicsManager2::isColliding( IPhysicsBody2D *bodyA, IPhysicsBody2D *bodyB ) const
    {
        WP_ASSERT( bodyA );
        WP_ASSERT( bodyB );
        if( !bodyA || !bodyB )
        {
            return false;
        }

        const auto maskA = bodyA->getCollisionMask();
        const auto maskB = bodyB->getCollisionMask();
        const auto typeA = bodyA->getCollisionType();
        const auto typeB = bodyB->getCollisionType();
        if( !collisionFilterPasses( typeA, maskA, typeB, maskB ) || !bodyA->isEnabled() ||
            !bodyB->isEnabled() )
        {
            return false;
        }

        const auto shapeA = getBodyShape( bodyA );
        const auto shapeB = getBodyShape( bodyB );
        if( !shapeA || !shapeB || !shapeA->isEnabled() || !shapeB->isEnabled() )
        {
            return false;
        }

        const auto sphereA = workphone::dynamic_pointer_cast<ISphereShape2>( shapeA );
        const auto sphereB = workphone::dynamic_pointer_cast<ISphereShape2>( shapeB );
        if( sphereA && sphereB )
        {
            const auto centerA = worldShapeCenter( bodyA, sphereA->getSphere().getCenter() );
            const auto centerB = worldShapeCenter( bodyB, sphereB->getSphere().getCenter() );
            const auto radius = sphereA->getRadius() + sphereB->getRadius();
            return ( centerB - centerA ).lengthSquared() <= radius * radius;
        }
        if( sphereA )
        {
            const auto boxB = workphone::dynamic_pointer_cast<IBoxShape2>( shapeB );
            if( !boxB )
            {
                return false;
            }
            const auto center = worldShapeCenter( bodyA, sphereA->getSphere().getCenter() );
            return sphereIntersectsOrientedBox( center, sphereA->getRadius(), bodyB, *boxB );
        }
        if( sphereB )
        {
            const auto boxA = workphone::dynamic_pointer_cast<IBoxShape2>( shapeA );
            if( !boxA )
            {
                return false;
            }
            const auto center = worldShapeCenter( bodyB, sphereB->getSphere().getCenter() );
            return sphereIntersectsOrientedBox( center, sphereB->getRadius(), bodyA, *boxA );
        }

        const auto boxA = workphone::dynamic_pointer_cast<IBoxShape2>( shapeA );
        const auto boxB = workphone::dynamic_pointer_cast<IBoxShape2>( shapeB );
        return boxA && boxB && orientedBoxesIntersect( bodyA, *boxA, bodyB, *boxB );
    }
} // namespace workphone::physics
