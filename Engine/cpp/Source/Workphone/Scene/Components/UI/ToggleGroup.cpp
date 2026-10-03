#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/UI/ToggleGroup.hpp>
#include <Workphone/Scene/Components/UI/Toggle.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, ToggleGroup, UIComponent );

    ToggleGroup::ToggleGroup() = default;

    ToggleGroup::~ToggleGroup() = default;

    void ToggleGroup::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            createUI();
            UIComponent::load( data );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ToggleGroup::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                if( auto actor = getActor() )
                {
                    auto children = actor->getChildren();
                    for( auto &child : children )
                    {
                        if( auto toggle = child->getComponent<Toggle>() )
                        {
                            toggle->removeListener( this );
                        }
                    }
                }

                UIComponent::unload( data );

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    FSMReturnType ToggleGroup::handleComponentEvent( u32 state, FSMEvent eventType )
    {
        UIComponent::handleComponentEvent( state, eventType );

        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Edit:
            case State::Play:
            {
                createUI();
            }
            break;
            default:
                break;
            }
        }
        break;
        default:
            break;
        }

        return FSMReturnType::Ok;
    }

    void ToggleGroup::createUI()
    {
        try
        {
            auto state = getState();
            switch( state )
            {
            case State::Edit:
            case State::Play:
            {
                auto actor = getActor();
                if( !actor )
                {
                    WP_LOG_ERROR( "Actor is null" );
                    return;
                }

                auto children = actor->getChildren();
                for( auto &child : children )
                {
                    if( auto toggle = child->getComponent<Toggle>() )
                    {
                        toggle->addListener( this );
                    }
                }
            }
            break;
            default:
                break;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Parameter ToggleGroup::handleEvent( EventType eventType, hash_type eventValue,
                                        const Array<Parameter> &arguments,
                                        SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                        SmartPtr<IEvent> event )
    {
        ScopedLock lock( this );

        if( eventValue == IEvent::CLICK_HASH )
        {
            if( auto toggle = workphone::dynamic_pointer_cast<Toggle>( object ) )
            {
                // When a toggle is clicked, ensure it's the only one selected
                auto actor = getActor();
                auto children = actor->getChildren();
                for( auto &child : children )
                {
                    if( auto otherToggle = child->getComponent<Toggle>() )
                    {
                        if( otherToggle != toggle )
                        {
                            otherToggle->setToggled( false );
                        }
                    }
                }
            }
        }

        return UIComponent::handleEvent( eventType, eventValue, arguments, sender, object, event );
    }

}  // namespace workphone::scene
