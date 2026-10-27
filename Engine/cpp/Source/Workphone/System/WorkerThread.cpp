#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/WorkerThread.hpp>
#include <Workphone/System/ProjectRuntimeSettings.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/System/TimerChrono.hpp>
#include <thread>
#include <stdexcept>
#if defined( _WIN32 )
#    include <windows.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, WorkerThread, IWorkerThread );

    WorkerThread::WorkerThread()
    {
        m_thread = nullptr;
    }

    WorkerThread::~WorkerThread() = default;

    void WorkerThread::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );
            m_shutdownRequested = false;
#if defined( _WIN32 )
            auto app = core::IApplicationManager::instancePtr();
            auto settings = app ? app->getPlayerSettings() : nullptr;
            if( settings && settings->getPropertyAsInt( "tasks.jobSystem", 0 ) == 1 )
            {
                auto work = CreateThreadpoolWork( []( PTP_CALLBACK_INSTANCE callback, void *owner, PTP_WORK ) {
                    CallbackMayRunLong( callback );
                    const int priority = GetThreadPriority( GetCurrentThread() );
                    GROUP_AFFINITY affinity = {};
                    GetThreadGroupAffinity( GetCurrentThread(), &affinity );
                    static_cast<WorkerThread *>( owner )->run();
                    SetThreadPriority( GetCurrentThread(), priority );
                    if( affinity.Mask ) SetThreadGroupAffinity( GetCurrentThread(), &affinity, nullptr );
                    using SelectSets = BOOL ( WINAPI * )( HANDLE, const ULONG *, ULONG );
                    auto selectSets = reinterpret_cast<SelectSets>( GetProcAddress(
                        GetModuleHandleW( L"kernel32.dll" ), "SetThreadSelectedCpuSets" ) );
                    if( selectSets ) selectSets( GetCurrentThread(), nullptr, 0 );
                }, this, nullptr );
                if( !work ) throw std::runtime_error( "Unable to create a platform worker" );
                m_platformWork = work;
                SubmitThreadpoolWork( work );
            }
            else
#endif
                m_thread = new std::thread( &WorkerThread::run, this );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void WorkerThread::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                setLoadingState( LoadingState::Unloading );
                m_shutdownRequested = true;
                setState( State::Stop );
#if defined( _WIN32 )
                if( m_platformWork )
                {
                    auto work = static_cast<PTP_WORK>( m_platformWork );
                    WaitForThreadpoolWorkCallbacks( work, TRUE );
                    CloseThreadpoolWork( work );
                    m_platformWork = nullptr;
                }
#endif

                if( auto thread = getThread() )
                {
                    if( thread->joinable() )
                    {
                        thread->join();
                    }

                    delete thread;
                    setThread( nullptr );
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void WorkerThread::run()
    {
        try
        {
            WP_ASSERT( isAlive() );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto pTimer = workphone::make_ptr<TimerChrono>();
            auto timer = pTimer.get();
            WP_ASSERT( timer );

            SmartPtr<Properties> previousPolicy;
            while( !m_shutdownRequested && applicationManager->isRunning() && !applicationManager->getQuit() )
            {
                switch( auto state = getState() )
                {
                case State::Start:
                {
                    Thread::setCurrentThreadId( m_workerThreadId );

                    while( !m_shutdownRequested && applicationManager->isRunning() && !applicationManager->getQuit() &&
                           getState() != State::Stop )
                    {
                        auto policy = applicationManager->getPlayerSettings();
                        if( policy.get() != previousPolicy.get() )
                        {
                            applyProjectWorkerPolicy( policy, static_cast<u32>( getThreadId() ) );
                            previousPolicy = policy;
                        }
                        if( !projectRunsBackgroundTasks() )
                        {
                            Thread::sleep( 0.01 );
                            continue;
                        }
                        auto jobQueue = applicationManager->getJobQueuePtr();
                        auto taskManager = applicationManager->getTaskManagerPtr();

                        setUpdating( true );

                        auto fps = getTargetFPS();
                        if( fps > 0.0 )
                        {
                            auto start = timer->now();

                            timer->update();

                            if( jobQueue )
                            {
                                jobQueue->update();
                            }

                            if( taskManager )
                            {
                                taskManager->update();
                            }

                            auto end = timer->now();
                            auto timeTaken = end - start;

                            auto fRate = 1.0 / fps;
                            auto sleepTime = fRate - timeTaken;
                            Thread::sleep( sleepTime );
                        }
                        else
                        {
                            Thread::sleep( 1.0 );
                        }

                        setUpdating( false );
                    }
                }
                break;
                default:
                {
                    Thread::sleep( 1.0 );
                }
                break;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void WorkerThread::setState( State state )
    {
        m_state = state;
    }

    void WorkerThread::stop()
    {
        auto stopCount = 0;
        auto updatingCount = 0;
        auto maxCount = 10000;

        while( getState() != State::Stop && stopCount < maxCount )
        {
            setState( State::Stop );

            while( isUpdating() && updatingCount < maxCount )
            {
                Thread::yield();
                updatingCount++;
            }

            stopCount++;
        }
    }

    auto WorkerThread::isUpdating() const -> bool
    {
        return m_isUpdating;
    }

    void WorkerThread::setUpdating( bool updating )
    {
        m_isUpdating = updating;
    }

    auto WorkerThread::getThreadId() const -> Thread::ThreadId
    {
        return m_workerThreadId;
    }

    void WorkerThread::setThreadId( Thread::ThreadId threadId )
    {
        m_workerThreadId = threadId;
    }

    void WorkerThread::setTargetFPS( time_interval framesPerSecond )
    {
        m_targetFPS = framesPerSecond;
    }

    auto WorkerThread::getThread() const -> std::thread *
    {
        return m_thread;
    }

    void WorkerThread::setThread( std::thread *thread )
    {
        m_thread = thread;
    }

    auto WorkerThread::getTargetFPS() const -> time_interval
    {
        WP_ASSERT( !Math<time_interval>::equals( m_targetFPS, 0.0 ) );
        WP_ASSERT( Math<time_interval>::isFinite( m_targetFPS ) );
        return m_targetFPS;
    }

    auto WorkerThread::getState() const -> IWorkerThread::State
    {
        return m_state;
    }

}  // namespace workphone
