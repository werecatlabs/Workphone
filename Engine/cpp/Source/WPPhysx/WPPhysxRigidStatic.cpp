#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <WPPhysx/WPPhysxScene.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxRigidStatic, PhysxRigidBody3<RigidStatic3> );

    PhysxRigidStatic::PhysxRigidStatic()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto stateContext = stateManager->addStateContext();
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<StateListener>();
        setStateListener( stateListener );

        stateListener->setOwner( this );
        stateContext->addStateListener( m_stateListener );

        // Transform and actor flags live in PhysicsBodyState, separate from RigidbodyState.
        auto physicsBodyState = factoryManager->make_ptr<State>();
        stateContext->addState( physicsBodyState );
        physicsBodyState->setData( factoryManager->make_ptr<PhysicsBodyState>() );
        stateContext->setTaskId( TaskId::Physics );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );
        stateContext->setOwner( this );

        auto stateData = factoryManager->make_ptr<RigidbodyState>();
        state->setData( stateData );
    }

    PhysxRigidStatic::~PhysxRigidStatic()
    {
        unload( nullptr );
    }

    void PhysxRigidStatic::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager =
                workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );

            if( auto physics = physicsManager->getPhysics() )
            {
                auto t = PhysxUtil::toPx( getTransform() );
                auto actor = physics->createRigidStatic( t );

                setActor( actor );
                setRigidStatic( actor );
            }

            auto scene = workphone::static_pointer_cast<PhysxScene>( getScene() );
            if( scene )
            {
                auto pxScene = scene->getScene();
                pxScene->addActor( *getRigidStatic() );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxRigidStatic::unload( SmartPtr<ISharedObject> data )
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

            if( auto stateContext = getStateContext() )
            {
                stateContext->clear();

                if( auto stateListener = getStateListener() )
                {
                    stateContext->removeStateListener( stateListener );
                    stateListener->unload( nullptr );
                    setStateListener( nullptr );
                }

                if( stateManager )
                {
                    stateManager->removeStateContext( stateContext );
                }

                stateContext->unload( nullptr );
                setStateContext( nullptr );
            }

            for( auto shape : m_shapes )
            {
                shape->unload( nullptr );
            }

            m_shapes.clear();

            if( m_rigidStatic )
            {
                auto numShapes = m_rigidStatic->getNbShapes();
                if( numShapes > 0 )
                {
                    Array<physx::PxShape *> shapes;
                    shapes.resize( numShapes );

                    m_rigidStatic->getShapes( &shapes[0], numShapes );

                    for( u32 i = 0; i < numShapes; ++i )
                    {
                        physx::PxShape *shape = shapes[i];
                        shape->userData = nullptr;

                        m_rigidStatic->detachShape( *shape, false );
                    }
                }

                auto pScene = m_rigidStatic->getScene();
                if( pScene )
                {
                    pScene->removeActor( *m_rigidStatic );
                }

                m_rigidStatic->release();
                m_rigidStatic = nullptr;
            }

            m_scene = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto PhysxRigidStatic::getRigidStatic() const -> physx::PxRigidStatic *
    {
        return m_rigidStatic;
    }

    void PhysxRigidStatic::setRigidStatic( physx::PxRigidStatic *rigidStatic )
    {
        m_rigidStatic = rigidStatic;

        if( rigidStatic )
        {
            rigidStatic->userData = this;
        }
    }

    auto PhysxRigidStatic::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> objects;
        return objects;
    }

    auto PhysxRigidStatic::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();

        auto mass = getMass();
        properties->setProperty( "mass", mass );

        return properties;
    }

    void PhysxRigidStatic::setProperties( SmartPtr<Properties> properties )
    {
        auto mass = static_cast<physics_Num>( 1000.0 );
        properties->getPropertyValue( "mass", mass );
        setMass( mass );
    }

    void PhysxRigidStatic::setLinearVelocity( const Vector3<physics_Num> &linVel,
                                              bool                        autowake /*= true*/ )
    {
    }

    auto PhysxRigidStatic::getLinearVelocity() const -> Vector3<physics_Num>
    {
        return Vector3<physics_Num>::zero();
    }

    void PhysxRigidStatic::setAngularVelocity( const Vector3<physics_Num> &angVel,
                                               bool                        autowake /*= true*/ )
    {
    }

    auto PhysxRigidStatic::getAngularVelocity() const -> Vector3<physics_Num>
    {
        return Vector3<physics_Num>::zero();
    }

    void PhysxRigidStatic::addForce( const Vector3<physics_Num> &force )
    {
    }

    void PhysxRigidStatic::clearForce( ForceModeEnum mode /*= ForceModeEnum::Force*/ )
    {
    }

    void PhysxRigidStatic::addTorque( const Vector3<physics_Num> &torque )
    {
    }

    void PhysxRigidStatic::clearTorque( ForceModeEnum mode /*= ForceModeEnum::Force*/ )
    {
    }

    auto PhysxRigidStatic::clone() -> SmartPtr<IPhysicsBody3>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        auto physxManager = workphone::static_pointer_cast<PhysxManager>( physicsManager );
        WP_ASSERT( physxManager );

        auto body = workphone::make_ptr<PhysxRigidStatic>();
        auto clonedActor =
            PxCloneStatic( *physxManager->getPhysics(), m_rigidStatic->getGlobalPose(), *m_rigidStatic );

        body->setRigidStatic( clonedActor );

        return body;
    }

    bool PhysxRigidStatic::StateListener::handleStateChanged( SmartPtr<IState> &state )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        ScopedLock lock( physicsManager );

        if( auto owner = getOwner() )
        {
            owner->handleStateChanged( state );

            auto stateData = state->getData();
            if( stateData->isDerived<PhysicsBodyState>() )
            {
                auto rigidState = workphone::static_pointer_cast<PhysicsBodyState>( stateData );

                auto actor = owner->getRigidStatic();

                if( actor )
                {
                    auto t = PhysxUtil::toPx( rigidState->transform );
                    auto pose = actor->getGlobalPose();

                    if( !PhysxUtil::equals( t, pose ) )
                    {
                        auto scene = actor->getScene();
                        if( scene )
                        {
                            scene->removeActor( *actor );
                        }

                        WP_ASSERT( t.isSane() );

                        if( t.isSane() )
                        {
                            if( actor )
                            {
                                actor->setGlobalPose( t );
                                WP_ASSERT( actor->getGlobalPose().isSane() );
                            }
                        }

                        if( scene )
                        {
                            if( actor->getNbShapes() > 0 )
                            {
                                scene->addActor( *actor );
                            }
                        }
                    }

                    if( actor->getNbShapes() == 0 )
                    {
                        auto scene = actor->getScene();
                        if( scene )
                        {
                            scene->removeActor( *actor );
                        }
                    }

                    return true;
                }
            }
        }

        return false;
    }

    bool PhysxRigidStatic::StateListener::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        ScopedLock lock( physicsManager );

        if( auto owner = getOwner() )
        {
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
        }

        return false;
    }

    auto PhysxRigidStatic::StateListener::getOwner() const -> SmartPtr<PhysxRigidStatic>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void PhysxRigidStatic::StateListener::setOwner( SmartPtr<PhysxRigidStatic> owner )
    {
        m_owner = owner;
    }

    PhysxRigidStatic::StateListener::StateListener() = default;

    PhysxRigidStatic::StateListener::~StateListener() = default;

} // namespace workphone::physics
