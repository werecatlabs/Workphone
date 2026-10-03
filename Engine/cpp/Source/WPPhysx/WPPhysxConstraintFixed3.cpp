#include <WPPhysx/WPPhysxPCH.hpp>
#include <WPPhysx/WPPhysxConstraintFixed3.hpp>
#include <Workphone/Workphone.hpp>
#include "extensions/PxFixedJoint.h"

namespace workphone::physics
{

    WP_CLASS_REGISTER_DERIVED( workphone::physics, PhysxConstraintFixed3,
                               PhysxConstraint<ConstraintFixed3> );

    PhysxConstraintFixed3::PhysxConstraintFixed3()
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

        auto stateFixed = factoryManager->make_ptr<State>();
        stateContext->addState( stateFixed );

        auto stateFixedData = factoryManager->make_ptr<ConstraintFixedStateData>();
        stateFixed->setData( stateFixedData );

        auto physicsTask = TaskId::Physics;
        stateContext->setTaskId( physicsTask );
    }

    PhysxConstraintFixed3::~PhysxConstraintFixed3()
    {
        unload( nullptr );
    }

    void PhysxConstraintFixed3::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            PhysxConstraint<ConstraintFixed3>::load( data );
            setLoadingState( LoadingState::Loaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxConstraintFixed3::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( m_joint )
            {
                m_joint->release();
                m_joint = nullptr;
            }

            PhysxConstraint<ConstraintFixed3>::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void PhysxConstraintFixed3::handleStateChanged( SmartPtr<IState> &state )
    {
        PhysxConstraint<ConstraintFixed3>::handleStateChanged( state );
    }

    void PhysxConstraintFixed3::handleStateChanged( const SmartPtr<IStateMessage> &message )
    {
        PhysxConstraint<ConstraintFixed3>::handleStateChanged( message );
    }

} // namespace workphone::physics
