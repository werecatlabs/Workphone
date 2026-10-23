#include <Workphone/Workphone.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/State/States/State.hpp>
#include <Workphone/State/Messages/StateMessage.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/FactoryManager.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/System/FactoryTemplate.hpp>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <functional>

using namespace workphone;
namespace
{
    void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }
    class Listener : public IStateListener
    {
    public:
        bool handleStateMessage(const SmartPtr<IStateMessage> &) override { return true; }
        bool handleStateChanged(SmartPtr<IState> &) override { ++notifications; return true; }
        unsigned notifications = 0;
    };
    class EventListener : public IEventListener
    {
    public:
        Parameter handleEvent( EventType, hash_type, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject>,
                               SmartPtr<IEvent> ) override { return {}; }
    };
    class DirtyState : public State
    {
    public:
        bool isDirty() const override { return dirty; }
        void setDirty(bool value) override { dirty = value; }
        bool dirty = true;
    };
    class InventoryListener : public IStateListener
    {
    public:
        bool handleStateMessage(const SmartPtr<IStateMessage> &) override { return true; }
        bool handleStateChanged(SmartPtr<IState> &) override
        {
            ++notifications;
            if(onFirst) { auto callback = std::move(onFirst); callback(); }
            return true;
        }
        unsigned notifications = 0;
        std::function<void()> onFirst;
    };

    class QueueContext : public StateContext
    {
    public:
        size_t queueCount() const { return m_stateQueues.size(); }
        void installQueue( SmartPtr<IStateQueue> queue ) { appendStateQueue( queue ); }
    };
    class QueueMessage : public StateMessage
    {
    public:
        std::atomic<unsigned> unloads{ 0 };
        void unload( SmartPtr<ISharedObject> data ) override
        {
            ++unloads;
            StateMessage::unload( data );
        }
    };
    class QueueListener : public IStateListener
    {
    public:
        unsigned messages = 0;
        bool handleStateMessage( const SmartPtr<IStateMessage> & ) override { ++messages; return true; }
        bool handleStateChanged( SmartPtr<IState> & ) override { return true; }
    };
    class ReentrantStateManager : public StateManager
    {
    public:
        using StateManager::addDirty;
        std::atomic<unsigned> registrations{ 0 };
        void addDirty( SmartPtr<IStateContext> context, TaskId task ) override
        {
            if( auto concrete = dynamic_cast<StateContext *>( context.get() ) )
                concrete->getStateQueue( static_cast<u32>( task ) );
            ++registrations;
            StateManager::addDirty( context, task );
        }
    };
    class NotifyingQueueFactory : public FactoryTemplate<StateQueue>
    {
    public:
        void *createObject() override
        {
            auto object = static_cast<StateQueue *>( FactoryTemplate<StateQueue>::createObject() );
            object->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
            object->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
            object->setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
            return object;
        }
    };
    class DerivedQueue : public StateQueue
    {
    public:
        QueueContext *context = nullptr;
        std::atomic<unsigned> loads{ 0 }, taskAssignments{ 0 }, writes{ 0 };
        mutable std::atomic<unsigned> taskReads{ 0 };
        std::atomic<bool> enqueueLookupSucceeded{ false };
        void load( SmartPtr<ISharedObject> data ) override
        {
            ++loads;
            if( context ) context->getStateQueue( m_task );
            StateQueue::load( data );
        }
        void unload( SmartPtr<ISharedObject> data ) override { clear(); StateQueue::unload( data ); }
        void setTaskId( u32 task ) override
        {
            ++taskAssignments; m_task = task;
            if( context ) context->getStateQueue( task );
        }
        u32 getTaskId() const override
        {
            ++taskReads;
            if( context ) context->queueCount();
            return m_task;
        }
        void queueMessage( const SmartPtr<IStateMessage> &message ) override
        {
            ++writes;
            enqueueLookupSucceeded = context && context->getStateQueue( m_task ).get() == this;
            m_messages.push( message );
        }
        bool isEmpty() const override { return m_messages.empty(); }
        Array<SmartPtr<IStateMessage>> getMessages() const override { return {}; }
        Array<SmartPtr<IStateMessage>> getMessagesAndClear() override
        {
            Array<SmartPtr<IStateMessage>> result; SmartPtr<IStateMessage> message;
            while( m_messages.try_pop( message ) ) result.push_back( message );
            return result;
        }
        void clear() override
        {
            auto messages = getMessagesAndClear();
            for( auto &message : messages ) message->unload( nullptr );
        }
    private:
        // A separate task id and buffer make bypassed virtual methods observable.
        u32 m_task = static_cast<u32>( TaskId::None );
        ConcurrentQueue<SmartPtr<IStateMessage>> m_messages;
    };
    class DerivedQueueFactory : public FactoryTemplate<DerivedQueue>
    {
    public:
        QueueContext *context = nullptr;
        void *createObject() override
        {
            auto object = static_cast<DerivedQueue *>( FactoryTemplate<DerivedQueue>::createObject() );
            object->context = context;
            object->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
            object->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
            object->setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );
            return object;
        }
    };
    struct QueueFixture
    {
        TaskId previousTask = Thread::getCurrentTask();
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();
        SmartPtr<ReentrantStateManager> states = make_ptr<ReentrantStateManager>();
        SmartPtr<State> owner = make_ptr<State>();
        SmartPtr<DerivedQueueFactory> derivedFactory;
        explicit QueueFixture( bool derivedQueues = false )
        {
            core::IApplicationManager::setInstance( app );
            factories->load( nullptr ); app->setFactoryManager( factories );
            // Replace before StateManager owns any queues. This deliberately
            // enables initialization events to exercise the internal protocol.
            if( auto old = factories->getFactoryById( StateQueue::typeInfo() ) ) factories->removeFactory( old );
            SmartPtr<IFactory> queues;
            if( derivedQueues ) { derivedFactory = make_ptr<DerivedQueueFactory>(); queues = derivedFactory; }
            else queues = make_ptr<NotifyingQueueFactory>();
            queues->setObjectTypeName( TypeManager::instance()->getName( StateQueue::typeInfo() ) );
            queues->setObjectTypeHash( TypeManager::instance()->getHash( StateQueue::typeInfo() ) );
            factories->addFactory( queues );
            app->setStateManager( states ); states->load( nullptr );
            owner->setLoadingState( LoadingState::Loaded );
            app->setLoadingState( LoadingState::Loaded );
            Thread::setCurrentTask( TaskId::Primary );
        }
        ~QueueFixture()
        {
            app->setLoadingState( LoadingState::Unloaded );
            states->unload( nullptr ); app->setStateManager( nullptr ); states = nullptr;
            owner = nullptr;
            app->setFactoryManager( nullptr );
            factories->unload( nullptr ); factories = nullptr;
            core::IApplicationManager::setInstance( nullptr ); app = nullptr;
            Thread::setCurrentTask( previousTask );
        }
        SmartPtr<QueueContext> context()
        {
            auto context = make_ptr<QueueContext>();
            context->setOwner( owner ); context->load( nullptr );
            require( context->isLoaded(), "Queue fixture context loads" );
            return context;
        }
        SmartPtr<QueueMessage> message()
        {
            auto message = make_ptr<QueueMessage>(); message->setSender( owner ); return message;
        }
        void drainScheduling()
        {
            Thread::setCurrentTask( TaskId::Primary ); states->update();
            Thread::setCurrentTask( TaskId::Render ); states->update();
            Thread::setCurrentTask( TaskId::Primary );
        }
    };
    class ReentrantDrainMessage : public QueueMessage
    {
    public:
        StateContext *context = nullptr;
        SmartPtr<QueueMessage> rejected;
        bool detachedDuringCallback = false;
        void unload( SmartPtr<ISharedObject> data ) override
        {
            detachedDuringCallback = !context->getStateQueue( static_cast<u32>( TaskId::Primary ) );
            context->addMessage( TaskId::Primary, rejected );
            QueueMessage::unload( data );
        }
    };
    class PausedSenderMessage : public QueueMessage
    {
    public:
        mutable std::atomic<bool> entered{ false };
        mutable std::atomic<bool> resume{ false };
        SmartPtr<ISharedObject> getSender() const override
        {
            entered.store( true );
            while( !resume.load() ) std::this_thread::yield();
            return StateMessage::getSender();
        }
    };
    class ReentrantSetterMessage : public QueueMessage
    {
    public:
        StateContext *context = nullptr;
        SmartPtr<ISharedObject> owner;
        bool reload = false;
        bool callbackRan = false;
        void setStateContext( SmartPtr<IStateContext> value ) override
        {
            StateMessage::setStateContext( value );
            if( value && !callbackRan )
            {
                callbackRan = true;
                context->getStateQueue( static_cast<u32>( TaskId::Primary ) );
                if( reload )
                {
                    context->unload( nullptr ); context->setOwner( owner ); context->load( nullptr );
                }
            }
        }
    };
    class LoadedQueueEvent : public IEventListener
    {
    public:
        StateContext *context = nullptr;
        SmartPtr<QueueMessage> message;
        bool unloadOnLoaded = false;
        unsigned callbacks = 0;
        Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject>, SmartPtr<IEvent> ) override
        {
            if( type == EventType::Loading && value == IEvent::loadingStateChanged && context->isLoaded() )
            {
                ++callbacks;
                if( unloadOnLoaded ) context->unload( nullptr );
                else context->addMessage( TaskId::Primary, message );
            }
            return {};
        }
    };
    class QueueInitializationEvent : public IEventListener
    {
    public:
        StateContext *context = nullptr;
        SmartPtr<QueueMessage> nested;
        unsigned callbacks = 0;
        Parameter handleEvent( EventType type, hash_type value, const Array<Parameter> &,
                               SmartPtr<ISharedObject>, SmartPtr<ISharedObject> object, SmartPtr<IEvent> ) override
        {
            if( type == EventType::Loading && value == IEvent::loadingStateChanged &&
                dynamic_cast<StateQueue *>( object.get() ) && object->isLoaded() )
            {
                ++callbacks;
                if( callbacks == 1 ) context->addMessage( TaskId::Primary, nested );
            }
            return {};
        }
    };
    class ExtensionQueue : public IStateQueue
    {
    public:
        StateContext *context = nullptr;
        mutable std::atomic<unsigned> taskLookups{ 0 };
        mutable bool removeDuringLookup = false;
        bool enqueueLookupSucceeded = false;
        void load( SmartPtr<ISharedObject> ) override { setLoadingState( LoadingState::Loaded ); }
        void unload( SmartPtr<ISharedObject> ) override { clear(); setLoadingState( LoadingState::Unloaded ); }
        void update() override {}
        void queueMessage( const SmartPtr<IStateMessage> &message ) override
        {
            enqueueLookupSucceeded = context->getStateQueue( m_task ).get() == this;
            m_messages.push( message );
        }
        void clear() override
        {
            auto messages = getMessagesAndClear();
            for( auto &message : messages ) message->unload( nullptr );
        }
        u32 getTaskId() const override
        {
            ++taskLookups;
            if( removeDuringLookup )
            {
                removeDuringLookup = false; context->clear(); return m_task;
            }
            // The other task uses a core queue. Reentrant lookup must not run
            // under the context's nonrecursive read lock.
            context->getStateQueue( static_cast<u32>( TaskId::Render ) );
            return m_task;
        }
        void setTaskId( u32 value ) override { m_task = value; }
        bool isEmpty() const override { return m_messages.empty(); }
        Array<SmartPtr<IStateMessage>> getMessages() const override { return {}; }
        Array<SmartPtr<IStateMessage>> getMessagesAndClear() override
        {
            Array<SmartPtr<IStateMessage>> result; SmartPtr<IStateMessage> message;
            while( m_messages.try_pop( message ) ) result.push_back( message );
            return result;
        }
    private:
        u32 m_task = static_cast<u32>( TaskId::Primary );
        ConcurrentQueue<SmartPtr<IStateMessage>> m_messages;
    };

    void queueContracts()
    {
        QueueFixture fixture;
        {
            auto context = fixture.context();
            const auto &readOnly = *context;
            for( u32 task = 0; task < static_cast<u32>( TaskId::Count ); ++task )
                require( !context->getStateQueue( task ) && !readOnly.getStateQueue( task ), "Idle queue lookup stays absent" );
            context->update();
            require( context->queueCount() == 0, "Load and idle update allocate no task queue objects" );
            auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
            auto initialization = make_ptr<QueueInitializationEvent>(); initialization->context = context.get();
            initialization->nested = fixture.message(); fixture.app->addObjectListener( initialization );
            auto primary = fixture.message(), render = fixture.message();
            context->addMessage( TaskId::Primary, primary ); context->addMessage( TaskId::Render, render );
            require( context->queueCount() == 2 && context->getStateQueue( 0 )->isLoaded() &&
                     context->getStateQueue( static_cast<u32>( TaskId::Render ) )->isLoaded(), "First messages create loaded task-local queues" );
            require( initialization->callbacks == 0 && context->getStateQueue( 0 )->getObjectFlag( OBJECT_FLAG_LOADING_EVENTS ),
                     "Unpublished queue initialization suppresses factory-enabled callbacks and restores flags" );
            fixture.app->removeObjectListener( initialization );
            context->update();
            require( listener->messages == 1 && primary->unloads == 1 && render->unloads == 0,
                     "Primary update delivers and unloads its first queued message once" );
            Thread::setCurrentTask( TaskId::Render ); context->update(); Thread::setCurrentTask( TaskId::Primary );
            require( listener->messages == 2 && render->unloads == 1, "Other tasks retain independent dispatch queues" );
            auto invalid = fixture.message();
            context->addMessage( TaskId::Count, invalid ); context->addMessage( static_cast<TaskId>( 255 ), invalid );
            require( context->queueCount() == 2 && !invalid->getStateContext(), "Invalid task identifiers do not queue or bind messages" );
            auto duringDrain = make_ptr<ReentrantDrainMessage>(); duringDrain->setSender( fixture.owner );
            duringDrain->context = context.get(); duringDrain->rejected = fixture.message();
            context->addMessage( TaskId::Primary, duringDrain );
            context->unload( nullptr );
            require( duringDrain->unloads == 1 && duringDrain->detachedDuringCallback &&
                     !duringDrain->rejected->getStateContext() && context->queueCount() == 0,
                     "Unload detaches and drains before reentrant message callbacks" );
            auto after = fixture.message(); context->addMessage( TaskId::Primary, after );
            require( !after->getStateContext() && context->queueCount() == 0, "Unloaded contexts reject new messages" );
            fixture.drainScheduling();
            context->setOwner( fixture.owner ); context->load( nullptr ); context->addStateListener( listener );
            require( context->queueCount() == 0, "Reload starts with no old task queues" );
            context->addMessage( TaskId::Primary, after ); context->update();
            require( after->unloads == 1 && listener->messages == 3, "Reload creates a fresh queue and dispatches normally" );
            context->unload( nullptr ); fixture.drainScheduling();
        }
        {
            auto context = fixture.context(); auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
            constexpr unsigned workers = 4, perWorker = 64;
            std::vector<SmartPtr<QueueMessage>> messages;
            for( unsigned i = 0; i < workers * perWorker; ++i ) messages.push_back( fixture.message() );
            std::atomic<unsigned> ready{ 0 }; std::atomic<bool> start{ false };
            std::vector<std::thread> writers;
            for( unsigned worker = 0; worker < workers; ++worker ) writers.emplace_back( [&, worker]() {
                ++ready; while( !start.load() ) std::this_thread::yield();
                for( unsigned i = 0; i < perWorker; ++i ) context->addMessage( TaskId::Primary, messages[worker * perWorker + i] );
            } );
            while( ready.load() != workers ) std::this_thread::yield(); start.store( true );
            for( auto &writer : writers ) writer.join();
            require( context->queueCount() == 1, "Concurrent first writers publish one queue for their task" );
            context->update(); context->update();
            require( listener->messages == messages.size(), "Concurrent messages are delivered once without loss" );
            for( const auto &message : messages ) require( message->unloads == 1 && !message->getStateContext(), "Each concurrent message is retired once" );
            context->unload( nullptr ); fixture.drainScheduling();
        }
        {
            auto context = fixture.context();
            auto message = make_ptr<PausedSenderMessage>(); message->setSender( fixture.owner );
            std::thread writer( [&]() { context->addMessage( TaskId::Primary, message ); } );
            while( !message->entered.load() ) std::this_thread::yield();
            context->unload( nullptr ); message->resume.store( true ); writer.join();
            require( context->queueCount() == 0 && !message->getStateContext() && message->unloads == 0,
                     "A producer overtaken by unload cannot publish a late queue or retain the context" );
            fixture.drainScheduling();
        }
        {
            auto context = fixture.context(); auto message = fixture.message();
            context->addMessage( TaskId::Primary, message ); context->setLoadingState( LoadingState::Unloaded );
            context->unload( nullptr );
            require( message->unloads == 1 && !message->getStateContext() && context->queueCount() == 0,
                     "An already-Unloaded marker still drains any retained queued messages" );
            fixture.drainScheduling();
        }
        for( bool reloadInSetter : { false, true } )
        {
            auto context = fixture.context(); auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
            auto message = make_ptr<ReentrantSetterMessage>(); message->setSender( fixture.owner );
            message->context = context.get(); message->owner = fixture.owner; message->reload = reloadInSetter;
            context->addMessage( TaskId::Primary, message );
            require( message->callbackRan, "Custom message context setters run outside the queue lock" );
            if( reloadInSetter )
                require( context->isLoaded() && context->queueCount() == 0 && !message->getStateContext(),
                         "A context reloaded by a message callback rejects the preceding lifetime's enqueue" );
            else
            {
                context->update();
                require( listener->messages == 1 && message->unloads == 1, "Reentrant context lookup in a message setter preserves delivery" );
            }
            context->unload( nullptr ); fixture.drainScheduling();
        }
        for( bool unloadOnLoaded : { false, true } )
        {
            auto context = make_ptr<QueueContext>(); context->setOwner( fixture.owner );
            auto event = make_ptr<LoadedQueueEvent>(); event->context = context.get();
            event->message = fixture.message(); event->unloadOnLoaded = unloadOnLoaded;
            context->setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true ); context->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, true );
            context->addObjectListener( event ); context->load( nullptr );
            require( event->callbacks == 1, "Loaded-event fixture receives synchronous notification" );
            if( unloadOnLoaded )
                require( !context->isLoaded() && context->queueCount() == 0, "Unload inside a Loaded callback does not reopen queue admission" );
            else
            {
                auto listener = make_ptr<QueueListener>(); context->addStateListener( listener ); context->update();
                require( event->message->unloads == 1 && listener->messages == 1, "A synchronous Loaded callback can enqueue its first message" );
            }
            context->unload( nullptr ); fixture.drainScheduling();
        }
        {
            auto context = fixture.context(); auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
            auto render = fixture.message(); context->addMessage( TaskId::Render, render );
            auto extension = make_ptr<ExtensionQueue>(); extension->context = context.get(); extension->load( nullptr );
            context->installQueue( extension );
            const auto &readOnly = *context;
            require( context->getStateQueue( 0 ) == extension && readOnly.getStateQueue( 0 ) == extension,
                     "A subclass-installed IStateQueue remains discoverable through both getters" );
            auto primary = fixture.message(); context->addMessage( TaskId::Primary, primary );
            require( extension->enqueueLookupSucceeded && extension->taskLookups > 0 && context->queueCount() == 2,
                     "Custom queue virtual getters and enqueue callbacks can reenter context lookup" );
            context->update(); Thread::setCurrentTask( TaskId::Render ); context->update(); Thread::setCurrentTask( TaskId::Primary );
            require( listener->messages == 2 && primary->unloads == 1 && render->unloads == 1,
                     "Custom and core task queues dispatch through the existing context listener path" );
            auto pending = fixture.message(); context->addMessage( TaskId::Primary, pending ); context->unload( nullptr );
            require( pending->unloads == 1 && !pending->getStateContext() && context->queueCount() == 0,
                     "Context teardown drains and unloads subclass-installed queues" );
            fixture.drainScheduling();
        }
        {
            auto context = fixture.context(); auto extension = make_ptr<ExtensionQueue>();
            extension->context = context.get(); extension->load( nullptr ); context->installQueue( extension );
            extension->removeDuringLookup = true;
            auto message = fixture.message(); context->addMessage( TaskId::Primary, message );
            require( context->queueCount() == 0 && extension->isEmpty() && !message->getStateContext(),
                     "A custom queue removed by its task getter cannot receive an orphaned enqueue" );
            context->unload( nullptr ); fixture.drainScheduling();
        }
        {
            auto context = fixture.context(); auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
            auto extension = make_ptr<DerivedQueue>(); extension->context = context.get();
            extension->setTaskId( static_cast<u32>( TaskId::Primary ) ); extension->load( nullptr );
            context->installQueue( extension );
            const auto &readOnly = *context;
            require( context->getStateQueue( 0 ) == extension && readOnly.getStateQueue( 0 ) == extension,
                     "Manually installed StateQueue subclasses retain virtual task lookup" );
            auto message = fixture.message(); context->addMessage( TaskId::Primary, message );
            require( extension->writes == 1 && extension->taskReads > 0 && extension->enqueueLookupSucceeded &&
                     extension->StateQueue::isEmpty() && !extension->isEmpty(),
                     "A StateQueue subclass receives enqueue in its own buffer with reentrant callbacks" );
            context->update();
            require( listener->messages == 1 && message->unloads == 1 && extension->isEmpty(),
                     "StateQueue subclass retrieval consumes the same buffer used by virtual enqueue" );
            auto pending = fixture.message(); context->addMessage( TaskId::Primary, pending ); context->unload( nullptr );
            require( pending->unloads == 1 && context->queueCount() == 0, "Teardown drains manually installed StateQueue subclasses" );
            fixture.drainScheduling();
        }
        std::cout << "Lazy task queues: idle storage, first delivery, concurrent writers, invalid tasks, reload and reentrant teardown PASS\n";
    }

    void factoryDerivedQueueContracts()
    {
        QueueFixture fixture( true ); auto context = fixture.context();
        fixture.derivedFactory->context = context.get();
        auto listener = make_ptr<QueueListener>(); context->addStateListener( listener );
        auto initialization = make_ptr<QueueInitializationEvent>(); initialization->context = context.get();
        initialization->nested = fixture.message(); fixture.app->addObjectListener( initialization );
        auto message = fixture.message(); context->addMessage( TaskId::Primary, message );
        auto queue = dynamic_pointer_cast<DerivedQueue>( context->getStateQueue( 0 ) );
        const auto &readOnly = *context;
        require( queue && readOnly.getStateQueue( 0 ) == queue && queue->loads == 1 && queue->taskAssignments == 1 &&
                 queue->writes == 1 && queue->taskReads > 0 && queue->enqueueLookupSucceeded,
                 "Factory-created StateQueue subclasses retain virtual initialization, discovery and enqueue" );
        require( queue->StateQueue::isEmpty() && !queue->isEmpty() && initialization->callbacks == 0 &&
                 queue->getObjectFlag( OBJECT_FLAG_LOADING_EVENTS ),
                 "Derived factory queues keep their own buffer and suppress only unpublished loading events" );
        fixture.app->removeObjectListener( initialization );
        context->update();
        require( listener->messages == 1 && message->unloads == 1 && queue->isEmpty(),
                 "Factory-created subclass messages reach existing context listeners once" );
        auto pending = fixture.message(); context->addMessage( TaskId::Primary, pending ); context->unload( nullptr );
        require( pending->unloads == 1 && context->queueCount() == 0, "Factory-created subclass queues drain during teardown" );
        fixture.drainScheduling();
        {
            auto concurrent = fixture.context(); fixture.derivedFactory->context = concurrent.get();
            auto deliveries = make_ptr<QueueListener>(); concurrent->addStateListener( deliveries );
            constexpr unsigned workers = 4, perWorker = 64;
            std::vector<SmartPtr<QueueMessage>> messages;
            for( unsigned i = 0; i < workers * perWorker; ++i ) messages.push_back( fixture.message() );
            std::atomic<unsigned> ready{ 0 }; std::atomic<bool> start{ false };
            std::vector<std::thread> writers;
            for( unsigned worker = 0; worker < workers; ++worker ) writers.emplace_back( [&, worker]() {
                ++ready; while( !start.load() ) std::this_thread::yield();
                for( unsigned i = 0; i < perWorker; ++i )
                    concurrent->addMessage( TaskId::Primary, messages[worker * perWorker + i] );
            } );
            while( ready.load() != workers ) std::this_thread::yield(); start.store( true );
            for( auto &writer : writers ) writer.join();
            auto winner = dynamic_pointer_cast<DerivedQueue>( concurrent->getStateQueue( 0 ) );
            require( concurrent->queueCount() == 1 && winner && winner->writes == messages.size(),
                     "Concurrent factory-derived first writers publish and adopt one task queue" );
            concurrent->update(); concurrent->update();
            require( deliveries->messages == messages.size() && winner->isEmpty(),
                     "Every concurrent subclass message is delivered once from the winning queue" );
            for( const auto &message : messages ) require( message->unloads == 1 && !message->getStateContext(),
                "Concurrent factory-derived messages retire once without losing a candidate's buffer" );
            concurrent->unload( nullptr ); fixture.drainScheduling();
        }
        fixture.derivedFactory->context = nullptr;
        std::cout << "StateQueue subclasses: manual and factory overrides, own buffers and reentrant lookup PASS\n";
    }
}
int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance(&types);
    int result = 0;
    try
    {
        StateContext context;
        context.setLoadingState(LoadingState::Loaded);
        std::vector<SmartPtr<Listener>> listeners;
        for(unsigned i = 0; i < 256; ++i) listeners.push_back(make_ptr<Listener>());
        // Exercise material-style fan-out beyond the old 32-listener bound,
        // with concurrent registration of the same listeners.
        std::vector<std::thread> threads;
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&]() {
                for(const auto &listener : listeners) context.addStateListener(listener);
            });
        for(auto &thread : threads) thread.join();
        auto snapshot = context.getStateListeners();
        require(snapshot.size() == listeners.size(), "Listeners must grow and remain unique");
        SmartPtr<IState> state = make_ptr<State>();
        context._processStateUpdate(state);
        for(const auto &listener : listeners)
            require(listener->notifications == 1, "Each listener must receive the state update");
        require(context.removeStateListener(listeners[100]), "Registered listener must be removable");
        require(snapshot.size() == 256 && context.getStateListeners().size() == 255,
                "Dispatch snapshots must remain independent of later removals");
        context._processStateUpdate(state);
        require(listeners[100]->notifications == 1 && listeners[101]->notifications == 2,
                "Removed listeners must stop receiving updates");
        std::vector<SmartPtr<EventListener>> events;
        for(unsigned i = 0; i < 256; ++i) events.push_back(make_ptr<EventListener>());
        threads.clear();
        // Duplicate attempts race with growth unless search and insertion share a lock.
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&]() {
                for(const auto &listener : events) context.addEventListener(listener);
            });
        for(auto &thread : threads) thread.join();
        const auto eventSnapshot = context.getEventListeners();
        require(eventSnapshot.size() == events.size(), "Event listeners must grow and remain unique");
        threads.clear();
        for(unsigned worker = 0; worker < 4; ++worker)
            threads.emplace_back([&, worker]() {
                for(unsigned i = worker; i < events.size(); i += 4)
                    context.removeEventListener(events[i]);
            });
        for(auto &thread : threads) thread.join();
        require(context.getEventListeners().empty() && eventSnapshot.size() == events.size(),
                "Concurrent removal must preserve event snapshots");
        for(const auto &listener : events) context.addEventListener(listener);
        context.setLoadingState(LoadingState::Unloaded);
        context.unload(nullptr);
        require(context.getStateListeners().empty() && context.getEventListeners().empty(),
                "Teardown must release all registrations");
        {
            auto inventory = make_ptr<StateContext>();
            auto owner = make_ptr<State>();
            owner->setLoadingState(LoadingState::Loaded);
            inventory->setLoadingState(LoadingState::Loaded);
            inventory->setOwner(owner);
            std::vector<SmartPtr<DirtyState>> states;
            for(unsigned i = 0; i < 128; ++i) states.push_back(make_ptr<DirtyState>());
            threads.clear();
            for(unsigned worker = 0; worker < 4; ++worker)
                threads.emplace_back([&]() {
                    for(const auto &entry : states) inventory->addState(entry);
                });
            for(auto &thread : threads) thread.join();
            require(inventory->getStates().size() == states.size(),
                    "Concurrent state registration must not publish duplicates");
            auto retained = inventory->getStates();
            auto dispatch = make_ptr<InventoryListener>();
            std::promise<void> beginRemoval, removed;
            auto begin = beginRemoval.get_future();
            auto complete = removed.get_future();
            std::thread remover([&]() {
                begin.wait();
                for(const auto &entry : states) inventory->removeState(entry);
                removed.set_value();
            });
            bool removedInsideCallback = false;
            dispatch->onFirst = [&]() {
                beginRemoval.set_value();
                removedInsideCallback = complete.wait_for(std::chrono::seconds(2)) ==
                                        std::future_status::ready;
            };
            inventory->addStateListener(dispatch);
            inventory->update();
            remover.join();
            require(removedInsideCallback, "State callbacks must not hold the inventory lock");
            require(dispatch->notifications == states.size(),
                    "A dispatch snapshot must survive concurrent removal of every state");
            require(inventory->getStates().empty() && retained.size() == states.size(),
                    "State removal must preserve previously retained snapshots");
            inventory->update();
            require(dispatch->notifications == states.size(),
                    "Removed states must not be dispatched in a later update");
            for(auto &entry : states) entry->setStateContext(nullptr);
            inventory->setOwner(nullptr);
            inventory->unload(nullptr);
        }
        std::cout << "State inventory/listeners: concurrent registration, retained dispatch and cleanup PASS\n";
        queueContracts();
        factoryDerivedQueueContracts();
        std::cout << "State/event listeners: fan-out, concurrent deduplication, dispatch and cleanup PASS\n";
    }
    catch(const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    TypeManager::setInstance(nullptr);
    types.unload();
    return result;
}
