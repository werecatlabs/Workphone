#ifndef TimerWin_h__
#define TimerWin_h__

#include <Workphone/System/Timer.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>
#include <deque>

#ifdef WP_PLATFORM_WIN32
#    include <windows.h>
#endif

namespace workphone
{
#ifdef WP_PLATFORM_WIN32

    /**
     * @class TimerWin32
     * @brief High-resolution timer implementation for Win32 using QueryPerformance* functions.
     *
     * This class provides a virtual (game) timer with support for speed adjustment, pausing,
     * frame smoothing, and high-precision time measurement. It is designed for use on Windows
     * platforms and leverages the system's high-performance counters for accurate timing.
     */
    class WPCore_API TimerWin32 : public Timer
    {
    public:
        /**
         * @brief Constructs a new TimerWin32 object and initializes the timer.
         */
        TimerWin32();

        /**
         * @brief Destroys the TimerWin32 object.
         */
        ~TimerWin32() override;

        /**
         * @brief Gets the current virtual (game) time in milliseconds.
         * @return The current virtual time in milliseconds.
         */
        u32 getTimeMilliseconds() const override;

        /**
         * @brief Initializes the real timer and sets up high-performance counters.
         */
        void initTimer();

        /**
         * @brief Sets the current virtual (game) time.
         * @param time The new virtual time in milliseconds.
         */
        void setTime( u32 time );

        /**
         * @brief Stops (pauses) the virtual (game) timer.
         */
        void stopTimer();

        /**
         * @brief Starts (unpauses) the virtual (game) timer.
         */
        void startTimer();

        /**
         * @brief Sets the speed multiplier for the virtual timer.
         * @param speed The new speed (1.0 = real time, 2.0 = double speed, etc.).
         */
        void setSpeed( f32 speed );

        /**
         * @brief Gets the current speed multiplier of the virtual timer.
         * @return The speed multiplier.
         */
        f32 getSpeed();

        /**
         * @brief Checks if the timer is currently stopped (paused).
         * @return True if the timer is stopped, false otherwise.
         */
        bool isStopped() const;

        /**
         * @brief Updates the virtual timer based on the current real time.
         *
         * This should be called once per frame to advance the virtual time.
         */
        void update() override;

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
         * @brief Gets the current virtual (game) time as a time_interval object.
         * @return The current virtual time.
         */
        time_interval getTime() const override;

        /**
         * @brief Gets the time difference (delta) between the last two frames as a time_interval object.
         * @return The delta time.
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

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Initializes the virtual timer state.
         */
        void initVirtualTimer();

        /**
         * @brief Calculates the event time based on the current time and smoothing.
         * @param now The current time in milliseconds.
         * @return The smoothed event time.
         */
        f32 calculateEventTime( u32 now );

        /**
         * @brief Gets the current high-precision time as a time_interval object.
         * @return The current time.
         */
        time_interval now() const override;

        LARGE_INTEGER HighPerformanceFreq;   ///< Frequency of the high-performance timer.
        bool32 HighPerformanceTimerSupport;  ///< True if high-performance timer is supported.
        bool32 MultiCore;                    ///< True if running on a multi-core system.

        f32 VirtualTimerSpeed;               ///< Speed multiplier for the virtual timer.
        atomic_s32 VirtualTimerStopCounter;  ///< Counter for nested stop/start calls.
        atomic_u32 StartRealTime;            ///< Real time when the timer was started.
        atomic_u32 LastVirtualTime;          ///< Last recorded virtual time.
        atomic_u32 StaticTime;               ///< Last recorded real time.
        atomic_u32 TimeDelta;                ///< Time difference between the last two updates.
        atomic_u32 SmoothingPeriod;          ///< Frame smoothing period in milliseconds.

        using EventTimesQueue =
            Deque<unsigned long>;     ///< Queue for storing recent event times for smoothing.
        EventTimesQueue mEventTimes;  ///< Recent event times for frame smoothing.

        RecursiveMutex m_mutex;  ///< Mutex for thread safety.
    };

    /**
     * @brief Gets the current virtual (game) time in milliseconds.
     * @return The current virtual time in milliseconds.
     */
    inline u32 TimerWin32::getTimeMilliseconds() const
    {
        if( isStopped() )
        {
            return LastVirtualTime;
        }

        return LastVirtualTime + static_cast<u32>( ( StaticTime - StartRealTime ) * VirtualTimerSpeed );
    }

    /**
     * @brief Checks if the timer is currently stopped (paused).
     * @return True if the timer is stopped, false otherwise.
     */
    inline bool TimerWin32::isStopped() const
    {
        return VirtualTimerStopCounter != 0;
    }

#endif
}  // namespace workphone

#endif  // TimerWin_h__
