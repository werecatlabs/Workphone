#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/ThreadPool.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/Exception.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/System/WorkerThread.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IWorkerThread.hpp>

#if defined( _MSC_VER ) && _MSC_VER > 1600  // (Visual Studio 2010)
#    include <thread>
#else
#    ifdef WP_USE_BOOST
#        include <boost/thread/thread.hpp>
#    endif
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, ThreadPool, IThreadPool );

    ThreadPool::ThreadPool()
    {
        static const auto ThreadPoolStr = String( "ThreadPool" );
        setName( ThreadPoolStr );
    }

    ThreadPool::~ThreadPool() = default;

    void ThreadPool::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto fsmManager = applicationManager->getFsmManager();
            WP_ASSERT( fsmManager );

            auto fsm = fsmManager->createFSM();

            auto fsmListener = workphone::make_ptr<ThreadPoolFSMListener>();
            fsmListener->setOwner( this );
            fsm->addListener( fsmListener );

            m_fsmListener = fsmListener;
            m_fsm = fsm;

            auto numThreads = getNumThreads();
            m_workerThreads.resize( numThreads );
            m_states.resize( numThreads );
            m_threads.resize( numThreads );
            m_targetFPS.resize( numThreads );
            m_threadId.resize( numThreads );
            m_queueLengthMilliseconds.resize( numThreads );
            m_reserveFlags.resize( numThreads );

            u32 workerIndex = 0;
            for( auto &m_workerThread : m_workerThreads )
            {
                auto workerThread = factoryManager->make_ptr<WorkerThread>();
                WP_ASSERT( workerThread );

                // Task affinity uses indices in this pool, including after a reload.
                workerThread->setThreadId( static_cast<Thread::ThreadId>( workerIndex++ ) );
                workerThread->load( data );
                m_workerThread = workerThread;
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ThreadPool::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );

                stop();

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fsmManager = applicationManager->getFsmManager();
                WP_ASSERT( fsmManager );

                if( auto fsm = getFSM() )
                {
                    if( auto fsmListener = getFSMListener() )
                    {
                        fsm->removeListener( fsmListener );
                        setFSMListener( nullptr );
                    }

                    fsmManager->destroyFSM( fsm );
                    setFSM( nullptr );
                }

                for( auto &workerThread : m_workerThreads )
                {
                    workerThread->unload( data );
                }

                m_workerThreads.clear();

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto ThreadPool::addWorkerThread() -> SmartPtr<IWorkerThread>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        if( factoryManager )
        {
            auto workerThread = factoryManager->make_object<IWorkerThread>();
            WP_ASSERT( workerThread );

            workerThread->setThreadId( static_cast<Thread::ThreadId>( m_workerThreads.size() ) );
            m_workerThreads.push_back( workerThread );
            ++m_numThreads;

            return workerThread;
        }

        return nullptr;
    }

    auto ThreadPool::getThread( u32 index ) -> SmartPtr<IWorkerThread>
    {
        if( isLoaded() )
        {
            if( index < m_workerThreads.size() )
            {
                return m_workerThreads[index];
            }
        }

        return nullptr;
    }

    auto ThreadPool::getNumThreads() const -> u32
    {
        return m_numThreads;
    }

    void ThreadPool::setNumThreads( u32 numThreads )
    {
        m_numThreads = numThreads;
    }

    auto ThreadPool::getState() const -> IThreadPool::State
    {
        if( auto fsm = getFSM() )
        {
            return fsm->getState<State>();
        }

        return State::None;
    }

    void ThreadPool::setState( State state )
    {
        auto fsm = getFSM();
        WP_ASSERT( fsm );

        if( fsm )
        {
            WP_ASSERT( fsm->isValid() );
            fsm->setState( state, true );
        }
    }

    void ThreadPool::stop()
    {
        for( auto &workerThread : m_workerThreads )
        {
            workerThread->stop();
        }
    }

    auto ThreadPool::isValid() const -> bool
    {
        const auto &loadingState = getLoadingState();

        switch( loadingState )
        {
        case LoadingState::Unloaded:
        {
        }
        break;
        case LoadingState::Loaded:
        {
            auto fsm = getFSM();
            return fsm && fsm->isValid();
        }
        break;
        }

        return true;
    }

    auto ThreadPool::getFSM() const -> SmartPtr<IFSM>
    {
        return m_fsm;
    }

    void ThreadPool::setFSM( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

    auto ThreadPool::getFSMListener() const -> SmartPtr<IFSMListener>
    {
        return m_fsmListener;
    }

    void ThreadPool::setFSMListener( SmartPtr<IFSMListener> listener )
    {
        m_fsmListener = listener;
    }

    auto ThreadPool::handleEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
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
            case State::Start:
            {
                for( auto &workerThread : m_workerThreads )
                {
                    workerThread->setState( IWorkerThread::State::Start );
                }
            }
            break;
            case State::Stop:
            {
                for( auto &workerThread : m_workerThreads )
                {
                    workerThread->setState( IWorkerThread::State::Stop );
                }
            }
            break;
            }
        }
        break;
        case FSMEvent::Leave:
        {
            auto eState = static_cast<State>( state );
            switch( eState )
            {
            case State::Start:
            {
            }
            break;
            case State::Stop:
            {
            }
            break;
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

    ThreadPool::ThreadPoolFSMListener::ThreadPoolFSMListener() = default;

    ThreadPool::ThreadPoolFSMListener::~ThreadPoolFSMListener() = default;

    auto ThreadPool::ThreadPoolFSMListener::handleEvent( u32 state, FSMEvent eventType ) -> FSMReturnType
    {
        if( auto owner = getOwner() )
        {
            if( owner->isLoaded() )
            {
                return owner->handleEvent( state, eventType );
            }
        }

        return FSMReturnType::NotLoaded;
    }

    auto ThreadPool::ThreadPoolFSMListener::getOwner() const -> SmartPtr<ThreadPool>
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void ThreadPool::ThreadPoolFSMListener::setOwner( SmartPtr<ThreadPool> owner )
    {
        m_owner = owner;
    }
}  // namespace workphone
