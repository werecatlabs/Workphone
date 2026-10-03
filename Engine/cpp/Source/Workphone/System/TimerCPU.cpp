#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TimerCPU.hpp>

#ifdef WP_USE_BOOST

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TimerCPU, Timer );

    TimerCPU::TimerCPU()
    {
        m_prevTime = m_timer.elapsed();
        m_curTime = m_prevTime;
    }

    TimerCPU::~TimerCPU() = default;

    void TimerCPU::update()
    {
        m_prevTime = m_curTime;
        m_curTime = m_timer.elapsed();

#    if 1
        m_deltaTime =
            static_cast<f64>( m_curTime.wall - m_prevTime.wall ) / static_cast<f64>( 1000000000LL );
        m_time = m_time + m_deltaTime;
#    else
        m_deltaTime = calculateEventTime( (f64)( m_curTime.wall ) / (f64)( 1000000000LL ) );
#    endif
    }

    auto TimerCPU::getTimeMilliseconds() const -> u32
    {
        return static_cast<u32>( m_time * 1000.0 );
    }

    auto TimerCPU::getRealTime() const -> u32
    {
        return static_cast<u32>( m_time * 1000.0 );
    }

    auto TimerCPU::getTimeIntervalMilliseconds() const -> u32
    {
        return static_cast<u32>( m_deltaTime * 1000.0 );
    }

    auto TimerCPU::getDeltaTime() const -> time_interval
    {
        return m_deltaTime;
    }

    void TimerCPU::setFrameSmoothingPeriod( [[maybe_unused]] u32 milliSeconds )
    {
    }

    auto TimerCPU::getFrameSmoothingPeriod() const -> u32
    {
        return static_cast<u32>( m_frameSmoothingTime );
    }

    void TimerCPU::resetSmoothing()
    {
    }

    auto TimerCPU::getTime() const -> time_interval
    {
        return m_time;
    }

    auto TimerCPU::now() const -> time_interval
    {
        return static_cast<f64>( m_timer.elapsed().wall ) / static_cast<f64>( 1000000000LL );
    }
}  // namespace workphone

#endif
