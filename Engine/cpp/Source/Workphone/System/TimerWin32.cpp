#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TimerWin32.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

#ifdef WP_PLATFORM_WIN32
#    include <WTypes.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TimerWin32, Timer );

    TimerWin32::TimerWin32() : HighPerformanceTimerSupport( FALSE ), MultiCore( FALSE )
    {
        initTimer();
    }

    TimerWin32::~TimerWin32() = default;

    void TimerWin32::initTimer()
    {
        VirtualTimerSpeed = 1.0f;
        VirtualTimerStopCounter = 0;
        LastVirtualTime = 0;
        StartRealTime = 0;
        StaticTime = 0;
        TimeDelta = 0;
        SmoothingPeriod = 5000;  // default value

#    if !defined( _WIN32_WCE )
        // disable hires timer on multiple core systems, bios bugs result in bad hires timers.
        SYSTEM_INFO sysinfo;
        GetSystemInfo( &sysinfo );
        MultiCore = ( sysinfo.dwNumberOfProcessors > 1 );
#    endif

        HighPerformanceTimerSupport = QueryPerformanceFrequency( &HighPerformanceFreq );
        initVirtualTimer();
    }

    auto TimerWin32::getRealTime() const -> u32
    {
        if( HighPerformanceTimerSupport )
        {
#    if !defined( _WIN32_WCE )
            // Avoid potential timing inaccuracies across multiple cores by
            // temporarily setting the affinity of this process to one core.
            DWORD_PTR affinityMask = 0;
            if( MultiCore )
            {
                affinityMask = SetThreadAffinityMask( GetCurrentThread(), 1 );
            }
#    endif
            LARGE_INTEGER nTime;
            bool32 queriedOK = QueryPerformanceCounter( &nTime );

#    if !defined( _WIN32_WCE )
            // Restore the true affinity.
            if( MultiCore )
            {
                (void)SetThreadAffinityMask( GetCurrentThread(), affinityMask );
            }
#    endif
            if( queriedOK )
            {
                return static_cast<u32>( ( nTime.QuadPart ) * 1000 / HighPerformanceFreq.QuadPart );
            }
        }

        return GetTickCount();
    }

    auto TimerWin32::calculateEventTime( u32 now ) -> f32
    {
        // Calculate the average time passed between events of the given type
        // during the last mFrameSmoothingTime seconds.

        EventTimesQueue &times = mEventTimes;
        times.push_back( now );

        if( times.size() == 1 )
        {
            return 0;
        }

        // Times up to mFrameSmoothingTime seconds old should be kept
        unsigned long discardThreshold = SmoothingPeriod;

        // Find the oldest time to keep
        auto it = times.begin(),
             end = times.end() - 2;  // We need at least two times
        while( it != end )
        {
            if( now - *it > discardThreshold )
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

        return static_cast<f32>( ( times.back() - times.front() ) / ( times.size() - 1 ) );
    }

    void TimerWin32::update()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        u32 lastTime = StaticTime;
        StaticTime = getRealTime();

        auto nowTime = static_cast<u32>( StaticTime );
        TimeDelta = static_cast<u32>( calculateEventTime( nowTime ) );
    }

    void TimerWin32::setTime( u32 time )
    {
        StaticTime = getRealTime();
        LastVirtualTime = time;
        StartRealTime = static_cast<u32>( StaticTime );
    }

    void TimerWin32::stopTimer()
    {
        if( !isStopped() )
        {
            // stop the virtual timer
            LastVirtualTime = getTimeMilliseconds();
        }

        --VirtualTimerStopCounter;
    }

    void TimerWin32::startTimer()
    {
        ++VirtualTimerStopCounter;

        if( !isStopped() )
        {
            // restart virtual timer
            setTime( LastVirtualTime );
        }
    }

    void TimerWin32::setSpeed( f32 speed )
    {
        setTime( getTimeMilliseconds() );

        VirtualTimerSpeed = speed;
        if( VirtualTimerSpeed < 0.0f )
        {
            VirtualTimerSpeed = 0.0f;
        }
    }

    auto TimerWin32::getSpeed() -> f32
    {
        return VirtualTimerSpeed;
    }

    void TimerWin32::initVirtualTimer()
    {
        StaticTime = getRealTime();
        StartRealTime = static_cast<u32>( StaticTime );
    }

    auto TimerWin32::getTimeIntervalMilliseconds() const -> u32
    {
        return TimeDelta;
    }

    void TimerWin32::setFrameSmoothingPeriod( u32 frames )
    {
        SmoothingPeriod = frames;
    }

    auto TimerWin32::getFrameSmoothingPeriod() const -> u32
    {
        return SmoothingPeriod;
    }

    void TimerWin32::resetSmoothing()
    {
    }

    auto TimerWin32::getTime() const -> time_interval
    {
        return static_cast<f64>( StaticTime ) / 1000.0;
    }

    auto TimerWin32::getDeltaTime() const -> time_interval
    {
        return static_cast<f64>( TimeDelta ) / 1000.0;
    }

    auto TimerWin32::now() const -> time_interval
    {
        return static_cast<time_interval>( getRealTime() ) / 1000.0;
    }
}  // namespace workphone

#endif
