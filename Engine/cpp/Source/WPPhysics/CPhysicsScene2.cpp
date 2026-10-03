#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPhysicsScene2.hpp>
#include <WPPhysics/CPhysicsRigidBody2.hpp>
#include <WPPhysics/Particle2.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

namespace workphone::physics
{
    namespace
    {
        wp_vec2f toWp( const Vector2<real_Num> &v )
        {
            wp_vec2f r = { static_cast<wp_f32>( v.X() ), static_cast<wp_f32>( v.Y() ) };
            return r;
        }

        Vector2<real_Num> fromWp( wp_vec2f v )
        {
            return Vector2<real_Num>( static_cast<real_Num>( v.x ), static_cast<real_Num>( v.y ) );
        }

        wp_rigidbody *getNativeBody( const SmartPtr<IRigidBody2> &body )
        {
            WP_ASSERT( body );
            if( !body )
            {
                return nullptr;
            }

            auto nativeObject = dynamic_cast<INativePhysicsObject2 *>( body.get() );
            WP_ASSERT( nativeObject );
            if( !nativeObject )
            {
                return nullptr;
            }

            auto nativeBody = static_cast<wp_rigidbody *>( nativeObject->getNativeObject() );
            WP_ASSERT( nativeBody );
            return nativeBody;
        }
    } // namespace

    CPhysicsScene2::CPhysicsScene2() : m_scene( wp_physics_scene_create() )
    {
        if( !m_scene )
        {
            throw std::runtime_error( "Failed to create a WPPhysics 2D scene." );
        }
    }

    CPhysicsScene2::~CPhysicsScene2()
    {
        wp_physics_scene_clear( m_scene );
        m_particles.clear();
        m_bodies.clear();
        wp_physics_scene_destroy( m_scene );
        m_scene = nullptr;
    }

    void *CPhysicsScene2::getNativeObject() const
    {
        WP_ASSERT( m_scene );
        return m_scene;
    }

    wp_physics_scene *CPhysicsScene2::getScene() const
    {
        WP_ASSERT( m_scene );
        return m_scene;
    }

    void CPhysicsScene2::updateRigidBodies()
    {
        simulate( m_fixedTimeStep );
    }

    void CPhysicsScene2::simulate( real_Num elapsedTime )
    {
        if( !Math<real_Num>::isFinite( elapsedTime ) || elapsedTime <= static_cast<real_Num>( 0 ) )
        {
            return;
        }

        for( auto &body : m_bodies )
        {
            auto backendBody = dynamic_cast<CPhysicsRigidBody2 *>( body.get() );
            if( !backendBody )
            {
                continue;
            }

            if( backendBody->getKinematicMode() )
            {
                backendBody->setPosition( backendBody->getTargetPosition() );
            }
        }

        wp_physics_scene2_simulate( getScene(), static_cast<wp_f32>( elapsedTime ) );
        WP_ASSERT( wp_physics_scene_fetch_results( getScene(), 1 ) != 0 );

        for( auto &body : m_bodies )
        {
            auto backendBody = dynamic_cast<CPhysicsRigidBody2 *>( body.get() );
            if( !backendBody )
            {
                continue;
            }

            auto       velocity = backendBody->getVelocity();
            const auto maxVelocity = backendBody->getMaxVelocity();
            if( maxVelocity.X() > static_cast<real_Num>( 0 ) )
            {
                velocity.X() = std::clamp( velocity.X(), -maxVelocity.X(), maxVelocity.X() );
            }
            if( maxVelocity.Y() > static_cast<real_Num>( 0 ) )
            {
                velocity.Y() = std::clamp( velocity.Y(), -maxVelocity.Y(), maxVelocity.Y() );
            }
            backendBody->setVelocity( velocity );
        }
    }

    void CPhysicsScene2::updateParticles()
    {
        for( auto &particle : m_particles )
        {
            if( auto backendParticle = dynamic_cast<Particle2 *>( particle.get() ) )
            {
                backendParticle->update( 0, 0.0, m_fixedTimeStep );
            }
        }
    }

    void CPhysicsScene2::addRigidBody( SmartPtr<IRigidBody2> body )
    {
        WP_ASSERT( body );
        if( !body || std::find( m_bodies.begin(), m_bodies.end(), body ) != m_bodies.end() )
        {
            return;
        }

        if( auto nativeBody = getNativeBody( body ) )
        {
            const auto result = wp_physics_scene_add_actor( getScene(), nativeBody );
            WP_ASSERT( result != 0 );
            if( result != 0 )
            {
                m_bodies.push_back( body );
                WP_ASSERT( wp_physics_scene_get_actor_count( getScene() ) >=
                           static_cast<wp_s32>( m_bodies.size() ) );
            }
        }
    }

    void CPhysicsScene2::removeRigidBody( SmartPtr<IRigidBody2> body )
    {
        WP_ASSERT( body );
        if( auto nativeBody = getNativeBody( body ) )
        {
            wp_physics_scene_remove_actor( getScene(), nativeBody );
        }
        m_bodies.erase( std::remove( m_bodies.begin(), m_bodies.end(), body ), m_bodies.end() );
    }

    void CPhysicsScene2::addParticle( SmartPtr<IPhysicsParticle2> particle )
    {
        WP_ASSERT( particle );
        if( particle &&
            std::find( m_particles.begin(), m_particles.end(), particle ) == m_particles.end() )
        {
            m_particles.push_back( particle );
        }
    }

    void CPhysicsScene2::removeParticle( SmartPtr<IPhysicsParticle2> particle )
    {
        m_particles.erase( std::remove( m_particles.begin(), m_particles.end(), particle ),
                           m_particles.end() );
    }

    void CPhysicsScene2::setSize( const Vector2<real_Num> &size )
    {
        WP_ASSERT( size.X() >= static_cast<real_Num>( 0 ) );
        WP_ASSERT( size.Y() >= static_cast<real_Num>( 0 ) );
        wp_physics_scene2_set_size( getScene(), toWp( size ) );
    }

    Vector2<real_Num> CPhysicsScene2::getSize() const
    {
        return fromWp( wp_physics_scene2_get_size( getScene() ) );
    }

    void CPhysicsScene2::setGravity( const Vector2<real_Num> &gravity )
    {
        wp_physics_scene2_set_gravity( getScene(), toWp( gravity ) );
        WP_ASSERT( getGravity() == gravity );
    }

    Vector2<real_Num> CPhysicsScene2::getGravity() const
    {
        return fromWp( wp_physics_scene2_get_gravity( getScene() ) );
    }

    void CPhysicsScene2::setFixedTimeStep( real_Num fixedTimeStep )
    {
        if( Math<real_Num>::isFinite( fixedTimeStep ) && fixedTimeStep > static_cast<real_Num>( 0 ) )
        {
            m_fixedTimeStep = fixedTimeStep;
        }
    }

    real_Num CPhysicsScene2::getFixedTimeStep() const
    {
        return m_fixedTimeStep;
    }
} // namespace workphone::physics
