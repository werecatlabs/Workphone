#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/StateContext.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IEvent.hpp>
#include <Workphone/Interface/System/IEventListener.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateManager.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Jobs/EventJob.hpp>
#include <Workphone/System/DebugUtil.hpp>
#include <Workphone/Memory/TypeManager.hpp>
#include <unordered_set>
#include <typeinfo>

namespace workphone
{
    namespace
    {
        bool isCoreStateQueue( const IStateQueue *queue )
        {
            // Public StateQueue subclasses retain their virtual behavior just
            // like other IStateQueue extensions. Only the exact engine class
            // has a callback-free enqueue implementation.
            return queue && typeid( *queue ) == typeid( StateQueue );
        }
    }
    WP_CLASS_REGISTER_DERIVED( workphone, StateContext, IStateContext );
    WP_CLASS_REGISTER_DERIVED( workphone, StateContext::SharedObjectListener, IEventListener );

    u32 StateContext::m_nextGeneratedNameExt = 1000;

    StateContext::StateContext() : IStateContext(), m_isDirty( false )
    {
#if WP_ENABLE_MEMORY_TRACKER
#    if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#        if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        setDebugStr( stack );
#        endif
#    endif
#endif
    }

    StateContext::StateContext( u32 id ) : IStateContext()
    {
#if WP_ENABLE_MEMORY_TRACKER
#    if WP_ARCH_TYPE == WP_ARCHITECTURE_64
#        if WP_ENABLE_TRACE
        const auto stack = DebugUtil::getStackTrace();
        setDebugStr( stack );
#        endif
#    endif
#endif
    }

    StateContext::~StateContext()
    {
        unload( nullptr );
        clearStates();
        clearStateListeners();
        clearStateQueues();
        clearEventListeners();

        WP_ASSERT( getOwnerPtr() == nullptr );
    }

    Array<SmartPtr<IState>> StateContext::snapshotStates() const
    {
        auto values = m_states.snapshot();
        return Array<SmartPtr<IState>>( values.begin(), values.end() );
    }

    void StateContext::clearStates()
    {
        // Keep removed states alive until after the inventory lock is released;
        // their destruction can reenter a context or another subsystem.
        Array<SmartPtr<IState>> removed;
        {
            ScopedLock lock( &m_states );
            removed = m_states.snapshot();
            m_states.releaseStorage();
        }
    }

    Array<SmartPtr<IStateListener>> StateContext::snapshotStateListeners() const
    {
        return m_stateListeners.snapshot();
    }

    void StateContext::clearStateListeners()
    {
        // Release references outside the lock: destructors may re-enter the context.
        Array<SmartPtr<IStateListener>> removed;
        {
            ScopedLock lock( &m_stateListeners );
            removed = m_stateListeners.snapshot();
            m_stateListeners.releaseStorage();
        }
    }

    Array<SmartPtr<IStateQueue>> StateContext::snapshotStateQueues() const
    {
        auto values = m_stateQueues.readLocked();
        return Array<SmartPtr<IStateQueue>>( values.begin(), values.end() );
    }

    void StateContext::clearStateQueues()
    {
        // Release references outside the lock: destructors may re-enter the context.
        decltype( m_stateQueues )::storage_type removed;
        {
            auto values = m_stateQueues.writeLocked();
            removed = std::move( *values );
        }
    }

    Array<SmartPtr<IEventListener>> StateContext::snapshotEventListeners() const
    {
        return m_eventListeners.snapshot();
    }

    void StateContext::clearEventListeners()
    {
        // Retain references until outside the lock, as listener teardown may re-enter.
        Array<SmartPtr<IEventListener>> removed;
        {
            ScopedLock lock( &m_eventListeners );
            removed = m_eventListeners.snapshot();
            m_eventListeners.releaseStorage();
        }
    }

    void StateContext::appendStateQueue( SmartPtr<IStateQueue> stateQueue )
    {
        if( stateQueue )
        {
            m_stateQueues.push_back( stateQueue );
        }
    }

    void StateContext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            u32 loadGeneration;
            {
                auto queues = m_stateQueues.writeLocked();
                m_acceptQueuedMessages = false;
                loadGeneration = m_queueGeneration;
            }
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "StateContext::load: null application manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "StateContext::load: null factory manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            m_bUpdateState = true;

            // Most material contexts never receive messages. Keep their task
            // queues absent until the first enqueue for that task.

            auto sharedObjectListener = factoryManager->make_ptr<SharedObjectListener>();
            if( !sharedObjectListener )
            {
                WP_LOG_ERROR( "StateContext::load: failed to create SharedObjectListener." );
                setLoadingState( LoadingState::Error );
                return;
            }

            sharedObjectListener->setOwner( this );
            m_sharedObjectListener = sharedObjectListener;

            m_states.reserve( stateCapacity );

