#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Task.hpp>
#include <Workphone/System/TaskManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/FSMManager.hpp>
#include <Workphone/System/FSMListenerT.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/IProfile.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Jobs/EventJob.hpp>

namespace workphone
{
    namespace
    {
        template <class Queue>
        void clearEventJobsFromQueue( Queue &queue )
        {
            Array<SmartPtr<IJob>> remainingJobs;
            SmartPtr<IJob> job;

            while( queue.try_pop( job ) )
            {
                if( auto eventJob = dynamic_cast<EventJob *>( job.get() ) )
                {
                    eventJob->unload( nullptr );
                }
                else if( job )
                {
                    remainingJobs.push_back( job );
                }
            }

            for( auto &remainingJob : remainingJobs )
            {
                queue.push( remainingJob );
            }
        }

        template <class Queue>
        void stopJobsFromQueue( Queue &queue )
        {
            SmartPtr<IJob> job;
            while( queue.try_pop( job ) )
            {
                if( job )
                {
                    job->stop();
                    job->setState( IJob::State::Finish );
                }
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, Task, ITask );

    Task::Task() = default;

    Task::Task( const Task &other )
    {
    }

    Task::~Task() = default;

    void Task::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto taskManager = (TaskManager *)applicationManager->getTaskManagerPtr();

        auto taskId = getTask();

        auto fsmManager = workphone::make_ptr<FSMManager>();
        fsmManager->load( data );

        applicationManager->setFsmManagerByTask( taskId, fsmManager );
        m_fsm = fsmManager->createFSM();

        auto fsmListener = workphone::make_ptr<FSMListenerT<Task>>();
        fsmListener->setOwner( this );
        m_fsm->addListener( fsmListener );

        auto iTaskId = static_cast<u32>( taskId );

        m_taskFlags = taskManager->getFlagsPtr( iTaskId );
        m_affinity = taskManager->getAffinityPtr( iTaskId );
        m_targetfps = taskManager->getTargetFPSPtr( iTaskId );
        m_autoFPS = taskManager->getAutoFPSPtr( iTaskId );
        m_nextUpdateTime = taskManager->getNextUpdateTimePtr( iTaskId );
        m_tickCount = taskManager->getTickCountPtr( iTaskId );
        m_stopped = taskManager->getStoppedPtr( iTaskId );
        m_threadTaskFlags = taskManager->getThreadTaskFlagsPtr( iTaskId );

        setLoadingState( LoadingState::Loaded );
    }

    void Task::reload( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        unload( data );
        load( data );
    }

