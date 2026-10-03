#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/StateManager.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IState.hpp>
#include <Workphone/Interface/System/IStateMessage.hpp>
#include <Workphone/Interface/System/IStateContext.hpp>
#include <Workphone/Interface/System/IStateListener.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/StateQueue.hpp>
#include <Workphone/System/StateContext.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, StateManager, IStateManager );
    const u32 StateManager::maxDirtyQueueSize = 8192;
    const String StateManager::nameStr = String( "StateManagerOO" );

    StateManager::StateManager()
    {
        setName( nameStr );
    }

    StateManager::~StateManager() = default;

    void StateManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "StateManager::load: null application manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "StateManager::load: null factory manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            const auto iTaskCount = static_cast<u32>( TaskId::Count );
            m_stateQueues.resize( iTaskCount );

            for( u32 i = 0; i < static_cast<u32>( m_stateQueues.size() ); ++i )
            {
                auto queue = factoryManager->make_ptr<StateQueue>();
                if( !queue )
                {
                    WP_LOG_ERROR( "StateManager::load: failed to create StateQueue for task slot " +
                                  StringUtil::toString( i ) + "." );
                    setLoadingState( LoadingState::Error );
                    return;
                }

                m_stateQueues[i] = queue;
            }

            m_dirtyQueue.resize( iTaskCount );

            const auto numContexts = 100000;
            m_stateContexts.reserve( numContexts );

            for( auto &dirtyArray : m_dirtyArray )
            {
                dirtyArray.reserve( maxDirtyQueueSize );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void StateManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            const auto &loadingState = getLoadingState();
            if( loadingState != LoadingState::Unloaded )
            {
                setLoadingState( LoadingState::Unloading );

                auto stateObjects = m_stateContexts.snapshot();
                for( auto &stateContext : stateObjects )
                {
                    if( stateContext )
                    {
                        stateContext->unload( nullptr );
                    }
                }

                m_stateContexts.clear();

                auto numStateQueues = getNumStateQueues();
                for( size_t i = 0; i < numStateQueues; ++i )
                {
                    if( auto stateQueue = getStateQueue( static_cast<u32>( i ) ) )
                    {
                        stateQueue->unload( nullptr );
                    }
                }

                m_stateQueues.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto StateManager::addStateContext() -> SmartPtr<IStateContext>
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "StateManager::addStateContext: null application manager." );
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManagerPtr();
        if( !factoryManager )
        {
            WP_LOG_ERROR( "StateManager::addStateContext: null factory manager." );
            return nullptr;
        }

        auto stateContext = factoryManager->make_ptr<StateContext>();
        if( !stateContext )
        {
            WP_LOG_ERROR( "StateManager::addStateContext: failed to create StateContext." );
            return nullptr;
        }

        stateContext->load( nullptr );

        if( !stateContext->isLoaded() )
        {
            WP_LOG_ERROR( "StateManager::addStateContext: StateContext failed to load." );
            return nullptr;
        }

        m_stateContexts.push_back( stateContext );
        return stateContext;
    }

    auto StateManager::removeStateContext( SmartPtr<IStateContext> stateContext ) -> bool
    {
        std::fprintf( stderr, "TRACE StateManager remove context begin %p\n", stateContext.get() );
        if( !stateContext )
        {
            WP_LOG_WARNING( "StateManager::removeStateContext: null context supplied; ignoring." );
            return false;
        }

        const auto sizeBefore = m_stateContexts.size();
        std::fprintf( stderr, "TRACE StateManager context list size %zu\n", sizeBefore );
        m_stateContexts.erase( stateContext );
        std::fprintf( stderr, "TRACE StateManager context erase complete\n" );

        if( m_stateContexts.size() == sizeBefore )
        {
            WP_LOG_WARNING( "StateManager::removeStateContext: context not found in managed list." );
            // A context can be removed from the list by a concurrent teardown after the caller
            // obtained its strong reference. Keep removal idempotent and release its state even
            // when it is no longer present in the manager's collection.
            std::fprintf( stderr, "TRACE unlisted StateContext unload begin\n" );
            stateContext->unload( nullptr );
            std::fprintf( stderr, "TRACE unlisted StateContext unload end\n" );
            return false;
        }

        stateContext->unload( nullptr );

        return true;
    }

    auto StateManager::removeStateContext( u32 id ) -> bool
    {
        auto stateContext = findStateContext( id );
        return removeStateContext( stateContext );
    }

    auto StateManager::findStateContext( u32 id ) const -> SmartPtr<IStateContext>
    {
        auto stateObjects = m_stateContexts.snapshot();
        for( auto &stateContext : stateObjects )
        {
            if( !stateContext )
            {
                WP_LOG_WARNING(
                    "StateManager::findStateContext: null context in collection; skipping." );
                continue;
            }

            if( auto handle = stateContext->getHandle() )
            {
                if( handle->getId() == id )
                {
                    return stateContext;
                }
            }
        }

        return nullptr;
    }

    auto StateManager::getStateContexts() const -> Array<SmartPtr<IStateContext>>
    {
        auto snapshot = m_stateContexts.snapshot();
        return Array<SmartPtr<IStateContext>>( snapshot.begin(), snapshot.end() );
    }

    void StateManager::update()
    {
        try
        {
            if( !isLoaded() )
                return;

            auto task = Thread::getCurrentTask();
            auto iTask = static_cast<u32>( task );

            /*
            auto stateContexts = m_stateContexts.snapshot();
            for( auto context : stateContexts )
            {
                if( context )
                {
                    if( context->getTaskId() == task )
                    {
                        if( context->isDirty() )
                        {
                            context->update();
                        }
                    }
                }
            }

            return;
            */

            if( iTask >= m_dirtyArray.size() )
            {
                WP_LOG_ERROR( "StateManager::update: task index " + StringUtil::toString( iTask ) +
                              " is out of range for dirtyArray (size=" +
                              StringUtil::toString( static_cast<u32>( m_dirtyArray.size() ) ) + ")." );
                return;
            }

            auto &contexts = m_dirtyArray[iTask];

            if( iTask >= m_dirtyQueue.size() )
            {
                WP_LOG_ERROR( "StateManager::update: task index " + StringUtil::toString( iTask ) +
                              " is out of range for dirtyQueue (size=" +
                              StringUtil::toString( static_cast<u32>( m_dirtyQueue.size() ) ) + ")." );
                return;
            }

            // Drain the concurrent queue into the per-task set.
            // The set deduplicates automatically — no sort/unique step is needed.
            auto &queue = m_dirtyQueue[iTask];
            if( !queue.empty() )
            {
                SmartPtr<IStateContext> context;
                while( queue.try_pop( context ) )
                {
                    if( !context )
                    {
                        WP_LOG_WARNING(
                            "StateManager::update: null context popped from dirty queue; skipping." );
                        continue;
                    }

                    if( context->getTaskId() == task )
                    {
                        contexts.push_back( context );

                        if( contexts.size() >= maxDirtyQueueSize )
                        {
                            WP_LOG_WARNING( "StateManager::update: dirty set reached max size (" +
                                            StringUtil::toString( maxDirtyQueueSize ) + ") for task " +
                                            StringUtil::toString( iTask ) +
                                            "; remaining items will be processed next frame." );
                            break;
                        }
                    }
                }
            }

            // sort by memory address to improve cache coherence and reduce contention on the dirty queue
            std::sort( contexts.begin(), contexts.end(),
                       []( const SmartPtr<IStateContext> &a, const SmartPtr<IStateContext> &b ) {
                           return a.get() < b.get();
                       } );

            // Remove duplicates
            auto last = std::unique( contexts.begin(), contexts.end() );
            contexts.erase( last, contexts.end() );

            for( auto &context : contexts )
            {
                try
                {
                    context->update();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }

            // Remove non-dirty objects
            contexts.erase( std::remove_if( contexts.begin(), contexts.end(),
                                            []( const SmartPtr<IStateContext> &context ) {
                                                return !context || !context->isDirty();
                                            } ),
                            contexts.end() );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void StateManager::sendMessage( TaskId taskId, SmartPtr<IStateMessage> message )
    {
        if( !message )
        {
            WP_LOG_WARNING( "StateManager::sendMessage: null message supplied; ignoring." );
            return;
        }

        const auto iTask = static_cast<u32>( taskId );
        auto stateQueue = getStateQueue( iTask );
        if( !stateQueue )
        {
            WP_LOG_ERROR( "StateManager::sendMessage: no queue for task id " +
                          StringUtil::toString( iTask ) + "; message dropped." );
            return;
        }

        stateQueue->queueMessage( message );
    }

    auto StateManager::getQueue( TaskId taskId ) -> SmartPtr<IStateQueue>
    {
        const auto iTask = static_cast<u32>( taskId );
        if( iTask >= m_stateQueues.size() )
        {
            WP_LOG_ERROR( "StateManager::getQueue: task id " + StringUtil::toString( iTask ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_stateQueues.size() ) ) + ")." );
            return nullptr;
        }

        return m_stateQueues[iTask];
    }

    void StateManager::destroyQueue( SmartPtr<IStateQueue> queue )
    {
        if( !queue )
        {
            WP_LOG_WARNING( "StateManager::destroyQueue: null queue supplied; ignoring." );
            return;
        }

        queue->unload( nullptr );
    }

    auto StateManager::getNumStateQueues() const -> u32
    {
        return static_cast<u32>( m_stateQueues.size() );
    }

    auto StateManager::getStateQueue( u32 index ) const -> SmartPtr<IStateQueue>
    {
        if( index < m_stateQueues.size() )
        {
            return m_stateQueues[index];
        }

        return nullptr;
    }

    void StateManager::makeDirty( SmartPtr<IStateContext> context )
    {
        if( !context )
        {
            WP_LOG_WARNING( "StateManager::makeDirty: null context supplied; ignoring." );
            return;
        }

        auto states = context->getStates();
        for( auto &state : states )
        {
            if( !state )
            {
                WP_LOG_WARNING( "StateManager::makeDirty: null state in context; skipping." );
                continue;
            }

            state->setDirty( true );
        }

        auto taskId = context->getTaskId();
        addDirty( context, taskId );
    }

    void StateManager::makeAllDirty()
    {
        auto stateContexts = getStateContexts();
        for( auto &stateContext : stateContexts )
        {
            if( !stateContext )
            {
                WP_LOG_WARNING( "StateManager::makeAllDirty: null context in collection; skipping." );
                continue;
            }

            auto states = stateContext->getStates();
            for( auto &state : states )
            {
                if( !state )
                {
                    WP_LOG_WARNING( "StateManager::makeAllDirty: null state in context; skipping." );
                    continue;
                }

                state->setDirty( true );
            }

            auto taskId = stateContext->getTaskId();
            addDirty( stateContext, taskId );
        }
    }

    void StateManager::addDirty( SmartPtr<IStateContext> context )
    {
        if( !context )
        {
            WP_LOG_WARNING( "StateManager::addDirty: null context supplied; ignoring." );
            return;
        }

        const auto taskCount = static_cast<u32>( TaskId::Count );
        for( u32 i = 0; i < taskCount; ++i )
        {
            if( i >= m_dirtyQueue.size() )
            {
                WP_LOG_ERROR( "StateManager::addDirty: task index " + StringUtil::toString( i ) +
                              " out of range for dirtyQueue (size=" +
                              StringUtil::toString( static_cast<u32>( m_dirtyQueue.size() ) ) + ")." );
                break;
            }

            m_dirtyQueue[i].push( context );
        }
    }

    void StateManager::addDirty( SmartPtr<IStateContext> context, TaskId task )
    {
        if( !context )
        {
            WP_LOG_WARNING( "StateManager::addDirty (task): null context supplied; ignoring." );
            return;
        }

        const auto iTask = static_cast<u32>( task );
        if( iTask >= m_dirtyQueue.size() )
        {
            WP_LOG_ERROR( "StateManager::addDirty (task): task id " + StringUtil::toString( iTask ) +
                          " out of range for dirtyQueue (size=" +
                          StringUtil::toString( static_cast<u32>( m_dirtyQueue.size() ) ) + ")." );
            return;
        }

        m_dirtyQueue[iTask].push( context );
    }

    void StateManager::lock()
    {
        m_mutex.lock();
    }

    bool StateManager::try_lock()
    {
        return m_mutex.try_lock();
    }

    void StateManager::unlock()
    {
        m_mutex.unlock();
    }

}  // namespace workphone
