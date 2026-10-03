#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/FiniteStateMachine.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, FiniteStateMachine, Component );

    const String FiniteStateMachine::newStateStr = String( "newState" );
    const String FiniteStateMachine::previousStateStr = String( "previousState" );
    const String FiniteStateMachine::currentStateStr = String( "currentState" );

    FiniteStateMachine::FiniteStateMachine() = default;

    FiniteStateMachine::~FiniteStateMachine()
    {
        unload( nullptr );
    }

    void FiniteStateMachine::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto sceneManager = applicationManager->getGameManager();
            WP_ASSERT( sceneManager );

            auto fsmManager = sceneManager->getFsmManager();
            WP_ASSERT( fsmManager );

            m_fsm = fsmManager->createFSM();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FiniteStateMachine::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fsmManager = applicationManager->getFsmManager();

                if( fsmManager )
                {
                    if( m_fsm )
                    {
                        // if( m_componentFsmListener )
                        //{
                        //     m_componentFSM->removeListener( m_componentFsmListener );
                        //     m_componentFsmListener = nullptr;
                        // }

                        fsmManager->destroyFSM( m_fsm );
                        m_fsm = nullptr;
                    }
                }

                Component::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void FiniteStateMachine::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( data );
            load( data );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto FiniteStateMachine::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto objects = Array<SmartPtr<ISharedObject>>();
        objects.reserve( 1 );

        objects.emplace_back( m_fsm );
        return objects;
    }

    auto FiniteStateMachine::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Component::getProperties();

        if( m_fsm )
        {
            s32 newState = m_fsm->getNewState();
            properties->setProperty( newStateStr, newState );

            s32 previousState = m_fsm->getPreviousState();
            properties->setProperty( previousStateStr, previousState );

            s32 currentState = m_fsm->getCurrentState();
            properties->setProperty( currentStateStr, currentState );
        }

        return properties;
    }

    void FiniteStateMachine::setProperties( SmartPtr<Properties> properties )
    {
        if( m_fsm )
        {
            s32 newState = m_fsm->getNewState();
            properties->getPropertyValue( newStateStr, newState );

            s32 previousState = m_fsm->getPreviousState();
            properties->getPropertyValue( previousStateStr, previousState );

            s32 currentState = m_fsm->getCurrentState();
            properties->getPropertyValue( currentStateStr, currentState );

            if( m_fsm->getNewState() != newState )
            {
                m_fsm->setNewState( newState );
            }
        }
    }
}  // namespace workphone::scene
