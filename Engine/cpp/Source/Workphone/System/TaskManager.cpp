#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TaskManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <Workphone/Core/BitUtil.hpp>
#include <Workphone/Core/DebugTrace.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/IProfile.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Interface/System/IFSM.hpp>
#include <Workphone/System/FSMListenerT.hpp>
#include <Workphone/System/Task.hpp>
#include <Workphone/System/FSMManager.hpp>
#include <Workphone/System/TaskLock.hpp>
#include <utility>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TaskManager, ITaskManager );
    u32 TaskManager::m_idExt = 0;

    TaskManager::TaskManager()
    {
        static const String name = "TaskManager";
        setName( name );

        m_idCount = 0;
    }

    TaskManager::~TaskManager() = default;

    void TaskManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "TaskManager::load: null application manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            if( !factoryManager )
            {
                WP_LOG_ERROR( "TaskManager::load: null factory manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto appFsmManager = applicationManager->getFsmManagerPtr();
            if( !appFsmManager )
            {
                WP_LOG_ERROR( "TaskManager::load: null FSM manager from application manager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            m_fsm = appFsmManager->createFSM();
            if( !m_fsm )
            {
                WP_LOG_ERROR( "TaskManager::load: failed to create FSM." );
                setLoadingState( LoadingState::Error );
                return;
            }

            auto fsmListener = workphone::make_ptr<FSMListenerT<TaskManager>>();
            if( !fsmListener )
            {
                WP_LOG_ERROR( "TaskManager::load: failed to create FSMListener." );
                setLoadingState( LoadingState::Error );
                return;
            }

            fsmListener->setOwner( this );
            m_fsm->addListener( fsmListener );

            const auto numTasks = static_cast<u32>( TaskId::Count );

            auto fsmManager = factoryManager->make_ptr<FSMManager>();
            if( !fsmManager )
            {
                WP_LOG_ERROR( "TaskManager::load: failed to create FSMManager." );
                setLoadingState( LoadingState::Error );
                return;
            }

            fsmManager->setGrowSize( numTasks );
            fsmManager->load( nullptr );
            setFSMManager( fsmManager );

            for( u32 count = 0; count < numTasks; ++count )
            {
                auto task = factoryManager->make_ptr<Task>();
                if( !task )
                {
                    WP_LOG_ERROR( "TaskManager::load: failed to create Task for slot " +
                                  StringUtil::toString( count ) + "." );
                    setLoadingState( LoadingState::Error );
                    return;
                }

                task->setTask( static_cast<TaskId>( count ) );
                task->load( nullptr );
                m_tasks[count] = task;
            }

            for( auto &nextUpdateTime : m_nextUpdateTimes )
            {
                nextUpdateTime = 0.0;
            }

            for( auto &affinity : m_affinity )
            {
                affinity = -1;
            }

            for( size_t i = 0; i < numTasks; ++i )
            {
                m_taskIds[i] = static_cast<TaskId>( i );
            }

            for( u32 i = 0; i < numTasks; ++i )
            {
                setFlags( i, Task::enabled_flag, true );
                setFlags( i, Task::executing_flag, false );
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
            setLoadingState( LoadingState::Error );
        }
    }

    void TaskManager::reload( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        unload( data );
        load( data );
    }

    void TaskManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            if( isLoaded() )
            {
                ScopedLock lock( this );

                setLoadingState( LoadingState::Unloading );

                auto applicationManager = core::IApplicationManager::instance();
                if( !applicationManager )
                {
                    WP_LOG_WARNING(
                        "TaskManager::unload: null application manager; skipping FSM cleanup." );
                }
                else
                {
                    if( m_fsm )
                    {
                        if( auto appFsmManager = applicationManager->getFsmManager() )
                        {
                            appFsmManager->destroyFSM( m_fsm );
                        }

                        m_fsm = nullptr;
                    }
                }

                if( m_fsmManager )
                {
                    m_fsmManager->unload( nullptr );
                    m_fsmManager = nullptr;
                }

                for( auto &task : m_tasks )
                {
                    if( task )
                    {
                        task->unload( nullptr );
                    }
                    else
                    {
                        WP_LOG_WARNING( "TaskManager::unload: null task encountered; skipping." );
                    }
                }

                setLoadingState( LoadingState::Unloaded );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TaskManager::calculateTaskAffinity()
    {
        if( !isLoaded() )
            return;

        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            WP_LOG_ERROR( "TaskManager::calculateTaskAffinity: null application manager." );
            return;
        }

        if( auto threadPool = applicationManager->getThreadPoolPtr() )
        {
            auto taskCount = 0;
            const auto numThreads = threadPool->getNumThreads();
            const auto numTasks = static_cast<u32>( TaskId::Count );

            for( u32 i = 0; i < numTasks; ++i )
            {
                if( i >= m_affinity.size() || i >= m_threadHint.size() )
                {
                    WP_LOG_ERROR( "TaskManager::calculateTaskAffinity: index " +
                                  StringUtil::toString( static_cast<s64>( i ) ) +
                                  " out of range for affinity/hint arrays." );
                    break;
                }

                auto &affinity = m_affinity[i];
                auto &hint = m_threadHint[i];

                if( affinity == -1 )
                {
                    if( getFlags( i, Task::enabled_flag ) )
                    {
                        hint = MathI::Mod( taskCount, (s32)numThreads );
                        taskCount++;
                    }
                }
                else
                {
                    if( getFlags( i, Task::enabled_flag ) )
                    {
                        hint = MathI::Mod( affinity, (s32)numThreads );
                        taskCount++;
                    }
                }
            }
        }
        else
        {
            for( auto &hint : m_threadHint )
            {
                hint = static_cast<s32>( Thread::ThreadId::Primary );
            }
        }
    }

    void TaskManager::update()
    {
        WP_DEBUG_TRACE;

        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                WP_LOG_ERROR( "TaskManager::update: null application manager." );
                return;
            }

            auto threadPool = applicationManager->getThreadPoolPtr();

            auto timer = applicationManager->getTimerPtr();
            if( !timer )
            {
                WP_LOG_ERROR( "TaskManager::update: null timer." );
                return;
            }

            auto t = timer->now();

            calculateTaskAffinity();

            auto threadId = Thread::getCurrentThreadId();

            switch( threadId )
            {
            case Thread::ThreadId::Primary:
            {
                if( auto fsmManager = getFSMManager() )
                {
                    fsmManager->update();
                }
            }
            break;
            default:
                break;
            }

            auto &tasks = m_tasks;

            auto taskManagerState = getState();
            switch( taskManagerState )
            {
            case State::FreeStep:
            {
                if( threadPool )
                {
                    switch( threadId )
                    {
                    case Thread::ThreadId::Primary:
                    {
                        auto numThreads = threadPool->getNumThreads();
                        if( numThreads == 0 )
                        {
                            const auto numTasks = static_cast<size_t>( TaskId::Count );
                            for( size_t i = 0; i < numTasks; ++i )
                            {
                                if( !getFlags( static_cast<u32>( i ), Task::enabled_flag ) )
                                    continue;

                                if( i >= tasks.size() )
                                    continue;

                                auto &pTask = tasks[i];
                                if( !pTask )
                                {
                                    WP_LOG_WARNING( "TaskManager::update: null task at index " +
                                                    StringUtil::toString( static_cast<s64>( i ) ) +
                                                    "; skipping." );
                                    continue;
                                }

                                if( i < m_nextUpdateTimes.size() && m_nextUpdateTimes[i] < t )
                                {
                                    pTask->update();
                                }
                            }
                        }
                        else
                        {
                            const auto numTasks = static_cast<size_t>( TaskId::Count );
                            for( size_t i = 0; i < numTasks; ++i )
                            {
                                if( !getFlags( static_cast<u32>( i ), Task::primary_flag ) )
                                    continue;

                                if( !getFlags( static_cast<u32>( i ), Task::enabled_flag ) )
                                    continue;

                                if( i >= tasks.size() )
                                    continue;

                                auto &pTask = tasks[i];
                                if( !pTask )
                                {
                                    WP_LOG_WARNING( "TaskManager::update: null task at index " +
                                                    StringUtil::toString( static_cast<s64>( i ) ) +
                                                    "; skipping." );
                                    continue;
                                }

                                if( i < m_nextUpdateTimes.size() && m_nextUpdateTimes[i] < t )
                                {
                                    pTask->update();
                                }
                            }
                        }
                    }
                    break;
                    default:
                    {
                        if( !m_threadHint.empty() )
                        {
                            const auto numTasks = static_cast<size_t>( TaskId::Count );
                            for( size_t i = 0; i < numTasks; ++i )
                            {
                                if( i >= m_threadHint.size() )
                                    break;

                                if( m_threadHint[i] != static_cast<u32>( threadId ) )
                                    continue;

                                if( !getFlags( static_cast<u32>( i ), Task::enabled_flag ) )
                                    continue;

                                if( getFlags( static_cast<u32>( i ), Task::primary_flag ) )
                                    continue;

                                if( i >= tasks.size() )
                                    continue;

                                auto &pTask = tasks[i];
                                if( !pTask )
                                {
                                    WP_LOG_WARNING( "TaskManager::update: null task at index " +
                                                    StringUtil::toString( static_cast<s64>( i ) ) +
                                                    "; skipping." );
                                    continue;
                                }

                                if( i < m_nextUpdateTimes.size() && m_nextUpdateTimes[i] < t )
                                {
                                    pTask->update();
                                }
                            }
                        }
                    }
                    }
                }
                else
                {
                    if( !m_threadHint.empty() )
                    {
                        const auto numTasks = static_cast<size_t>( TaskId::Count );
                        for( size_t i = 0; i < numTasks; ++i )
                        {
                            if( i >= m_threadHint.size() )
                                break;

                            if( m_threadHint[i] != static_cast<u32>( threadId ) )
                                continue;

                            if( !getFlags( static_cast<u32>( i ), Task::enabled_flag ) )
                                continue;

                            if( i >= tasks.size() )
                                continue;

                            auto &pTask = tasks[i];
                            if( !pTask )
                            {
                                WP_LOG_WARNING( "TaskManager::update: null task at index " +
                                                StringUtil::toString( static_cast<s64>( i ) ) +
                                                "; skipping." );
                                continue;
                            }

                            if( i < m_nextUpdateTimes.size() && m_nextUpdateTimes[i] < t )
                            {
                                pTask->update();
                            }
                        }
                    }
                }
            }
            break;
            default:
            {
            }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void TaskManager::setAffinity( u32 id, u32 affinity )
    {
        if( id >= m_affinity.size() )
        {
            WP_LOG_ERROR( "TaskManager::setAffinity: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_affinity.size() ) ) + ")." );
            return;
        }

        m_affinity[id] = affinity;
    }

    atomic_s32 *TaskManager::getStoppedPtr( u32 id )
    {
        return &m_stopped[id];
    }

    atomic_s32 *TaskManager::getAffinityPtr( u32 id )
    {
        if( id >= m_affinity.size() )
        {
            WP_LOG_ERROR( "TaskManager::getAffinityPtr: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_affinity.size() ) ) + ")." );
            return nullptr;
        }

        return &m_affinity[id];
    }

    u32 TaskManager::getAffinity( u32 id ) const
    {
        if( id >= m_affinity.size() )
        {
            WP_LOG_ERROR( "TaskManager::getAffinity: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_affinity.size() ) ) + ")." );
            return 0;
        }

        return m_affinity[id];
    }

    SmartPtr<ITask> TaskManager::getTask( TaskId taskId ) const
    {
        if( isLoaded() )
        {
            const auto iTaskIndex = static_cast<size_t>( taskId );
            if( iTaskIndex >= m_tasks.size() )
            {
                WP_LOG_ERROR( "TaskManager::getTask: taskId " +
                              StringUtil::toString( static_cast<u32>( taskId ) ) +
                              " out of range (size=" +
                              StringUtil::toString( static_cast<u32>( m_tasks.size() ) ) + ")." );
                return nullptr;
            }

            return m_tasks[iTaskIndex];
        }

        return nullptr;
    }

    ITask *TaskManager::getTaskPtr( TaskId taskId ) const
    {
        if( isLoaded() )
        {
            const auto iTaskIndex = static_cast<size_t>( taskId );
            if( iTaskIndex >= m_tasks.size() )
            {
                WP_LOG_ERROR( "TaskManager::getTaskPtr: taskId " +
                              StringUtil::toString( static_cast<u32>( taskId ) ) +
                              " out of range (size=" +
                              StringUtil::toString( static_cast<u32>( m_tasks.size() ) ) + ")." );
                return nullptr;
            }

            return m_tasks[iTaskIndex].get();
        }

        return nullptr;
    }

    void TaskManager::setState( State state )
    {
        if( auto fsm = getFsm() )
        {
            fsm->setState( static_cast<u8>( state ), true );
        }
    }

    ITaskManager::State TaskManager::getState() const
    {
        if( auto fsm = getFsm() )
        {
            return fsm->getState<State>();
        }

        return State::None;
    }

    atomic_u32 TaskManager::getTaskFlagsPtr( u32 id )
    {
        return m_taskFlags[id];
    }

    u32 TaskManager::getTaskFlags( u32 id ) const
    {
        if( id >= m_taskFlags.size() )
        {
            WP_LOG_ERROR( "TaskManager::getTaskFlags: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskFlags.size() ) ) + ")." );
            return 0;
        }

        return m_taskFlags[id];
    }

    void TaskManager::setTaskFlags( u32 id, u32 flags )
    {
        if( id >= m_taskFlags.size() )
        {
            WP_LOG_ERROR( "TaskManager::setTaskFlags: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskFlags.size() ) ) + ")." );
            return;
        }

        m_taskFlags[id] = flags;
    }

    atomic_u32 *TaskManager::getThreadTaskFlagsPtr( u32 id )
    {
        return &m_threadTaskFlags[id];
    }

    TaskId TaskManager::getTaskId( u32 id ) const
    {
        if( id >= m_taskIds.size() )
        {
            WP_LOG_ERROR( "TaskManager::getTaskId: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskIds.size() ) ) + ")." );
            return TaskId::None;
        }

        return m_taskIds[id];
    }

    void TaskManager::setTaskId( u32 id, TaskId task )
    {
        if( id >= m_taskIds.size() )
        {
            WP_LOG_ERROR( "TaskManager::setTaskId: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskIds.size() ) ) + ")." );
            return;
        }

        m_taskIds[id] = task;
    }

    void TaskManager::setFlags( u32 id, u32 flag, bool value )
    {
        if( id >= m_taskFlags.size() )
        {
            WP_LOG_ERROR( "TaskManager::setFlags: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskFlags.size() ) ) + ")." );
            return;
        }

        const auto &flags = m_taskFlags[id].load();
        m_taskFlags[id] = BitUtil::setFlagValue( flags, flag, value );
    }

    atomic_u32 *TaskManager::getFlagsPtr( u32 id ) const
    {
        return const_cast<atomic_u32 *>( &m_taskFlags[id] );
    }

    atomic_f64 *TaskManager::getTargetFPSPtr( u32 id )
    {
        return &m_targetfps[id];
    }

    f64 TaskManager::getTargetFPS( u32 id ) const
    {
        if( id >= m_targetfps.size() )
        {
            WP_LOG_ERROR( "TaskManager::getTargetFPS: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_targetfps.size() ) ) + ")." );
            return 0.0;
        }

        return m_targetfps[id];
    }

    void TaskManager::setTargetFPS( u32 id, f64 targetfps )
    {
        if( id >= m_targetfps.size() )
        {
            WP_LOG_ERROR( "TaskManager::setTargetFPS: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_targetfps.size() ) ) + ")." );
            return;
        }

        m_targetfps[id] = targetfps;
    }

    atomic_f64 *TaskManager::getAutoFPSPtr( u32 id )
    {
        return &m_autoFPS[id];
    }

    atomic_f64 *TaskManager::getNextUpdateTimePtr( u32 id )
    {
        return &m_nextUpdateTimes[id];
    }

    f64 TaskManager::getNextUpdateTime( u32 id ) const
    {
        return m_nextUpdateTimes[id];
    }

    void TaskManager::setNextUpdateTime( u32 id, f64 nextUpdateTime )
    {
        m_nextUpdateTimes[id] = nextUpdateTime;
    }

    atomic_u32 *TaskManager::getTickCountPtr( u32 id )
    {
        return &m_tickCount[id];
    }

    TaskId *TaskManager::getTaskIdsPtr( u32 id )
    {
        return &m_taskIds[id];
    }

    TaskId TaskManager::getTaskIds( u32 id ) const
    {
        if( id >= m_taskIds.size() )
        {
            WP_LOG_ERROR( "TaskManager::getTaskIds: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskIds.size() ) ) + ")." );
            return TaskId::None;
        }

        return m_taskIds[id];
    }

    void TaskManager::setTaskIds( u32 id, TaskId taskId )
    {
        if( id >= m_taskIds.size() )
        {
            WP_LOG_ERROR( "TaskManager::setTaskIds: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskIds.size() ) ) + ")." );
            return;
        }

        m_taskIds[id] = taskId;
    }

    ITask::State *TaskManager::getTaskStatesPtr( u32 id )
    {
        return &m_states[id];
    }

    ITask::State TaskManager::getTaskState( u32 id ) const
    {
        if( id >= m_states.size() )
        {
            WP_LOG_ERROR( "TaskManager::getTaskState: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_states.size() ) ) + ")." );
            return ITask::State::None;
        }

        return m_states[id];
    }

    void TaskManager::setTaskState( u32 id, ITask::State state )
    {
        if( id >= m_states.size() )
        {
            WP_LOG_ERROR( "TaskManager::setTaskState: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_states.size() ) ) + ")." );
            return;
        }

        m_states[id] = state;
    }

    bool TaskManager::isValid() const
    {
        return true;
    }

    SmartPtr<IFSMManager> TaskManager::getFSMManager() const
    {
        return m_fsmManager;
    }

    void TaskManager::setFSMManager( SmartPtr<IFSMManager> fsmManager )
    {
        m_fsmManager = fsmManager;
    }

    SmartPtr<IFSM> TaskManager::getFsm() const
    {
        return m_fsm;
    }

    void TaskManager::setFsm( SmartPtr<IFSM> fsm )
    {
        m_fsm = fsm;
    }

    FSMReturnType TaskManager::handleEvent( u32 state, FSMEvent eventType )
    {
        auto eState = static_cast<State>( state );
        switch( eventType )
        {
        case FSMEvent::Enter:
        {
            switch( eState )
            {
            case State::Shutdown:
            {
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

    void TaskManager::setState( u32 id, ITask::State state )
    {
        if( id >= m_states.size() )
        {
            WP_LOG_ERROR( "TaskManager::setState: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_states.size() ) ) + ")." );
            return;
        }

        m_states[id] = state;
    }

    ITask::State TaskManager::getState( u32 id ) const
    {
        if( id >= m_states.size() )
        {
            WP_LOG_ERROR( "TaskManager::getState: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_states.size() ) ) + ")." );
            return ITask::State::None;
        }

        return m_states[id];
    }

    void TaskManager::addJobAllTasks( SmartPtr<IJob> job )
    {
        if( !job )
        {
            WP_LOG_WARNING( "TaskManager::addJobAllTasks: null job supplied; ignoring." );
            return;
        }

        for( auto &task : m_tasks )
        {
            if( task )
            {
                task->addJob( job );
            }
            else
            {
                WP_LOG_WARNING( "TaskManager::addJobAllTasks: null task encountered; skipping." );
            }
        }
    }

    void TaskManager::clearEventJobs()
    {
        for( auto &task : m_tasks )
        {
            if( task )
            {
                task->clearEventJobs();
            }
        }
    }

    Array<SmartPtr<ITask>> TaskManager::getTasks() const
    {
        return Array<SmartPtr<ITask>>( m_tasks.begin(), m_tasks.end() );
    }

    void TaskManager::wait()
    {
        for( auto &t : m_tasks )
        {
            if( !t )
            {
                WP_LOG_WARNING( "TaskManager::wait: null task encountered; skipping." );
                continue;
            }

            while( !( t->getState() == ITask::State::None || t->getState() == ITask::State::Idle ) )
            {
                Thread::yield();
            }
        }
    }

    void TaskManager::stop()
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( !applicationManager )
        {
            WP_LOG_WARNING(
                "TaskManager::stop: null application manager; stopping all tasks unconditionally." );
            for( auto &t : m_tasks )
            {
                if( t )
                    t->stop();
            }
            return;
        }

        if( applicationManager->isRunning() )
        {
            for( auto &t : m_tasks )
            {
                if( t )
                {
                    t->stop();
                }
                else
                {
                    WP_LOG_WARNING( "TaskManager::stop: null task encountered; skipping." );
                }
            }
        }
    }

    void TaskManager::reset()
    {
        for( auto &t : m_tasks )
        {
            if( t )
            {
                t->reset();
            }
            else
            {
                WP_LOG_WARNING( "TaskManager::reset: null task encountered; skipping." );
            }
        }
    }

    void TaskManager::shutdown()
    {
        if( !m_fsm )
        {
            WP_LOG_ERROR( "TaskManager::shutdown: no FSM set; cannot transition to Shutdown state." );
            return;
        }

        m_fsm->setNewState( static_cast<u8>( State::Shutdown ), true );
    }

    TaskLock TaskManager::lockTask( TaskId taskId )
    {
        if( auto task = getTask( taskId ) )
        {
            return TaskLock( task );
        }

        return {};
    }

    u32 TaskManager::getNumTasks() const
    {
        ScopedLock lock( this, false );

        auto count = 0;

        for( auto &flags : m_taskFlags )
        {
            if( ( flags & Task::enabled_flag ) != 0 )
            {
                ++count;
            }
        }

        return count;
    }

    bool TaskManager::getFlags( u32 id, u32 flag ) const
    {
        ScopedLock lock( this, false );

        if( id >= m_taskFlags.size() )
        {
            WP_LOG_ERROR( "TaskManager::getFlags: id " + StringUtil::toString( id ) +
                          " out of range (size=" +
                          StringUtil::toString( static_cast<u32>( m_taskFlags.size() ) ) + ")." );
            return false;
        }

        const auto &flags = m_taskFlags[id].load();
        return ( flags & flag ) != 0;
    }

    TaskManager::Lock::Lock( SmartPtr<ITaskManager> taskManager ) :
        m_taskManager( std::move( taskManager ) )
    {
        auto tasks = m_taskManager->getTasks();
        for( auto task : tasks )
        {
            if( task )
            {
                task->stop();
            }
        }
    }

    TaskManager::Lock::Lock() = default;

    TaskManager::Lock::~Lock()
    {
        if( !m_taskManager )
        {
            WP_LOG_WARNING( "TaskManager::Lock::~Lock: null task manager; cannot restart tasks." );
            return;
        }

        auto tasks = m_taskManager->getTasks();
        for( auto task : tasks )
        {
            if( task )
            {
                task->start();
            }
        }
    }
}  // namespace workphone
