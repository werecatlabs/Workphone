#include "WPPhysics/WPPhysicsPCH.hpp"
#include "WPPhysics/Particle2.hpp"
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>

namespace workphone::physics
{
    u32 Particle2::m_nextId = 0;

    Particle2::Particle2( IPhysicsManager2D *creator ) :
        m_creator( creator ),
        m_position( Vector2<real_Num>::ZERO ),
        m_velocity( Vector2<real_Num>::ZERO ),
        m_maxVelocity( Vector2<real_Num>( 1e10, 1e10 ) ),
        m_targetPosition( Vector2<real_Num>::ZERO ),
        m_force( Vector2<real_Num>::ZERO ),
        m_gravity( Vector2<real_Num>( 0, -9.81 ) ),

        m_userData( nullptr ),
        m_restitution( 0.0f ),
        m_flags( FPF_ENABLE | FPF_ENABLECOLLISION ),
        m_id( 0 ),
        m_objectType( 0 ),
        m_worldId( 0 ),
        m_collisionType( 0xFFFFFFFFu ),
        m_mask( 0xFFFFFFFFu ),

        m_kinematicMode( false )
    {
        m_id = m_nextId++;

        // PlatformManagerPtr& platformManager = Engine::getSingletonPtr()->getPlatformManager();
        // m_stateContext = platformManager->createStateObject();
    }

    Particle2::~Particle2()
    {
        for( auto &effect : m_effects )
        {
            if( auto bodyEffect = workphone::dynamic_pointer_cast<IPhysicsBodyEffect2>( effect ) )
            {
                if( bodyEffect->getOwner() == this )
                {
                    bodyEffect->setOwner( nullptr );
                }
            }
        }
        m_effects.clear();
    }

    const String &Particle2::getComponentType() const
    {
        static const String componentType = "Particle2";
        return componentType;
    }

    u32 Particle2::getComponentTypeId() const
    {
        return static_cast<u32>( StringUtil::getHash( getComponentType() ) );
    }

    void Particle2::update( const s32 &task, const time_interval &t, const time_interval &dt )
    {
        const auto elapsedTime = static_cast<real_Num>( dt );
        if( !isEnabled() || m_sleeping || !Math<real_Num>::isFinite( elapsedTime ) ||
            elapsedTime <= static_cast<real_Num>( 0 ) )
        {
            return;
        }

        if( m_kinematicMode )
        {
            m_position = m_targetPosition;
            m_force = Vector2<real_Num>::ZERO;
            m_torque = static_cast<real_Num>( 0 );
            return;
        }

        auto acceleration =
            m_mass > Math<real_Num>::epsilon() ? m_force / m_mass : Vector2<real_Num>::ZERO;
        if( getEnableGravity() )
        {
            acceleration += m_gravity;
        }
        m_velocity += acceleration * elapsedTime;

        const auto linearDamping =
            static_cast<real_Num>( 1 ) /
            ( static_cast<real_Num>( 1 ) + ( m_linearDamping + m_airResistance ) * elapsedTime );
        m_velocity *= linearDamping;
        m_velocity.X() = std::clamp( m_velocity.X(), -m_maxVelocity.X(), m_maxVelocity.X() );
        m_velocity.Y() = std::clamp( m_velocity.Y(), -m_maxVelocity.Y(), m_maxVelocity.Y() );
        m_position += m_velocity * elapsedTime;

        if( m_mass > Math<real_Num>::epsilon() )
        {
            m_angularVelocity += ( m_torque / m_mass ) * elapsedTime;
        }
        m_angularVelocity /= static_cast<real_Num>( 1 ) + m_angularDamping * elapsedTime;
        m_orientation += m_angularVelocity * elapsedTime;

        if( m_constraintAABB.isValid() )
        {
            const auto worldBounds = getWorldAABB();
            auto       correction = Vector2<real_Num>::ZERO;
            if( worldBounds.getMin().X() < m_constraintAABB.getMin().X() )
            {
                correction.X() = m_constraintAABB.getMin().X() - worldBounds.getMin().X();
            }
            else if( worldBounds.getMax().X() > m_constraintAABB.getMax().X() )
            {
                correction.X() = m_constraintAABB.getMax().X() - worldBounds.getMax().X();
            }
            if( worldBounds.getMin().Y() < m_constraintAABB.getMin().Y() )
            {
                correction.Y() = m_constraintAABB.getMin().Y() - worldBounds.getMin().Y();
            }
            else if( worldBounds.getMax().Y() > m_constraintAABB.getMax().Y() )
            {
                correction.Y() = m_constraintAABB.getMax().Y() - worldBounds.getMax().Y();
            }
            m_position += correction;
        }

        m_force = Vector2<real_Num>::ZERO;
        m_torque = static_cast<real_Num>( 0 );

        // StatePhysicsPosition2Ptr message( new StatePhysicsPosition2( m_position ) );
        // m_stateContext->addMessage(Thread::TASK_ID_APPLICATION_LOGIC, message);
    }

