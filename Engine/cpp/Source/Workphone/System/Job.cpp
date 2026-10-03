#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Job.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/System/JobYield.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Job, IJob );

    Job::Job() = default;

    Job::~Job() = default;

    auto Job::getState() const -> IJob::State
    {
        return m_state;
    }

    void Job::setState( State state )
    {
        if( m_state != state )
        {
            if( m_callbackFunction )
            {
                m_callbackFunction( static_cast<int>( state ) );
            }
        }

        m_state = state;
    }

    auto Job::getProgress() const -> u32
    {
        return m_progress;
    }

    void Job::setProgress( u32 progress )
    {
        m_progress = progress;
    }

    auto Job::getPriority() const -> s32
    {
        return m_priority;
    }

    void Job::setPriority( s32 priority )
    {
        m_priority = priority;
    }

    auto Job::isPrimary() const -> bool
    {
        return m_isPrimary;
    }

    void Job::setPrimary( bool primary )
    {
        m_isPrimary = primary;
    }

    auto Job::isFinished() const -> bool
    {
        auto state = getState();
        return state == State::Finish || state == State::Ready;
    }

    void Job::setInterrupted( bool interrupted )
    {
        m_interrupted = interrupted;
    }

    bool Job::isInterrupted() const
    {
        return m_interrupted;
    }

    void Job::stop()
    {
        setInterrupted( true );
    }

    auto Job::wait() -> bool
    {
        auto maxWaitTime = (f64)3.0;
        return wait( maxWaitTime );
    }

    auto Job::wait( f64 maxWaitTime ) -> bool
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto threadPool = applicationManager->getThreadPool();
        auto timer = applicationManager->getTimer();

        auto startTime = timer->now();

        if( threadPool )
        {
            if( threadPool->getNumThreads() > 0 )
            {
                while( getState() == IJob::State::Queue && ( timer->now() - startTime ) < maxWaitTime )
                {
                    Thread::yield();
                }

                while( !isFinished() && ( timer->now() - startTime ) < maxWaitTime )
                {
                    Thread::yield();
                }

                if( ( timer->now() - startTime ) < maxWaitTime )
                {
                    return true;
                }
            }
        }

        return false;
    }

    auto Job::getAffinity() const -> s32
    {
        return m_affinity;
    }

    void Job::setAffinity( s32 affinity )
    {
        m_affinity = affinity;
    }

    void Job::execute()
    {
    }

    void Job::coroutine_execute()
    {
        if( !m_yieldObject )
        {
            m_yieldObject = workphone::make_ptr<core::JobYield>( this );
        }

        coroutine_execute_step( m_yieldObject );
    }

    void Job::coroutine_execute_step( SmartPtr<ICoroutineData> &yield )
    {
        setState( State::Finish );
    }

    auto Job::isCoroutine() const -> bool
    {
        return m_isCoroutine;
    }

    void Job::setCoroutine( bool coroutine )
    {
        m_isCoroutine = coroutine;
    }

    void Job::setCallbackFunction( std::function<void( int )> callbackFunction )
    {
        m_callbackFunction = callbackFunction;
    }

}  // namespace workphone
