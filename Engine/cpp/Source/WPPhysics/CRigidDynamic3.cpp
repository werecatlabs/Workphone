#include <WPPhysics/WPPhysicsPCH.hpp>
#include <WPPhysics/CPhysicsBounds3.hpp>
#include <WPPhysics/CRigidDynamic3.hpp>
#include <Workphone/Workphone.hpp>
#include <algorithm>
#include <stdexcept>

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

    CRigidDynamic3::CRigidDynamic3( wp_rigidbody_type type ) : m_body( wp_rigidbody_create( type ) )
    {
        if( !m_body )
        {
            throw std::runtime_error( "Failed to create a WPPhysics dynamic rigid body." );
        }

        if( type == WORKPHONE_RIGIDBODY_KINEMATIC )
        {
            updateFlags( m_rigidBodyFlags, RigidBodyFlagEnum::eKINEMATIC, true );
        }
    }
    CRigidDynamic3::~CRigidDynamic3()
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
    SmartPtr<IPhysicsScene3> CRigidDynamic3::getScene() const
    {
        return m_scene.lock();
    }
    void CRigidDynamic3::setScene( SmartPtr<IPhysicsScene3> scene )
    {
        m_scene = scene;
    }
    void CRigidDynamic3::setTransform( const Transform3<real_Num> &transform )
    {
        wp_rigidbody_set_position( m_body, detail::toWp( transform.getPosition() ) );
        wp_rigidbody_set_orientation( m_body, detail::toWp( transform.getOrientation() ) );
        if( wp_rigidbody_get_type( m_body ) != WORKPHONE_RIGIDBODY_STATIC )
        {
            wp_rigidbody_wake_up( m_body );
        }
    }
    Transform3<real_Num> CRigidDynamic3::getTransform() const
    {
        return Transform3<real_Num>( detail::fromWp( wp_rigidbody_get_position( m_body ) ),
                                     detail::fromWp( wp_rigidbody_get_orientation( m_body ) ) );
    }
    void CRigidDynamic3::setActorFlag( ActorFlagEnum flag, bool value )
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
            const auto simulationEnabled =
                m_enabled && !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_SIMULATION );
            wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_ENABLED, simulationEnabled );
            if( simulationEnabled )
            {
                wakeUp();
            }
        }
    }
    ActorFlagEnum CRigidDynamic3::getActorFlags() const
    {
        return m_actorFlags;
    }
    real_Num CRigidDynamic3::getMass() const
    {
        return wp_rigidbody_get_mass( m_body );
    }
    void CRigidDynamic3::setMass( real_Num mass )
    {
        wp_rigidbody_set_mass(
            m_body, static_cast<wp_f32>( Math<real_Num>::max( mass, static_cast<real_Num>( 0 ) ) ) );
    }
    void CRigidDynamic3::setCollisionType( u32 type )
    {
        wp_rigidbody_set_collision_type( m_body, type );
    }
    u32 CRigidDynamic3::getCollisionType() const
    {
        return wp_rigidbody_get_collision_type( m_body );
    }
    void CRigidDynamic3::setCollisionMask( u32 mask )
    {
        wp_rigidbody_set_collision_mask( m_body, mask );
    }
    u32 CRigidDynamic3::getCollisionMask() const
    {
        return wp_rigidbody_get_collision_mask( m_body );
    }
    void CRigidDynamic3::setEnabled( bool enabled )
    {
        m_enabled = enabled;
        wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_ENABLED,
                               m_enabled &&
                                   !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_SIMULATION ) );
        if( m_enabled && !hasFlag( m_actorFlags, ActorFlagEnum::eDISABLE_SIMULATION ) )
        {
            wakeUp();
        }
    }
    bool CRigidDynamic3::isEnabled() const
    {
        return m_enabled;
    }
    void *CRigidDynamic3::getUserDataById( u32 id ) const
    {
        return id < 4 ? m_userDataById[id] : nullptr;
    }
    void CRigidDynamic3::setUserDataById( u32 id, void *userData )
    {
        if( id < 4 )
            m_userDataById[id] = userData;
    }
    void *CRigidDynamic3::getUserData() const
    {
        return wp_rigidbody_get_user_data( m_body );
    }
    void CRigidDynamic3::setUserData( void *userData )
    {
        wp_rigidbody_set_user_data( m_body, userData );
    }
    bool CRigidDynamic3::getKinematicMode() const
    {
        return wp_rigidbody_get_type( m_body ) == WORKPHONE_RIGIDBODY_KINEMATIC;
    }
    void CRigidDynamic3::setKinematicMode( bool kinematicMode )
    {
        setKinematic( kinematicMode );
    }
    SmartPtr<IPhysicsBody3> CRigidDynamic3::clone()
    {
        auto result = workphone::make_ptr<CRigidDynamic3>( wp_rigidbody_get_type( m_body ) );
        result->setTransform( getTransform() );
        result->setMass( getMass() );
        result->setActorFlag( getActorFlags(), true );
        result->setRigidBodyFlag( getRigidBodyFlags(), true );
        result->setCollisionType( getCollisionType() );
        result->setCollisionMask( getCollisionMask() );
        result->setEnabled( isEnabled() );
        result->setLinearVelocity( getLinearVelocity() );
        result->setAngularVelocity( getAngularVelocity() );
        result->setLinearDamping( getLinearDamping() );
        result->setAngularDamping( getAngularDamping() );
        result->setMaxAngularVelocity( getMaxAngularVelocity() );
        result->setSleepThreshold( getSleepThreshold() );
        result->setStabilizationThreshold( getStabilizationThreshold() );
        result->setWakeCounter( getWakeCounter() );
        result->setContactReportThreshold( getContactReportThreshold() );
        result->setCMassLocalPose( getCMassLocalPose() );
        result->setMassSpaceInertiaTensor( getMassSpaceInertiaTensor() );
        result->setSolverIterationCounts( m_minPositionIters, m_minVelocityIters );
        for( auto shape : m_shapes )
        {
            if( shape )
            {
                result->addShape( shape->clone() );
            }
        }
        if( isSleeping() )
        {
            result->putToSleep();
        }
        return result;
    }
    void CRigidDynamic3::wakeUp()
    {
        wp_rigidbody_wake_up( m_body );
    }
    SmartPtr<IStateContext> CRigidDynamic3::getStateContext() const
    {
        return m_stateContext;
    }
    void CRigidDynamic3::setStateContext( SmartPtr<IStateContext> stateContext )
    {
        m_stateContext = stateContext;
    }
    void CRigidDynamic3::_getObject( void **object ) const
    {
        if( object )
        {
            *object = m_body;
        }
    }
    void CRigidDynamic3::setRigidBodyFlag( RigidBodyFlagEnum flag, bool value )
    {
        updateFlags( m_rigidBodyFlags, flag, value );

        if( hasFlag( flag, RigidBodyFlagEnum::eKINEMATIC ) )
        {
            setKinematic( hasFlag( m_rigidBodyFlags, RigidBodyFlagEnum::eKINEMATIC ) );
        }
        if( hasFlag( flag, RigidBodyFlagEnum::eENABLE_CCD ) )
        {
            wp_rigidbody_set_flag( m_body, WORKPHONE_RIGIDBODY_FLAG_CCD,
                                   hasFlag( m_rigidBodyFlags, RigidBodyFlagEnum::eENABLE_CCD ) );
        }
        if( hasFlag( flag, RigidBodyFlagEnum::eENABLE_CCD_FRICTION ) )
        {
            wp_rigidbody_set_flag(
                m_body, WORKPHONE_RIGIDBODY_FLAG_CCD_FRICTION,
                hasFlag( m_rigidBodyFlags, RigidBodyFlagEnum::eENABLE_CCD_FRICTION ) );
        }
    }
    RigidBodyFlagEnum CRigidDynamic3::getRigidBodyFlags() const
    {
        return m_rigidBodyFlags;
    }
    void CRigidDynamic3::addShape( SmartPtr<IPhysicsShape3> shape )
    {
        if( !shape || std::find( m_shapes.begin(), m_shapes.end(), shape ) != m_shapes.end() )
        {
            return;
        }

        if( auto actor = shape->getActor() )
        {
            if( actor.get() != this )
            {
                WP_LOG_WARNING(
                    "CRigidDynamic3::addShape: shape is already attached to another actor." );
                return;
            }
        }

        void *raw = nullptr;
        shape->_getObject( &raw );
        if( raw && wp_rigidbody_add_shape( m_body, static_cast<wp_collision_shape *>( raw ) ) >= 0 )
        {
            shape->setActor( getSharedFromThis<CRigidDynamic3>() );
            m_shapes.push_back( shape );
        }
    }
    void CRigidDynamic3::removeShape( SmartPtr<IPhysicsShape3> shape, bool )
    {
        for( u32 i = 0; i < m_shapes.size(); ++i )
        {
            if( m_shapes[i] == shape )
            {
                wp_rigidbody_remove_shape( m_body, static_cast<wp_s32>( i ) );
                shape->setActor( nullptr );
                m_shapes.erase( m_shapes.begin() + i );
                return;
            }
        }
    }
    Array<SmartPtr<IPhysicsShape3>> CRigidDynamic3::getShapes() const
    {
        return m_shapes;
    }
    u32 CRigidDynamic3::getNumShapes() const
    {
        return static_cast<u32>( m_shapes.size() );
    }
    void CRigidDynamic3::setLinearVelocity( const Vector3<real_Num> &linVel, bool )
    {
        wp_rigidbody_set_linear_velocity( m_body, detail::toWp( linVel ) );
    }
    Vector3<real_Num> CRigidDynamic3::getLinearVelocity() const
    {
        return detail::fromWp( wp_rigidbody_get_linear_velocity( m_body ) );
    }
    void CRigidDynamic3::setAngularVelocity( const Vector3<real_Num> &angVel, bool )
    {
        wp_rigidbody_set_angular_velocity( m_body, detail::toWp( angVel ) );
    }
    Vector3<real_Num> CRigidDynamic3::getAngularVelocity() const
    {
        return detail::fromWp( wp_rigidbody_get_angular_velocity( m_body ) );
    }
    void CRigidDynamic3::addForce( const Vector3<real_Num> &force )
    {
        wp_rigidbody_add_force( m_body, detail::toWp( force ), WORKPHONE_FORCE_MODE_FORCE );

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->queueDebugForce( getId(), getTransform().getPosition(), force );
            }
        }
    }
    void CRigidDynamic3::clearForce( ForceModeEnum )
    {
        wp_rigidbody_clear_force( m_body );
    }
    void CRigidDynamic3::addTorque( const Vector3<real_Num> &torque )
    {
        wp_rigidbody_add_torque( m_body, detail::toWp( torque ), WORKPHONE_FORCE_MODE_FORCE );
    }
    void CRigidDynamic3::clearTorque( ForceModeEnum )
    {
        wp_rigidbody_clear_torque( m_body );
    }
    AABB3<real_Num> CRigidDynamic3::getLocalAABB() const
    {
        return detail::mergeShapeBounds( m_shapes );
    }
    AABB3<real_Num> CRigidDynamic3::getWorldAABB() const
    {
        return detail::transformBounds( getLocalAABB(), getTransform() );
    }
    void CRigidDynamic3::setCMassLocalPose( const Transform3<real_Num> &pose )
    {
        m_cmassLocalPose = pose;
        wp_rigidbody_set_cmass_local_position( m_body, detail::toWp( pose.getPosition() ) );
    }
    Transform3<real_Num> CRigidDynamic3::getCMassLocalPose() const
    {
        auto pose = m_cmassLocalPose;
        pose.setPosition( detail::fromWp( wp_rigidbody_get_cmass_local_position( m_body ) ) );
        return pose;
    }
    void CRigidDynamic3::setMassSpaceInertiaTensor( const Vector3<real_Num> &m )
    {
        wp_rigidbody_set_inertia_tensor( m_body, detail::toWp( m ) );
    }
    Vector3<real_Num> CRigidDynamic3::getMassSpaceInertiaTensor() const
    {
        return detail::fromWp( wp_rigidbody_get_inertia_tensor( m_body ) );
    }
    Vector3<real_Num> CRigidDynamic3::getMassSpaceInvInertiaTensor() const
    {
        auto i = getMassSpaceInertiaTensor();
        return Vector3<real_Num>( i.X() != 0 ? 1 / i.X() : 0, i.Y() != 0 ? 1 / i.Y() : 0,
                                  i.Z() != 0 ? 1 / i.Z() : 0 );
    }
    void CRigidDynamic3::setKinematicTarget( const Transform3<real_Num> &destination )
    {
        m_kinematicTarget = destination;
        m_hasKinematicTarget = true;
        setTransform( destination );
    }
    bool CRigidDynamic3::getKinematicTarget( Transform3<real_Num> &target )
    {
        target = m_kinematicTarget;
        return m_hasKinematicTarget;
    }
    bool CRigidDynamic3::isKinematic() const
    {
        return getKinematicMode();
    }
    void CRigidDynamic3::setKinematic( bool kinematic )
    {
        updateFlags( m_rigidBodyFlags, RigidBodyFlagEnum::eKINEMATIC, kinematic );
        if( getKinematicMode() == kinematic )
        {
            return;
        }

        wp_rigidbody_set_type( m_body,
                               kinematic ? WORKPHONE_RIGIDBODY_KINEMATIC : WORKPHONE_RIGIDBODY_DYNAMIC );
        if( !kinematic )
        {
            m_hasKinematicTarget = false;
        }
        wakeUp();
    }
    void CRigidDynamic3::setLinearDamping( real_Num damping )
    {
        wp_rigidbody_set_linear_damping(
            m_body, static_cast<wp_f32>( Math<real_Num>::max( damping, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num CRigidDynamic3::getLinearDamping() const
    {
        return wp_rigidbody_get_linear_damping( m_body );
    }
    void CRigidDynamic3::setAngularDamping( real_Num damping )
    {
        wp_rigidbody_set_angular_damping(
            m_body, static_cast<wp_f32>( Math<real_Num>::max( damping, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num CRigidDynamic3::getAngularDamping() const
    {
        return wp_rigidbody_get_angular_damping( m_body );
    }
    void CRigidDynamic3::setMaxAngularVelocity( real_Num maxAngVel )
    {
        wp_rigidbody_set_max_angular_velocity( m_body, static_cast<wp_f32>( Math<real_Num>::max(
                                                           maxAngVel, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num CRigidDynamic3::getMaxAngularVelocity() const
    {
        return wp_rigidbody_get_max_angular_velocity( m_body );
    }
    bool CRigidDynamic3::isSleeping() const
    {
        return wp_rigidbody_is_sleeping( m_body ) != 0;
    }
    void CRigidDynamic3::setSleepThreshold( real_Num threshold )
    {
        wp_rigidbody_set_sleep_threshold( m_body, static_cast<wp_f32>( Math<real_Num>::max(
                                                      threshold, static_cast<real_Num>( 0 ) ) ) );
    }
    real_Num CRigidDynamic3::getSleepThreshold() const
    {
        return wp_rigidbody_get_sleep_threshold( m_body );
    }
    void CRigidDynamic3::setStabilizationThreshold( real_Num threshold )
    {
        m_stabilizationThreshold = Math<real_Num>::max( threshold, static_cast<real_Num>( 0 ) );
    }
    real_Num CRigidDynamic3::getStabilizationThreshold() const
    {
        return m_stabilizationThreshold;
    }
    void CRigidDynamic3::setWakeCounter( real_Num wakeCounterValue )
    {
        m_wakeCounter = Math<real_Num>::max( wakeCounterValue, static_cast<real_Num>( 0 ) );
    }
    real_Num CRigidDynamic3::getWakeCounter() const
    {
        return m_wakeCounter;
    }
    void CRigidDynamic3::putToSleep()
    {
        wp_rigidbody_put_to_sleep( m_body );
    }
    void CRigidDynamic3::setSolverIterationCounts( u32 minPositionIters, u32 minVelocityIters )
    {
        m_minPositionIters = std::max( minPositionIters, 1u );
        m_minVelocityIters = std::max( minVelocityIters, 1u );
    }
    void CRigidDynamic3::getSolverIterationCounts( u32 &minPositionIters, u32 &minVelocityIters ) const
    {
        minPositionIters = m_minPositionIters;
        minVelocityIters = m_minVelocityIters;
    }
    real_Num CRigidDynamic3::getContactReportThreshold() const
    {
        return m_contactReportThreshold;
    }
    void CRigidDynamic3::setContactReportThreshold( real_Num threshold )
    {
        m_contactReportThreshold = Math<real_Num>::max( threshold, static_cast<real_Num>( 0 ) );
    }
    wp_rigidbody *CRigidDynamic3::getBody() const
    {
        return m_body;
    }
} // namespace workphone::physics