    u32 Particle2::getId() const
    {
        return m_id;
    }

    void Particle2::addVelocity( const Vector2<real_Num> &velocity, const Vector2<real_Num> &relPos )
    {
        m_velocity += velocity;
        const auto radiusSquared = relPos.lengthSquared();
        if( radiusSquared > Math<real_Num>::epsilon() )
        {
            m_angularVelocity +=
                ( relPos.X() * velocity.Y() - relPos.Y() * velocity.X() ) / radiusSquared;
        }
        m_sleeping = false;
    }

    void Particle2::_addVector( const Vector2<real_Num> &vector )
    {
        m_position = m_position + vector;
    }

    void Particle2::setPosition( const Vector2<real_Num> &position )
    {
        // m_state = BodyState2(position, m_velocity, 0.0f);
        m_position = position;
    }

    Vector2<real_Num> Particle2::getPosition() const
    {
        return m_position;
    }

    void Particle2::setRelativePosition( const Vector2<real_Num> &position )
    {
        m_position = position;
    }

    Vector2<real_Num> Particle2::getRelativePosition() const
    {
        return m_position;
    }

    void Particle2::setVelocity( const Vector2<real_Num> &velocity )
    {
        m_velocity = velocity;
        m_sleeping = false;
    }

    Vector2<real_Num> Particle2::getVelocity() const
    {
        return m_velocity;
    }

    void Particle2::_setVelocity( const Vector2<real_Num> &velocity )
    {
        m_velocity = velocity;
        m_sleeping = false;
    }

    void Particle2::setMaxVelocity( const Vector2<real_Num> &velocity )
    {
        m_maxVelocity = Vector2<real_Num>( Math<real_Num>::Abs( velocity.X() ),
                                           Math<real_Num>::Abs( velocity.Y() ) );
    }

    Vector2<real_Num> Particle2::getMaxVelocity() const
    {
        return m_maxVelocity;
    }

    void Particle2::setLinearDampValue( real_Num linearDampValue )
    {
        m_linearDamping = Math<real_Num>::max( linearDampValue, static_cast<real_Num>( 0 ) );
    }

    real_Num Particle2::getLinearDampValue() const
    {
        return m_linearDamping;
    }

    void Particle2::setAngularDampValue( real_Num angularDampValue )
    {
        m_angularDamping = Math<real_Num>::max( angularDampValue, static_cast<real_Num>( 0 ) );
    }

    real_Num Particle2::getAngularDampValue() const
    {
        return m_angularDamping;
    }

    void Particle2::setCollisionShape( SmartPtr<IPhysicsShape2> shape )
    {
        m_shape = shape;
    }

    const SmartPtr<IPhysicsShape2> &Particle2::getCollisionShape() const
    {
        return m_shape;
    }

    Transform2<real_Num> Particle2::_getTransformState() const
    {
        return Transform2<real_Num>( m_position, 0.0 );
    }

    void Particle2::setFlag( u32 flag, bool value )
    {
        u32 flags = m_flags;
        if( value )
            flags |= flag;
        else
            flags &= ~flag;

        m_flags = flags;

        if( flag == FPF_ENABLE && m_creator )
        {
            m_creator->OnChangeFlags( this );
        }
    }

    u32 Particle2::getBodyType() const
    {
        return 0;
    }

    void Particle2::setObjectType( hash_type type )
    {
        m_objectType = type;
    }

    hash_type Particle2::getObjectType() const
    {
        return m_objectType;
    }

    void Particle2::setWorldId( hash_type worldId )
    {
        m_worldId = worldId;
    }

    hash_type Particle2::getWorldId() const
    {
        return m_worldId;
    }

    void Particle2::setEnabled( bool enabled )
    {
        setFlag( FPF_ENABLE, enabled );
    }

    bool Particle2::isEnabled() const
    {
        return getFlag( FPF_ENABLE );
    }

    AABB2<real_Num> Particle2::getLocalAABB() const
    {
        return m_shape ? m_shape->getAABB() : AABB2<real_Num>();
    }

    AABB2<real_Num> Particle2::getWorldAABB() const
    {
        return getLocalAABB() + m_position;
    }

    void Particle2::setMaterialId( hash_type materialId )
    {
        m_materialId = materialId;
    }

    hash_type Particle2::getMaterialId() const
    {
        return m_materialId;
    }

    void Particle2::setUserData( void *userData )
    {
        m_userData = userData;
    }

    void *Particle2::getUserData() const
    {
        return m_userData;
    }

    void Particle2::setTargetPosition( const Vector2<real_Num> &position )
    {
        m_targetPosition = position;
    }

    Vector2<real_Num> Particle2::getTargetPosition() const
    {
        return m_targetPosition;
    }

    void Particle2::setRestitution( real_Num restitution )
    {
        m_restitution =
            std::clamp( restitution, static_cast<real_Num>( 0 ), static_cast<real_Num>( 1 ) );
    }

