#include <WPProcedural/WPProceduralPCH.hpp>
#include <WPProcedural/ProceduralComponent.hpp>

namespace workphone
{
    namespace procedural
    {

        WP_CLASS_REGISTER_DERIVED( workphone::procedural, ProceduralComponent, scene::Component );

        ProceduralComponent::ProceduralComponent() = default;

        ProceduralComponent::~ProceduralComponent() = default;

        void ProceduralComponent::load( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Loaded );
        }

        void ProceduralComponent::unload( SmartPtr<ISharedObject> data )
        {
            setLoadingState( LoadingState::Unloaded );
        }

        SmartPtr<Properties> ProceduralComponent::getProperties() const
        {
            return nullptr;
        }

        void ProceduralComponent::setProperties( SmartPtr<Properties> properties )
        {
        }

        SmartPtr<ISharedObject> ProceduralComponent::getObject() const
        {
            return m_object;
        }

        void ProceduralComponent::setObject( SmartPtr<ISharedObject> object )
        {
            m_object = object;
        }

        FSMReturnType ProceduralComponent::handleComponentEvent( u32 state, FSMEvent eventType )
        {
            Component::handleComponentEvent( state, eventType );

            switch( eventType )
            {
            case FSMEvent::Change:
            {
            }
            break;
            case FSMEvent::Enter:
            {
                auto eState = static_cast<State>( state );
                switch( eState )
                {
                case State::Destroyed:
                {
                }
                break;
                case State::Edit:
                case State::Play:
                {
                    if (m_objectType == 0)
                    {

                    }
                }
                break;
                default:
                {
                }
                }
            }
            break;
            case FSMEvent::Leave:
            {
                auto eState = static_cast<State>( state );
                switch( eState )
                {
                case State::Edit:
                case State::Play:
                {
                }
                break;
                default:
                {
                }
                }
            }
            break;
            case FSMEvent::Pending:
            {
            }
            break;
            case FSMEvent::Complete:
            {
            }
            break;
            case FSMEvent::NewState:
            {
            }
            break;
            case FSMEvent::WaitForChange:
            {
            }
            break;
            default:
            {
            }
            break;
            }

            return FSMReturnType::Ok;
        }

    }  // namespace procedural
}  // namespace workphone
