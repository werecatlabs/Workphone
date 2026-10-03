#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TimerBoost.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TimerBoost, Timer );

    TimerBoost::TimerBoost() : m_bStarted( false )
    {
        auto size = static_cast<s32>( TaskId::Count );
        m_accumulated.resize( size );
        m_startOffset.resize( size );
        m_fixedOffset.resize( size );

        m_frameSmoothingTime.resize( size );

        m_fixedTimePoints.resize( size );
        m_fixedTime.resize( size );
        m_fixedTimeInterval.resize( size );

        m_prevTime.resize( size );
        m_time.resize( size );
        m_deltaTime.resize( size );

        m_smoothTime.resize( size );
        m_prevSmoothTime.resize( size );
        m_smoothDeltaTime.resize( size );

        m_ticks.resize( size );

        m_minDeltaTime.resize( size );
        m_maxDeltaTime.resize( size );

        m_eventTimes.resize( size );

        m_startPoint = boost::chrono::steady_clock::now();
    }

    TimerBoost::~TimerBoost() = default;

    void TimerBoost::update()
    {
        auto end = boost::chrono::steady_clock::now();
        auto seconds = boost::chrono::duration<f64>( end - m_startPoint );
        auto nowTime = seconds.count();

        auto eTask = Thread::getCurrentTask();
        auto task = static_cast<s32>( eTask );

        auto delta = nowTime - m_time[task];

        m_deltaTime[task] = delta;

        auto time = m_time[task];
        m_prevTime[task] = time;
        m_time[task] = nowTime;

        if( m_enableSmoothing && m_frameSmoothingTime[task] != 0.0 )
        {
            auto smoothDeltaTime = calculateEventTime( nowTime, m_eventTimes[task] );

            m_smoothDeltaTime[task] = smoothDeltaTime;
            m_prevSmoothTime[task] = m_smoothTime[task];
            m_smoothTime[task] = m_smoothTime[task] + smoothDeltaTime;
        }
        else
        {
            m_smoothDeltaTime[task] = delta;
            m_prevSmoothTime[task] = m_smoothTime[task];
            m_smoothTime[task] = m_time[task];
        }

        updateFixed();

        addAccumulated( eTask, delta );

        ++m_ticks[task];
    }

    void TimerBoost::update( f64 dt )
    {
        auto eTask = Thread::getCurrentTask();
        auto task = static_cast<s32>( eTask );

        m_deltaTime[task] = dt;

        auto time = m_time[task];
        m_prevTime[task] = time;
        m_time[task] = time + dt;

        m_smoothDeltaTime[task] = dt;
        m_prevSmoothTime[task] = m_smoothTime[task];
        m_smoothTime[task] = m_time[task];

        updateFixed();
        addAccumulated( eTask, dt );

        ++m_ticks[task];
    }

    void TimerBoost::updateFixed()
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_fixedTime[task] = m_fixedTime[task] + m_fixedTimeInterval[task];

        auto now = boost::chrono::high_resolution_clock::now();
        m_fixedTimePoints[task] = now;
    }

    auto TimerBoost::calculateEventTime( f64 fNow, Deque<f64> &times ) -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );

        // Calculate the average time passed between events of the given type
        // during the last mFrameSmoothingTime seconds.
        times.push_back( fNow );

        if( times.size() == 1 )
        {
            return 0;
        }

        // Find the oldest time to keep
        auto it = times.begin(),
             end = times.end() - 2;  // We need at least two times
        while( it != end )
        {
            if( fNow - *it > m_frameSmoothingTime[task] )
            {
                ++it;
            }
            else
            {
                break;
            }
        }

        // Remove old times
        times.erase( times.begin(), it );

        return ( times.back() - times.front() ) / ( times.size() - 1 );
    }

    void TimerBoost::reset()
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );

        m_currentPoint = m_startPoint = boost::chrono::high_resolution_clock::now();

        // for (size_t i = 0; i < s32(TaskId::Count); ++i)
        //{
        //	mEventTimes[i].fill(0.0);
        // }

        // for (size_t i = 0; i < s32(TaskId::Count); ++i)
        //{
        //	m_fixedTime[i] = 0.0;
        // }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_prevSmoothTime[i] = m_prevTime[i];
        }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_smoothTime[i] = m_time[i];
        }

        // for (size_t i = 0; i < s32(TaskId::Count); ++i)
        //{
        //	m_time[i] = 0.0;
        // }

        for( auto &value : m_ticks )
        {
            value = 0;
        }
    }

    void TimerBoost::reset( f64 t )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );

        setStartOffset( t );

        // for (size_t i = 0; i < s32(TaskId::Count); ++i)
        //{
        //	m_fixedTime[i] = 0.0;
        // }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_prevTime[i] = t;
        }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_time[i] = t;
        }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_prevSmoothTime[i] = m_prevTime[i];
        }

        for( size_t i = 0; i < static_cast<s32>( TaskId::Count ); ++i )
        {
            m_smoothTime[i] = m_time[i];
        }

        for( auto &value : m_ticks )
        {
            value = 0;
        }
    }

    auto TimerBoost::getTickCount() -> u32
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_ticks[task];
    }

    auto TimerBoost::getTickCount( TaskId task ) -> u32
    {
        auto iTask = static_cast<s32>( task );
        return m_ticks[iTask];
    }

    auto TimerBoost::isSteady() const -> bool
    {
        return boost::chrono::high_resolution_clock::is_steady;
    }

    auto TimerBoost::getDerivedFixedTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        s32 ticks = m_ticks[task];
        return static_cast<f64>( ticks ) * getFixedTimeInterval();
    }

    auto TimerBoost::getFixedTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_fixedTime[task];
    }

    auto TimerBoost::getFixedTime( u32 task ) const -> f64
    {
        return m_fixedTime[task];
    }

    auto TimerBoost::getFixedTimeNow() const -> f64
    {
        auto now = boost::chrono::high_resolution_clock::now();
        auto task = static_cast<s32>( Thread::getCurrentTask() );

        boost::chrono::duration<f64> p = m_fixedTimePoints[task] - m_startPoint;
        boost::chrono::duration<f64> seconds = now - m_fixedTimePoints[task];
        return getFixedTime() + seconds.count();
    }

    auto TimerBoost::getFixedTimeNow( u32 task ) const -> f64
    {
        boost::chrono::duration<f64> p = m_fixedTimePoints[task] - m_startPoint;
        return getFixedTime( task ) + ( now() - p.count() );
    }

    void TimerBoost::setFixedTime( f64 value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_fixedTime[task] = value;
    }

    auto TimerBoost::getFixedTimeInterval( TaskId task ) const -> f64
    {
        auto iTask = static_cast<s32>( task );
        return m_fixedTimeInterval[iTask];
    }

    auto TimerBoost::getFixedTimeInterval() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_fixedTimeInterval[task];
    }

    void TimerBoost::setFixedTimeInterval( f64 value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_fixedTimeInterval[task] = value;
    }

    void TimerBoost::setFixedTimeInterval( TaskId task, f64 value )
    {
        m_fixedTimeInterval[static_cast<u32>( task )] = value;
    }

    void TimerBoost::setFrameSmoothingTime( time_interval value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_frameSmoothingTime[task] = value;

        for( auto &t : m_frameSmoothingTime )
        {
            t = value;
        }
    }

    auto TimerBoost::getFrameSmoothingTime() const -> time_interval
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return (f32)m_frameSmoothingTime[task];
    }

    void TimerBoost::setStartTime( f64 time )
    {
        auto timeDuration = boost::chrono::microseconds( static_cast<long long>( time * 1000000.0 ) );
        m_startPoint = boost::chrono::high_resolution_clock::now() - timeDuration;
    }

    auto TimerBoost::getSmoothTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_smoothTime[task];
    }

    auto TimerBoost::getSmoothDeltaTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_smoothDeltaTime[task];
    }

    void TimerBoost::setSmoothDeltaTime( f64 smoothDT )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_smoothDeltaTime[task] = smoothDT;
        m_prevSmoothTime[task] = m_smoothTime[task];
        m_smoothTime[task] = m_smoothTime[task] + smoothDT;
    }

    auto TimerBoost::getMaxDeltaTime( TaskId task ) const -> f64
    {
        return m_maxDeltaTime[static_cast<s32>( task )];
    }

    void TimerBoost::setMaxDeltaTime( TaskId task, const f64 t )
    {
        m_maxDeltaTime[static_cast<s32>( task )] = t;
    }

    auto TimerBoost::getMinDeltaTime( TaskId task ) const -> f64
    {
        return m_minDeltaTime[static_cast<s32>( task )];
    }

    void TimerBoost::setMinDeltaTime( TaskId task, const f64 t )
    {
        m_minDeltaTime[static_cast<s32>( task )] = t;
    }

    auto TimerBoost::getStartOffset() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_startOffset[task];
    }

    void TimerBoost::setStartOffset( f64 value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_startOffset[task] = value;
    }

    auto TimerBoost::getStartOffset( TaskId task ) const -> f64
    {
        return m_startOffset[static_cast<s32>( task )];
    }

    void TimerBoost::setStartOffset( TaskId task, f64 value )
    {
        m_startOffset[static_cast<s32>( task )] = value;
    }

    auto TimerBoost::getFixedOffset( TaskId task ) const -> f64
    {
        return m_fixedOffset[static_cast<s32>( task )];
    }

    void TimerBoost::setFixedOffset( TaskId task, f64 offset )
    {
        m_fixedOffset[static_cast<s32>( task )] = offset;
    }

    auto TimerBoost::getAccumulated( TaskId task ) const -> f64
    {
        return m_accumulated[static_cast<s32>( task )];
    }

    void TimerBoost::setAccumulated( TaskId task, f64 value )
    {
        m_accumulated[static_cast<s32>( task )] = value;
    }

    void TimerBoost::addAccumulated( TaskId task, f64 value )
    {
        m_accumulated[static_cast<s32>( task )] += value;
    }

    auto TimerBoost::now() const -> f64
    {
        auto t = boost::chrono::steady_clock::now();
        boost::chrono::duration<f64> interval = t - m_startPoint;
        return interval.count();
    }
}  // namespace workphone
