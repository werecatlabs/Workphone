#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxBoxShape.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <WPPhysx/WPPhysxScene.hpp>
#include <extensions/PxRigidBodyExt.h>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxRigidDynamic, PhysxRigidBody3<RigidDynamic3> );

    PhysxRigidDynamic::PhysxRigidDynamic()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        stateListener->setOwner( this );
        stateContext->addStateListener( stateListener );
        setStateListener( stateListener );

        auto physicsBodyState = factoryManager->make_ptr<State>();
        stateContext->addState( physicsBodyState );

        auto physicsBodyStateData = factoryManager->make_ptr<PhysicsBodyState>();
        physicsBodyState->setData( physicsBodyStateData );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<RigidbodyState>();
        state->setData( stateData );

        auto bodyMassState = factoryManager->make_ptr<State>();
        stateContext->addState( bodyMassState );

        auto bodyMassStateData = factoryManager->make_ptr<PhysicsBodyMassState>();
        bodyMassState->setData( bodyMassStateData );

        auto bodyMotionState = factoryManager->make_ptr<State>();
        stateContext->addState( bodyMotionState );

        auto bodyMotionStateData = factoryManager->make_ptr<PhysicsBodyMotionState>();
        bodyMotionState->setData( bodyMotionStateData );

        auto physicsTask = TaskId::Physics;
        stateContext->setTaskId( physicsTask );
    }

    PhysxRigidDynamic::~PhysxRigidDynamic()
    {
        unload( nullptr );
    }

    void PhysxRigidDynamic::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            if( isLoaded() )
            {
                return;
            }

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            auto physxManager = workphone::static_pointer_cast<PhysxManager>( physicsManager );
            WP_ASSERT( physxManager );

            auto physics = physxManager->getPhysics();
            WP_ASSERT( physics );

            auto transform = getTransform();
            auto t = PhysxUtil::toPx( transform );

            if( !getActor() )
            {
                auto actor = physics->createRigidDynamic( t );
                actor->setActorFlag( physx::PxActorFlag::eVISUALIZATION, true );
                actor->userData = this;

                setActor( actor );
                setActorDynamic( actor );
            }

            setSolverIterationCounts( 100, 100 ); // todo

            auto scene = workphone::static_pointer_cast<PhysxScene>( getScene() );
            if( scene )
            {
                auto pxScene = scene->getScene();

                if( auto actor = getActor() )
                {
                    pxScene->addActor( *actor );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxRigidDynamic::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            auto stateManager = applicationManager->getStateManager();
            WP_ASSERT( stateManager );

            if( auto stateContext = getStateContext() )
            {
                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                    stateListener->unload( nullptr );
                    setStateListener( nullptr );
                }

                stateManager->removeStateContext( stateContext );
                stateContext->unload( nullptr );
                setStateContext( nullptr );
            }

            for( auto shape : m_shapes )
            {
                shape->unload( nullptr );
            }

            m_shapes.clear();

            if( auto actor = getActor() )
            {
                auto numShapes = actor->getNbShapes();
                if( numShapes > 0 )
                {
                    Array<physx::PxShape *> shapes;
                    shapes.resize( numShapes );

                    actor->getShapes( &shapes[0], numShapes );

                    for( u32 i = 0; i < numShapes; ++i )
                    {
                        physx::PxShape *shape = shapes[i];
                        shape->userData = nullptr;

                        actor->detachShape( *shape, false );
                    }
                }

                auto pScene = actor->getScene();
                if( pScene )
                {
                    pScene->removeActor( *actor );
                }

                actor->release();
                setActor( nullptr );
            }

            m_scene = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxRigidDynamic::addVelocity( const Vector3<physics_Num> &velocity,
                                         const Vector3<physics_Num> &relPos /*= Vector3F( ) */ )
    {
    }

    void PhysxRigidDynamic::setVelocity( const Vector3<physics_Num> &velocity )
    {
    }

    auto PhysxRigidDynamic::getVelocity() const -> Vector3<physics_Num>
    {
        if( auto actor = getActorDynamic() )
        {
            auto velocity = actor->getLinearVelocity();
            auto v = PhysxUtil::toFB( velocity );
            WP_ASSERT( v.isFinite() );

            WP_ASSERT( Math<physics_Num>::Abs( v[0] ) < static_cast<physics_Num>( 1e3 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[1] ) < static_cast<physics_Num>( 1e3 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[2] ) < static_cast<physics_Num>( 1e3 ) );

            return v;
        }

        return Vector3<physics_Num>::zero();
    }

    auto PhysxRigidDynamic::getAngularVelocity() const -> Vector3<physics_Num>
    {
        if( auto actor = getActorDynamic() )
        {
            auto angularVelocity = actor->getAngularVelocity();
            auto v = PhysxUtil::toFB( angularVelocity );
            WP_ASSERT( v.isFinite() );

            WP_ASSERT( Math<physics_Num>::Abs( v[0] ) < static_cast<physics_Num>( 1e3 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[1] ) < static_cast<physics_Num>( 1e3 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[2] ) < static_cast<physics_Num>( 1e3 ) );

            return v;
        }

        return Vector3<physics_Num>::zero();
    }

    void PhysxRigidDynamic::addForce( const Vector3<physics_Num> &force )
    {
        ScopedLock lock( this, true );

        if( auto actor = getActorDynamic() )
        {
            if( auto scene = actor->getScene() )
            {
                auto f = PhysxUtil::toPx( force );
                actor->addForce( f, physx::PxForceMode::eFORCE, true );
            }
        }

        if( auto applicationManager = core::IApplicationManager::instancePtr() )
        {
            if( auto physicsManager = applicationManager->getPhysicsManagerPtr() )
            {
                physicsManager->queueDebugForce( getId(), getTransform().getPosition(), force );
            }
        }
    }

    void PhysxRigidDynamic::setForce( const Vector3<physics_Num> &force )
    {
    }

    auto PhysxRigidDynamic::getForce() const -> Vector3<physics_Num>
    {
        return Vector3<physics_Num>::ZERO;
    }

    void PhysxRigidDynamic::addTorque( const Vector3<physics_Num> &torque )
    {
        ScopedLock lock( this );

        if( auto actor = getActorDynamic() )
        {
            if( auto scene = actor->getScene() )
            {
                auto t = physx::PxVec3( torque.X(), torque.Y(), torque.Z() );
                actor->addTorque( t, physx::PxForceMode::eFORCE, true );
            }
        }
    }

    void PhysxRigidDynamic::setTorque( const Vector3<physics_Num> &torque )
    {
    }

    auto PhysxRigidDynamic::getTorque() const -> Vector3<physics_Num>
    {
        return Vector3<physics_Num>::ZERO;
    }

    auto PhysxRigidDynamic::getActorDynamic() const -> physx::PxRigidDynamic *
    {
        return (physx::PxRigidDynamic *)m_actor.load();
    }

    void PhysxRigidDynamic::setActorDynamic( physx::PxRigidDynamic *actor )
    {
        m_actor = actor;

        if( actor )
        {
            actor->setActorFlag( physx::PxActorFlag::eVISUALIZATION, true );
            actor->userData = this;
        }
    }

    void PhysxRigidDynamic::setRigidBodyFlag( RigidBodyFlagEnum flag, bool value )
    {
        if( auto actor = getActorDynamic() )
        {
            auto iFlag = static_cast<s32>( flag );
            auto f = static_cast<physx::PxRigidBodyFlag::Enum>( iFlag );
            actor->setRigidBodyFlag( f, value );
        }
    }

    auto PhysxRigidDynamic::getRigidBodyFlags() const -> RigidBodyFlagEnum
    {
        return static_cast<RigidBodyFlagEnum>( 0 );
    }

    void PhysxRigidDynamic::setLinearVelocity( const Vector3<physics_Num> &linVel,
                                               bool                        autowake /*= true*/ )
    {
        if( auto actor = getActorDynamic() )
        {
            auto v = PhysxUtil::toPx( linVel );
            actor->setLinearVelocity( v );
        }
    }

    auto PhysxRigidDynamic::getLinearVelocity() const -> Vector3<physics_Num>
    {
        if( auto actor = getActorDynamic() )
        {
            auto linearVelocity = actor->getLinearVelocity();
            auto v = PhysxUtil::toFB( linearVelocity );
            WP_ASSERT( v.isFinite() );

            WP_ASSERT( Math<physics_Num>::Abs( v[0] ) < static_cast<physics_Num>( 1e8 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[1] ) < static_cast<physics_Num>( 1e8 ) );
            WP_ASSERT( Math<physics_Num>::Abs( v[2] ) < static_cast<physics_Num>( 1e8 ) );

            return v;
        }

        return Vector3<physics_Num>::zero();
    }

    void PhysxRigidDynamic::setAngularVelocity( const Vector3<physics_Num> &angVel,
                                                bool                        autowake /*= true*/ )
    {
        if( auto actor = getActorDynamic() )
        {
            auto v = PhysxUtil::toPx( angVel );
            actor->setAngularVelocity( v, autowake );
        }
    }

    void PhysxRigidDynamic::clearForce( ForceModeEnum mode /*= ForceModeEnum::Force*/ )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->clearForce();
        }
    }

    void PhysxRigidDynamic::clearTorque( ForceModeEnum mode /*= ForceModeEnum::Force*/ )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->clearTorque();
        }
    }

    void PhysxRigidDynamic::setActorFlag( ActorFlagEnum flag, bool value )
    {
        if( auto actor = getActor() )
        {
            auto iFlag = static_cast<int>( flag );
            actor->setActorFlag( static_cast<physx::PxActorFlag::Enum>( iFlag ), value );
        }
    }

    auto PhysxRigidDynamic::getActorFlags() const -> ActorFlagEnum
    {
        if( auto actor = getActor() )
        {
            auto flags = actor->getActorFlags();
            auto iFlags = flags.operator physx::PxU32();
            return static_cast<ActorFlagEnum>( iFlags );
        }

        return static_cast<ActorFlagEnum>( 0 );
    }

    void PhysxRigidDynamic::setKinematicTarget( const Transform3<physics_Num> &destination )
    {
        if( auto actor = getActorDynamic() )
        {
            auto t = PhysxUtil::toPx( destination );
            actor->setKinematicTarget( t );
        }
    }

    auto PhysxRigidDynamic::getKinematicTarget( Transform3<physics_Num> &target ) -> bool
    {
        if( auto actor = getActorDynamic() )
        {
            auto t = physx::PxTransform();
            if( actor->getKinematicTarget( t ) )
            {
                target = PhysxUtil::toFB( t );
                return true;
            }
        }

        return false;
    }

    auto PhysxRigidDynamic::isKinematic() const -> bool
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getRigidBodyFlags().isSet( physx::PxRigidBodyFlag::eKINEMATIC );
        }

        return false;
    }

    void PhysxRigidDynamic::setKinematic( bool kinematic )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setRigidBodyFlag( physx::PxRigidBodyFlag::eKINEMATIC, kinematic );
        }
    }

    void PhysxRigidDynamic::setLinearDamping( physics_Num damping )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setLinearDamping( damping );
        }
    }

    auto PhysxRigidDynamic::getLinearDamping() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getLinearDamping();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::setAngularDamping( physics_Num damping )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setAngularDamping( damping );
        }
    }

    auto PhysxRigidDynamic::getAngularDamping() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getAngularDamping();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::setMaxAngularVelocity( physics_Num maxAngVel )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setMaxAngularVelocity( maxAngVel );
        }
    }

    auto PhysxRigidDynamic::getMaxAngularVelocity() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getMaxAngularVelocity();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    auto PhysxRigidDynamic::isSleeping() const -> bool
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->isSleeping();
        }

        return false;
    }

    void PhysxRigidDynamic::setSleepThreshold( physics_Num threshold )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setSleepThreshold( threshold );
        }
    }

    auto PhysxRigidDynamic::getSleepThreshold() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getSleepThreshold();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::setStabilizationThreshold( physics_Num threshold )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setStabilizationThreshold( threshold );
        }
    }

    auto PhysxRigidDynamic::getStabilizationThreshold() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getStabilizationThreshold();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::setWakeCounter( physics_Num wakeCounterValue )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setWakeCounter( wakeCounterValue );
        }
    }

    auto PhysxRigidDynamic::getWakeCounter() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getWakeCounter();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::wakeUp()
    {
        if( auto actor = getActorDynamic() )
        {
            actor->wakeUp();
        }
    }

    void PhysxRigidDynamic::putToSleep()
    {
        if( auto actor = getActorDynamic() )
        {
            actor->putToSleep();
        }
    }

    void PhysxRigidDynamic::setSolverIterationCounts( u32 minPositionIters,
                                                      u32 minVelocityIters /*= 1*/ )
    {
        WP_ASSERT( minPositionIters > 0 );
        WP_ASSERT( minVelocityIters > 0 );

        if( auto actor = getActorDynamic() )
        {
            actor->setSolverIterationCounts( minPositionIters, minVelocityIters );
        }
    }

    void PhysxRigidDynamic::getSolverIterationCounts( u32 &minPositionIters,
                                                      u32 &minVelocityIters ) const
    {
        if( auto actor = getActorDynamic() )
        {
            actor->getSolverIterationCounts( minPositionIters, minVelocityIters );
        }
    }

    auto PhysxRigidDynamic::getContactReportThreshold() const -> physics_Num
    {
        if( auto actor = getActorDynamic() )
        {
            return actor->getContactReportThreshold();
        }

        return static_cast<physics_Num>( 0.0 );
    }

    void PhysxRigidDynamic::setContactReportThreshold( physics_Num threshold )
    {
        if( auto actor = getActorDynamic() )
        {
            actor->setContactReportThreshold( threshold );
        }
    }

    auto PhysxRigidDynamic::clone() -> SmartPtr<IPhysicsBody3>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        auto physxManager = workphone::static_pointer_cast<PhysxManager>( physicsManager );
        WP_ASSERT( physxManager );

        if( auto actor = getActorDynamic() )
        {
            auto body = workphone::make_ptr<PhysxRigidDynamic>();
            auto clonedActor =
                PxCloneDynamic( *physxManager->getPhysics(), actor->getGlobalPose(), *actor );

            body->setActorDynamic( clonedActor );

            return body;
        }

        return nullptr;
    }

    void PhysxRigidDynamic::setActiveTransform( const Transform3<physics_Num> &transform )
    {
        if( auto stateContext = getStateContext() )
        {
            if( auto state = stateContext->getStateData<PhysicsBodyState>() )
            {
                state->transform = transform;
            }
        }

        auto args = Parameters();
        args.resize( 2 );

        Parameter();
        args[0].setVector3( transform.getPosition() );
        args[1].setQuaternion( transform.getOrientation() );

        auto listeners = getObjectListeners();
        for( auto &listener : listeners )
        {
            if( listener )
            {
                listener->handleEvent( EventType::Scene, IEvent::transform, args, this, nullptr,
                                       nullptr );
            }
        }
    }

    auto PhysxRigidDynamic::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 10 );

        objects.emplace_back( getStateContext() );
        objects.emplace_back( getStateListener() );

        if( auto stateContext = getStateContext() )
        {
            auto states = stateContext->getStates();
            for( auto state : states )
            {
                objects.emplace_back( state );
            }
        }

        return objects;
    }

    auto PhysxRigidDynamic::getProperties() const -> SmartPtr<Properties>
    {
        try
        {
            auto properties = workphone::make_ptr<Properties>();

            auto transform = getTransform();

            auto position = transform.getPosition();
            properties->setProperty( "position", position );

            auto rotation = transform.getRotation();
            properties->setProperty( "rotation", rotation );

            auto velocity = getVelocity();
            properties->setProperty( "velocity", velocity );

            auto mass = getMass();
            properties->setProperty( "mass", mass );

            auto kinematic = isKinematic();
            properties->setProperty( "kinematic", kinematic );

            auto massSpaceInertiaTensor = getMassSpaceInertiaTensor();
            properties->setProperty( "massSpaceInertiaTensor", massSpaceInertiaTensor );

            return properties;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    void PhysxRigidDynamic::setProperties( SmartPtr<Properties> properties )
    {
        try
        {
            auto transform = getTransform();

            auto position = transform.getPosition();
            properties->getPropertyValue( "position", position );

            auto rotation = transform.getRotation();
            properties->getPropertyValue( "rotation", rotation );

            auto velocity = getVelocity();
            properties->getPropertyValue( "velocity", velocity );

            auto mass = static_cast<physics_Num>( 1000.0 );
            properties->getPropertyValue( "mass", mass );
            setMass( mass );

            auto kinematic = isKinematic();
            properties->getPropertyValue( "kinematic", kinematic );
            setKinematic( kinematic );

            auto massSpaceInertiaTensor = getMassSpaceInertiaTensor();
            properties->getPropertyValue( "massSpaceInertiaTensor", massSpaceInertiaTensor );
            setMassSpaceInertiaTensor( massSpaceInertiaTensor );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxRigidDynamic::addShape( SmartPtr<IPhysicsShape3> shape )
    {
        try
        {
            if( shape->isDerived<IPlaneShape3>() )
            {
                return;
            }

            if( shape->isDerived<IMeshShape>() )
            {
                auto meshShape = workphone::static_pointer_cast<IMeshShape>( shape );
                if( !meshShape->isConvex() )
                {
                    return;
                }
            }

            if( shape->isDerived<ITerrainShape>() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManagerPtr();
            WP_ASSERT( physicsManager );

            ScopedLock lock( this, true );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto task = Thread::getCurrentTask();
            auto physicsTask = physicsManager->getPhysicsTask();

            if( !isLoaded() )
            {
                load( nullptr );
            }

            if( shape )
            {
                auto pThis = getSharedFromThis<ISharedObject>();
                shape->setActor( pThis );
                shape->setCollisionType( getCollisionType() );
                shape->setCollisionMask( getCollisionMask() );

                if( !shape->isLoaded() )
                {
                    shape->load( nullptr );
                }

                shape->setCollisionType( getCollisionType() );
                shape->setCollisionMask( getCollisionMask() );

                physx::PxShape *pShape = nullptr;
                shape->_getObject( (void **)&pShape );

                WP_ASSERT( pShape );

                if( pShape )
                {
                    if( auto pxActor = getActor() )
                    {
                        pxActor->attachShape( *pShape );
                    }
                }

                m_shapes.push_back( shape );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool PhysxRigidDynamic::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        if( auto owner = getOwner() )
        {
            owner->handleStateChanged( state );

            if( owner->isLoaded() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto physicsManager = applicationManager->getPhysicsManagerPtr();
                WP_ASSERT( physicsManager );

                ScopedLock lock( physicsManager );

                auto actor = owner->getActorDynamic();
                if( actor )
                {
                    auto stateData = state->getData();
                    if( stateData->isDerived<PhysicsBodyState>() )
                    {
                        auto rigidbodyState =
                            workphone::static_pointer_cast<PhysicsBodyState>( stateData );
                        if( rigidbodyState )
                        {
                            auto transform = rigidbodyState->transform;

                            WP_ASSERT( transform.isValid() );
                            WP_ASSERT( transform.isSane() );

                            auto t = PhysxUtil::toPx( transform );
                            WP_ASSERT( t.isFinite() );
                            WP_ASSERT( t.isSane() );

                            if( t.isSane() )
                            {
                                WP_ASSERT( actor );

                                if( actor )
                                {
                                    actor->setGlobalPose( t );
                                    WP_ASSERT( actor->getGlobalPose().isSane() );
                                }
                            }

                            auto mass = rigidbodyState->mass;
                            if( !Math<physics_Num>::equals( mass, actor->getMass() ) )
                            {
                                actor->setMass( mass );
                            }

                            return true;
                        }
                    }
                    else if( stateData->isDerived<PhysicsBodyMassState>() )
                    {
                        auto massState =
                            workphone::static_pointer_cast<PhysicsBodyMassState>( stateData );
                        if( massState )
                        {
                            auto t = PhysxUtil::toPx( massState->massSpaceLocalPose );
                            actor->setCMassLocalPose( t );

                            auto pxTensor = PhysxUtil::toPx( massState->inertiaTensor );
                            actor->setMassSpaceInertiaTensor( pxTensor );

                            return true;
                        }
                    }
                    else if( stateData->isDerived<PhysicsBodyMotionState>() )
                    {
                        auto motionState =
                            workphone::static_pointer_cast<PhysicsBodyMotionState>( stateData );
                        if( motionState )
                        {
                            /*
                            auto linearVelocity = PhysxUtil::toPx( motionState->linearVelocity );
                            actor->setLinearVelocity( linearVelocity );
                            auto angularVelocity = PhysxUtil::toPx( motionState->angularVelocity );
                            actor->setAngularVelocity( angularVelocity );
                            auto force = PhysxUtil::toPx( motionState->force );
                            actor->addForce( force, physx::PxForceMode::eFORCE, true );
                            auto torque = PhysxUtil::toPx( motionState->torque );
                            actor->addTorque( torque, physx::PxForceMode::eFORCE, true );
                            auto addedForce = PhysxUtil::toPx( motionState->addedForce );
                            actor->addForce( addedForce, physx::PxForceMode::eFORCE, true );
                            auto addedTorque = PhysxUtil::toPx( motionState->addedTorque );
                            actor->addTorque( addedTorque, physx::PxForceMode::eFORCE, true );

                            auto &flags = motionState->flags;
                            if( flags & PhysicsBodyMotionFlagClearForce )
                            {
                                actor->clearForce();
                            }
                            if( flags & PhysicsBodyMotionFlagClearTorque )
                            {
                                actor->clearTorque();
                            }
                            if( flags & PhysicsBodyMotionFlagSetVelocity )
                            {
                                actor->setLinearVelocity( linearVelocity );
                            }
                            if( flags & PhysicsBodyMotionFlagSetAngularVelocity )
                            {
                                actor->setAngularVelocity( angularVelocity );
                            }
                            */

                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool PhysxRigidDynamic::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                WP_ASSERT( applicationManager );

                auto physicsManager = applicationManager->getPhysicsManager();
                WP_ASSERT( physicsManager );

                ScopedLock lock( physicsManager );

                if( message->isExactly<StateMessageTransform3>() )
                {
                    auto stateMessageTransform =
                        workphone::static_pointer_cast<StateMessageTransform3>( message );
                    if( stateMessageTransform )
                    {
                        auto t = stateMessageTransform->getTransform();
                        owner->setTransform( t );
                    }
                }
                else if( message->isExactly<StateMessageObject>() )
                {
                    WP_ASSERT( workphone::dynamic_pointer_cast<StateMessageObject>( message ) );
                    auto objectMessage = workphone::static_pointer_cast<StateMessageObject>( message );
                    WP_ASSERT( objectMessage );

                    auto messageType = objectMessage->getType();
                    auto object = objectMessage->getObject();

                    if( messageType == STATE_MESSAGE_ATTACH_SHAPE )
                    {
                        owner->addShape( object );
                    }
                    else if( messageType == STATE_MESSAGE_DETACH_SHAPE )
                    {
                        owner->removeShape( object );
                    }
                }
                else if( message->isExactly<StateMessageFloatValue>() )
                {
                    auto stateMessageFloat =
                        workphone::static_pointer_cast<StateMessageFloatValue>( message );
                    auto messageType = stateMessageFloat->getType();
                    if( messageType == STATE_MESSAGE_MASS )
                    {
                        owner->setMass( stateMessageFloat->getValue() );
                    }
                }
                else if( message->isExactly<StateMessageVector3>() )
                {
                    auto stateMessageVector =
                        workphone::static_pointer_cast<StateMessageVector3>( message );
                    auto messageType = stateMessageVector->getType();

                    if( messageType == STATE_MESSAGE_INERTIA_TENSOR )
                    {
                        owner->setMassSpaceInertiaTensor( stateMessageVector->getValue() );
                    }
                }
            }
        }

        return false;
    }

    auto PhysxRigidDynamic::StateListener::getOwner() const -> SmartPtr<PhysxRigidDynamic>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PhysxRigidDynamic::StateListener::setOwner( SmartPtr<PhysxRigidDynamic> owner )
    {
        m_owner = owner;
    }

    PhysxRigidDynamic::StateListener::StateListener() = default;

    PhysxRigidDynamic::StateListener::~StateListener() = default;

} // namespace workphone::physics
