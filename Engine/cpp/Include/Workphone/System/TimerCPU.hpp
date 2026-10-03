#ifndef TimerCPU_h__
#define TimerCPU_h__

#include <Workphone/System/Timer.hpp>
#include <deque>

#ifdef WP_USE_BOOST

#    include <boost/timer/timer.hpp>

namespace workphone
{

    /**
     * @class TimerCPU
     * @brief High-precision CPU timer implementation using Boost.
     *
     * This class provides a timer based on the CPU time measured by Boost's cpu_timer.
     * It is designed for profiling and precise time measurements of code execution on the CPU.
     * The timer supports frame smoothing and provides both real and virtual time intervals.
     *
     * @note This class is only available if WP_USE_BOOST is defined.
     */
    class WPCore_API TimerCPU : public Timer
    {
    public:
        /**
         * @brief Constructs a new TimerCPU object and initializes the CPU timer.
         */
        TimerCPU();

        /**
         * @brief Destroys the TimerCPU object.
         */
        ~TimerCPU() override;

        /**
         * @brief Updates the timer state, measuring elapsed CPU time since the last update.
         *
         * This should be called once per frame or measurement interval to update the internal timing
         * values.
         */
        void update() override;

        /**
         * @brief Gets the current virtual (measured) time in milliseconds.
         * @return The current virtual time in milliseconds.
         */
        u32 getTimeMilliseconds() const override;

        /**
         * @brief Gets the current real (system) time in milliseconds.
         * @return The current real time in milliseconds.
         */
        u32 getRealTime() const override;

        /**
         * @brief Gets the time interval in milliseconds between the last two update ticks.
         * @return The time interval in milliseconds.
         */
        u32 getTimeIntervalMilliseconds() const override;

        /**
         * @brief Gets the current virtual (measured) time as a time_interval (seconds).
         * @return The current virtual time in seconds.
         */
        time_interval getTime() const override;

        /**
         * @brief Gets the time difference (delta) between the last two updates as a time_interval
         * (seconds).
         * @return The delta time in seconds.
         */
        time_interval getDeltaTime() const override;

        /**
         * @brief Sets the period (in milliseconds) over which frame time smoothing is applied.
         * @param milliSeconds The smoothing period in milliseconds.
         */
        void setFrameSmoothingPeriod( u32 milliSeconds ) override;

        /**
         * @brief Gets the current frame smoothing period in milliseconds.
         * @return The smoothing period in milliseconds.
         */
        u32 getFrameSmoothingPeriod() const override;

        /**
         * @brief Resets the frame time smoothing history.
         */
        void resetSmoothing() override;

        /**
         * @brief Gets the current high-precision CPU time as a time_interval (seconds).
         * @return The current CPU time in seconds.
         */
        time_interval now() const override;

        /**
         * @brief Registers the class type information for RTTI and factory systems.
         */
        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Boost CPU timer used for measuring elapsed CPU time.
         */
        boost::timer::cpu_timer m_timer;

        /**
         * @brief Previous CPU time snapshot.
         */
        boost::timer::cpu_times m_prevTime;

        /**
         * @brief Current CPU time snapshot.
         */
        boost::timer::cpu_times m_curTime;

        /**
         * @brief Current measured time in seconds.
         */
        time_interval m_time;

        /**
         * @brief Time difference (delta) between the last two updates in seconds.
         */
        time_interval m_deltaTime;

        /**
         * @brief Frame smoothing time interval in seconds.
         */
        time_interval m_frameSmoothingTime;

        /**
         * @brief Queue of recent event times (in seconds) for frame smoothing.
         */
        using EventTimesQueue = Deque<f64>;
        EventTimesQueue m_eventTimes;
    };
}  // namespace workphone

#endif

#endif  // TimerCPU_h__
