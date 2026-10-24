#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <array>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, EventJob, Job );

    EventJob::EventJob() = default;

    EventJob::~EventJob()
    {
        m_event = nullptr;
        m_object = nullptr;
        m_sender = nullptr;
        m_owner = nullptr;
        std::atomic_store(&m_arguments, std::shared_ptr<const Array<Parameter>>{});
    }

    void EventJob::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        Job::load( data );
        setLoadingState( LoadingState::Loaded );
    }

    void EventJob::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_event = nullptr;
        m_object = nullptr;
        m_sender = nullptr;
        m_owner = nullptr;
        std::atomic_store(&m_arguments, std::shared_ptr<const Array<Parameter>>{});

        Job::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    void EventJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "EventJob::execute: null application manager." );
            return;
        }

        auto sender = getSender();
        auto object = getObject();

        auto eventType = getEventType();
        auto eventValue = getEventValue();
        auto retainedArguments = std::atomic_load(&m_arguments);
        const Array<Parameter> emptyArguments;
        const auto &arguments = retainedArguments ? *retainedArguments : emptyArguments;
        auto event = getEvent();

        // A listener can be registered globally and on the sender/object at the same time. Treat
        // one EventJob execution as one delivery so those overlapping routes do not invoke it
        // repeatedly. Snapshots keep listener removal during a callback safe.
        std::array<ISharedObject::ObjectListenerSnapshot, 3> routes;
        size_t routeCount = 0;
        auto dispatch = [&]( ISharedObject::ObjectListenerSnapshot listeners ) {
            const auto route = routeCount++;
            routes[route] = std::move(listeners);
            if(!routes[route]) return;
            for( auto listener : *routes[route] )
            {
                bool delivered = false;
                for(size_t previous = 0; previous < route && !delivered; ++previous)
                    if(routes[previous]) delivered = std::find(routes[previous]->begin(),
                        routes[previous]->end(), listener) != routes[previous]->end();
                if( !listener || delivered )
                {
                    continue;
                }

                try
                {
                    listener->handleEvent( eventType, eventValue, arguments, sender, object, event );
                }
                catch( std::exception &e )
                {
                    // A faulty listener must not prevent the remaining subscribers from receiving
                    // a critical engine event.
                    WP_LOG_EXCEPTION( e );
                }
                catch( ... )
                {
                    WP_LOG_ERROR( "EventJob::execute: event listener threw an unknown exception." );
                }
            }
        };

        // A null sender is an explicit anonymous broadcast. Otherwise the sender must opt in to
        // application-wide delivery with OBJECT_FLAG_GLOBAL_EVENTS.
        if( !sender || sender->getObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS ) )
        {
            dispatch( applicationManager->getObjectListenerSnapshot() );
        }

        if( sender )
        {
            if( sender->getObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS ) )
            {
                dispatch( sender->getObjectListenerSnapshot() );
            }
        }

        if( object && object->getObjectFlag( OBJECT_FLAG_RECEIVE_EVENTS ) )
        {
            dispatch( object->getObjectListenerSnapshot() );
        }
    }

    auto EventJob::getObject() const -> SmartPtr<ISharedObject>
    {
        return m_object;
    }

    void EventJob::setObject( SmartPtr<ISharedObject> object )
    {
        m_object = object;
    }

    auto EventJob::getSender() const -> SmartPtr<ISharedObject>
    {
        return m_sender;
    }

    void EventJob::setSender( SmartPtr<ISharedObject> sender )
    {
        m_sender = sender;
    }

    auto EventJob::getOwner() const -> IStateContext *
    {
        return m_owner.get();
    }

    void EventJob::setOwner( IStateContext *owner )
    {
        m_owner = owner;
    }

    auto EventJob::getEventType() const -> EventType
    {
        return m_eventType;
    }

    void EventJob::setEventType( EventType eventType )
    {
        m_eventType = eventType;
    }

    auto EventJob::getEventValue() const -> hash_type
    {
        return m_eventValue;
    }

    void EventJob::setEventValue( hash_type eventValue )
    {
        m_eventValue = eventValue;
    }

    auto EventJob::getArguments() const -> Array<Parameter>
    {
        auto arguments = std::atomic_load(&m_arguments);
        return arguments ? *arguments : makeSourceBudgetSnapshot<Parameter>();
    }

    void EventJob::setArguments( const Array<Parameter> &arguments )
    {
        std::shared_ptr<const Array<Parameter>> next;
        if(!arguments.empty())
            next = std::allocate_shared<Array<Parameter>>(SourceBudgetAllocator<Array<Parameter>>{},
                                                          makeSourceBudgetSnapshot(arguments));
        std::atomic_store(&m_arguments, std::move(next));
    }

    auto EventJob::getEvent() const -> SmartPtr<IEvent>
    {
        return m_event;
    }

    void EventJob::setEvent( SmartPtr<IEvent> event )
    {
        m_event = event;
    }
}  // namespace workphone
