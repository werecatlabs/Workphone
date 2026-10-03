#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TimerMT.hpp>
#include <Workphone/Math/MathUtil.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TimerMT, Timer );

    namespace
    {
        using TimerClock = std::chrono::steady_clock;

        auto durationToSeconds( TimerClock::duration duration ) -> f64
        {
            return std::chrono::duration<f64>( duration ).count();
        }

        auto secondsToDuration( f64 seconds ) -> TimerClock::duration
        {
            return std::chrono::duration_cast<TimerClock::duration>(
                std::chrono::duration<f64>( seconds ) );
        }

        auto clampDeltaTime( f64 delta, f64 minDelta, f64 maxDelta ) -> f64
        {
            if( minDelta > 0.0 )
            {
                delta = std::max( delta, minDelta );
            }
            if( maxDelta > 0.0 )
            {
                delta = std::min( delta, maxDelta );
            }
            return delta;
        }

        auto isValidTaskIndex( s32 task, size_t size ) -> bool
        {
            return task >= 0 && static_cast<size_t>( task ) < size;
        }
    }  // namespace

    TimerMT::TimerMT() = default;

    TimerMT::~TimerMT() = default;

    void TimerMT::load( SmartPtr<ISharedObject> data )
    {
        if( getLoadingState() == LoadingState::Loaded )
        {
            return;
        }

        setLoadingState( LoadingState::Loading );

        const auto size = static_cast<s32>( TaskId::Count );
        WP_ASSERT( size > 0 );
        if( size <= 0 )
        {
            setLoadingState( LoadingState::Unloaded );
            return;
        }

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

        // Explicitly zero-initialise all per-task values so no indeterminate
        // reads can occur before the first update() call.
        for( s32 i = 0; i < size; ++i )
        {
            m_accumulated[i] = 0.0;
            m_startOffset[i] = 0.0;
            m_fixedOffset[i] = 0.0;

            m_frameSmoothingTime[i] = 0.0;

            m_fixedTimePoints[i] = {};
            m_fixedTime[i] = 0.0;
            m_fixedTimeInterval[i] = 0.0;

            m_prevTime[i] = 0.0;
            m_time[i] = 0.0;
            m_deltaTime[i] = 0.0;

            m_smoothTime[i] = 0.0;
            m_prevSmoothTime[i] = 0.0;
            m_smoothDeltaTime[i] = 0.0;

            m_ticks[i] = 0;

            m_minDeltaTime[i] = 0.0;
            m_maxDeltaTime[i] = 0.0;

            m_eventTimes[i].clear();
        }

        m_startPoint = m_currentPoint = TimerClock::now();

        // Fixed-time queries are valid immediately after load().  Anchoring
        // them here also avoids subtracting a real clock time point from the
        // default-constructed epoch time point on the first query.
        for( auto &point : m_fixedTimePoints )
        {
            point = m_startPoint;
        }

        m_started = false;

        setLoadingState( LoadingState::Loaded );
    }

    void TimerMT::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );
        m_accumulated.clear();
        m_startOffset.clear();
        m_fixedOffset.clear();
        m_frameSmoothingTime.clear();
        m_fixedTimePoints.clear();
        m_fixedTime.clear();
        m_fixedTimeInterval.clear();
        m_prevTime.clear();
        m_time.clear();
        m_deltaTime.clear();
        m_smoothTime.clear();
        m_prevSmoothTime.clear();
        m_smoothDeltaTime.clear();
        m_ticks.clear();
        m_minDeltaTime.clear();
        m_maxDeltaTime.clear();
        m_eventTimes.clear();
        setLoadingState( LoadingState::Unloaded );
    }

    void TimerMT::update()
    {
        WP_ASSERT( isLoaded() );

        // Guard: arrays must have been populated by load().
        WP_ASSERT( !m_time.empty() );
        if( m_time.empty() )
        {
            return;
        }

        auto eTask = Thread::getCurrentTask();
        auto task = static_cast<s32>( eTask );

        // Guard: task index must be within the allocated range.
        WP_ASSERT( isValidTaskIndex( task, m_time.size() ) );
        if( !isValidTaskIndex( task, m_time.size() ) )
        {
            return;
        }

        const auto end = TimerClock::now();
        const auto nowTime = durationToSeconds( end - m_startPoint );

        // Guard: clock must produce a finite, non-negative value.
        WP_ASSERT( std::isfinite( nowTime ) );
        WP_ASSERT( nowTime >= 0.0 );
        if( !std::isfinite( nowTime ) || nowTime < 0.0 )
        {
            return;
        }

        WP_ASSERT( task < m_time.size() );

        auto delta = nowTime - static_cast<f64>( m_time[task] );
        WP_ASSERT( std::isfinite( delta ) );
        WP_ASSERT( delta >= 0.0 );

        // Guard: delta must be non-negative — a negative value indicates a
        // clock discontinuity (e.g. reset between calls).  Clamp to zero so
        // accumulated time and smoothing are never corrupted.
        if( delta < 0.0 )
        {
            delta = 0.0;
        }

        // Apply per-task min/max clamping when limits are active (> 0).
        const f64 minDT = m_minDeltaTime[task];
        const f64 maxDT = m_maxDeltaTime[task];
        WP_ASSERT( std::isfinite( minDT ) );
        WP_ASSERT( std::isfinite( maxDT ) );
        WP_ASSERT( minDT <= 0.0 || maxDT <= 0.0 || minDT <= maxDT );
        delta = clampDeltaTime( delta, minDT, maxDT );

        m_deltaTime[task] = delta;

        auto time = static_cast<f64>( m_time[task] );
        m_prevTime[task] = time;
        m_time[task] = nowTime;

        if( m_enableSmoothing && m_frameSmoothingTime[task] != 0.0 )
        {
            auto smoothDeltaTime = calculateEventTime( nowTime, m_eventTimes[task] );

            // Guard: smoothing calculation must yield a finite, non-negative value.
            WP_ASSERT( std::isfinite( smoothDeltaTime ) );
            WP_ASSERT( smoothDeltaTime >= 0.0 );
            if( !std::isfinite( smoothDeltaTime ) || smoothDeltaTime < 0.0 )
            {
                smoothDeltaTime = delta;
            }

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

    void TimerMT::update( f64 dt )
    {
        // Guard: arrays must have been populated by load().
        WP_ASSERT( !m_time.empty() );
        if( m_time.empty() )
        {
            return;
        }

        auto eTask = Thread::getCurrentTask();
        auto task = static_cast<s32>( eTask );

        // Guard: task index must be within the allocated range.
        WP_ASSERT( isValidTaskIndex( task, m_time.size() ) );
        if( !isValidTaskIndex( task, m_time.size() ) )
        {
            return;
        }

        // Guard: caller-supplied dt must be a finite, non-negative value.
        // A negative or non-finite dt would corrupt time, smoothing, and
        // accumulated values for the task.
        WP_ASSERT( std::isfinite( dt ) );
        WP_ASSERT( dt >= 0.0 );
        if( !std::isfinite( dt ) || dt < 0.0 )
        {
            return;
        }

        // Apply per-task min/max clamping when limits are active (> 0).
        const f64 minDT = m_minDeltaTime[task];
        const f64 maxDT = m_maxDeltaTime[task];
        WP_ASSERT( std::isfinite( minDT ) );
        WP_ASSERT( std::isfinite( maxDT ) );
        WP_ASSERT( minDT <= 0.0 || maxDT <= 0.0 || minDT <= maxDT );
        dt = clampDeltaTime( dt, minDT, maxDT );

        m_deltaTime[task] = dt;

        const f64 time = m_time[task];
        m_prevTime[task] = time;
        m_time[task] = time + dt;

        m_smoothDeltaTime[task] = dt;
        m_prevSmoothTime[task] = m_smoothTime[task];
        m_smoothTime[task] = m_time[task];

        updateFixed();
        addAccumulated( eTask, dt );

        ++m_ticks[task];
    }

    void TimerMT::updateFixed()
    {
        // Guard: arrays must have been populated by load().
        if( m_fixedTime.empty() || m_fixedTimeInterval.empty() || m_fixedTimePoints.empty() )
        {
            return;
        }

        auto task = static_cast<s32>( Thread::getCurrentTask() );

        // Guard: task index must be within the allocated range.
        if( task < 0 || task >= static_cast<s32>( m_fixedTime.size() ) )
        {
            return;
        }

        const f64 interval = m_fixedTimeInterval[task];

        // Guard: interval must be a finite, positive value.
        // Zero or negative intervals would stall or reverse fixed time;
        // non-finite values would permanently corrupt m_fixedTime.
        if( !std::isfinite( interval ) || interval <= 0.0 )
        {
            return;
        }

        const f64 newFixedTime = static_cast<f64>( m_fixedTime[task] ) + interval;

        // Guard: the accumulated fixed time must remain finite after the addition.
        if( !std::isfinite( newFixedTime ) )
        {
            return;
        }

        m_fixedTime[task] = newFixedTime;
        m_fixedTimePoints[task] = TimerClock::now();
    }

    auto TimerMT::calculateEventTime( f64 fNow, Deque<f64> &times ) -> f64
    {
        // Guard: fNow must be finite and non-negative — a NaN or inf pushed into
        // the queue would corrupt every subsequent average derived from it.
        if( !std::isfinite( fNow ) || fNow < 0.0 )
        {
            return 0.0;
        }

        auto task = static_cast<s32>( Thread::getCurrentTask() );

        // Guard: task index must be within the allocated range.
        if( task < 0 || task >= static_cast<s32>( m_frameSmoothingTime.size() ) )
        {
            return 0.0;
        }

        const f64 smoothingWindow = m_frameSmoothingTime[task];

        // Guard: smoothing window must be finite and positive.
        // A zero, negative, or non-finite window would cause all historical
        // entries to be pruned immediately, leaving a single-entry queue that
        // cannot produce a meaningful average.
        if( !std::isfinite( smoothingWindow ) || smoothingWindow <= 0.0 )
        {
            return 0.0;
        }

        // Purge any non-finite timestamps that may have been left by a prior
        // corrupted call, then append the current time.
        times.erase( std::remove_if( times.begin(), times.end(),
                                     []( f64 t ) { return !std::isfinite( t ) || t < 0.0; } ),
                     times.end() );

        times.push_back( fNow );

        // Need at least two samples to compute an interval.
        if( times.size() < 2 )
        {
            return 0.0;
        }

        // Advance past entries that fall outside the smoothing window, while
        // always retaining at least two entries so the division is safe.
        auto it = times.begin();
        while( std::next( it ) != times.end() )
        {
            if( fNow - *it > smoothingWindow )
            {
                ++it;
            }
            else
            {
                break;
            }
        }

        // Remove entries that are older than the smoothing window.
        times.erase( times.begin(), it );

        // Need at least two samples after pruning to compute a meaningful average.
        if( times.size() < 2 )
        {
            return 0.0;
        }

        const f64 elapsed = times.back() - times.front();
        const f64 count = static_cast<f64>( times.size() - 1 );

        // Guard: elapsed must be non-negative (entries could theoretically be
        // out of order if a clock was reset between calls).
        if( elapsed < 0.0 )
        {
            times.clear();
            return 0.0;
        }

        const f64 result = elapsed / count;

        // Final sanity check: the averaged delta must be finite.
        if( !std::isfinite( result ) )
        {
            return 0.0;
        }

        return result;
    }

    void TimerMT::reset()
    {
        // Guard: arrays must have been populated by load().
        if( m_time.empty() )
        {
            return;
        }

        const auto now = TimerClock::now();
        m_currentPoint = m_startPoint = now;
        setStartOffset( 0.0 );

        const auto size = m_time.size();

        for( size_t i = 0; i < size; ++i )
        {
            // Reset elapsed and smoothed time to the new zero-valued origin.
            m_prevTime[i] = 0.0;
            m_time[i] = 0.0;
            m_prevSmoothTime[i] = 0.0;
            m_smoothTime[i] = 0.0;

            // Zero delta times so no stale interval is visible to callers
            // between this reset and the next update().
            m_deltaTime[i] = 0.0;
            m_smoothDeltaTime[i] = 0.0;

            // Resync fixed time points to the new start so getFixedTimeNow()
            // does not compute a bogus offset against the old time point.
            m_fixedTimePoints[i] = now;

            // Discard smoothing history — stale timestamps from before the
            // reset would immediately bias calculateEventTime().
            m_eventTimes[i].clear();

            // Reset tick counters.
            m_ticks[i] = 0;
        }
    }

    void TimerMT::reset( f64 t )
    {
        // Guard: arrays must have been populated by load().
        if( m_time.empty() )
        {
            return;
        }

        // Guard: t must be a finite, non-negative time value.
        // NaN, inf, or a negative offset would corrupt every per-task time array
        // and the start offset, making all subsequent time queries meaningless.
        if( !std::isfinite( t ) || t < 0.0 )
        {
            return;
        }

        // Wind the physical start point back so the clock-driven update() produces
        // a nowTime consistent with t on the very next tick.  Without this,
        // getTime() returns t immediately after reset but jumps to the true elapsed
        // wall-clock time on the first update(), causing a large spurious delta.
        const auto now = TimerClock::now();
        const auto tDuration = secondsToDuration( t );
        m_startPoint = now - tDuration;
        m_currentPoint = now;

        setStartOffset( t );

        const auto size = m_time.size();

        for( size_t i = 0; i < size; ++i )
        {
            // Seed all time tracking to the requested time value.
            m_prevTime[i] = t;
            m_time[i] = t;

            // Resync smooth tracking so there is no discontinuity after the reset.
            m_prevSmoothTime[i] = t;
            m_smoothTime[i] = t;

            // Zero delta times so no stale interval is visible to callers
            // between this reset and the next update().
            m_deltaTime[i] = 0.0;
            m_smoothDeltaTime[i] = 0.0;

            // Resync fixed time points to now so getFixedTimeNow() does not
            // compute a bogus offset against the old time point.
            m_fixedTimePoints[i] = now;

            // Discard smoothing history — stale timestamps from before the
            // reset would immediately bias calculateEventTime().
            m_eventTimes[i].clear();

            // Reset tick counters.
            m_ticks[i] = 0;
        }
    }

    auto TimerMT::getTickCount() -> u32
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_ticks[task];
    }

    auto TimerMT::getTickCount( TaskId task ) -> u32
    {
        auto iTask = static_cast<s32>( task );
        return m_ticks[iTask];
    }

    auto TimerMT::isSteady() const -> bool
    {
        return TimerClock::is_steady;
    }

    auto TimerMT::getDerivedFixedTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        s32 ticks = m_ticks[task];
        return static_cast<f64>( ticks ) * getFixedTimeInterval();
    }

    auto TimerMT::getFixedTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_fixedTime[task];
    }

    auto TimerMT::getFixedTime( u32 task ) const -> f64
    {
        return m_fixedTime[task];
    }

    auto TimerMT::getFixedTimeNow() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return getFixedTime() + durationToSeconds( TimerClock::now() - m_fixedTimePoints[task] );
    }

    auto TimerMT::getFixedTimeNow( u32 task ) const -> f64
    {
        return getFixedTime( task ) + durationToSeconds( TimerClock::now() - m_fixedTimePoints[task] );
    }

    void TimerMT::setFixedTime( f64 value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_fixedTime[task] = value;
    }

    auto TimerMT::getFixedTimeInterval( TaskId task ) const -> f64
    {
        auto iTask = static_cast<s32>( task );
        return m_fixedTimeInterval[iTask];
    }

    auto TimerMT::getFixedTimeInterval() const -> time_interval
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_fixedTimeInterval[task];
    }

    void TimerMT::setFixedTimeInterval( time_interval fixedTimeInterval )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_fixedTimeInterval[task] = fixedTimeInterval;
    }

    void TimerMT::setFixedTimeInterval( TaskId task, f64 value )
    {
        m_fixedTimeInterval[static_cast<u32>( task )] = value;
    }

    void TimerMT::setFrameSmoothingTime( time_interval frameSmoothingTime )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_frameSmoothingTime[task] = frameSmoothingTime;

        for( auto &t : m_frameSmoothingTime )
        {
            t = frameSmoothingTime;
        }
    }

    auto TimerMT::getFrameSmoothingTime() const -> time_interval
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return (f32)m_frameSmoothingTime[task];
    }

    void TimerMT::setStartTime( f64 time )
    {
        WP_ASSERT( std::isfinite( time ) );
        if( !std::isfinite( time ) )
        {
            return;
        }

        // Convert directly to the clock's native duration.  The former
        // microsecond conversion discarded up to 999ns on every call.
        m_startPoint = TimerClock::now() - secondsToDuration( time );
    }

    auto TimerMT::now128() const -> f64
    {
        return durationToSeconds( TimerClock::now() - m_startPoint );
    }

    auto TimerMT::getSmoothTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_smoothTime[task];
    }

    auto TimerMT::getSmoothDeltaTime() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_smoothDeltaTime[task];
    }

    void TimerMT::setSmoothDeltaTime( f64 smoothDT )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_smoothDeltaTime[task] = smoothDT;
        m_prevSmoothTime[task] = m_smoothTime[task];
        m_smoothTime[task] = m_smoothTime[task] + smoothDT;
    }

    auto TimerMT::getMaxDeltaTime( TaskId task ) const -> f64
    {
        return m_maxDeltaTime[static_cast<s32>( task )];
    }

    void TimerMT::setMaxDeltaTime( TaskId task, const f64 t )
    {
        m_maxDeltaTime[static_cast<s32>( task )] = t;
    }

    auto TimerMT::getMinDeltaTime( TaskId task ) const -> f64
    {
        return m_minDeltaTime[static_cast<s32>( task )];
    }

    void TimerMT::setMinDeltaTime( TaskId task, const f64 t )
    {
        m_minDeltaTime[static_cast<s32>( task )] = t;
    }

    auto TimerMT::getStartOffset() const -> f64
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_startOffset[task];
    }

    void TimerMT::setStartOffset( f64 value )
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        m_startOffset[task] = value;
    }

    auto TimerMT::getStartOffset( TaskId task ) const -> f64
    {
        return m_startOffset[static_cast<s32>( task )];
    }

    void TimerMT::setStartOffset( TaskId task, f64 value )
    {
        m_startOffset[static_cast<s32>( task )] = value;
    }

    auto TimerMT::getFixedOffset( TaskId task ) const -> f64
    {
        return m_fixedOffset[static_cast<s32>( task )];
    }

    void TimerMT::setFixedOffset( TaskId task, f64 offset )
    {
        m_fixedOffset[static_cast<s32>( task )] = offset;
    }

    auto TimerMT::getAccumulated( TaskId task ) const -> f64
    {
        return m_accumulated[static_cast<s32>( task )];
    }

    void TimerMT::setAccumulated( TaskId task, f64 value )
    {
        m_accumulated[static_cast<s32>( task )] = value;
    }

    void TimerMT::addAccumulated( TaskId task, f64 value )
    {
        m_accumulated[static_cast<s32>( task )] += value;
    }

    auto TimerMT::now() const -> f64
    {
        return durationToSeconds( TimerClock::now() - m_startPoint );
    }
}  // namespace workphone