            {
                auto queues = m_stateQueues.writeLocked();
                if( m_queueGeneration != loadGeneration || getLoadingState() != LoadingState::Loading ) return;
                // isLoaded() still rejects producers until the transition.
                // Arm first so synchronous Loaded-event handlers can enqueue;
                // an unload in that callback closes the gate without reopening.
                m_acceptQueuedMessages = true;
            }
            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            {
                auto queues = m_stateQueues.writeLocked();
                m_acceptQueuedMessages = false;
            }
            setLoadingState( LoadingState::Error );
        }
    }

    void StateContext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            decltype( m_stateQueues )::storage_type stateQueues;
            {
                auto queues = m_stateQueues.writeLocked();
                m_acceptQueuedMessages = false;
                ++m_queueGeneration;
                stateQueues = std::move( *queues );
            }
            std::fprintf( stderr, "TRACE StateContext unload start %p state %u\n", this,
                          static_cast<u32>( loadingState ) );
            if( loadingState == LoadingState::Unloaded && stateQueues.empty() )
            {
                if( auto sharedObjectListener = m_sharedObjectListener.load() )
                {
                    sharedObjectListener->unload( nullptr );
                    m_sharedObjectListener = nullptr;
                }

                clearStateListeners();
                clearEventListeners();
                m_owner.forceReset();
                return;
            }

            if( loadingState != LoadingState::Loaded && loadingState != LoadingState::Error )
            {
                WP_LOG_WARNING( "StateContext::unload: called in unexpected loading state " +
                                StringUtil::toString( static_cast<u32>( loadingState ) ) + "." );
            }

            setLoadingState( LoadingState::Unloading );

            // Queues were detached before the loading event. Producers cannot
            // publish or append after that boundary, and callbacks/destruction
            // below run without the nonrecursive queue-array lock.
            std::fprintf( stderr, "TRACE StateContext queues snapshotted %zu\n", stateQueues.size() );
            size_t stateQueueIndex = 0;
            for( auto &stateQueue : stateQueues )
            {
                std::fprintf( stderr, "TRACE StateContext queue %zu %p start\n", stateQueueIndex,
                              stateQueue.get() );
                if( stateQueue )
                {
                    auto messages = stateQueue->getMessagesAndClear();
                    std::fprintf( stderr, "TRACE StateContext queue %zu messages %zu\n", stateQueueIndex,
                                  messages.size() );
                    for( auto &message : messages )
                    {
                        if( message )
                        {
                            std::fprintf( stderr, "TRACE StateContext message %p unload start\n",
                                          message.get() );
                            message->unload( nullptr );
                            std::fprintf( stderr, "TRACE StateContext message unload end\n" );
                        }
                    }
                    std::fprintf( stderr, "TRACE StateContext queue %zu messages clear start\n",
                                  stateQueueIndex );
                    for( size_t messageIndex = 0; messageIndex < messages.size(); ++messageIndex )
                    {
                        std::fprintf(
                            stderr, "TRACE StateContext message %zu release %p refs %u\n", messageIndex,
                            messages[messageIndex].get(),
                            messages[messageIndex] ? messages[messageIndex]->getReferences() : 0 );
                        if( messages[messageIndex] )
                        {
                            auto typeManager = TypeManager::instance();
                            const auto typeName =
                                typeManager->getName( messages[messageIndex]->getTypeInfo() );
                            std::fprintf( stderr, "TRACE StateContext message type %s listener %p\n",
                                          typeName ? typeName : "<unknown>",
                                          messages[messageIndex]->getSharedObjectListener() );
                        }
                        messages[messageIndex] = nullptr;
                        std::fprintf( stderr, "TRACE StateContext message %zu released\n",
                                      messageIndex );
                    }
                    messages.clear();
                    std::fprintf( stderr, "TRACE StateContext queue %zu messages clear end\n",
                                  stateQueueIndex );
                }
                std::fprintf( stderr, "TRACE StateContext queue %zu complete\n", stateQueueIndex );
                ++stateQueueIndex;
            }
            std::fprintf( stderr, "TRACE StateContext all queues drained\n" );

            // Detach and unload all managed states.
            auto states = snapshotStates();
            std::fprintf( stderr, "TRACE StateContext states snapshotted %zu\n", states.size() );
            for( auto &state : states )
            {
                if( state )
                {
                    state->setStateContext( nullptr );
                    state->unload( nullptr );
                }
                else
                {
                    WP_LOG_WARNING( "StateContext::unload: encountered null state during teardown." );
                }
            }
            clearStates();
            std::fprintf( stderr, "TRACE StateContext states cleared\n" );

            // Unload state queues.
            for( auto &stateQueue : stateQueues )
            {
                if( stateQueue )
                {
                    stateQueue->unload( nullptr );
                }
                else
                {
                    WP_LOG_WARNING(
                        "StateContext::unload: encountered null state queue during teardown." );
                }
            }

            std::fprintf( stderr, "TRACE StateContext queues cleared\n" );

            // Release the context-owned listener before dropping the weak owner reference. The
            // listener is not registered with the owner here, and resolving the owner during
            // late teardown can touch an object that is already being destroyed.
            if( auto sharedObjectListener = m_sharedObjectListener.load() )
            {
                sharedObjectListener->unload( nullptr );
                m_sharedObjectListener = nullptr;
            }
            std::fprintf( stderr, "TRACE StateContext shared listener cleared\n" );

            clearStateListeners();
            clearEventListeners();
            m_owner.forceReset();

            setLoadingState( LoadingState::Unloaded );
            std::fprintf( stderr, "TRACE StateContext unload end %p\n", this );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void StateContext::update()
    {
        try
        {
            auto owner = getOwner();
            if( !owner )
                return;

            if( !owner->isLoaded() )
                return;

            auto task = Thread::getCurrentTask();

            // --- Process dirty states ---
            // Streaming can retire nodes from the shared scene context while
            // state callbacks run. Retain entries without exposing array slots
            // or holding the inventory lock across subsystem callbacks.
            auto states = snapshotStates();
            for( auto &state : states )
            {
                if( !state )
                {
                    WP_LOG_WARNING( "StateContext::update: null state encountered; skipping." );
                    continue;
                }

                if( !state->isDirty() )
                    continue;

                auto listeners = getStateListeners();
                bool handled = false;

                for( auto &listener : listeners )
                {
                    if( !listener )
                    {
                        WP_LOG_WARNING(
                            "StateContext::update: null state listener encountered; skipping." );
                        continue;
                    }

                    state->addSendCount();

                    if( listener->handleStateChanged( state ) )
                    {
                        handled = true;
                        state->setDirty( false );
                    }
                }

                // Clear the dirty flag regardless of whether any listener claimed the
                // state.  Without this, an unhandled-but-dirty state would spin forever.
                if( state->getSendCount() > 1000 )
                {
                    WP_LOG_ERROR( "StateContext::update: state " + state->getName() +
                                  " has been dirty for over 1000 updates; forcing clean." );
                    state->setDirty( false );
                    state->setSendCount( 0 );
                }

#if _DEBUG
                if( !handled )
                {
                    WP_LOG_WARNING( "StateContext::update: no listener handled dirty state (id=" +
                                    StringUtil::toString( state->getId() ) + ")." );
                }
#endif

                // State listeners apply the data to the subsystem first. Afterwards, publish a
                // lightweight event so dependent objects can invalidate their own derived state.
                // Dispatching snapshots synchronously keeps the notification on this context's
                // task and makes listener removal from inside a callback safe.
                std::unordered_set<IEventListener *> notifiedListeners;
                auto notifyListeners = [&]( auto eventListeners ) {
                    Array<SmartPtr<IEventListener>> retainedListeners( eventListeners.begin(),
                                                                       eventListeners.end() );
                    for( auto &eventListener : retainedListeners )
                    {
                        if( !eventListener || !notifiedListeners.insert( eventListener.get() ).second )
                        {
                            continue;
                        }

                        try
                        {
                            eventListener->handleEvent( EventType::Object, IEvent::stateChanged, {},
                                                        state->getOwner(), state, nullptr );
                        }
                        catch( std::exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                        }
                        catch( ... )
                        {
                            WP_LOG_ERROR(
                                "StateContext::update: state event listener threw an "
                                "unknown exception." );
                        }
                    }
                };

                notifyListeners( snapshotEventListeners() );
                if( auto stateOwner = state->getOwner() )
                {
                    notifyListeners( stateOwner->getObjectListeners() );
                }
            }

            // --- Process queued messages for this task ---
            auto stateQueue = getStateQueue( static_cast<u32>( task ) );
            if( !stateQueue )
                return;

            if( stateQueue->isEmpty() )
                return;

            auto messages = stateQueue->getMessagesAndClear();
            auto listeners = getStateListeners();

            for( auto &message : messages )
            {
                if( !message )
                {
                    WP_LOG_WARNING( "StateContext::update: null message in queue; skipping." );
                    continue;
                }

                for( auto &listener : listeners )
                {
                    try
                    {
                        if( !listener )
                        {
                            WP_LOG_WARNING(
                                "StateContext::update: null listener while dispatching "
                                "message; skipping." );
                            continue;
                        }

                        listener->handleStateMessage( message );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }

                message->unload( nullptr );
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void StateContext::addMessage( TaskId taskId, SmartPtr<IStateMessage> message )
    {
        const auto iTask = static_cast<u32>( taskId );
        if( iTask >= static_cast<u32>( TaskId::Count ) )
        {
            WP_LOG_WARNING( "StateContext::addMessage: invalid task id; message dropped." );
            return;
        }

        u32 entryGeneration;
        {
            auto queues = m_stateQueues.readLocked();
            if( !isLoaded() || !m_acceptQueuedMessages ) return;
            entryGeneration = m_queueGeneration;
        }

        if( !message )
        {
            WP_LOG_WARNING( "StateContext::addMessage: null message supplied; ignoring." );
            return;
        }

        if( !message->getSender() )
        {
            WP_LOG_WARNING(
                "StateContext::addMessage: message has no sender; proceeding but this "
                "may indicate a logic error." );
        }

        if( getEnableMessageQueues() )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            auto factoryManager = applicationManager ? applicationManager->getFactoryManager() : nullptr;
            auto stateManager = applicationManager ? applicationManager->getStateManager() : nullptr;
            if( !factoryManager || !stateManager )
            {
                WP_LOG_ERROR( "StateContext::addMessage: missing factory/state manager; message dropped." );
                return;
            }

            // Retain previous owners and losing candidates until outside the
            // queue lock: their destructors may re-enter this context.
            auto previousContext = message->getStateContext();
            bool retainedByQueue = false;
            struct RestoreMessageContext
            {
                SmartPtr<IStateMessage> message;
                SmartPtr<IStateContext> previous;
                bool &retained;
                ~RestoreMessageContext()
                {
                    if( !retained )
                        try { message->setStateContext(previous); }
                        catch( ... ) {} // Extension callbacks cannot escape rollback.
                }
            } restoreMessageContext{message, previousContext, retainedByQueue};
            message->setStateContext( this );
            const auto enqueueExtension = [&]( SmartPtr<IStateQueue> queue ) {
                bool accepted = false;
                {
                    auto queues = m_stateQueues.readLocked();
                    if( isLoaded() && m_acceptQueuedMessages && m_queueGeneration == entryGeneration )
                        for( const auto &registered : *queues )
                            if( registered == queue ) { accepted = true; break; }
                }
                if( !accepted ) return false;
                queue->queueMessage( message );
                retainedByQueue = true;
                setDirtyFlag( 1u << iTask, true );
                return true;
            };
            // Subclasses may install their own IStateQueue implementation.
            // Preserve its existing virtual enqueue/lifecycle contract without
            // running extension callbacks under the core queue-array lock.
            auto extensionQueue = getStateQueue( iTask );
            if( extensionQueue && !isCoreStateQueue( extensionQueue.get() ) )
            {
                if( !enqueueExtension( extensionQueue ) )
                {
                    return;
                }
                stateManager->addDirty( this, taskId );
                return;
            }
            SmartPtr<StateQueue> candidate;
            SmartPtr<IStateQueue> stateQueue;
            bool queued = false;
            for( ;; )
            {
                if( candidate && !isCoreStateQueue( candidate.get() ) )
                {
                    // A factory-provided subclass has virtual task semantics.
                    // Resolve outside the lock, then publish only if the exact
                    // registration snapshot is still current. Concurrent first
                    // writers retry and adopt the winner rather than installing
                    // multiple queues for the same task.
                    const auto observed = snapshotStateQueues();
                    SmartPtr<IStateQueue> selected;
                    for( const auto &queue : observed )
                        if( queue && queue->getTaskId() == iTask ) { selected = queue; break; }
                    {
                        auto queues = m_stateQueues.writeLocked();
                        if( !isLoaded() || !m_acceptQueuedMessages || m_queueGeneration != entryGeneration ) break;
                        bool unchanged = queues->size() == observed.size();
                        for( size_t i = 0; unchanged && i < observed.size(); ++i )
                            unchanged = ( *queues )[i].get() == observed[i].get();
                        if( !unchanged ) continue;
                        if( selected ) stateQueue = selected;
                        else { queues->push_back( candidate ); stateQueue = candidate; }
                        if( isCoreStateQueue( stateQueue.get() ) )
                        {
                            static_cast<StateQueue *>( stateQueue.get() )->StateQueue::queueMessage( message );
                            retainedByQueue = true;
                            setDirtyFlag( 1u << iTask, true ); queued = true;
                        }
                    }
                    break;
                }
                {
                    auto queues = m_stateQueues.writeLocked();
                    if( !isLoaded() || !m_acceptQueuedMessages || m_queueGeneration != entryGeneration ) break;
                    for( const auto &queue : *queues )
                        if( isCoreStateQueue( queue.get() ) &&
                            static_cast<StateQueue *>( queue.get() )->StateQueue::getTaskId() == iTask )
                        { stateQueue = queue; break; }
                    if( !stateQueue && candidate )
                    {
                        queues->push_back( candidate );
                        stateQueue = candidate;
                    }
                    if( stateQueue )
                    {
                        // Context-created queues are concrete StateQueues. The
                        // qualified call has no user callbacks under this lock.
                        if( !isCoreStateQueue( stateQueue.get() ) ) break;
                        auto concrete = static_cast<StateQueue *>( stateQueue.get() );
                        concrete->StateQueue::queueMessage( message );
                        retainedByQueue = true;
                        setDirtyFlag( 1u << iTask, true );
                        queued = true;
                        break;
                    }
                }
                // Loading can trigger events, so construct and load before
                // taking the queue lock again to publish the first queue.
                candidate = factoryManager->make_ptr<StateQueue>();
                if( !candidate ) break;
                // The queue is an unpublished implementation detail. Suppress
                // initialization events even if a factory enables them: a
                // callback enqueueing this task must not recursively create
                // another candidate before the first one can be published.
                const auto loadingEvents = candidate->getObjectFlag( OBJECT_FLAG_LOADING_EVENTS );
                candidate->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, false );
                if( isCoreStateQueue( candidate.get() ) )
                {
                    candidate->StateQueue::setTaskId( iTask );
                    candidate->StateQueue::load( nullptr );
                }
                else
                {
                    candidate->setTaskId( iTask );
                    candidate->load( nullptr );
                }
                candidate->setObjectFlag( OBJECT_FLAG_LOADING_EVENTS, loadingEvents );
                if( !candidate->isLoaded() ) break;
            }
            if( !queued && stateQueue && !isCoreStateQueue( stateQueue.get() ) )
                queued = enqueueExtension( stateQueue );
            if( !queued )
            {
                return;
            }
            // A concurrent unload may make this scheduling entry obsolete;
            // dispatch already tolerates an unloaded/ownerless context. Keep
            // the virtual manager callback outside the queue-array lock.
            stateManager->addDirty( this, taskId );
            return;
        }
        else
        {
            message->setStateContext( this );
            sendMessage( message );
        }

        setDirtyFlag( 1u << iTask, true );

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "StateContext::addMessage: null application manager; cannot mark dirty." );
            return;
        }

        auto stateManager = applicationManager->getStateManagerPtr();
        if( !stateManager )
        {
            WP_LOG_ERROR( "StateContext::addMessage: null state manager; cannot mark dirty." );
            return;
        }

        stateManager->addDirty( this, taskId );
    }

    void StateContext::addStateListener( SmartPtr<IStateListener> stateListener )
    {
        if( !stateListener )
        {
            WP_LOG_WARNING( "StateContext::addStateListener: null listener supplied; ignoring." );
            return;
        }

        ScopedLock lock( &m_stateListeners );
        if( std::find( m_stateListeners.begin(), m_stateListeners.end(), stateListener ) ==
            m_stateListeners.end() )
        {
            m_stateListeners.push_back( stateListener );
        }
    }

    bool StateContext::removeStateListener( SmartPtr<IStateListener> stateListener )
    {
        if( !isLoaded() )
        {
            return false;
        }

        if( !stateListener )
        {
            WP_LOG_WARNING( "StateContext::removeStateListener: null listener supplied; ignoring." );
            return false;
        }

        bool removed = false;
        {
            ScopedLock lock( &m_stateListeners );
            auto it = std::find( m_stateListeners.begin(), m_stateListeners.end(), stateListener );
            if( it != m_stateListeners.end() )
            {
                std::move(std::next(it), m_stateListeners.end(), it);
                m_stateListeners.pop_back();
                removed = true;
            }
        }

        if( !removed )
        {
            WP_LOG_WARNING( "StateContext::removeStateListener: listener not found in context." );
        }

        return removed;
    }

    Array<SmartPtr<IStateListener>> StateContext::getStateListeners() const
    {
        return snapshotStateListeners();
    }

    void StateContext::addEventListener( SmartPtr<IEventListener> eventListener )
    {
        if( !eventListener )
        {
            WP_LOG_WARNING( "StateContext::addEventListener: null listener supplied; ignoring." );
            return;
        }

        ScopedLock lock( &m_eventListeners );
        if( std::find( m_eventListeners.begin(), m_eventListeners.end(), eventListener ) !=
            m_eventListeners.end() )
        {
            WP_LOG_WARNING( "StateContext::addEventListener: listener is already in context." );
            return;
        }

        m_eventListeners.push_back( eventListener );
    }

    bool StateContext::removeEventListener( SmartPtr<IEventListener> eventListener )
    {
        if( !eventListener )
        {
            WP_LOG_WARNING( "StateContext::removeEventListener: null listener supplied; ignoring." );
            return false;
        }

        bool removed = false;
        {
            ScopedLock lock( &m_eventListeners );
            auto it = std::find( m_eventListeners.begin(), m_eventListeners.end(), eventListener );
            if( it != m_eventListeners.end() )
            {
                std::move(std::next(it), m_eventListeners.end(), it);
                m_eventListeners.pop_back();
                removed = true;
            }
        }

        if( !removed )
        {
            WP_LOG_WARNING( "StateContext::removeEventListener: listener not found in context." );
        }

        return removed;
    }

    Array<SmartPtr<IEventListener>> StateContext::getEventListeners() const
    {
        return snapshotEventListeners();
    }

    void StateContext::setOwner( SmartPtr<ISharedObject> owner )
    {
#if !WP_FINAL
        String debugStr = getDebugStr() + DebugUtil::getStackTrace();
        setDebugStr( debugStr );
#endif

        m_owner = owner;
    }

    SmartPtr<ISharedObject> StateContext::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    ISharedObject *StateContext::getOwnerPtr() const
    {
        return m_owner.get();
    }

    SmartPtr<IStateQueue> StateContext::getStateQueue( u32 taskId )
    {
        return static_cast<const StateContext *>( this )->getStateQueue( taskId );
    }

    SmartPtr<IStateQueue> StateContext::getStateQueue( u32 taskId ) const
    {
        bool hasExtensions = false;
        {
            auto queues = m_stateQueues.readLocked();
            for( const auto &stateQueue : queues )
            {
                if( isCoreStateQueue( stateQueue.get() ) )
                {
                    auto concrete = static_cast<StateQueue *>( stateQueue.get() );
                    if( concrete->StateQueue::getTaskId() == taskId ) return stateQueue;
                }
                else if( stateQueue ) hasExtensions = true;
            }
        }
        if( hasExtensions )
        {
            // Only the extension path takes an owning snapshot. Its virtual
            // task getter may perform another context lookup or remove queues.
            const auto queues = snapshotStateQueues();
            for( const auto &stateQueue : queues )
                if( stateQueue && !isCoreStateQueue( stateQueue.get() ) && stateQueue->getTaskId() == taskId )
                    return stateQueue;
        }
        return nullptr;
    }

    void StateContext::sendMessage( SmartPtr<IStateMessage> message )
    {
        if( !message )
        {
            WP_LOG_WARNING( "StateContext::sendMessage: null message supplied; ignoring." );
            return;
        }

        auto listeners = snapshotStateListeners();
        for( auto &listener : listeners )
        {
            if( !listener )
            {
                WP_LOG_WARNING( "StateContext::sendMessage: null listener encountered; skipping." );
                continue;
            }

            try
            {
                listener->handleStateMessage( message );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    void StateContext::_processStateUpdate( SmartPtr<IState> &state )
    {
        if( !state )
        {
            WP_LOG_WARNING( "StateContext::_processStateUpdate: null state supplied; ignoring." );
            return;
        }

        auto listeners = snapshotStateListeners();
        for( auto &listener : listeners )
        {
            if( !listener )
            {
                WP_LOG_WARNING(
                    "StateContext::_processStateUpdate: null listener encountered; skipping." );
                continue;
            }

            try
            {
                listener->handleStateChanged( state );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
    }

    void StateContext::addState( SmartPtr<IState> state )
    {
        if( !isLoaded() )
        {
            WP_LOG_WARNING( "StateContext::addState: context is not loaded; cannot add state." );
            return;
        }

        if( !state )
        {
            WP_LOG_WARNING( "StateContext::addState: null state supplied; ignoring." );
            return;
        }

        bool duplicate = false;
        {
            // Search and insertion form one transaction. A snapshot-only search
            // allows concurrent publishers to insert the same state twice.
            ScopedLock lock( &m_states );
            duplicate = std::find( m_states.begin(), m_states.end(), state ) != m_states.end();
            if( !duplicate ) m_states.push_back( state );
        }
        if( duplicate )
        {
            WP_LOG_WARNING( "StateContext::addState: state is already in context." );
            return;
        }

        state->setStateContext( this );
    }

    void StateContext::removeState( SmartPtr<IState> state )
    {
        if( !state )
        {
            WP_LOG_WARNING( "StateContext::removeState: null state supplied; ignoring." );
            return;
        }

        bool removed = false;

        {
            // ConcurrentArray iterators do not retain a lock. Keep lookup and
            // erase together; the argument keeps the removed state alive.
            ScopedLock lock( &m_states );
            auto it = std::find( m_states.begin(), m_states.end(), state );
            if( it != m_states.end() )
            {
                std::move(std::next(it), m_states.end(), it);
                m_states.pop_back();
                removed = true;
            }
        }

        if( !removed )
        {
            WP_LOG_WARNING( "StateContext::removeState: state not found in context." );
        }
    }

    void StateContext::removeStatesById( hash_type id )
    {
        if( !isLoaded() )
        {
            return;
        }

        auto states = snapshotStates();
        u32 removedCount = 0;

        for( auto &state : states )
        {
            if( !state )
            {
                WP_LOG_WARNING( "StateContext::removeStatesById: null state in collection; skipping." );
                continue;
            }

            if( state->getId() == id )
            {
                removeState( state );
                state->setStateContext( nullptr );
                ++removedCount;
            }
        }

        if( removedCount == 0 )
        {
            WP_LOG_WARNING( "StateContext::removeStatesById: no states found with id " +
                            StringUtil::toString( id ) + "." );
        }
    }

    void StateContext::clear()
    {
        auto stateQueues = snapshotStateQueues();
        for( auto &stateQueue : stateQueues )
        {
            if( stateQueue )
            {
                stateQueue->clear();
            }
            else
            {
                WP_LOG_WARNING( "StateContext::clear: null state queue encountered; skipping." );
            }
        }

        clearStateQueues();

        auto states = snapshotStates();
        for( auto &state : states )
        {
            if( state )
            {
                state->setStateContext( nullptr );
            }
            else
            {
                WP_LOG_WARNING( "StateContext::clear: null state encountered; skipping." );
            }
        }

        clearStates();
    }

    SmartPtr<IState> StateContext::getStateById( hash_type id ) const
    {
        auto states = snapshotStates();
        for( auto &state : states )
        {
            if( !state )
            {
                WP_LOG_WARNING( "StateContext::getStateById: null state encountered; skipping." );
                continue;
            }

            if( state->getId() == id )
            {
                return state;
            }
        }

        return nullptr;
    }

    SmartPtr<IState> StateContext::getStateById( hash_type id, u32 type ) const
    {
        auto states = snapshotStates();
        for( auto &state : states )
        {
            if( state )
            {
                if( state->getId() == id )
                {
                    if( auto data = state->getData() )
                    {
                        if( data->getTypeInfo() == type )
                        {
                            return state;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IState> StateContext::getStateByTypeId( u32 typeId ) const
    {
        auto states = snapshotStates();
        for( auto &state : states )
        {
            if( !state )
            {
                WP_LOG_WARNING( "StateContext::getStateByTypeId: null state encountered; skipping." );
                continue;
            }

            if( state->derived( typeId ) )
            {
                return state;
            }
        }

        return nullptr;
    }

    void *StateContext::getStateDataPtrById( hash_type id, hash_type typeinfo ) const
    {
        auto states = snapshotStates();
        for( const auto &state : states )
        {
            if( state && state->getId() == id )
            {
                if( auto data = state->getDataPtr() )
                {
                    if( data->getTypeInfo() == typeinfo )
                    {
                        return data;
                    }
                }
            }
        }

        return nullptr;
    }

    Array<SmartPtr<IState>> StateContext::getStates() const
    {
        return snapshotStates();
    }

    bool StateContext::isDirty() const
    {
        auto states = snapshotStates();
        for( const auto &state : states )
        {
            if( state )
            {
                if( state->isDirty() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void StateContext::setDirty( bool dirty, bool cascade )
    {
        m_isDirty = dirty;

        if( cascade )
        {
            auto states = snapshotStates();
            for( auto &state : states )
            {
                if( state )
                {
                    state->setDirty( dirty );
                }
            }
        }
    }

    bool StateContext::isStateDirty() const
    {
        return m_stateChangeCount != m_stateUpdateCount;
    }

    void StateContext::setStateDirty( bool dirty )
    {
        if( dirty )
        {
            ++m_stateChangeCount;
        }
        else
        {
            m_stateUpdateCount = static_cast<u32>( m_stateChangeCount );
        }
    }

    bool StateContext::isBitSet( u32 flags, s32 bitIdx ) const
    {
        u32 flag = ( 1 << bitIdx );
        if( ( flags & flag ) != 0 )
        {
            return true;
        }

        return false;
    }

    u32 StateContext::getDirtyFlags() const
    {
        return m_isDirty;
    }

    void StateContext::setDirtyFlags( u32 dirtyFlags )
    {
        m_isDirty = dirtyFlags;
    }

    void StateContext::setDirtyFlag( u32 flag, bool value )
    {
        auto dirtyFlags = getDirtyFlags();

        if( value )
        {
            dirtyFlags |= flag;
        }
        else
        {
            dirtyFlags &= ~flag;
        }

        setDirtyFlags( dirtyFlags );
    }

    void StateContext::setStateListeners( Array<SmartPtr<IStateListener>> listeners )
    {
        clearStateListeners();
        for( auto &listener : listeners )
        {
            addStateListener( listener );
        }
    }

    void StateContext::setEventListeners( Array<SmartPtr<IEventListener>> eventListeners )
    {
        clearEventListeners();
        for( auto &eventListener : eventListeners )
        {
            addEventListener( eventListener );
        }
    }

    bool StateContext::getEnableMessageQueues() const
    {
        return m_enableMessageQueues;
    }

    bool StateContext::getUpdateState() const
    {
        return m_bUpdateState;
    }

    void StateContext::setUpdateState( bool updateState )
    {
        m_bUpdateState = updateState;
    }

    SmartPtr<Properties> StateContext::getProperties() const
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "StateContext::getProperties: null application manager." );
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "StateContext::getProperties: null factory manager." );
            return nullptr;
        }

        auto properties = factoryManager->make_ptr<Properties>();
        if( !properties )
        {
            WP_LOG_ERROR( "StateContext::getProperties: failed to create Properties object." );
            return nullptr;
        }

        return properties;
    }

    void StateContext::setProperties( SmartPtr<Properties> properties )
    {
    }

    TaskId StateContext::getTaskId() const
    {
        return m_taskId;
    }

    void StateContext::setTaskId( TaskId task )
    {
        m_taskId = task;
    }

    void StateContext::invalidateState()
    {
    }

    Parameter StateContext::triggerEvent( EventType eventType, hash_type eventValue,
                                          const Array<Parameter> &arguments,
                                          SmartPtr<ISharedObject> sender, SmartPtr<ISharedObject> object,
                                          SmartPtr<IEvent> event )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "StateContext::triggerEvent: null application manager." );
            return {};
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "StateContext::triggerEvent: null factory manager." );
            return {};
        }

        auto jobQueue = applicationManager->getJobQueuePtr();
        if( jobQueue )
        {
            auto eventJob = factoryManager->make_ptr<EventJob>();
            if( !eventJob )
            {
                WP_LOG_ERROR( "StateContext::triggerEvent: failed to create EventJob." );
                return {};
            }

            eventJob->setOwner( this );
            eventJob->setEventType( eventType );
            eventJob->setEventValue( eventValue );
            eventJob->setArguments( arguments );
            eventJob->setSender( sender );
            eventJob->setObject( object );
            eventJob->setEvent( event );

            jobQueue->addJobAllTasks( eventJob );
        }
        else
        {
            WP_LOG_WARNING(
                "StateContext::triggerEvent: no job queue available; event will only be "
                "dispatched synchronously to registered listeners." );
        }

        auto eventListeners = snapshotEventListeners();
        for( auto &eventListener : eventListeners )
        {
            if( !eventListener )
            {
                WP_LOG_WARNING(
                    "StateContext::triggerEvent: null event listener encountered; skipping." );
                continue;
            }

            try
            {
                eventListener->handleEvent( eventType, eventValue, arguments, sender, object, event );
            }
            catch( std::exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }

        return {};
    }

    bool StateContext::isValid() const
    {
        if( auto owner = getOwner() )
        {
            auto states = snapshotStates();
            for( auto &state : states )
            {
                if( state )
                {
                    return true;
                }
            }
        }

        return false;
    }

    StateContext::SharedObjectListener::SharedObjectListener() = default;
    StateContext::SharedObjectListener::~SharedObjectListener() = default;

    void StateContext::SharedObjectListener::unload( SmartPtr<ISharedObject> data )
    {
        m_owner = nullptr;
    }

    Parameter StateContext::SharedObjectListener::handleEvent( EventType eventType, hash_type eventValue,
                                                               const Array<Parameter> &arguments,
                                                               SmartPtr<ISharedObject> sender,
                                                               SmartPtr<ISharedObject> object,
                                                               SmartPtr<IEvent> event )
    {
        if( eventValue == IEvent::loadingStateChanged )
        {
            if( arguments.size() < 2 )
            {
                WP_LOG_WARNING(
                    "StateContext::SharedObjectListener::handleEvent: loadingStateChanged event "
                    "has insufficient arguments (expected >= 2, got " +
                    StringUtil::toString( static_cast<u32>( arguments.size() ) ) + ")." );
                return {};
            }

            auto loadingState = static_cast<LoadingState>( arguments[1].getU32() );
            if( loadingState == LoadingState::Loaded )
            {
                auto context = getOwner();
                if( !context )
                {
                    WP_LOG_WARNING(
                        "StateContext::SharedObjectListener::handleEvent: owner context is null." );
                    return {};
                }

                if( sender == context->getOwnerPtr() )
                {
                    auto applicationManager = core::IApplicationManager::instancePtr();
                    if( !applicationManager )
                    {
                        WP_LOG_ERROR(
                            "StateContext::SharedObjectListener::handleEvent: null application "
                            "manager." );
                        return {};
                    }

                    auto stateManager = applicationManager->getStateManagerPtr();
                    if( !stateManager )
                    {
                        WP_LOG_ERROR(
                            "StateContext::SharedObjectListener::handleEvent: null state "
                            "manager." );
                        return {};
                    }

                    stateManager->makeDirty( context );
                }
            }
        }

        return {};
    }

    SmartPtr<StateContext> StateContext::SharedObjectListener::getOwner() const
    {
        return m_owner.get();
    }

    void StateContext::SharedObjectListener::setOwner( SmartPtr<StateContext> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone
