#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxSphereShape.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxSphereShape, PhysxShape<SphereShape> );

    PhysxSphereShape::PhysxSphereShape()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto stateManager = applicationManager->getStateManager();
        WP_ASSERT( stateManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto physicsManager = applicationManager->getPhysicsManager();
        WP_ASSERT( physicsManager );

        auto stateContext = stateManager->addStateContext();
        stateContext->setOwner( this );
        setStateContext( stateContext );

        auto stateListener = factoryManager->make_ptr<ShapeStateListener>();
        stateListener->setOwner( this );
        setStateListener( stateListener );
        stateContext->addStateListener( stateListener );

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto sphereShapeState = factoryManager->make_ptr<SphereShapeStateData>();
        state->setData( sphereShapeState );

        auto shapeState = factoryManager->make_ptr<State>();
        stateContext->addState( shapeState );

        auto shapeStateData = factoryManager->make_ptr<ShapeStateData>();
        shapeState->setData( shapeStateData );

        auto physicsTask = physicsManager->getPhysicsTask();
        stateContext->setTaskId( physicsTask );
    }

    PhysxSphereShape::~PhysxSphereShape()
    {
        unload( nullptr );
    }

    void PhysxSphereShape::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( physicsManager );

            if( auto body = getActor() )
            {
                if( body->isExactly<PhysxRigidDynamic>() )
                {
                    auto rigidDynamic = workphone::static_pointer_cast<PhysxRigidDynamic>( body );

                    auto pRigidDynamic = rigidDynamic->getActor();
                    setPxActor( pRigidDynamic );
                }

                if( body->isExactly<PhysxRigidStatic>() )
                {
                    auto rigidStatic = workphone::static_pointer_cast<PhysxRigidStatic>( body );

                    auto pRigidStatic = rigidStatic->getRigidStatic();
                    setPxActor( pRigidStatic );
                }
            }

            createShape();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxSphereShape::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            PhysxShape<SphereShape>::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxSphereShape::createShape()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto physicsManager =
            workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
        WP_ASSERT( physicsManager );

        auto physics = physicsManager->getPhysics();
        WP_ASSERT( physics );

        ScopedLock lock( physicsManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto task = Thread::getCurrentTask();
        auto physicsTask = physicsManager->getPhysicsTask();

        physx::PxSphereGeometry geometry( getRadius() );
        if( geometry.isValid() )
        {
            auto localPose = PhysxUtil::toPx( getLocalPose() );
            auto material = getMaterial();
            WP_ASSERT( material );
            auto physicsMaterial = workphone::static_pointer_cast<PhysxMaterial>( material );
            WP_ASSERT( physicsMaterial );
            auto m = physicsMaterial->getMaterial();

            auto shape = physics->createShape( geometry, *m );
            setShape( shape );

            if( shape )
            {
                shape->setLocalPose( localPose );
                setupCollisionMask( shape );
            }
        }
        else
        {
            WP_LOG_ERROR( "Invalid geometry for sphere shape" );
        }
    }

} // namespace workphone::physics
