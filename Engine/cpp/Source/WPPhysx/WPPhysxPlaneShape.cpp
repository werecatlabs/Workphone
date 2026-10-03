#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxPlaneShape.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxPlaneShape, PhysxShape<PlaneShape> );

    PhysxPlaneShape::PhysxPlaneShape()
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

        auto planeShapeState = factoryManager->make_ptr<PlaneShapeState>();
        state->setData( planeShapeState );

        auto shapeState = factoryManager->make_ptr<State>();
        stateContext->addState( shapeState );

        auto shapeStateData = factoryManager->make_ptr<ShapeStateData>();
        shapeState->setData( shapeStateData );

        auto physicsTask = physicsManager->getPhysicsTask();
        stateContext->setTaskId( physicsTask );
    }

    PhysxPlaneShape::~PhysxPlaneShape()
    {
        unload( nullptr );
    }

    void PhysxPlaneShape::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager =
                workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
            WP_ASSERT( physicsManager );

            auto physics = physicsManager->getPhysics();
            WP_ASSERT( physics );

            ScopedLock lock( physicsManager );

            createShape();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxPlaneShape::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto stateManager = applicationManager->getStateManager();

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( physicsManager );

            if( auto shape = getShape() )
            {
                if( m_pxActor )
                {
                    m_pxActor->detachShape( *shape, false );
                }
            }

            setShape( nullptr );

            PhysxShape<PlaneShape>::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxPlaneShape::createShape()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto physicsManager =
            workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );
        WP_ASSERT( physicsManager );

        ScopedLock lock( physicsManager );

        auto physics = physicsManager->getPhysics();
        WP_ASSERT( physics );

        auto plane = getPlane();
        auto n = plane.getNormal();
        auto d = plane.getDistance();

        physx::PxPlane testPlane;
        testPlane.n = physx::PxVec3( n.X(), n.Y(), n.Z() );
        testPlane.d = 0.0; // d;
        physx::PxTransform localTransform = PxTransformFromPlaneEquation( testPlane );

        auto material = getMaterial();
        auto physicsMaterial = workphone::static_pointer_cast<PhysxMaterial>( material );
        WP_ASSERT( physicsMaterial );

        auto m = physicsMaterial->getMaterial();

        auto planeShape = physics->createShape( physx::PxPlaneGeometry(), *m );
        planeShape->setLocalPose( localTransform );

        if( auto attachedBody = getActor() )
        {
            setCollisionType( attachedBody->getCollisionType() );
            setCollisionMask( attachedBody->getCollisionMask() );
        }

        setupCollisionMask( planeShape );
        setShape( planeShape );
    }

    void PhysxPlaneShape::destroyShape()
    {
        if( auto shape = getShape() )
        {
            if( auto pxActor = getPxActor() )
            {
                pxActor->detachShape( *shape, false );
            }
        }
    }

} // namespace workphone::physics
