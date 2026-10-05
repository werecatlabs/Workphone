#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/Component.hpp>
#include <Workphone/Scene/Components/ComponentEventListener.hpp>
#include <Workphone/Scene/Components/SubComponent.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/Scene/IComponentEvent.hpp>
#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Interface/Scene/IGameManager.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/System/DebugUtil.hpp>

namespace workphone::scene
{

    WP_CLASS_REGISTER_DERIVED( workphone::scene, Component, Resource<IComponent> );
    WP_CLASS_REGISTER_DERIVED( workphone::scene, Component::ComponentFSMListener, FSMListener );

    u32 Component::m_idExt = 0;

    Component::Component()
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
        setEventTaskFlags( Thread::Application_Flag );

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    Component::Component( u32 poolTypeId ) : Resource<IComponent>( poolTypeId )
    {
        setObjectFlag( OBJECT_FLAG_GARBAGE_COLLECTED, true );
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS, true );
        setEventTaskFlags( Thread::Application_Flag );

#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif
    }

    Component::~Component()
    {
        //WP_ASSERT( m_componentFsmListener == nullptr );
    }

    void Component::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto sceneManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( sceneManager );

            auto typeManager = TypeManager::instance();

            const auto iTypeInfo = getTypeInfo();
            //const auto typeName = typeManager->getName( typeInfo );
            //const auto componentName = typeName ? String( typeName ) : String();
            //setName( componentName );

            setEnabled( true );

            auto fsmManager = sceneManager->getComponentFsmManager( iTypeInfo );
            WP_ASSERT( fsmManager );

            m_componentFSM = fsmManager->createFSM();

            auto componentFsmListener = factoryManager->make_ptr<ComponentFSMListener>();
            componentFsmListener->setOwner( this );
            m_componentFsmListener = componentFsmListener;

            m_componentFSM->addListener( m_componentFsmListener );

            auto className = typeManager->getName( iTypeInfo );

            if( data )
            {
                if( data->isDerived<Properties>() )
                {
                    auto properties = workphone::static_pointer_cast<Properties>( data );
                    setProperties( properties );
                }
            }

            if( auto actor = getActorPtr() )
            {
                auto actorName = String( " Actor: " ) + actor->getName();
                WP_LOG( String( "Component Loaded: " ) + className + actorName );
            }
        }
        catch( std::exception &e )
        {
            setLoadingState( LoadingState::Error );
            WP_LOG_EXCEPTION( e );
            throw;
        }
    }

    void Component::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( getLoadingState() == LoadingState::Unloaded )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            removeEvents();

            auto componentFSM = getFsm();
            if( componentFSM )
            {
                if( m_componentFsmListener )
                {
                    componentFSM->removeListener( m_componentFsmListener );
                }

                if( auto gameManager = applicationManager->getGameManagerPtr() )
                {
                    auto iTypeInfo = getTypeInfo();
                    if( auto fsmManager = gameManager->getComponentFsmManager( iTypeInfo ) )
                    {
                        fsmManager->destroyFSM( componentFSM );
                    }
                }

                setFsm( nullptr );
            }

            if( m_componentFsmListener )
            {
                m_componentFsmListener->unload( nullptr );
            }

            m_componentFsmListener = nullptr;

            for( auto &subComponent : m_children )
            {
                subComponent->unload( nullptr );
            }

            if( auto system = getComponentSystem() )
            {
                system->removeComponent( this );
            }

            Resource<IComponent>::unload( nullptr );

            m_actor = nullptr;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void Component::reload( SmartPtr<ISharedObject> data )
    {
        unload( nullptr );
        load( nullptr );
    }

    void Component::updateFlags( u32 flags, u32 oldFlags )
    {
        if( auto actor = getActorPtr() )
        {
            auto enabled = isEnabled() && actor->isEnabledInScene();
            auto args = Array<Parameter>{ Parameter( enabled ) };

            const auto inSceneChanged = BitUtil::getFlagValue( flags, IGameActor::ActorFlagInScene ) !=
                                        BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagInScene );
            const auto enabledChanged = BitUtil::getFlagValue( flags, IGameActor::ActorFlagEnabled ) !=
                                        BitUtil::getFlagValue( oldFlags, IGameActor::ActorFlagEnabled );

            if( getObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS ) && ( inSceneChanged || enabledChanged ) )
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                auto event = workphone::make_ptr<IEvent>();
                event->setTarget( this );
                applicationManager->triggerEvent( EventType::Scene, IEvent::enabled, args, this, this,
                                                  event );
            }
        }

        updateTransform();
        updateVisibility();

        updateComponentState();
    }

    u32 Component::getComponentFlags() const
    {
        return m_componentFlags;
    }

    void Component::setComponentFlags( u32 flags )
    {
        m_componentFlags = flags;
    }

    void Component::setComponentFlag( u32 flag, bool value )
    {
        u32 flags = m_componentFlags;
        if( value )
        {
            flags |= flag;
        }
        else
        {
            flags &= ~flag;
        }

        m_componentFlags = flags;
    }

    auto Component::getComponentFlag( u32 flag ) const -> bool
    {
        return ( m_componentFlags & flag ) != 0;
    }

    void Component::setEnabled( bool enabled )
    {
        const auto flags = getComponentFlags();
        m_componentFlags = BitUtil::setFlagValue( flags, ComponentEnabledFlag, enabled );

        updateVisibility();
    }

    auto Component::isEnabled() const -> bool
    {
        auto flags = getComponentFlags();
        return BitUtil::getFlagValue( flags, ComponentEnabledFlag );
    }

    auto Component::handleComponentEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
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
            case State::Edit:
            case State::Play:
            {
                updateTransform();
                updateVisibility();
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

    void Component::updateComponentState()
    {
        if( auto actor = getActorPtr() )
        {
            auto actorState = actor->getState();
            switch( actorState )
            {
            case IGameActor::State::Edit:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                if( applicationManager->isEditor() )
                {
                    if( !applicationManager->isPlaying() )
                    {
                        if( m_componentFSM )
                        {
                            m_componentFSM->setState( State::Edit );
                        }
                    }
                }
            }
            break;
            case IGameActor::State::Play:
            {
                auto applicationManager = core::IApplicationManager::instancePtr();
                if( applicationManager->isPlaying() )
                {
                    if( m_componentFSM )
                    {
                        m_componentFSM->setState( State::Play );
                    }
                }
            }
            break;
            }
        }
    }

    SmartPtr<IGameActor> Component::getActor() const
    {
        auto p = m_actor.load();
        return p.lock();
    }

    void Component::setActor( SmartPtr<IGameActor> actor )
    {
        m_actor = actor;
    }

    auto Component::toData() const -> SmartPtr<ISharedObject>
    {
        auto componentData = getProperties();

        if( auto handle = getHandle() )
        {
            auto uuid = handle->getUUIDAsString();
            if( StringUtil::isNullOrEmpty( uuid ) )
            {
                uuid = StringUtil::getUUID();
            }

            componentData->setProperty( "uuid", uuid );
        }

        auto subComponents = getSubComponents();
        for( auto &subComponent : subComponents )
        {
            if( auto subComponentData =
                    workphone::static_pointer_cast<Properties>( subComponent->toData() ) )
            {
                subComponentData->setName( "subComponent" );
                componentData->addChild( subComponentData );
            }
        }

        auto events = getEvents();
        for( auto &event : events )
        {
            auto eventData = workphone::make_ptr<Properties>();

            auto listeners = event->getListeners();
            for( auto &listener : listeners )
            {
                if( auto properties = listener->getProperties() )
                {
                    properties->setName( "listener" );
                    eventData->addChild( properties );
                }
            }

            eventData->setName( "event" );
            componentData->addChild( eventData );
        }

        return componentData;
    }

    void Component::fromData( SmartPtr<ISharedObject> data )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto componentData = workphone::static_pointer_cast<Properties>( data );

        auto name = String();
        componentData->getPropertyValue( "name", name );

        if( auto handle = getHandle() )
        {
            auto uuid = String();
            componentData->getPropertyValue( "uuid", uuid );

            if( StringUtil::isNullOrEmpty( uuid ) )
            {
                uuid = StringUtil::getUUID();
            }

            handle->setUUID( uuid );
        }

        //auto properties = componentData->getChild( "properties" );
        //if( !properties )
        //{
        //    properties = factoryManager->make_ptr<Properties>();

        //    if( auto oldStyleProperties = componentData->getChild( "properties_" ) )
        //    {
        //        auto childProperties = oldStyleProperties->getChildrenByName( "properties_" );
        //        for( auto child : childProperties )
        //        {
        //            Property property;
        //            property.setName( child->getProperty( "name" ) );
        //            property.setValue( child->getProperty( "value" ) );
        //            properties->addProperty( property );
        //        }
        //    }
        //}

        //sceneManager->queueProperties( this, data );
        setProperties( data );

        auto subComponentData = componentData->getChildrenByName( "subComponent" );

        auto components = Array<SmartPtr<ISubComponent>>();
        components.reserve( subComponentData.size() );

        for( auto &component : subComponentData )
        {
            auto componentType = String();
            component->getPropertyValue( "componentType", componentType );

            auto pComponent = factoryManager->createObjectFromType<ISubComponent>( componentType );
            if( !pComponent )
            {
                auto componentTypeClean = StringUtil::replaceAll( componentType, "fb::", "" );
                pComponent = factoryManager->createObjectFromType<ISubComponent>( componentTypeClean );
            }

            if( pComponent )
            {
                components.push_back( pComponent );
            }
        }

        for( auto &pComponent : components )
        {
            try
            {
                addSubComponent( pComponent );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        auto count = 0;
        for( auto pComponent : components )
        {
            try
            {
                if( pComponent )
                {
                    pComponent->setParent( this );

                    if( count < subComponentData.size() )
                    {
                        auto &pComponentData = subComponentData[count];
                        if( pComponentData )
                        {
                            pComponent->fromData( componentData );
                        }
                    }

                    count++;

                    pComponent->load( nullptr );
                }
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        auto eventsData = componentData->getChildrenByName( "event" );
        auto events = getEvents();

        count = 0;
        for( auto &rEvent : eventsData )
        {
            if( count >= events.size() )
            {
                break;
            }

            auto event = events[count];
            event->removeListeners();

            auto listenerData = rEvent->getChildrenByName( "listener" );

            for( auto &rListener : listenerData )
            {
                auto eventListener = factoryManager->make_ptr<ComponentEventListener>();
                eventListener->setEvent( event );

                sceneManager->queueProperties( eventListener, rListener );
                //eventListener->setProperties( pProperties );

                event->addListener( eventListener );
            }

            count++;
        }
    }

    auto Component::getChildObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        Array<SmartPtr<ISharedObject>> childObjects;
        childObjects.reserve( 12 );

        childObjects.emplace_back( getFsm() );
        childObjects.emplace_back( m_componentFsmListener );

        if( auto actor = getActor() )
        {
            childObjects.emplace_back( actor );
        }

        return childObjects;
    }

    auto Component::getProperties() const -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        WP_ASSERT( factoryManager );

        auto properties = factoryManager->make_ptr<Properties>();

        const auto enabled = isEnabled();
        properties->setProperty( enabledStr, enabled );

        const auto componentFlags = getComponentFlags();
        properties->setProperty( componentFlagsStr, componentFlags );

        auto iState = static_cast<s32>( getState() );
        properties->setPropertyAsEnum( stateStr, iState, componentStateNames );

        return properties;
    }

    void Component::setProperties( SmartPtr<Properties> properties )
    {
        Resource<IComponent>::setProperties( properties );
        if( !properties )
        {
            return;
        }

        auto enabled = isEnabled();
        properties->getPropertyValue( enabledStr, enabled );

        auto componentFlags = getComponentFlags();
        properties->getPropertyValue( componentFlagsStr, componentFlags );
        m_componentFlags = componentFlags;

        auto iState = static_cast<s32>( getState() );
        properties->getPropertyValue( stateStr, iState );

        setEnabled( enabled );
        updateStatic();

        auto eState = static_cast<State>( iState );
        if( getState() != eState )
        {
            setState( eState );
        }
        else
        {
            updateComponentState();
        }
    }

    void Component::updateTransform()
    {
    }

    void Component::updateTransform( const Transform3<real_Num> &transform )
    {
    }

    void Component::updateVisibility()
    {
    }

    void Component::updateOrder()
    {
    }

    void Component::updateMaterials()
    {
    }

    void Component::updateDependentComponents()
    {
    }

    void Component::setState( State state )
    {
        if( m_componentFSM )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto threadPool = applicationManager->getThreadPoolPtr();

            if( threadPool && threadPool->getNumThreads() > 0 )
            {
                auto changeNow = Thread::getTaskFlag( Thread::Application_Flag );
                m_componentFSM->setState( state, changeNow );
            }
            else
            {
                m_componentFSM->setState( state, true );
            }
        }
    }

    auto Component::getState() const -> IComponent::State
    {
        if( m_componentFSM )
        {
            return m_componentFSM->getState<State>();
        }

        return State::Count;
    }

    auto Component::getEvents() const -> Array<SmartPtr<IComponentEvent>>
    {
        return m_events;
    }

    void Component::setEvents( Array<SmartPtr<IComponentEvent>> events )
    {
        m_events = events;
    }

    void Component::addEvent( SmartPtr<IComponentEvent> event )
    {
        m_events.push_back( event );
    }

    void Component::removeEvent( SmartPtr<IComponentEvent> event )
    {
        auto it = std::find( m_events.begin(), m_events.end(), event );
        if( it != m_events.end() )
        {
            m_events.erase( it );
        }
    }

    void Component::removeEvents()
    {
        for( auto event : m_events )
        {
            event->unload( nullptr );
        }

        m_events.clear();
    }

    void Component::addSubComponent( SmartPtr<ISubComponent> child )
    {
        m_children.push_back( child );
    }

    void Component::removeSubComponent( SmartPtr<ISubComponent> child )
    {
        child->setParent( nullptr );
        m_children.erase( std::remove( m_children.begin(), m_children.end(), child ), m_children.end() );
    }

    void Component::removeSubComponentByIndex( u32 index )
    {
        auto it = m_children.begin();
        std::advance( it, index );

        if( it != m_children.end() )
        {
            m_children.erase( it );
        }
    }

    auto Component::getNumSubComponents() const -> u32
    {
        return (u32)m_children.size();
    }

    auto Component::getSubComponentByIndex( u32 index ) const -> SmartPtr<ISubComponent>
    {
        return m_children.at( index );
    }

    auto Component::getSubComponents() const -> Array<SmartPtr<ISubComponent>>
    {
        return m_children.snapshot();
    }

    void Component::setSubComponents( const Array<SmartPtr<ISubComponent>> &components )
    {
        m_children = { components.begin(), components.end() };
    }

    SmartPtr<IComponent> Component::getParent() const
    {
        auto p = m_parent.load();
        return p.lock();
    }

    void Component::setParent( SmartPtr<IComponent> parent )
    {
        m_parent = parent;
    }

    Component::ComponentFSMListener::ComponentFSMListener() = default;

    Component::ComponentFSMListener::~ComponentFSMListener() = default;

    void Component::ComponentFSMListener::unload( SmartPtr<ISharedObject> data )
    {
        WP_ASSERT( isLoadLocked() == false );
        setLoadingState( LoadingState::Unloading );
        m_owner = nullptr;
        FSMListener::unload( nullptr );
        setLoadingState( LoadingState::Unloaded );
    }

    auto Component::ComponentFSMListener::handleEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
            {
                return owner->handleComponentEvent( state, eventType );
            }
        }

        return FSMReturnType::NotHandled;
    }

    auto Component::ComponentFSMListener::getOwner() const -> SmartPtr<Component>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Component::ComponentFSMListener::setOwner( SmartPtr<Component> owner )
    {
        m_owner = owner;
    }

    auto Component::compareTag( const String &tag ) const -> bool
    {
        if( auto actor = getActor() )
        {
            return actor->compareTag( tag );
        }

        return false;
    }

    auto Component::handleEvent( EventType eventType, hash_type eventValue,
                                 const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                 SmartPtr<ISharedObject> object, SmartPtr<IEvent> event ) -> Parameter
    {
        if( eventValue == IComponent::actorUnload )
        {
            if( auto actor = getActor() )
            {
                if( sender == actor )
                {
                    setActor( nullptr );
                }
            }
        }
        else if( eventValue == IComponent::hierarchyChanged )
        {
            updateTransform();
            updateComponentState();
        }
        else if( eventValue == IComponent::actorFlagsChanged )
        {
            auto oldFlags = arguments[0].getU32();
            auto newFlags = arguments[1].getU32();

            updateFlags( newFlags, oldFlags );
        }
        else if( eventValue == IEvent::addActor )
        {
            updateComponentState();
        }
        else if( eventValue == IEvent::loadScene )
        {
            updateComponentState();
        }

        return {};
    }

    void Component::addState( SmartPtr<ISharedObject> state )
    {
        m_componentStates.push_back( state );
    }

    void Component::removeState( SmartPtr<ISharedObject> state )
    {
        m_componentStates.erase(
            std::remove( m_componentStates.begin(), m_componentStates.end(), state ),
            m_componentStates.end() );
    }

    SmartPtr<ISharedObject> &Component::getStateByTypeId( u32 typeId )
    {
        for( auto &state : m_componentStates )
        {
            if( state->derived( typeId ) )
            {
                return state;
            }
        }

        static SmartPtr<ISharedObject> nullState;
        return nullState;
    }

    const SmartPtr<ISharedObject> &Component::getStateByTypeId( u32 typeId ) const
    {
        for( auto &state : m_componentStates )
        {
            if( state->derived( typeId ) )
            {
                return state;
            }
        }

        static SmartPtr<ISharedObject> nullState;
        return nullState;
    }

    Array<SmartPtr<ISharedObject>> Component::getStates() const
    {
        return m_componentStates.snapshot();
    }

    AABB3<real_Num> Component::getBoundingBox() const
    {
        return AABB3<real_Num>();
    }

    void Component::setComponentSystem( SmartPtr<IComponentSystem> componentSystem )
    {
        m_componentSystem = componentSystem;
    }

    SmartPtr<IComponentSystem> Component::getComponentSystem() const
    {
        auto p = m_componentSystem.load();
        return p.lock();
    }

    IComponentSystem *Component::getComponentSystemPtr() const
    {
        return m_componentSystem.get();
    }

    void Component::setComponentSystemData( void *componentSystemData )
    {
        m_componentSystemData = componentSystemData;
    }

    void *Component::getComponentSystemData() const
    {
        return m_componentSystemData.load();
    }

    void Component::updateSmoothTransformState()
    {
    }

    SmartPtr<IFSM> Component::getFsm() const
    {
        return m_componentFSM.lock();
    }

    void Component::setFsm( SmartPtr<IFSM> fsm )
    {
        m_componentFSM = fsm;
    }

    void Component::updateStatic()
    {
    }

    void Component::lock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->lock();
    }

    bool Component::try_lock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        return gameManager->try_lock();
    }

    void Component::unlock()
    {
        auto applicationManger = core::IApplicationManager::instancePtr();
        auto gameManager = applicationManger->getGameManagerPtr();
        gameManager->unlock();
    }

}  // namespace workphone::scene
