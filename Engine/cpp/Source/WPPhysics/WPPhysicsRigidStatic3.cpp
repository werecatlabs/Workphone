#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/WPPhysicsUtil.hpp>
#include <WPPhysics/WPPhysicsShape3T.hpp>
#include <Workphone/Interface/Physics/ISphereShape3.hpp>
#include <limits>
#include <WPPhysics/WPPhysicsRigidStatic3.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>

namespace workphone::physics
{
    namespace
    {
        template <class T>
        bool hasFlag( T flags, T flag )
        {
            return ( static_cast<u32>( flags ) & static_cast<u32>( flag ) ) != 0;
        }

        template <class T>
        void updateFlags( T &flags, T mask, bool value )
        {
            auto       bits = static_cast<u32>( flags );
            const auto maskBits = static_cast<u32>( mask );
            bits = value ? bits | maskBits : bits & ~maskBits;
            flags = static_cast<T>( bits );
        }
    } // namespace

    WPPhysicsRigidStatic3::WPPhysicsRigidStatic3() : m_body( wp_rigidbody_create( WORKPHONE_RIGIDBODY_STATIC ) )
    {
        if( !m_body )
        {
            throw std::runtime_error( "Failed to create WPPhysics static rigid body." );
        }
    }

    WPPhysicsRigidStatic3::~WPPhysicsRigidStatic3()
    {
        for( auto &shape : m_shapes )
        {
            if( shape )
            {
                shape->setActor( nullptr );
            }
        }
        m_shapes.clear();
        wp_rigidbody_destroy( m_body );
        m_body = nullptr;
    }

    SmartPtr<IPhysicsScene3> WPPhysicsRigidStatic3::getScene() const
    {
        return m_scene.lock();
    }

    void WPPhysicsRigidStatic3::setScene( SmartPtr<IPhysicsScene3> scene )
    {
        m_scene = scene;
    }

    void WPPhysicsRigidStatic3::setTransform( const Transform3<real_Num> &transform )
    {
        wp_rigidbody_set_position( m_body, detail::toWp( transform.getPosition() ) );
        wp_rigidbody_set_orientation( m_body, detail::toWp( transform.getOrientation() ) );
    }

    Transform3<real_Num> WPPhysicsRigidStatic3::getTransform() const
    {
        return Transform3<real_Num>( detail::fromWp( wp_rigidbody_get_position( m_body ) ),
                                     detail::fromWp( wp_rigidbody_get_orientation( m_body ) ) );
    }

    void WPPhysicsRigidStatic3::setActorFlag( ActorFlagEnum flag, bool value )
    {
        updateFlags( m_actorFlags, flag, value );

        if( hasFlag( flag, ActorFlagEnum::eDISABLE_GRAVITY ) )
        {
            wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_GRAVITY,
                                   !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_GRAVITY ) );
        }
        if( hasFlag( flag, ActorFlagEnum::eSEND_SLEEP_NOTIFIES ) )
        {
            wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_SLEEP_NOTIFY,
                                   hasFlag( m_actorFlags, ActorFlagEnum::eSEND_SLEEP_NOTIFIES ) );
        }
        if( hasFlag( flag, ActorFlagEnum::eDISABLE_SIMULATION ) )
        {
            wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_ENABLED,
                                   m_enabled &&
                                       !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_SIMULATION ) );
        }
    }

    ActorFlagEnum WPPhysicsRigidStatic3::getActorFlags() const
    {
        return m_actorFlags;
    }

    real_Num WPPhysicsRigidStatic3::getMass() const
    {
        return wp_rigidbody_get_mass( m_body );
    }

    void WPPhysicsRigidStatic3::setMass( real_Num mass )
    {
        wp_rigidbody_set_mass(
            m_body, static_cast<wp_f32>( Math<real_Num>::max( mass, static_cast<real_Num>( 0 ) ) ) );
    }

    void WPPhysicsRigidStatic3::setCollisionType( u32 type )
    {
        wp_rigidbody_set_collision_type( m_body, type );
    }

    u32 WPPhysicsRigidStatic3::getCollisionType() const
    {
        return wp_rigidbody_get_collision_type( m_body );
    }

    void WPPhysicsRigidStatic3::setCollisionMask( u32 mask )
    {
        wp_rigidbody_set_collision_mask( m_body, mask );
    }

    u32 WPPhysicsRigidStatic3::getCollisionMask() const
    {
        return wp_rigidbody_get_collision_mask( m_body );
    }

    void WPPhysicsRigidStatic3::setEnabled( bool enabled )
    {
        m_enabled = enabled;
        wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_ENABLED,
                               m_enabled &&
                                   !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_SIMULATION ) );
    }

    bool WPPhysicsRigidStatic3::isEnabled() const
    {
        return m_enabled;
    }

    void *WPPhysicsRigidStatic3::getUserDataById( u32 id ) const
    {
        return id < 4 ? m_userDataById[id] : nullptr;
    }

    void WPPhysicsRigidStatic3::setUserDataById( u32 id, void *userData )
    {
        if( id < 4 )
        {
            m_userDataById[id] = userData;
        }
    }

    void *WPPhysicsRigidStatic3::getUserData() const
    {
        return wp_rigidbody_get_user_data( m_body );
    }

    void WPPhysicsRigidStatic3::setUserData( void *userData )
    {
        wp_rigidbody_set_user_data( m_body, userData );
    }

    bool WPPhysicsRigidStatic3::getKinematicMode() const
    {
        return false;
    }

    void WPPhysicsRigidStatic3::setKinematicMode( bool )
    {
        // Static bodies cannot enter kinematic mode.
    }

    SmartPtr<IPhysicsBody3> WPPhysicsRigidStatic3::clone()
    {
        auto clone = workphone::make_ptr<WPPhysicsRigidStatic3>();
        clone->setTransform( getTransform() );
        clone->setMass( getMass() );
        clone->setActorFlag( getActorFlags(), true );
        clone->setRigidBodyFlag( getRigidBodyFlags(), true );
        clone->setCollisionType( getCollisionType() );
        clone->setCollisionMask( getCollisionMask() );
        clone->setEnabled( isEnabled() );
        clone->setMassSpaceInertiaTensor( getMassSpaceInertiaTensor() );
        for( auto &shape : m_shapes )
        {
            if( shape )
            {
                clone->addShape( shape->clone() );
            }
        }
        return clone;
    }

    void WPPhysicsRigidStatic3::wakeUp()
    {
        // Static bodies do not sleep.
    }

    SmartPtr<IStateContext> WPPhysicsRigidStatic3::getStateContext() const
    {
        return m_stateContext;
    }

    void WPPhysicsRigidStatic3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }

    void WPPhysicsRigidStatic3::_getObject( void **object ) const
    {
        if( object )
        {
            *object = m_body;
        }
    }

    void WPPhysicsRigidStatic3::setRigidBodyFlag( RigidBodyFlagEnum flag, bool value )
    {
        // Static actors retain the API-visible flags for serialization and
        // cloning, but dynamic-only flags must not change their native type.
        updateFlags( m_rigidBodyFlags, flag, value );
    }

    RigidBodyFlagEnum WPPhysicsRigidStatic3::getRigidBodyFlags() const
    {
        return m_rigidBodyFlags;
    }

    void WPPhysicsRigidStatic3::addShape( SmartPtr<IPhysicsShape3> shape )
    {
        if( !shape || std::find( m_shapes.begin(), m_shapes.end(), shape ) != m_shapes.end() )
        {
            return;
        }

        if( auto actor = shape->getActor() )
        {
            if( actor.get() != this )
            {
                WP_LOG_WARNING( "WPPhysicsRigidStatic3::addShape: shape is already attached to another actor." );
                return;
            }
        }

        void *rawShape = nullptr;
        shape->_getObject( &rawShape );
        if( rawShape &&
            wp_rigidbody_add_shape( m_body, static_cast<wp_collision_shape *>( rawShape ) ) >= 0 )
        {
            shape->setActor( getSharedFromThis<WPPhysicsRigidStatic3>() );
            m_shapes.push_back( shape );
        }
    }

    void WPPhysicsRigidStatic3::removeShape( SmartPtr<IPhysicsShape3> shape, bool )
    {
        auto it = std::find( m_shapes.begin(), m_shapes.end(), shape );
        if( it == m_shapes.end() )
        {
            return;
        }

        const auto index = static_cast<wp_s32>( std::distance( m_shapes.begin(), it ) );
        wp_rigidbody_remove_shape( m_body, index );
        ( *it )->setActor( nullptr );
        m_shapes.erase( it );
    }

    Array<SmartPtr<IPhysicsShape3>> WPPhysicsRigidStatic3::getShapes() const
    {
        return m_shapes;
    }

    u32 WPPhysicsRigidStatic3::getNumShapes() const
    {
        return static_cast<u32>( m_shapes.size() );
    }

    void WPPhysicsRigidStatic3::setMassSpaceInertiaTensor( const Vector3<real_Num> &inertia )
    {
        wp_rigidbody_set_inertia_tensor( m_body, detail::toWp( inertia ) );
    }

    Vector3<real_Num> WPPhysicsRigidStatic3::getMassSpaceInertiaTensor() const
    {
        return detail::fromWp( wp_rigidbody_get_inertia_tensor( m_body ) );
    }

    Vector3<real_Num> WPPhysicsRigidStatic3::getMassSpaceInvInertiaTensor() const
    {
        const auto inertia = getMassSpaceInertiaTensor();
        return Vector3<real_Num>( inertia.X() != 0 ? 1 / inertia.X() : 0,
                                  inertia.Y() != 0 ? 1 / inertia.Y() : 0,
                                  inertia.Z() != 0 ? 1 / inertia.Z() : 0 );
    }

    AABB3<real_Num> WPPhysicsRigidStatic3::getAABB() const
    {
        return getLocalAABB();
    }

    void WPPhysicsRigidStatic3::setAABB( const AABB3<real_Num> &bounds )
    {
        for( const auto &shape : m_shapes )
        {
            if( auto backend = dynamic_cast<WPPhysicsShape3Backend *>( shape.get() ) )
            {
                backend->setAABB( bounds );
            }
        }
    }

    void WPPhysicsRigidStatic3::setRadius( real_Num radius )
    {
        for( const auto &shape : m_shapes )
        {
            if( auto sphere = dynamic_cast<ISphereShape3 *>( shape.get() ) )
            {
                sphere->setRadius( radius );
            }
        }
    }

    real_Num WPPhysicsRigidStatic3::getRadius() const
    {
        const auto bounds = getLocalAABB();
        if( bounds.isNull() ) return static_cast<real_Num>( 0 );
        if( bounds.isInfinite() ) return std::numeric_limits<real_Num>::infinity();
        return bounds.getExtent().length() * static_cast<real_Num>( 0.5 );
    }
    AABB3<real_Num> WPPhysicsRigidStatic3::getLocalAABB() const
    {
        return WPPhysicsUtil::mergeShapeBounds( m_shapes );
    }

    AABB3<real_Num> WPPhysicsRigidStatic3::getWorldAABB() const
    {
        return WPPhysicsUtil::transformBounds( getLocalAABB(), getTransform() );
    }

    wp_rigidbody *WPPhysicsRigidStatic3::getBody() const
    {
        return m_body;
    }
} // namespace workphone::physics
