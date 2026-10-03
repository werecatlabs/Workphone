#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Scene/Components/Script.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Interface/Scene/IComponentEvent.hpp>
#include <Workphone/Interface/Scene/IComponentEventListener.hpp>
#include <Workphone/Interface/Script/IScriptInvoker.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone::scene
{

    const String ComponentEventListener::actorStr = String( "actor" );
    const String ComponentEventListener::functionStr = String( "function" );
    const String ComponentEventListener::componentStr = String( "component" );

    ComponentEventListener::ComponentEventListener() = default;

    ComponentEventListener::~ComponentEventListener() = default;

    auto ComponentEventListener::handleEvent( EventType eventType, hash_type eventValue,
                                              const Array<Parameter> &arguments,
                                              SmartPtr<ISharedObject> sender,
                                              SmartPtr<ISharedObject> object, SmartPtr<IEvent> event )
        -> Parameter
    {
        if( auto event = getEvent() )
        {
            if( eventValue == event->getEventHash() )
            {
                if( auto component = getComponent() )
                {
                    if( component->isDerived<Script>() )
                    {
                        if( auto actor = component->getActor() )
                        {
                            if( actor->isEnabledInScene() )
                            {
                                auto userComponent = workphone::static_pointer_cast<Script>( component );

#if !WP_FINAL
                                auto className = userComponent->getClassName();
                                if( StringUtil::isNullOrEmpty( className ) )
                                {
                                    WP_LOG( "Class name null." );
                                }
#endif

                                if( userComponent->getState() == IComponent::State::Edit )
                                {
                                    if( userComponent->getUpdateInEditMode() )
                                    {
                                        if( auto invoker = userComponent->getInvoker() )
                                        {
                                            const auto functionName = getFunction();
                                            invoker->callObjectMember( functionName );
                                        }
                                    }
                                }
                                else
                                {
                                    if( auto invoker = userComponent->getInvoker() )
                                    {
                                        const auto functionName = getFunction();
                                        invoker->callObjectMember( functionName );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return {};
    }

    auto ComponentEventListener::getComponent() const -> SmartPtr<IComponent>
    {
        return m_component.lock();
    }

    void ComponentEventListener::setComponent( SmartPtr<IComponent> component )
    {
        m_component = component;
    }

    auto ComponentEventListener::getFunction() const -> String
    {
        return m_function;
    }

    void ComponentEventListener::setFunction( const String &function )
    {
        m_function = function;
    }

    auto ComponentEventListener::toData() const -> SmartPtr<ISharedObject>
    {
        return nullptr;
    }

    void ComponentEventListener::fromData( SmartPtr<ISharedObject> data )
    {
    }

    auto ComponentEventListener::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = workphone::make_ptr<Properties>();
        WP_ASSERT( properties );

        auto actor = getActor();
        auto component = getComponent();
        auto function = getFunction();

        properties->setProperty( actorStr, actor );
        properties->setProperty( componentStr, component );
        properties->setProperty( functionStr, function );

        return properties;
    }

    void ComponentEventListener::setProperties( SmartPtr<Properties> properties )
    {
        SmartPtr<IComponent> component;
        String function = getFunction();

        SmartPtr<IGameActor> actor;
        properties->getPropertyValue( actorStr, actor );

        properties->getPropertyValue( componentStr, component );
        properties->getPropertyValue( functionStr, function );

        setFunction( function );

        if( m_actor != actor )
        {
            m_actor = actor;

            if( m_actor )
            {
                auto userComponent = m_actor->getComponent<Script>();
                m_component = userComponent;
            }
        }
        else
        {
            m_component = component;
        }
    }

    auto ComponentEventListener::getEvent() const -> SmartPtr<IComponentEvent>
    {
        return m_event.lock();
    }

    void ComponentEventListener::setEvent( SmartPtr<IComponentEvent> event )
    {
        m_event = event;
    }

    void ComponentEventListener::setActor( SmartPtr<IGameActor> actor )
    {
        m_actor = actor;
    }

    SmartPtr<IGameActor> ComponentEventListener::getActor() const
    {
        return m_actor.lock();
    }

}  // namespace workphone::scene
