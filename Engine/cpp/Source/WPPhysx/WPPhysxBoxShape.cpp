#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxBoxShape.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/PhysxUtil.hpp>
#include <WPPhysx/WPPhysxMaterial.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxRigidStatic.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>

namespace workphone::physics
{
    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxBoxShape, PhysxShape<BoxShape3> );

    PhysxBoxShape::PhysxBoxShape()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
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

        auto boxShapeState = factoryManager->make_ptr<BoxShapeStateData>();
        state->setData( boxShapeState );

        auto shapeState = factoryManager->make_ptr<State>();
        stateContext->addState( shapeState );

        auto shapeStateData = factoryManager->make_ptr<ShapeStateData>();
        shapeState->setData( shapeStateData );

        auto physicsTask = physicsManager->getPhysicsTask();
        stateContext->setTaskId( physicsTask );
    }

    PhysxBoxShape::~PhysxBoxShape()
    {
        unload( nullptr );
    }

    void PhysxBoxShape::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            ScopedLock lock( this );

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

    void PhysxBoxShape::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto physicsManager = applicationManager->getPhysicsManager();
            WP_ASSERT( physicsManager );

            PhysxShape<BoxShape3>::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto PhysxBoxShape::isValid() const -> bool
    {
        auto valid = false;

        const auto &loadingState = getLoadingState();
        if( loadingState == LoadingState::Loaded )
        {
            if( auto shape = getShape() )
            {
                valid = true;
            }
        }
        else if( loadingState == LoadingState::Unloaded )
        {
            auto shape = getShape();
            if( !shape )
            {
                valid = true;
            }
        }

        return valid;
    }

    void PhysxBoxShape::createShape()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto physicsManager = (PhysxManager *)applicationManager->getPhysicsManagerPtr();
        WP_ASSERT( physicsManager );

        ScopedLock lock( this, true );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto task = Thread::getCurrentTask();
        auto physicsTask = physicsManager->getPhysicsTask();

        const auto &loadingState = getLoadingState();

        auto physics = physicsManager->getPhysics();
        WP_ASSERT( physics );

        auto extents = getExtents();

        // validate extents
        if( extents.X() <= 0 || extents.Y() <= 0 || extents.Z() <= 0 )
        {
            WP_LOG_ERROR( "Invalid extents for box shape" );
            extents = Vector3<physics_Num>( 1, 1, 1 );
        }

        auto halfExtents = extents / static_cast<physics_Num>( 2.0 );

        auto transform = getLocalPose();
        auto dimensions = PhysxUtil::toPx( halfExtents * transform.getScale() );

        physx::PxBoxGeometry geometry( dimensions );
        if( geometry.isValid() )
        {
            auto localPose = PhysxUtil::toPx( transform );

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
            WP_LOG_ERROR( "Invalid geometry for box shape" );
        }
    }

    auto PhysxBoxShape::createGeometry() -> physx::PxBoxGeometry
    {
        auto extents = getExtents() / static_cast<physics_Num>( 2.0 );
        auto dimensions = physx::PxVec3( extents.X(), extents.Y(), extents.Z() );
        return { dimensions };
    }

    bool PhysxBoxShape::handleStateChanged( SmartPtr<IState> &state )
    {
        if( isLoaded() )
        {
            auto returnValue = PhysxShape<BoxShape3>::handleStateChanged( state );

            auto stateData = state->getData();
            if( stateData )
            {
                // Actor scale is stored in ShapeStateData, independently of box extents.
                // Rebuild geometry for either change so a scaled ground matches its mesh.
                if( stateData->isDerived<BoxShapeStateData>() ||
                    stateData->isDerived<ShapeStateData>() )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    auto physicsManager = applicationManager->getPhysicsManagerPtr();

                    ScopedLock lock( this );

                    if( auto shape = getShape() )
                    {
                        auto attached = isAttached();
                        auto shapeActor = getPxActor();
                        if( !shapeActor )
                        {
                            if( auto body = getActor() )
                            {
                                if( auto rigidDynamic =
                                        workphone::dynamic_pointer_cast<PhysxRigidDynamic>( body ) )
                                {
                                    shapeActor = rigidDynamic->getActor();
                                }
                                else if( auto rigidStatic =
                                             workphone::dynamic_pointer_cast<PhysxRigidStatic>( body ) )
                                {
                                    shapeActor = rigidStatic->getRigidStatic();
                                }

                                setPxActor( shapeActor );
                            }
                        }
                        if( !shapeActor )
                        {
                            shapeActor = shape->getActor();
                        }

                        if( shapeActor )
                        {
                            shapeActor->detachShape( *shape, false );
                        }

                        auto transform = getLocalPose();
                        auto extents = getExtents() / static_cast<physics_Num>( 2.0 );
                        auto dimensions = PhysxUtil::toPx( extents * transform.getScale() );

                        auto geometry = physx::PxBoxGeometry( dimensions );
                        if( geometry.isValid() )
                        {
                            shape->setGeometry( geometry );
                        }

                        auto localPose = PhysxUtil::toPx( transform );
                        shape->setLocalPose( localPose );

                        if( attached )
                        {
                            if( shapeActor )
                            {
                                shapeActor->attachShape( *shape );
                            }
                        }

                        return true;
                    }
                }
            }
            return returnValue;
        }

        return false;
    }

    bool PhysxBoxShape::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        PhysxShape<BoxShape3>::handleStateChanged( message );

        auto type = message->getType();
        if( type == CREATE_SHAPE_HASH )
        {
            createShape();
            return true;
        }

        return false;
    }

} // namespace workphone::physics
