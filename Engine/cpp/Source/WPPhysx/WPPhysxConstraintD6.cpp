#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxConstraintD6.hpp>
#include <WPPhysx/WPPhysxRigidDynamic.hpp>
#include <WPPhysx/WPPhysxManager.hpp>
#include "WPPhysx/PhysxUtil.hpp"
#include <Workphone/Workphone.hpp>
#include <PxPhysicsAPI.h>
#include <PxExtensionsAPI.h>

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxConstraintD6, PhysxConstraint<ConstraintD6> );

    PhysxConstraintD6::PhysxConstraintD6()
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

        auto state = factoryManager->make_ptr<State>();
        stateContext->addState( state );

        auto stateData = factoryManager->make_ptr<ConstraintStateData>();
        state->setData( stateData );

        auto stateD6 = factoryManager->make_ptr<State>();
        stateContext->addState( stateD6 );

        auto stateD6Data = factoryManager->make_ptr<ConstraintD6StateData>();
        stateD6->setData( stateD6Data );

        auto physicsTask = TaskId::Physics;
        stateContext->setTaskId( physicsTask );
    }

    PhysxConstraintD6::~PhysxConstraintD6()
    {
        unload( nullptr );
    }

    void PhysxConstraintD6::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            using namespace physx;

            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            auto physicsManager =
                workphone::static_pointer_cast<PhysxManager>( applicationManager->getPhysicsManager() );

            auto physics = physicsManager->getPhysics();
            auto j = workphone::make_ptr<PhysxConstraintD6>();

            RawPtr<PxRigidDynamic> pxActor0;
            RawPtr<PxRigidDynamic> pxActor1;

            if( auto actor0 = getBodyA() )
            {
                auto pActor0 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor0 );
                pxActor0 = pActor0->getActor();
            }

            if( auto actor1 = getBodyB() )
            {
                auto pActor1 = workphone::static_pointer_cast<PhysxRigidDynamic>( actor1 );
                pxActor1 = pActor1->getActor();
            }

            auto localFrame0 = getLocalPose( JointActorIndexEnum::eACTOR0 );
            auto localFrame1 = getLocalPose( JointActorIndexEnum::eACTOR1 );

            auto pxLocalFrame0 = PhysxUtil::toPx( localFrame0 );
            auto pxLocalFrame1 = PhysxUtil::toPx( localFrame1 );

            auto d6joint = PxD6JointCreate( *physics, pxActor0, pxLocalFrame0, pxActor1, pxLocalFrame1 );
            setJoint( d6joint );

            setLoadingState( LoadingState::Loaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxConstraintD6::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( auto joint = getJoint() )
            {
                joint->release();
                setJoint( nullptr );
            }

            setLoadingState( LoadingState::Unloaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxConstraintD6::setDrivePosition( const Transform3<physics_Num> &pose )
    {
        m_drivePosition = pose;
    }

    auto PhysxConstraintD6::getDrivePosition() const -> Transform3<physics_Num>
    {
        return m_drivePosition;
    }

    void PhysxConstraintD6::setDrive( D6DriveEnum index, SmartPtr<IConstraintDrive> drive )
    {
        if( auto joint = (physx::PxD6Joint *)getJoint() )
        {
            joint->setDrive( static_cast<physx::PxD6Drive::Enum>( index ),
                             physx::PxD6JointDrive( drive->getStiffness(), drive->getDamping(),
                                                    drive->getForceLimit(), drive->isAcceleration() ) );
        }
    }

    auto PhysxConstraintD6::getDrive( D6DriveEnum index ) const -> SmartPtr<IConstraintDrive>
    {
        return nullptr;
    }

    void PhysxConstraintD6::setLinearLimit( SmartPtr<IConstraintLinearLimit> limit )
    {
        if( auto joint = (physx::PxD6Joint *)getJoint() )
        {
            auto                      value = limit->getValue();
            physx::PxJointLinearLimit pxLimit( physx::PxTolerancesScale(), ( value ) );
            joint->setLinearLimit( pxLimit );
        }
    }

    auto PhysxConstraintD6::getLinearLimit() const -> SmartPtr<IConstraintLinearLimit>
    {
        if( auto joint = (physx::PxD6Joint *)getJoint() )
        {
            return nullptr;
        }

        return nullptr;
    }

    void PhysxConstraintD6::setMotion( D6AxisEnum axis, D6MotionEnum type )
    {
        if( auto joint = (physx::PxD6Joint *)getJoint() )
        {
            joint->setMotion( static_cast<physx::PxD6Axis::Enum>( axis ),
                              static_cast<physx::PxD6Motion::Enum>( type ) );
        }
    }

    auto PhysxConstraintD6::getMotion( D6AxisEnum axis ) const -> D6MotionEnum
    {
        if( auto joint = (physx::PxD6Joint *)getJoint() )
        {
            auto motion = joint->getMotion( static_cast<physx::PxD6Axis::Enum>( axis ) );
            return static_cast<D6MotionEnum>( motion );
        }

        return static_cast<D6MotionEnum>( 0 );
    }

    void PhysxConstraintD6::handleStateChanged( SmartPtr<IState> &state )
    {
        using namespace physx;

        PhysxConstraint<ConstraintD6>::handleStateChanged( state );

        auto stateData = state->getData();
        if( stateData->isDerived<ConstraintD6StateData>() )
        {
            auto constraintD6StateData =
                workphone::static_pointer_cast<ConstraintD6StateData>( stateData );
        }
    }

    void PhysxConstraintD6::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        PhysxConstraint<ConstraintD6>::handleStateChanged( message );
    }

} // namespace workphone::physics
