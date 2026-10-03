#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/System/Timer.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, Timer, ITimer );

    Timer::Timer() = default;

    Timer::~Timer() = default;

    auto Timer::getTimeSinceSceneLoad() -> f64
    {
        return getTime() - m_timeSinceLevelLoad;
    }

    auto Timer::getSceneLoadTime() const -> f64
    {
        return m_timeSinceLevelLoad;
    }

    void Timer::setSceneLoadTime( time_interval t )
    {
        m_timeSinceLevelLoad = t;
    }

    void Timer::setMaxDeltaTime( f64 value )
    {
        m_maxDeltaTime = value;
    }

    auto Timer::getDerivedFixedTime() const -> f64
    {
        return 0.0;
    }

    auto Timer::getFixedTime() const -> f64
    {
        return 0.0;
    }

    auto Timer::getFixedTime( u32 task ) const -> f64
    {
        return 0.0;
    }

    auto Timer::getFixedTimeNow() const -> f64
    {
        return 0.0;
    }

    auto Timer::getFixedTimeNow( u32 task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setFixedTime( f64 value )
    {
    }

    auto Timer::getFixedTimeInterval() const -> f64
    {
        return 0.0;
    }

    auto Timer::getFixedTimeInterval( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setFixedTimeInterval( f64 value )
    {
    }

    void Timer::setFixedTimeInterval( TaskId task, f64 value )
    {
    }

    void Timer::setStartTime( f64 time )
    {
    }

    auto Timer::getEnableSmoothing() const -> bool
    {
        return m_enableSmoothing;
    }

    void Timer::setEnableSmoothing( bool value )
    {
        m_enableSmoothing = value;
    }

    auto Timer::getMaxDeltaTime() const -> f64
    {
        return m_maxDeltaTime;
    }

    void Timer::setMinDeltaTime( f64 value )
    {
        m_minDeltaTime = value;
    }

    auto Timer::getStartOffset() const -> f64
    {
        return 0.0;
    }

    auto Timer::getMinDeltaTime() const -> f64
    {
        return m_minDeltaTime;
    }

    void Timer::reset()
    {
    }

    void Timer::reset( f64 t )
    {
    }

    auto Timer::isSteady() const -> bool
    {
        return false;
    }

    auto Timer::getTickCount() -> u32
    {
        return 0;
    }

    auto Timer::getTickCount( TaskId task ) -> u32
    {
        return 0;
    }

    void Timer::setFrameSmoothingTime( time_interval value )
    {
    }

    auto Timer::getFrameSmoothingTime() const -> time_interval
    {
        return 0.0f;
    }

    auto Timer::now() const -> f64
    {
        return 0.0;
    }

    auto Timer::getDeltaTime() const -> f64
    {
        return 0.0;
    }

    auto Timer::getTime() const -> f64
    {
        return 0.0;
    }

    void Timer::update()
    {
    }

    void Timer::updateFixed()
    {
    }

    auto Timer::getSmoothTime() const -> f64
    {
        return 0.0;
    }

    auto Timer::getSmoothDeltaTime() const -> f64
    {
        return 0.0;
    }

    auto Timer::getSmoothDeltaTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setSmoothDeltaTime( f64 smoothDT )
    {
    }

    auto Timer::getTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    auto Timer::getPreviousTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    auto Timer::getDeltaTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    auto Timer::getMaxDeltaTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setMaxDeltaTime( TaskId task, const f64 t )
    {
    }

    auto Timer::getMinDeltaTime( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setMinDeltaTime( TaskId task, const f64 t )
    {
    }

    void Timer::setStartOffset( f64 value )
    {
    }

    auto Timer::getStartOffset( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setStartOffset( TaskId task, f64 value )
    {
    }

    auto Timer::getFixedOffset( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setFixedOffset( TaskId task, f64 offset )
    {
    }

    auto Timer::getAccumulated( TaskId task ) const -> f64
    {
        return 0.0;
    }

    void Timer::setAccumulated( TaskId task, f64 value )
    {
    }

    void Timer::addAccumulated( TaskId task, f64 value )
    {
    }

    auto Timer::getTimeMilliseconds() const -> u32
    {
        return static_cast<u32>( getTime() * 1000.0 );
    }

    auto Timer::getRealTime() const -> u32
    {
        return static_cast<u32>( getTime() * 1000.0 );
    }

    auto Timer::getTimeIntervalMilliseconds() const -> u32
    {
        return static_cast<u32>( getDeltaTime() * 1000.0 );
    }

    void Timer::setFrameSmoothingPeriod( u32 milliSeconds )
    {
    }

    auto Timer::getFrameSmoothingPeriod() const -> u32
    {
        return 0;
    }

    void Timer::resetSmoothing()
    {
    }
}  // namespace workphone