    real_Num Particle2::getRestitution() const
    {
        return m_restitution;
    }

    void Particle2::removeEffect( SmartPtr<IPhysicsEffect2> effect )
    {
        if( auto bodyEffect = workphone::dynamic_pointer_cast<IPhysicsBodyEffect2>( effect ) )
        {
            if( bodyEffect->getOwner() == this )
            {
                bodyEffect->setOwner( nullptr );
            }
        }
        m_effects.erase( std::remove( m_effects.begin(), m_effects.end(), effect ), m_effects.end() );
    }

    void Particle2::addEffect( SmartPtr<IPhysicsEffect2> effect )
    {
        if( effect && std::find( m_effects.begin(), m_effects.end(), effect ) == m_effects.end() )
        {
            m_effects.push_back( effect );
            if( auto bodyEffect = workphone::dynamic_pointer_cast<IPhysicsBodyEffect2>( effect ) )
            {
                bodyEffect->setOwner( this );
            }
        }
    }

    void Particle2::setGravity( const Vector2<real_Num> &gravity )
    {
        m_gravity = gravity;
    }

    workphone::Vector2<workphone::real_Num> Particle2::getGravity() const
    {
        return m_gravity;
    }

    bool Particle2::getEnableGravity() const
    {
        return CPhysicsBody2<IPhysicsParticle2>::getEnableGravity();
    }

    void Particle2::setEnableGravity( bool enableGravity )
    {
        CPhysicsBody2<IPhysicsParticle2>::setEnableGravity( enableGravity );
    }

    void Particle2::setKinematicMode( bool kinematicMode )
    {
        m_kinematicMode = kinematicMode;
    }

    bool Particle2::getKinematicMode() const
    {
        return m_kinematicMode;
    }

    const workphone::SmartPtr<workphone::IStateContext> &Particle2::getStateContext() const
    {
        return m_stateContext;
    }

    workphone::SmartPtr<workphone::IStateContext> &Particle2::getStateContext()
    {
        return m_stateContext;
    }

    bool Particle2::isSleeping() const
    {
        return m_sleeping;
    }

    void Particle2::setSleep( bool sleep )
    {
        m_sleeping = sleep;
        if( sleep )
        {
            m_velocity = Vector2<real_Num>::ZERO;
            m_angularVelocity = static_cast<real_Num>( 0 );
            m_force = Vector2<real_Num>::ZERO;
            m_torque = static_cast<real_Num>( 0 );
        }
    }

    workphone::AABB2<workphone::real_Num> Particle2::getContraintAABB() const
    {
        return m_constraintAABB;
    }

    void Particle2::setContraintAABB( const AABB2<real_Num> &contraintRect )
    {
        m_constraintAABB = contraintRect;
        m_constraintAABB.repair();
    }

    u32 Particle2::getCollisionType() const
    {
        return m_collisionType;
    }

    void Particle2::setCollisionType( u32 mask )
    {
        m_collisionType = mask;
    }

    workphone::real_Num Particle2::getMassInv() const
    {
        return m_mass > Math<real_Num>::epsilon() ? static_cast<real_Num>( 1 ) / m_mass
                                                  : static_cast<real_Num>( 0 );
    }

    workphone::real_Num Particle2::getMass() const
    {
        return m_mass;
    }

    void Particle2::setMass( real_Num mass )
    {
        m_mass = Math<real_Num>::max( mass, static_cast<real_Num>( 0 ) );
    }

    bool Particle2::getFlag( u32 flag ) const
    {
        return ( m_flags & flag ) != 0;
    }

    workphone::real_Num Particle2::getAirResistance() const
    {
        return m_airResistance;
    }

    void Particle2::setAirResistance( real_Num airResistance )
    {
        m_airResistance = Math<real_Num>::max( airResistance, static_cast<real_Num>( 0 ) );
    }

    workphone::real_Num Particle2::getTorque() const
    {
        return m_torque;
    }

    void Particle2::setTorque( real_Num torque )
    {
        m_torque = torque;
        m_sleeping = false;
    }

    void Particle2::addTorque( real_Num torque )
    {
        m_torque += torque;
        m_sleeping = false;
    }

    workphone::Vector2<workphone::real_Num> Particle2::getForce() const
    {
        return m_force;
    }

    void Particle2::setForce( const Vector2<real_Num> &force )
    {
        m_force = force;
        m_sleeping = false;
    }

    void Particle2::addForce( const Vector2<real_Num> &force )
    {
        m_force += force;
        m_sleeping = false;
    }

    workphone::real_Num Particle2::getAngularVelocity() const
    {
        return m_angularVelocity;
    }

    workphone::real_Num Particle2::getOrientation() const
    {
        return m_orientation;
    }

    void Particle2::setOrientation( real_Num orientation )
    {
        m_orientation = orientation;
    }

} // namespace workphone::physics
