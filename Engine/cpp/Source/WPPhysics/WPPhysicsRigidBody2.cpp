#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsRigidBody2.hpp>
#include <WPPhysics/WPPhysicsShape2T.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <cmath>
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

        wp_collision_shape *getNativeShape( const SmartPtr<IPhysicsShape2> &shape )
        {
            WP_ASSERT( shape );
            if( !shape )
            {
                return nullptr;
            }

            wp_collision_shape *nativeShape = nullptr;
            if( auto box = dynamic_cast<WPPhysicsShape2T<BoxShape2> *>( shape.get() ) )
            {
                nativeShape = box->getShape();
            }
            else if( auto sphere = dynamic_cast<WPPhysicsShape2T<SphereShape2> *>( shape.get() ) )
            {
                nativeShape = sphere->getShape();
            }
            if( !nativeShape )
            {
                return nullptr;
            }

            WP_ASSERT( nativeShape );
            return nativeShape;
        }
    } // namespace

    WPPhysicsRigidBody2::WPPhysicsRigidBody2( wp_rigidbody_type type ) :
        m_body( wp_rigidbody2_create( type ) )
    {
        if( !m_body )
        {
            throw std::runtime_error( "Failed to create a WPPhysics 2D rigid body." );
        }
        m_targetPosition = getPosition();
        m_maxVelocity =
            Vector2<real_Num>( static_cast<real_Num>( 100000 ), static_cast<real_Num>( 100000 ) );
        m_flags = IRigidBody2::RBF_ENABLE | IRigidBody2::RBF_ENABLEPHYSICS |
                  IRigidBody2::RBF_ENABLE_COLLISION | IRigidBody2::RBF_ALLOWROTATION;
    }

    WPPhysicsRigidBody2::~WPPhysicsRigidBody2()
    {
        wp_rigidbody_destroy( m_body );
        m_body = nullptr;
    }

    void *WPPhysicsRigidBody2::getNativeObject() const
    {
        WP_ASSERT( m_body );
        return m_body;
    }

    wp_rigidbody *WPPhysicsRigidBody2::getBody() const
    {
        WP_ASSERT( m_body );
        return m_body;
    }

    void WPPhysicsRigidBody2::setCollisionShape( const SmartPtr<IPhysicsShape2> &shape )
    {
        WP_ASSERT( m_body );
        if( m_collisionShape )
        {
            wp_rigidbody_remove_shape( m_body, 0 );
        }

        m_collisionShape = shape;
        if( !shape )
        {
            return;
        }
        if( auto nativeShape = getNativeShape( shape ) )
        {
            const auto result = wp_rigidbody_add_shape( m_body, nativeShape );
            WP_ASSERT( result >= 0 );
            WP_ASSERT( wp_rigidbody_get_shape_count( m_body ) > 0 );
        }
    }

    const SmartPtr<IPhysicsShape2> &WPPhysicsRigidBody2::getCollisionShape() const
    {
        return m_collisionShape;
    }
    void WPPhysicsRigidBody2::setPosition( const Vector2<real_Num> &position )
    {
        wp_rigidbody2_set_position( getBody(), toWp( position ) );
        WP_ASSERT( getPosition() == position );
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getPosition() const
    {
        return fromWp( wp_rigidbody2_get_position( getBody() ) );
    }
    void WPPhysicsRigidBody2::setTargetPosition( const Vector2<real_Num> &position )
    {
        m_targetPosition = position;
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getTargetPosition() const
    {
        return m_targetPosition;
    }
    void WPPhysicsRigidBody2::setOrientation( real_Num orientation )
    {
        wp_rigidbody2_set_orientation( getBody(), static_cast<wp_f32>( orientation ) );
    }
    real_Num WPPhysicsRigidBody2::getOrientation() const
    {
        return wp_rigidbody2_get_orientation( getBody() );
    }
    real_Num WPPhysicsRigidBody2::getAngularVelocity() const
    {
        return wp_rigidbody2_get_angular_velocity( getBody() );
    }
    void WPPhysicsRigidBody2::addForce( const Vector2<real_Num> &force )
    {
        wp_rigidbody2_add_force( getBody(), toWp( force ) );
    }
    void WPPhysicsRigidBody2::setForce( const Vector2<real_Num> &force )
    {
        wp_rigidbody2_set_force( getBody(), toWp( force ) );
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getForce() const
    {
        return fromWp( wp_rigidbody2_get_force( getBody() ) );
    }
    void WPPhysicsRigidBody2::addTorque( real_Num torque )
    {
        wp_rigidbody2_add_torque( getBody(), static_cast<wp_f32>( torque ) );
    }
    void WPPhysicsRigidBody2::setTorque( real_Num torque )
    {
        wp_rigidbody2_set_torque( getBody(), static_cast<wp_f32>( torque ) );
    }
    real_Num WPPhysicsRigidBody2::getTorque() const
    {
        return wp_rigidbody2_get_torque( getBody() );
    }
    void WPPhysicsRigidBody2::addVelocity( const Vector2<real_Num> &velocity,
                                          const Vector2<real_Num> &relPos )
    {
        setVelocity( getVelocity() + velocity );
        const auto radiusSquared = relPos.lengthSquared();
        if( radiusSquared > Math<real_Num>::epsilon() )
        {
            const auto angularDelta =
                ( relPos.X() * velocity.Y() - relPos.Y() * velocity.X() ) / radiusSquared;
            wp_rigidbody2_set_angular_velocity(
                getBody(), static_cast<wp_f32>( getAngularVelocity() + angularDelta ) );
        }
    }
    void WPPhysicsRigidBody2::setVelocity( const Vector2<real_Num> &velocity )
    {
        wp_rigidbody2_set_linear_velocity( getBody(), toWp( velocity ) );
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getVelocity() const
    {
        return fromWp( wp_rigidbody2_get_linear_velocity( getBody() ) );
    }
    void WPPhysicsRigidBody2::setMaxVelocity( const Vector2<real_Num> &velocity )
    {
        m_maxVelocity = Vector2<real_Num>( Math<real_Num>::Abs( velocity.X() ),
                                           Math<real_Num>::Abs( velocity.Y() ) );
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getMaxVelocity() const
    {
        return m_maxVelocity;
    }
    void WPPhysicsRigidBody2::setLinearDampValue( real_Num linearDampValue )
    {
        WP_ASSERT( linearDampValue >= static_cast<real_Num>( 0 ) );
        m_linearDamping = std::max( linearDampValue, static_cast<real_Num>( 0 ) );
        wp_rigidbody_set_linear_damping( getBody(),
                                         static_cast<wp_f32>( m_linearDamping + m_airResistance ) );
    }
    real_Num WPPhysicsRigidBody2::getLinearDampValue() const
    {
        return m_linearDamping;
    }
    void WPPhysicsRigidBody2::setAngularDampValue( real_Num angularDampValue )
    {
        WP_ASSERT( angularDampValue >= static_cast<real_Num>( 0 ) );
        wp_rigidbody_set_angular_damping(
            getBody(), static_cast<wp_f32>( std::max( angularDampValue, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num WPPhysicsRigidBody2::getAngularDampValue() const
    {
        return wp_rigidbody_get_angular_damping( getBody() );
    }
    void WPPhysicsRigidBody2::setAirResistance( real_Num airResistance )
    {
        m_airResistance = std::max( airResistance, static_cast<real_Num>( 0 ) );
        wp_rigidbody_set_linear_damping( getBody(),
                                         static_cast<wp_f32>( m_linearDamping + m_airResistance ) );
    }
    real_Num WPPhysicsRigidBody2::getAirResistance() const
    {
        return m_airResistance;
    }
    void WPPhysicsRigidBody2::setRestitution( real_Num restitution )
    {
        m_restitution =
            std::clamp( restitution, static_cast<real_Num>( 0 ), static_cast<real_Num>( 1 ) );
        wp_rigidbody_set_restitution( getBody(), static_cast<wp_f32>( m_restitution ) );
    }
    real_Num WPPhysicsRigidBody2::getRestitution() const
    {
        return static_cast<real_Num>( wp_rigidbody_get_restitution( getBody() ) );
    }
    void WPPhysicsRigidBody2::setMass( real_Num mass )
    {
        WP_ASSERT( mass >= static_cast<real_Num>( 0 ) );
        wp_rigidbody_set_mass( getBody(),
                               static_cast<wp_f32>( std::max( mass, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num WPPhysicsRigidBody2::getMass() const
    {
        return wp_rigidbody_get_mass( getBody() );
    }
    real_Num WPPhysicsRigidBody2::getMassInv() const
    {
        const auto mass = getMass();
        return mass > static_cast<real_Num>( 0 ) ? static_cast<real_Num>( 1 ) / mass
                                                 : static_cast<real_Num>( 0 );
    }
    void WPPhysicsRigidBody2::setFlag( u32 flag, bool value )
    {
        if( value )
        {
            m_flags |= flag;
        }
        else
        {
            m_flags &= ~flag;
        }

        if( flag == IRigidBody2::RBF_ENABLE || flag == IRigidBody2::RBF_ENABLEPHYSICS )
        {
            setEnabled( value );
        }
        else if( flag == IRigidBody2::RBF_ENABLE_COLLISION && m_collisionShape )
        {
            m_collisionShape->setEnabled( value );
        }
    }
    bool WPPhysicsRigidBody2::getFlag( u32 flag ) const
    {
        return ( m_flags & flag ) != 0;
    }
    u32 WPPhysicsRigidBody2::getBodyType() const
    {
        return PBT_RIGID;
    }
    void WPPhysicsRigidBody2::setObjectType( hash_type type )
    {
        m_objectType = type;
    }
    hash_type WPPhysicsRigidBody2::getObjectType() const
    {
        return m_objectType;
    }
    void WPPhysicsRigidBody2::setWorldId( hash_type worldId )
    {
        m_worldId = worldId;
    }
    hash_type WPPhysicsRigidBody2::getWorldId() const
    {
        return m_worldId;
    }
    void WPPhysicsRigidBody2::setEnabled( bool enabled )
    {
        wp_rigidbody_set_flag( getBody(), WORKPHONE_RIGIDBODY_FLAG_ENABLED, enabled );
        if( enabled )
        {
            m_flags |= IRigidBody2::RBF_ENABLE;
        }
        else
        {
            m_flags &= ~IRigidBody2::RBF_ENABLE;
        }
    }
    bool WPPhysicsRigidBody2::isEnabled() const
    {
        return wp_rigidbody_has_flag( getBody(), WORKPHONE_RIGIDBODY_FLAG_ENABLED ) != 0;
    }
    AABB2<real_Num> WPPhysicsRigidBody2::getLocalAABB() const
    {
        return m_collisionShape ? m_collisionShape->getAABB() : AABB2<real_Num>();
    }
    AABB2<real_Num> WPPhysicsRigidBody2::getWorldAABB() const
    {
        const auto local = getLocalAABB();
        const auto center = local.getCenter();
        const auto halfSize = local.getHalfSize();
        const auto angle = getOrientation();
        const auto cosine = Math<real_Num>::Abs( static_cast<real_Num>( std::cos( angle ) ) );
        const auto sine = Math<real_Num>::Abs( static_cast<real_Num>( std::sin( angle ) ) );
        const auto worldHalfSize = Vector2<real_Num>( cosine * halfSize.X() + sine * halfSize.Y(),
                                                      sine * halfSize.X() + cosine * halfSize.Y() );
        const auto rotatedCenter =
            Vector2<real_Num>( static_cast<real_Num>( std::cos( angle ) ) * center.X() -
                                   static_cast<real_Num>( std::sin( angle ) ) * center.Y(),
                               static_cast<real_Num>( std::sin( angle ) ) * center.X() +
                                   static_cast<real_Num>( std::cos( angle ) ) * center.Y() ) +
            getPosition();
        return AABB2<real_Num>( rotatedCenter - worldHalfSize, rotatedCenter + worldHalfSize );
    }
    void WPPhysicsRigidBody2::setMaterialId( hash_type materialId )
    {
        m_materialId = materialId;
    }
    hash_type WPPhysicsRigidBody2::getMaterialId() const
    {
        return m_materialId;
    }
    void WPPhysicsRigidBody2::setCollisionType( u32 mask )
    {
        wp_rigidbody_set_collision_type( getBody(), mask );
    }
    u32 WPPhysicsRigidBody2::getCollisionType() const
    {
        return wp_rigidbody_get_collision_type( getBody() );
    }
    void WPPhysicsRigidBody2::setCollisionMask( u32 mask )
    {
        wp_rigidbody_set_collision_mask( getBody(), mask );
    }
    u32 WPPhysicsRigidBody2::getCollisionMask() const
    {
        return wp_rigidbody_get_collision_mask( getBody() );
    }
    void WPPhysicsRigidBody2::setSleep( bool sleep )
    {
        sleep ? wp_rigidbody_put_to_sleep( getBody() ) : wp_rigidbody_wake_up( getBody() );
    }
    bool WPPhysicsRigidBody2::isSleeping() const
    {
        return wp_rigidbody_is_sleeping( getBody() ) != 0;
    }
    void WPPhysicsRigidBody2::setContraintAABB( const AABB2<real_Num> &contraintRect )
    {
        m_constraintAABB = contraintRect;
    }
    AABB2<real_Num> WPPhysicsRigidBody2::getContraintAABB() const
    {
        return m_constraintAABB;
    }
    bool WPPhysicsRigidBody2::getKinematicMode() const
    {
        return m_kinematicMode;
    }
    void WPPhysicsRigidBody2::setKinematicMode( bool kinematicMode )
    {
        m_kinematicMode = kinematicMode;
        wp_rigidbody_set_type( getBody(), kinematicMode ? WORKPHONE_RIGIDBODY_KINEMATIC
                                                        : WORKPHONE_RIGIDBODY_DYNAMIC );
    }
    Vector2<real_Num> WPPhysicsRigidBody2::getGravity() const
    {
        const auto gravity = wp_rigidbody_get_gravity_override( getBody() );
        return Vector2<real_Num>( static_cast<real_Num>( gravity.x ),
                                  static_cast<real_Num>( gravity.y ) );
    }
    void WPPhysicsRigidBody2::setGravity( const Vector2<real_Num> &gravity )
    {
        m_gravity = gravity;
        const wp_vec3f gravity3 = { static_cast<wp_f32>( gravity.X() ),
                                    static_cast<wp_f32>( gravity.Y() ), 0.0f };
        wp_rigidbody_set_gravity_override( getBody(), gravity3 );
    }
    bool WPPhysicsRigidBody2::getEnableGravity() const
    {
        return getFlag( WORKPHONE_RIGIDBODY_FLAG_GRAVITY );
    }
    void WPPhysicsRigidBody2::setEnableGravity( bool enableGravity )
    {
        setFlag( WORKPHONE_RIGIDBODY_FLAG_GRAVITY, enableGravity );
    }

    Array<SmartPtr<IPhysicsConstraint2>> WPPhysicsRigidBody2::getConstraints() const
    {
        return m_constraints;
    }
    void WPPhysicsRigidBody2::removeConstraints()
    {
        m_constraints.clear();
    }
    void WPPhysicsRigidBody2::removeConstraint( SmartPtr<IPhysicsConstraint2> constraint )
    {
        m_constraints.erase( std::remove( m_constraints.begin(), m_constraints.end(), constraint ),
                             m_constraints.end() );
    }
    void WPPhysicsRigidBody2::addConstraint( SmartPtr<IPhysicsConstraint2> constraint )
    {
        WP_ASSERT( constraint );
        if( constraint &&
            std::find( m_constraints.begin(), m_constraints.end(), constraint ) == m_constraints.end() )
        {
            m_constraints.push_back( constraint );
        }
    }
} // namespace workphone::physics