    void Task::unload( SmartPtr<ISharedObject> data )
    {
        if( isLoaded() )
        {
            ScopedLock lock( this );

            setLoadingState( LoadingState::Unloading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = (TaskManager *)applicationManager->getTaskManagerPtr();

            auto taskId = getTask();

            if( taskManager )
            {
                if( auto fsmManager = applicationManager->getFsmManagerByTask( taskId ) )
                {
                    fsmManager->destroyFSM( m_fsm );
                }
            }

            applicationManager->setFsmManagerByTask( taskId, nullptr );

            m_fsm = nullptr;

            m_profile = nullptr;
            stopJobsFromQueue( m_jobs );

            m_owner = nullptr;

            setLoadingState( LoadingState::Unloaded );
        }
    }

    void Task::update()
    {
        try
        {
            if( !isLoaded() )
            {
                return;
            }

            ScopedLock lock( this );

            auto applicationManager = core::IApplicationManager::instancePtr();
            auto taskManager = applicationManager->getTaskManagerPtr();

            auto eTask = getTask();
            if( auto fsmManager = applicationManager->getFsmManagerByTask( eTask ) )
            {
                fsmManager->update();
            }

            if( auto fsm = getFSM() )
            {
                Flags flags( this );

                fsm->updateState();

                auto fsmCurrentState = fsm->getCurrentState();
                auto eState = static_cast<State>( fsmCurrentState );
                switch( eState )
                {
                case State::None:
                {
                }
                break;
                case State::Idle:
                {
                }
                break;
                case State::Stopped:
                {
                    //WP_LOG( "Task stopped: " + Thread::getTaskName( Thread::getCurrentTask() ) );
                }
                break;
                case State::Executing:
                {
                    if( isStopped() )
                    {
                        return;
                    }

                    auto timer = applicationManager->getTimerPtr();

                    auto nextUpdateTime = getNextUpdateTime();
                    if( nextUpdateTime < timer->now() )
                    {
                        auto profile = getProfile();
                        if( profile )
                        {
                            profile->start();
                        }

                        auto eCurrentTask = Thread::getCurrentTask();

                        Thread::setCurrentTask( eTask );

                        auto taskFlags = getThreadTaskFlags();

                        if( eTask != TaskId::Primary )
                        {
                            if( isPrimary() )
                            {
                                taskFlags = taskFlags | Thread::Primary_Flag;
                            }
                        }
                        else
                        {
                            auto tasks = taskManager->getTasks();
                            for( auto &task : tasks )
                            {
                                if( task == this )
                                {
                                    continue;
                                }

                                if( task->isPrimary() )
                                {
                                    taskFlags = taskFlags | task->getThreadTaskFlags();
                                }
                            }
                        }

                        Thread::setTaskFlags( taskFlags );

                        timer->update();

                        if( !m_jobs.empty() )
                        {
                            SmartPtr<IJob> job;
                            while( m_jobs.try_pop( job ) )
                            {
                                if( job )
                                {
                                    if( job->isInterrupted() )
                                    {
                                        job->setState( IJob::State::Finish );
                                    }
                                    else
                                    {
                                        job->setState( IJob::State::Executing );
                                        job->execute();
                                        job->setState( IJob::State::Finish );
                                    }

                                    job = nullptr;
                                }
                            }
                        }

                        if( auto owner = getOwner() )
                        {
                            try
                            {
                                owner->preUpdate();
                                owner->update();
                                owner->postUpdate();
                            }
                            catch( std::exception &e )
                            {
                                WP_LOG_EXCEPTION( e );
                            }
                        }

                        auto nextUpdateTime = getNextUpdateTime();
                        auto targetFPS = getTargetFPS();
                        if( targetFPS > std::numeric_limits<time_interval>::epsilon() )
                        {
                            auto rate = 1.0 / targetFPS;
                            nextUpdateTime = nextUpdateTime + rate;
                            setNextUpdateTime( nextUpdateTime );
                        }

                        if( profile )
                        {
                            profile->end();
                        }

                        Thread::setCurrentTask( eCurrentTask );
                    }
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

    void Task::reset()
    {
        m_tickCount = nullptr;
    }

    void Task::addJob( SmartPtr<IJob> job )
    {
        if( job )
        {
            auto state = getState();
            if( state != State::Shutdown )
            {
                job->setState( IJob::State::Queue );
                m_jobs.push( job );
            }
        }
    }

    void Task::clearEventJobs()
    {
        ScopedLock lock( this );
        clearEventJobsFromQueue( m_jobs );
    }

    void Task::setPrimary( bool usePrimary )
    {
        ScopedLock lock( this, true );

        if( usePrimary )
        {
            ( *m_taskFlags ) = *m_taskFlags | primary_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~primary_flag;
        }
    }

    bool Task::isPrimary() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & primary_flag ) != 0;
    }

    void Task::setRecycle( bool recycle )
    {
        ScopedLock lock( this, true );

        if( recycle )
        {
            ( *m_taskFlags ) = *m_taskFlags | recycle_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~recycle_flag;
        }
    }

    bool Task::getRecycle() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & recycle_flag ) != 0;
    }

    void Task::setAffinity( s32 id )
    {
        *m_affinity = id;
    }

    s32 Task::getAffinity() const
    {
        return *m_affinity;
    }

    bool Task::isParallel() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & parallel_flag ) != 0;
    }

    void Task::setParallel( bool parallel )
    {
        ScopedLock lock( this, true );

        if( parallel )
        {
            ( *m_taskFlags ) = *m_taskFlags | parallel_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~parallel_flag;
        }
    }

