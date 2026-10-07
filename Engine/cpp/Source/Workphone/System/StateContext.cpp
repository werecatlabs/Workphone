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
#include <Workphone/Thread/ScopedLock.hpp>
#include <unordered_set>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, StateContext, IStateContext );
    WP_CLASS_REGISTER_DERIVED( workphone, StateContext::SharedObjectListener, IEventListener );

    u32 StateContext::m_nextGeneratedNameExt = 1000;

    StateContext::StateContext() : IStateContext(), m_isDirty( false ), m_removeCount( 0 )
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
        clearStateNodes();
        clearStateListenerNodes();
        clearStateQueueNodes();
        clearEventListenerNodes();

        WP_ASSERT( getOwnerPtr() == nullptr );
    }

    Array<SmartPtr<IState>> StateContext::snapshotStates() const
    {
        Array<SmartPtr<IState>> states;
        ScopedLock lock( &m_statesMutex, false );

        for( auto state = m_statesHead; state != nullptr; state = state->m_next.load() )
        {
            states.push_back( state );
        }

        return states;
    }

    void StateContext::clearStateNodes()
    {
        {
            ScopedLock lock( &m_statesMutex );

            auto state = m_statesHead;
            m_statesHead = nullptr;
            m_statesTail = nullptr;

            while( state != nullptr )
            {
                auto next = state->m_next.load();
                state->m_next = nullptr;
                state->removeReference();
                state = next;
            }
        }
    }

    Array<SmartPtr<IStateListener>> StateContext::snapshotStateListeners() const
    {
        ScopedLock lock( &m_listenersMutex, false );
        return m_stateListeners;
    }

    void StateContext::clearStateListenerNodes()
    {
        Array<SmartPtr<IStateListener>> listeners;
        {
            ScopedLock lock( &m_listenersMutex );
            listeners.swap( m_stateListeners );
        }
        // Destructors may call back into this context; release outside the lock.
    }

    Array<SmartPtr<IStateQueue>> StateContext::snapshotStateQueues() const
    {
        Array<SmartPtr<IStateQueue>> stateQueues;
        ScopedLock lock( &m_stateQueuesMutex, false );

        for( auto stateQueue = m_stateQueuesHead; stateQueue != nullptr;
             stateQueue = stateQueue->m_next.load() )
        {
            stateQueues.push_back( stateQueue );
        }

        return stateQueues;
    }

    void StateContext::clearStateQueueNodes()
    {
        ScopedLock lock( &m_stateQueuesMutex );

        auto stateQueue = m_stateQueuesHead;
        m_stateQueuesHead = nullptr;
        m_stateQueuesTail = nullptr;

        while( stateQueue != nullptr )
        {
            auto next = stateQueue->m_next.load();
            stateQueue->m_next = nullptr;
            stateQueue->removeReference();
            stateQueue = next;
        }
    }

    void StateContext::appendStateQueue( SmartPtr<IStateQueue> stateQueue )
    {
        if( !stateQueue )
        {
            return;
        }

        ScopedLock lock( &m_stateQueuesMutex );
        stateQueue->m_next = nullptr;
        stateQueue->addReference();

        if( m_stateQueuesTail != nullptr )
        {
            m_stateQueuesTail->m_next = stateQueue.get();
        }
        else
        {
            m_stateQueuesHead = stateQueue.get();
        }

        m_stateQueuesTail = stateQueue.get();
    }

    Array<SmartPtr<IEventListener>> StateContext::snapshotEventListeners() const
    {
        Array<SmartPtr<IEventListener>> eventListeners;
        ScopedLock lock( &m_eventListenersMutex, false );

        for( auto eventListener = m_eventListenersHead; eventListener != nullptr;
             eventListener = eventListener->m_next.load() )
        {
            eventListeners.push_back( eventListener );
        }

        return eventListeners;
    }

    void StateContext::clearEventListenerNodes()
    {
        ScopedLock lock( &m_eventListenersMutex );

        auto eventListener = m_eventListenersHead;
        m_eventListenersHead = nullptr;
        m_eventListenersTail = nullptr;

        while( eventListener != nullptr )
        {
            auto next = eventListener->m_next.load();
            eventListener->m_next = nullptr;
            eventListener->removeReference();
            eventListener = next;
        }
    }

    void StateContext::load( SmartPtr<ISharedObject> data )
    {
        try
        {
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

            for( u32 i = 0; i < static_cast<u32>( TaskId::Count ); ++i )
            {
                auto queue = factoryManager->make_ptr<StateQueue>();
                if( !queue )
                {
                    WP_LOG_ERROR( "StateContext::load: failed to create StateQueue for task slot " +
                                  StringUtil::toString( i ) + "." );
                    setLoadingState( LoadingState::Error );
                    return;
                }

                queue->setTaskId( i );
                appendStateQueue( queue );
            }

            auto sharedObjectListener = factoryManager->make_ptr<SharedObjectListener>();
            if( !sharedObjectListener )
            {
                WP_LOG_ERROR( "StateContext::load: failed to create SharedObjectListener." );
                setLoadingState( LoadingState::Error );
                return;
            }

            sharedObjectListener->setOwner( this );
            m_sharedObjectListener = sharedObjectListener;

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void StateContext::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            std::fprintf( stderr, "TRACE StateContext unload start %p state %u\n", this,
                          static_cast<u32>( loadingState ) );
            if( loadingState == LoadingState::Unloaded )
            {
                if( auto sharedObjectListener = m_sharedObjectListener.load() )
                {
                    sharedObjectListener->unload( nullptr );
                    m_sharedObjectListener = nullptr;
                }

                clearStateListenerNodes();
                clearEventListenerNodes();
                m_owner.forceReset();
                return;
            }

            if( loadingState != LoadingState::Loaded && loadingState != LoadingState::Error )
            {
                WP_LOG_WARNING( "StateContext::unload: called in unexpected loading state " +
                                StringUtil::toString( static_cast<u32>( loadingState ) ) + "." );
            }

            setLoadingState( LoadingState::Unloading );

            // Drain and release all queued messages.
            auto stateQueues = snapshotStateQueues();
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
            clearStateNodes();
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

            clearStateQueueNodes();
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

            clearStateListenerNodes();
            clearEventListenerNodes();
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
            auto states = getStates();
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
                if( state->getSendCount() > 10000 )
                {
                    WP_LOG_ERROR( "StateContext::update: state " + state->getName() +
                                  " has been dirty for over 10000 updates; forcing clean." );
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
        if( !isLoaded() )
        {
            WP_LOG_WARNING( "StateContext::addMessage: dropping message — context is not loaded." );
            return;
        }

        if( !message )
        {
            WP_LOG_WARNING( "StateContext::addMessage: dropping null message." );
            return;
        }

        if( !message->getSender() )
        {
            WP_LOG_WARNING(
                "StateContext::addMessage: message has no sender; proceeding but this "
                "may indicate a logic error." );
        }

        message->setStateContext( this );

        if( getEnableMessageQueues() )
        {
            const auto iTask = static_cast<u32>( taskId );
            auto stateQueue = getStateQueue( iTask );
            if( stateQueue )
            {
                stateQueue->queueMessage( message );
            }
            else
            {
                WP_LOG_ERROR( "StateContext::addMessage: no queue for task id " +
                              StringUtil::toString( iTask ) + "; message dropped." );
                return;
            }
        }
        else
        {
            sendMessage( message );
        }

        setDirtyFlag( ( 1 << static_cast<u32>( taskId ) ), true );
        m_removeCount = 0;

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

        ScopedLock lock( &m_listenersMutex );
        if( std::find( m_stateListeners.begin(), m_stateListeners.end(), stateListener ) ==
            m_stateListeners.end() )
            m_stateListeners.push_back( stateListener );
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
            ScopedLock lock( &m_listenersMutex );
            auto it = std::find( m_stateListeners.begin(), m_stateListeners.end(), stateListener );
            if( it != m_stateListeners.end() )
            {
                m_stateListeners.erase( it );
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

        ScopedLock lock( &m_eventListenersMutex );
        for( auto current = m_eventListenersHead; current != nullptr; current = current->m_next.load() )
        {
            if( current == eventListener.get() )
            {
                WP_LOG_WARNING( "StateContext::addEventListener: listener is already in context." );
                return;
            }
        }

        eventListener->m_next = nullptr;
        eventListener->addReference();

        if( m_eventListenersTail != nullptr )
        {
            m_eventListenersTail->m_next = eventListener.get();
        }
        else
        {
            m_eventListenersHead = eventListener.get();
        }

        m_eventListenersTail = eventListener.get();
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
            ScopedLock lock( &m_eventListenersMutex );
            IEventListener *previous = nullptr;
            auto current = m_eventListenersHead;

            while( current != nullptr )
            {
                auto next = current->m_next.load();
                if( current == eventListener.get() )
                {
                    if( previous != nullptr )
                    {
                        previous->m_next = next;
                    }
                    else
                    {
                        m_eventListenersHead = next;
                    }

                    if( m_eventListenersTail == current )
                    {
                        m_eventListenersTail = previous;
                    }

                    current->m_next = nullptr;
                    current->removeReference();
                    removed = true;
                    break;
                }

                previous = current;
                current = next;
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
        ScopedLock lock( &m_stateQueuesMutex, false );
        for( auto stateQueue = m_stateQueuesHead; stateQueue != nullptr;
             stateQueue = stateQueue->m_next.load() )
        {
            if( stateQueue->getTaskId() == taskId )
            {
                return stateQueue;
            }
        }

        WP_LOG_WARNING( "StateContext::getStateQueue: no queue found for taskId " +
                        StringUtil::toString( taskId ) + "." );
        return nullptr;
    }

    SmartPtr<IStateQueue> StateContext::getStateQueue( u32 taskId ) const
    {
        ScopedLock lock( &m_stateQueuesMutex, false );
        for( auto stateQueue = m_stateQueuesHead; stateQueue != nullptr;
             stateQueue = stateQueue->m_next.load() )
        {
            if( stateQueue->getTaskId() == taskId )
            {
                return stateQueue;
            }
        }

        WP_LOG_WARNING( "StateContext::getStateQueue (const): no queue found for taskId " +
                        StringUtil::toString( taskId ) + "." );
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

        ScopedLock lock( &m_statesMutex );
        for( auto current = m_statesHead; current != nullptr; current = current->m_next.load() )
        {
            if( current == state.get() )
            {
                WP_LOG_WARNING( "StateContext::addState: state is already in context." );
                return;
            }
        }

        state->setStateContext( this );
        state->m_next = nullptr;
        state->addReference();

        if( m_statesTail != nullptr )
        {
            m_statesTail->m_next = state.get();
        }
        else
        {
            m_statesHead = state.get();
        }
        m_statesTail = state.get();
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
            ScopedLock lock( &m_statesMutex );
            IState *previous = nullptr;
            auto current = m_statesHead;

            while( current != nullptr )
            {
                auto next = current->m_next.load();
                if( current == state.get() )
                {
                    if( previous != nullptr )
                    {
                        previous->m_next = next;
                    }
                    else
                    {
                        m_statesHead = next;
                    }

                    if( m_statesTail == current )
                    {
                        m_statesTail = previous;
                    }

                    current->m_next = nullptr;
                    current->removeReference();
                    removed = true;
                    break;
                }

                previous = current;
                current = next;
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

        clearStateQueueNodes();

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

        clearStateNodes();
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
        ScopedLock lock( &m_statesMutex, false );
        for( auto state = m_statesHead; state != nullptr; state = state->m_next.load() )
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
        ScopedLock lock( &m_statesMutex, false );
        for( auto state = m_statesHead; state != nullptr; state = state->m_next.load() )
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
        clearStateListenerNodes();
        for( auto &listener : listeners )
        {
            addStateListener( listener );
        }
    }

    void StateContext::setEventListeners( Array<SmartPtr<IEventListener>> eventListeners )
    {
        clearEventListenerNodes();
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
