#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/JobQueue.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/System/JobCoroutine.hpp>
#include <Workphone/System/JobFunction.hpp>
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

        class ExecutingJobGuard
        {
        public:
            ExecutingJobGuard( ConcurrentArray<SmartPtr<IJob>> &executingJobs, SmartPtr<IJob> job ) :
                m_executingJobs( executingJobs ),
                m_job( job )
            {
                m_executingJobs.push_back( m_job );
            }

            ~ExecutingJobGuard()
            {
                m_executingJobs.erase( m_job );
            }

        private:
            ConcurrentArray<SmartPtr<IJob>> &m_executingJobs;
            SmartPtr<IJob> m_job;
        };
    }  // namespace

    void JobQueue::EventListener::setOwner( SmartPtr<JobQueue> owner )
    {
        m_owner = owner;
    }

    SmartPtr<JobQueue> JobQueue::EventListener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    Parameter JobQueue::EventListener::handleEvent( EventType eventType, hash_type eventValue,
                                                    const Array<Parameter> &arguments,
                                                    SmartPtr<ISharedObject> sender,
                                                    SmartPtr<ISharedObject> object,
                                                    SmartPtr<IEvent> event )
    {
        if (auto jobQueue = getOwner())
        {
            auto jobs = jobQueue->m_executingJobs;
            for( auto &job : jobs )
            {
                if( job )
                {
                    job->handleEvent( eventType, eventValue, arguments, sender, object, event );
                }
            }
        }

        return {};
    }

    JobQueue::EventListener::~EventListener()
    {
    }

    JobQueue::EventListener::EventListener()
    {
    }

    JobQueue::JobQueue() = default;

    JobQueue::~JobQueue() = default;

    void JobQueue::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_coroutineJobs.reserve( 32 );
        setLoadingState( LoadingState::Loaded );
    }

    void JobQueue::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        for( auto job : m_coroutineJobs )
        {
            job->stop();
            job->setState( IJob::State::Finish );
            job->unload( nullptr );
        }

        while( !m_primaryJobs.empty() )
        {
            SmartPtr<IJob> job;
            while( m_primaryJobs.try_pop( job ) )
            {
                if( job )
                {
                    job->stop();
                    job->setState( IJob::State::Finish );
                    job->unload( nullptr );
                }
            }
        }

        while( !m_jobs.empty() )
        {
            SmartPtr<IJob> job;
            while( m_jobs.try_pop( job ) )
            {
                if( job )
                {
                    job->stop();
                    job->setState( IJob::State::Finish );
                    job->unload( nullptr );
                }
            }
        }

        if( !m_executingJobs.empty() )
        {
            for( auto job : m_executingJobs )
            {
                if( job )
                {
                    job->stop();
                    job->setState( IJob::State::Finish );
                    job->unload( nullptr );
                }
            }
        }

        shutdown();
        m_coroutineJobs.clear();
        m_primaryJobs.clear();
        m_jobs.clear();
        m_executingJobs.clear();

        setLoadingState( LoadingState::Unloaded );
    }

    void JobQueue::update()
    {
        if( !isRunning() )
        {
            return;
        }

        auto currentTask = Thread::getCurrentTask();
        Thread::setCurrentTask( TaskId::None );

        auto threadId = Thread::getCurrentThreadId();
        switch( threadId )
        {
        case Thread::ThreadId::Primary:
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto removeCoroutineJobs = Array<SmartPtr<IJob>>();
            removeCoroutineJobs.reserve( m_coroutineJobs.size() );

            auto coroutineJobs = m_coroutineJobs.snapshot();
            for( auto &job : coroutineJobs )
            {
                if( job )
                {
                    if( job->isInterrupted() )
                    {
                        job->setState( IJob::State::Finish );
                        removeCoroutineJobs.push_back( job );
                        continue;
                    }

                    {
                        ExecutingJobGuard guard( m_executingJobs, job );
                        SmartPtr<ICoroutineData> yield;
                        job->coroutine_execute_step( yield );

                        if( job->getState() == IJob::State::Finish )
                        {
                            removeCoroutineJobs.push_back( job );
                        }
                    }
                }
            }

            // ConcurrentArray iterators are locking proxy iterators and must not be passed to
            // mutating STL algorithms. Remove through the container API instead, matching the
            // safe completion path used by JobQueueMT.
            for( auto &job : removeCoroutineJobs )
            {
                removeCoroutineJob( job );
            }

            SmartPtr<IJob> job;
            if( m_primaryJobs.try_pop( job ) )
            {
                executeJob( job );
            }

            auto threadPool = applicationManager->getThreadPool();
            if( ( threadPool == nullptr ) || ( threadPool && threadPool->getNumThreads() == 0 ) )
            {
                if( m_jobs.try_pop( job ) )
                {
                    executeJob( job );
                }
            }
        }
        break;
        default:
        {
            SmartPtr<IJob> job;
            if( m_jobs.try_pop( job ) )
            {
                executeJob( job );
            }
        }
        break;
        }

        Thread::setCurrentTask( currentTask );
    }

    bool JobQueue::hasJobs() const
    {
        return !m_primaryJobs.empty() || !m_jobs.empty() || !m_coroutineJobs.empty() ||
               hasExecutingJobs();
    }

    void JobQueue::addJob( SmartPtr<IJob> job )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        if( isRunning() == false )
        {
            return;
        }

        WP_ASSERT( job->getState() != IJob::State::Queue );
        job->setState( IJob::State::Queue );

        if( job->isPrimary() )
        {
            m_primaryJobs.push( job );
        }
        else
        {
            m_jobs.push( job );
        }
    }

    void JobQueue::addJob( SmartPtr<IJob> job, TaskId task )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        if( isRunning() == false )
        {
            return;
        }

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto pTask = taskManager->getTask( task );
        if( pTask )
        {
            pTask->addJob( job );
        }
    }

    void JobQueue::addJobAllTasks( SmartPtr<IJob> job )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        if( isRunning() == false )
        {
            return;
        }

        if( auto taskManager = applicationManager->getTaskManager() )
        {
            for( size_t i = 0; i < static_cast<size_t>( TaskId::Count ); ++i )
            {
                auto task = static_cast<TaskId>( i );
                auto pTask = taskManager->getTask( task );
                if( pTask )
                {
                    pTask->addJob( job );
                }
            }
        }
    }

    void JobQueue::clearEventJobs()
    {
        clearEventJobsFromQueue( m_primaryJobs );
        clearEventJobsFromQueue( m_jobs );

        auto coroutineJobs = m_coroutineJobs.snapshot();
        for( auto &job : coroutineJobs )
        {
            if( auto eventJob = dynamic_cast<EventJob *>( job.get() ) )
            {
                eventJob->unload( nullptr );
                m_coroutineJobs.erase( job );
            }
        }
    }

    bool JobQueue::isRunning() const
    {
        return m_isRunning;
    }

    void JobQueue::setRunning( bool running )
    {
        m_isRunning = running;
    }

    f32 JobQueue::getRate() const
    {
        return (f32)m_rate;
    }

    void JobQueue::setRate( f32 rate )
    {
        m_rate = (f64)rate;
    }

    bool JobQueue::getUseAffinity() const
    {
        return m_useAffinity;
    }

    void JobQueue::setUseAffinity( bool affinity )
    {
        m_useAffinity = affinity;
    }

    void JobQueue::shutdown()
    {
        setRunning( false );
        stopAllJobs();

        auto count = 0;
        while( hasExecutingJobs() && count++ < 100 )
        {
            stopAllJobs();
            Thread::sleep( 0.1 );
        }
    }

    void JobQueue::executeJob( SmartPtr<IJob> job )
    {
        if( !job )
        {
            return;
        }

        if( job->isInterrupted() )
        {
            job->setState( IJob::State::Finish );
            return;
        }

        ExecutingJobGuard guard( m_executingJobs, job );
        job->setState( IJob::State::Executing );

        try
        {
            if( !job->isInterrupted() )
            {
                job->execute();
            }
        }
        catch( ... )
        {
            job->setState( IJob::State::Finish );
            throw;
        }

        job->setState( IJob::State::Finish );
    }

    void JobQueue::stopAllJobs()
    {
        stopJobsFromQueue( m_primaryJobs );
        stopJobsFromQueue( m_jobs );

        auto coroutineJobs = m_coroutineJobs.snapshot();
        for( auto &job : coroutineJobs )
        {
            if( job )
            {
                job->stop();
                job->setState( IJob::State::Finish );
            }
        }
        m_coroutineJobs.clear();

        auto executingJobs = m_executingJobs.snapshot();
        for( auto &job : executingJobs )
        {
            if( job && !job->isFinished() )
            {
                job->stop();
            }
        }
    }

    bool JobQueue::hasExecutingJobs() const
    {
        auto executingJobs = m_executingJobs.snapshot();
        for( auto &job : executingJobs )
        {
            if( job && !job->isFinished() )
            {
                return true;
            }
        }

        return false;
    }

    void JobQueue::startCoroutine( std::function<void( ICoroutineData::PullType & )> func )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        if( isRunning() == false )
        {
            return;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        if( auto job = factoryManager->make_ptr<JobCoroutine>() )
        {
            job->setFunction( func );
            job->setCoroutine( true );
            addCoroutineJob( job );
        }
    }

    SmartPtr<IJob> JobQueue::startJob( std::function<void()> func )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return nullptr;
        }

        if( applicationManager->isRunning() == false )
        {
            return nullptr;
        }

        if( isRunning() == false )
        {
            return nullptr;
        }

        auto factoryManager = applicationManager->getFactoryManager();
        WP_ASSERT( factoryManager );

        if( auto job = factoryManager->make_ptr<JobFunction>() )
        {
            job->setFunction( func );
            addJob( job );

            return job;
        }

        return nullptr;
    }

    void JobQueue::lock()
    {
        m_mutex.lock();
    }

    bool JobQueue::try_lock()
    {
        return m_mutex.try_lock();
    }

    void JobQueue::unlock()
    {
        m_mutex.unlock();
    }

    Array<SmartPtr<IJob>> JobQueue::getCoroutineJobs() const
    {
        return m_coroutineJobs.snapshot();
    }

    void JobQueue::addCoroutineJob( SmartPtr<IJob> job )
    {
        m_coroutineJobs.push_back( job );
    }

    void JobQueue::removeCoroutineJob( SmartPtr<IJob> job )
    {
        auto it = std::find( m_coroutineJobs.begin(), m_coroutineJobs.end(), job );
        if( it != m_coroutineJobs.end() )
        {
            m_coroutineJobs.erase( it );
        }
    }

}  // namespace workphone
