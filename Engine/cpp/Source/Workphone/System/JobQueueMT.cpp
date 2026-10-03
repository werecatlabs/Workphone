#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/JobQueueMT.hpp>
#include <Workphone/System/JobCoroutine.hpp>
#include <Workphone/System/JobFunction.hpp>
#include <Workphone/Interface/System/IJob.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/StringUtil.hpp>
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

        class CurrentTaskGuard
        {
        public:
            CurrentTaskGuard() : m_task( Thread::getCurrentTask() )
            {
                Thread::setCurrentTask( TaskId::None );
            }

            ~CurrentTaskGuard()
            {
                Thread::setCurrentTask( m_task );
            }

        private:
            TaskId m_task = TaskId::None;
        };
    }  // namespace

    JobQueueMT::JobQueueMT()
    {
        m_numWorkerThreads = Thread::hardware_concurrency();
        createWorkerThreads();
    }

    JobQueueMT::~JobQueueMT()
    {
        shutdown();
        destroyWorkerThreads();
    }

    void JobQueueMT::update()
    {
        if( !isRunning() )
        {
            return;
        }

        auto thread = Thread::getCurrentThreadId();
        CurrentTaskGuard taskGuard;

        switch( thread )
        {
        case Thread::ThreadId::Primary:
        {
            Array<SmartPtr<IJob>> removeCoroutineJobs;
            if( auto coroutineJobs = getCoroutineJobsPtr() )
            {
                auto jobs = *coroutineJobs;
                removeCoroutineJobs.reserve( jobs.size() );

                for( auto &job : jobs )
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
                        }

                        if( job->getState() == IJob::State::Finish )
                        {
                            removeCoroutineJobs.push_back( job );
                        }
                    }
                }
            }

            for( auto &job : removeCoroutineJobs )
            {
                removeCoroutineJob( job );
            }

            SmartPtr<IJob> job;
            while( m_primaryJobQueue.try_pop( job ) )
            {
                executeJob( job );
            }
        }
        break;
        default:
        {
            try
            {
                SmartPtr<IJob> job;
                while( m_jobQueue.try_pop( job ) )
                {
                    executeJob( job );

                    Thread::yield();
                }
            }
            catch( Exception &e )
            {
                WP_LOG_EXCEPTION( e );
            }
        }
        }
    }

    bool JobQueueMT::hasJobs() const
    {
        return !m_primaryJobQueue.empty() || !m_jobQueue.empty() || !getCoroutineJobs().empty() ||
               hasExecutingJobs();
    }

    void JobQueueMT::addJob( SmartPtr<IJob> job )
    {
        if( !job || !isRunning() || isShutdownRequested() )
        {
            return;
        }

        job->setState( IJob::State::Queue );

        if( !job->isPrimary() )
        {
            m_jobQueue.push( job );
        }
        else
        {
            m_primaryJobQueue.push( job );
        }
    }

    void JobQueueMT::addJob( SmartPtr<IJob> job, TaskId task )
    {
        if( !job || !isRunning() || isShutdownRequested() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto pTask = taskManager->getTask( task );
        if( pTask )
        {
            pTask->addJob( job );
        }
    }

    void JobQueueMT::addJobAllTasks( SmartPtr<IJob> job )
    {
        if( !job || !isRunning() || isShutdownRequested() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

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

    void JobQueueMT::clearEventJobs()
    {
        clearEventJobsFromQueue( m_primaryJobQueue );
        clearEventJobsFromQueue( m_jobQueue );

        if( auto coroutineJobs = getCoroutineJobsPtr() )
        {
            auto &jobs = *coroutineJobs;
            jobs.erase( std::remove_if( jobs.begin(), jobs.end(),
                                        []( const SmartPtr<IJob> &job ) {
                                            if( auto eventJob = dynamic_cast<EventJob *>( job.get() ) )
                                            {
                                                eventJob->unload( nullptr );
                                                return true;
                                            }

                                            return false;
                                        } ),
                        jobs.end() );
        }
    }

    auto JobQueueMT::isRunning() const -> bool
    {
        return m_isRunning;
    }

    void JobQueueMT::setRunning( bool running )
    {
        m_isRunning = running;
    }

    auto JobQueueMT::getRate() const -> f32
    {
        return (f32)m_updateRate;
    }

    void JobQueueMT::setRate( f32 rate )
    {
        m_updateRate = (f64)rate;
    }

    auto JobQueueMT::getUseAffinity() const -> bool
    {
        return m_useAffinity;
    }

    void JobQueueMT::setUseAffinity( bool affinity )
    {
        m_useAffinity = affinity;
    }

    void JobQueueMT::shutdown()
    {
        setShutdownRequested( true );
        setRunning( false );
        stopAllJobs();

        u32 count = 0;
        while( hasExecutingJobs() && count++ < getShutdownWaitCount() )
        {
            stopAllJobs();
            Thread::sleep( getShutdownWaitInterval() );
        }
    }

    void JobQueueMT::executeJob( SmartPtr<IJob> job )
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

    void JobQueueMT::stopAllJobs()
    {
        stopJobsFromQueue( m_primaryJobQueue );
        stopJobsFromQueue( m_jobQueue );

        if( auto coroutineJobs = getCoroutineJobsPtr() )
        {
            auto &jobs = *coroutineJobs;
            for( auto &job : jobs )
            {
                if( job )
                {
                    job->stop();
                    job->setState( IJob::State::Finish );
                }
            }

            jobs.clear();
        }

        auto executingJobs = m_executingJobs.snapshot();
        for( auto &job : executingJobs )
        {
            if( job && !job->isFinished() )
            {
                job->stop();
            }
        }
    }

    bool JobQueueMT::hasExecutingJobs() const
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

    void JobQueueMT::startCoroutine( std::function<void( ICoroutineData::PullType & )> func )
    {
        if( !isRunning() || isShutdownRequested() )
        {
            return;
        }

        auto job = workphone::make_ptr<JobCoroutine>();
        job->setFunction( func );
        job->setCoroutine( true );
        addCoroutineJob( job );
    }

    SmartPtr<IJob> JobQueueMT::startJob( std::function<void()> func )
    {
        if( !isRunning() || isShutdownRequested() )
        {
            return nullptr;
        }

        auto job = workphone::make_ptr<JobFunction>();
        job->setFunction( func );
        addJob( job );

        return job;
    }

    u32 JobQueueMT::getNumWorkerThreads() const
    {
        return m_numWorkerThreads;
    }

    void JobQueueMT::setNumWorkerThreads( u32 numWorkerThreads )
    {
        if( getNumWorkerThreads() == numWorkerThreads )
        {
            return;
        }

        auto wasRunning = isRunning();
        auto wasShutdownRequested = isShutdownRequested();

        setShutdownRequested( true );
        setRunning( false );
        destroyWorkerThreads();

        m_numWorkerThreads = numWorkerThreads;
        setShutdownRequested( wasShutdownRequested );
        setRunning( wasRunning );

        if( !wasShutdownRequested )
        {
            createWorkerThreads();
        }
    }

    u32 JobQueueMT::getShutdownWaitCount() const
    {
        return m_shutdownWaitCount;
    }

    void JobQueueMT::setShutdownWaitCount( u32 shutdownWaitCount )
    {
        m_shutdownWaitCount = shutdownWaitCount;
    }

    f64 JobQueueMT::getShutdownWaitInterval() const
    {
        return m_shutdownWaitInterval;
    }

    void JobQueueMT::setShutdownWaitInterval( f64 shutdownWaitInterval )
    {
        m_shutdownWaitInterval = shutdownWaitInterval;
    }

    Array<SmartPtr<IJob>> JobQueueMT::getCoroutineJobs() const
    {
        if( auto p = getCoroutineJobsPtr() )
        {
            auto &coroutineJobs = *p;
            return coroutineJobs;
        }

        return Array<SmartPtr<IJob>>();
    }

    void JobQueueMT::addCoroutineJob( SmartPtr<IJob> job )
    {
        auto p = getCoroutineJobsPtr();
        if( !p )
        {
            p = workphone::make_shared<Array<SmartPtr<IJob>>>();
            setCoroutineJobsPtr( p );
        }

        if( p )
        {
            auto &coroutineJobs = *p;
            coroutineJobs.push_back( job );
        }
    }

    void JobQueueMT::removeCoroutineJob( SmartPtr<IJob> job )
    {
        auto p = getCoroutineJobsPtr();
        if( p )
        {
            auto &coroutineJobs = *p;
            auto it = std::find( coroutineJobs.begin(), coroutineJobs.end(), job );
            if( it != coroutineJobs.end() )
            {
                coroutineJobs.erase( it );
            }
        }
    }

    SharedPtr<Array<SmartPtr<IJob>>> JobQueueMT::getCoroutineJobsPtr() const
    {
        return m_coroutineJobs;
    }

    void JobQueueMT::setCoroutineJobsPtr( SharedPtr<Array<SmartPtr<IJob>>> coroutineJobs )
    {
        m_coroutineJobs = coroutineJobs;
    }

    bool JobQueueMT::isShutdownRequested() const
    {
        return m_shutdownRequested;
    }

    void JobQueueMT::setShutdownRequested( bool shutdownRequested )
    {
        m_shutdownRequested = shutdownRequested;
    }

    void JobQueueMT::createWorkerThreads()
    {
        if( !m_threads.empty() )
        {
            return;
        }

        auto numWorkerThreads = getNumWorkerThreads();
        m_threads.reserve( numWorkerThreads );

        for( u32 i = 0; i < numWorkerThreads; ++i )
        {
            auto threadId = getWorkerThreadId( i );
            m_threads.push_back( new boost::thread( WorkerThread( this, threadId ) ) );
        }
    }

    void JobQueueMT::destroyWorkerThreads()
    {
        for( auto thread : m_threads )
        {
            if( thread )
            {
                if( thread->joinable() )
                {
                    thread->join();
                }

                delete thread;
            }
        }

        m_threads.clear();
    }

    Thread::ThreadId JobQueueMT::getWorkerThreadId( u32 index ) const
    {
        auto workerThreadId = static_cast<s32>( Thread::ThreadId::WorkerThread );
        return static_cast<Thread::ThreadId>( workerThreadId + static_cast<s32>( index ) );
    }

    auto JobQueueMT::isProcessing( s32 id ) const -> bool
    {
        auto executingJobs = m_executingJobs.snapshot();
        for( auto &job : executingJobs )
        {
            if( job && !job->isFinished() && job->getAffinity() == id )
            {
                return true;
            }
        }

        return false;
    }

    JobQueueMT::WorkerThread::WorkerThread( JobQueueMT *jobQueue, Thread::ThreadId threadId ) :
        m_jobQueue( jobQueue ),
        m_threadId( threadId )
    {
    }

    void JobQueueMT::WorkerThread::operator()()
    {
        try
        {
            auto jobQueue = m_jobQueue;
            if( !jobQueue )
            {
                return;
            }

            Thread::setCurrentThreadId( m_threadId );

            while( !jobQueue->isShutdownRequested() )
            {
                if( jobQueue->isRunning() )
                {
                    jobQueue->update();
                }

                auto sleepTime = static_cast<time_interval>( jobQueue->getRate() );
                if( sleepTime > time_interval( 0.0 ) )
                {
                    Thread::sleep( sleepTime );
                }
                else
                {
                    Thread::yield();
                }
            }
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone
