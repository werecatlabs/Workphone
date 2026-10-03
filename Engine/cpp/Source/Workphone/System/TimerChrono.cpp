#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/TimerChrono.hpp>
#include <chrono>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, TimerChrono, Timer );

    TimerChrono::TimerChrono() = default;

    TimerChrono::~TimerChrono() = default;

    void TimerChrono::update()
    {
        time_interval time =
            static_cast<f64>( std::chrono::steady_clock::now().time_since_epoch().count() ) /
            static_cast<f64>( 1000000000LL );
        time_interval deltaTime = m_smoother.getValue( workphone::make_pair( time, time ) );
        time_interval elapsedTime = getTime() + deltaTime;

        setDeltaTime( deltaTime );
        setTime( elapsedTime );
    }

    auto TimerChrono::getTimeMilliseconds() const -> u32
    {
        return static_cast<u32>( getTime() * 1000.0 );
    }

    auto TimerChrono::getRealTime() const -> u32
    {
        time_interval time =
            static_cast<f64>( std::chrono::steady_clock::now().time_since_epoch().count() ) /
            static_cast<f64>( 1000000000LL );
        return static_cast<u32>( time * 1000.0 );
    }

    auto TimerChrono::getTimeIntervalMilliseconds() const -> u32
    {
        return static_cast<u32>( getDeltaTime() * 1000.0 );
    }

    auto TimerChrono::now() const -> time_interval
    {
        return static_cast<f64>( std::chrono::steady_clock::now().time_since_epoch().count() ) /
               static_cast<f64>( 1000000000LL );
    }

    auto TimerChrono::getTime() const -> time_interval
    {
        u32 time = m_time;
        return *reinterpret_cast<time_interval *>( &time );
    }

    auto TimerChrono::getDeltaTime() const -> time_interval
    {
        u32 deltaTime = m_deltaTime;
        return *reinterpret_cast<time_interval *>( &deltaTime );
    }

    void TimerChrono::setFrameSmoothingPeriod( u32 milliSeconds )
    {
        m_smoother.setSmoothInterval( static_cast<float>( milliSeconds ) / 1000.0f );
    }

    auto TimerChrono::getFrameSmoothingPeriod() const -> u32
    {
        return static_cast<u32>( m_smoother.getSmoothInterval() * 1000.0f );
    }

    void TimerChrono::resetSmoothing()
    {
    }

    void TimerChrono::setDeltaTime( time_interval deltaTime )
    {
        m_deltaTime = *reinterpret_cast<u32 *>( &deltaTime );
    }

    void TimerChrono::setTime( time_interval time )
    {
        m_time = *reinterpret_cast<u32 *>( &time );
    }
}  // namespace workphone