    bool Task::isEnabled() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & enabled_flag ) != 0;
    }

    void Task::setEnabled( bool enabled )
    {
        ScopedLock lock( this, true );

        if( enabled )
        {
            ( *m_taskFlags ) = *m_taskFlags | enabled_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~enabled_flag;
        }
    }

    TaskId Task::getTask() const
    {
        return m_taskId;
    }

    void Task::setTask( TaskId task )
    {
        m_taskId = task;
    }

    u32 Task::getThreadTaskFlags() const
    {
        ScopedLock lock( this, false );
        return *m_threadTaskFlags;
    }

    void Task::setThreadTaskFlags( u32 threadTaskFlags )
    {
        ScopedLock lock( this, true );
        *m_threadTaskFlags = threadTaskFlags;
    }

    void Task::stop()
    {
        if( !isLoaded() )
        {
            return;
        }

        WP_DEBUG_TRACE;

        --( *m_stopped );

        if( isUpdating() )
        {
            Thread::yield();
        }

        const auto task = getTask();
        const auto currentTask = Thread::getCurrentTask();

        if( task != currentTask )
        {
            if( auto fsm = getFSM() )
            {
                if( fsm->isLoaded() )
                {
                    auto currentState = static_cast<State>( fsm->getCurrentState() );

                    auto retryCount = 0;
                    while( currentState != State::Stopped && retryCount++ < 1000 )
                    {
                        if( try_lock() )
                        {
                            auto changeNow = !isUpdating();
                            fsm->setNewState( static_cast<u8>( State::Stopped ), changeNow );

                            currentState = static_cast<State>( fsm->getCurrentState() );
                            Thread::yield();
                            unlock();
                        }
                    }
                }
            }
        }

        WP_ASSERT( isStopped() == true );
    }

    void Task::start()
    {
        WP_DEBUG_TRACE;
        ++( *m_stopped );
    }

    bool Task::isStopped() const
    {
        return ( *m_stopped ) < 0;
    }

    bool Task::isUpdating() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & updating_flag ) != 0;
    }

    void Task::setUpdating( bool updating )
    {
        ScopedLock lock( this, true );

        if( updating )
        {
            ( *m_taskFlags ) = *m_taskFlags | updating_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~updating_flag;
        }
    }

    bool Task::isExecuting() const
    {
        return getState() == State::Executing;
    }

    f64 Task::getTargetFPS() const
    {
        return *m_targetfps;
    }

    void Task::setTargetFPS( f64 targetFPS )
    {
        *m_targetfps = targetFPS;
    }

    bool Task::getUseFixedTime() const
    {
        ScopedLock lock( this, false );
        return ( *m_taskFlags & fixed_time_flag ) != 0;
    }

    void Task::setUseFixedTime( bool useFixedTime )
    {
        ScopedLock lock( this, true );

        if( useFixedTime )
        {
            ( *m_taskFlags ) = *m_taskFlags | fixed_time_flag;
        }
        else
        {
            ( *m_taskFlags ) = *m_taskFlags & ~fixed_time_flag;
        }
    }

    time_interval Task::getNextUpdateTime() const
    {
        return *m_nextUpdateTime;
    }

    void Task::setNextUpdateTime( time_interval nextUpdateTime )
    {
        *m_nextUpdateTime = nextUpdateTime;
    }

    void Task::calculateAutoFPS()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto timer = applicationManager->getTimerPtr();

        auto fps = 1.0 / timer->getSmoothDeltaTime();
        if( fps > 0.0 )
        {
            auto targetFPS = static_cast<f64>( ( static_cast<s32>( fps ) - 50 ) / 100 * 100 );
            targetFPS = Math<f64>::clamp( targetFPS, 30.0, 60.0 );

            setAutoFPS( targetFPS );
        }
        else
        {
            setAutoFPS( 60.0 );
        }
    }

    f64 Task::getAutoFPS() const
    {
        return *m_autoFPS;
    }

    void Task::setAutoFPS( f64 autoFPS )
    {
        *m_autoFPS = autoFPS;
    }

    u32 Task::getTicks() const
    {
        return *m_tickCount;
    }

    SmartPtr<IProfile> Task::getProfile() const
    {
        return m_profile;
    }

    void Task::setProfile( SmartPtr<IProfile> profile )
    {
        m_profile = profile;
    }

    SmartPtr<IFSM> Task::getFSM() const
    {
        return m_fsm;
    }

    FSMReturnType Task::handleEvent( u32 state, FSMEvent eventType )
    {
        auto eState = static_cast<ITaskManager::State>( state );
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            switch( eState )
            {
            case ITaskManager::State::Shutdown:
            {
                stopJobsFromQueue( m_jobs );
            }
            break;
            }
        }
        break;
        case FSMEvent::Leave:
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

    void Task::lock()
    {
        m_mutex.lock();
    }

    bool Task::try_lock()
    {
        return m_mutex.try_lock();
    }

    void Task::unlock()
    {
        m_mutex.unlock();
    }

    void Task::setState( State state )
    {
        if( auto fsm = getFSM() )
        {
            auto iState = static_cast<u8>( state );
            fsm->setNewState( iState );
        }
    }

    Task::State Task::getState() const
    {
        if( auto fsm = getFSM() )
        {
            auto iState = fsm->getCurrentState();
            return static_cast<State>( iState );
        }

        return State::None;
    }

    SmartPtr<ISharedObject> Task::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    void Task::setOwner( SmartPtr<ISharedObject> owner )
    {
        m_owner = owner;
    }

    Task::Flags::Flags( SmartPtr<Task> task )
    {
        setTask( task );

        if( task )
        {
            task->setUpdating( true );

            if( auto fsm = task->getFSM() )
            {
                if( task->isStopped() )
                {
                    fsm->setNewState( static_cast<u32>( State::Stopped ), true );
                }
                else
                {
                    if( fsm->getCurrentState() == static_cast<u32>( State::Idle ) )
                    {
                        fsm->setNewState( static_cast<u32>( State::Executing ), true );
                    }
                    else
                    {
                        fsm->setNewState( static_cast<u32>( State::Idle ), true );
                    }
                }
            }
        }
    }

    Task::Flags::~Flags()
    {
        if( auto task = getTask() )
        {
            task->setUpdating( false );

            if( auto fsm = task->getFSM() )
            {
                auto currentState = static_cast<State>( fsm->getCurrentState() );
                if( currentState == State::Executing )
                {
                    fsm->setNewState( static_cast<u32>( State::Idle ), true );
                }
            }
        }
    }

    SmartPtr<Task> Task::Flags::getTask() const
    {
        return m_task;
    }

    void Task::Flags::setTask( SmartPtr<Task> task )
    {
        m_task = task;
    }
}  // namespace workphone
