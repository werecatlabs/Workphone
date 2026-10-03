#ifndef TimerChrono_h__
#define TimerChrono_h__

#include <Workphone/System/Timer.hpp>
#include <Workphone/Math/Smooth.hpp>

namespace workphone
{
    /**
     * @brief High-precision timer implementation using std::chrono
     *
     * TimerChrono is a concrete implementation of the Timer interface that provides
     * high-precision timing functionality using C++11's std::chrono library.
     * It uses std::chrono::steady_clock for monotonic time measurements that are
     * not affected by system clock adjustments.
     *
     * This timer implementation includes frame time smoothing capabilities to reduce
     * timing jitter and provide more stable delta time values for game loops and
     * animations. The smoothing is performed using a configurable time window.
     *
     * Key features:
     * - High-precision timing using std::chrono::steady_clock
     * - Thread-safe atomic storage for time values
     * - Configurable frame time smoothing
     * - Nanosecond precision converted to floating-point seconds
     *
     * @note All time values are internally stored and computed in seconds as
     *       floating-point values for consistency with the Timer interface.
     *
     * @see Timer Base timer class providing the interface
     * @see Smooth Template class used for frame time smoothing
     *
     * @since Engine v1.0
     */
    class WPCore_API TimerChrono : public Timer
    {
    public:
        /**
         * @brief Default constructor
         *
         * Initializes the timer with default settings. The timer starts with
         * zero elapsed time and default smoothing parameters.
         */
        TimerChrono();

        /**
         * @brief Copy constructor (deleted)
         *
         * TimerChrono instances cannot be copied to prevent timing state corruption.
         *
         * @param other The timer instance to copy from
         */
        TimerChrono( const TimerChrono &other ) = delete;

        /**
         * @brief Virtual destructor
         *
         * Cleans up any resources used by the timer. Declared virtual to ensure
         * proper cleanup when destroyed through base class pointers.
         */
        ~TimerChrono() override;

        /**
         * @brief Updates the timer with the current system time
         *
         * This method should be called once per frame to update the timer's
         * internal time tracking. It performs the following operations:
         * 1. Queries the current system time using std::chrono::steady_clock
         * 2. Calculates the delta time since the last update
         * 3. Applies smoothing to reduce timing jitter
         * 4. Updates the accumulated elapsed time
         *
         * The smoothing algorithm helps provide more stable frame times by
         * averaging delta times over a configurable time window.
         *
         * @note This method must be called regularly (typically once per frame)
         *       for the timer to function correctly.
         *
         * @see setFrameSmoothingPeriod() To configure smoothing behavior
         */
        void update() override;

        /**
         * @brief Gets the current elapsed time in milliseconds
         *
         * Returns the total elapsed time since the timer was started,
         * converted to milliseconds for compatibility with legacy code.
         *
         * @return Current elapsed time in milliseconds as an unsigned 32-bit integer
         *
         * @note The returned value is truncated to fit in a 32-bit unsigned integer.
         *       For high-precision applications, use getTime() instead.
         *
         * @see getTime() For high-precision elapsed time in seconds
         */
        u32 getTimeMilliseconds() const override;

        /**
         * @brief Gets the current real-time system time in milliseconds
         *
         * Returns the current system time directly from std::chrono::steady_clock,
         * independent of the timer's internal state. This represents the actual
         * time since an arbitrary epoch (typically system boot).
         *
         * @return Current system time in milliseconds
         *
         * @note This value is not affected by timer resets or pausing.
         *       It always reflects the current system steady clock time.
         *
         * @see now() For high-precision real-time in seconds
         */
        u32 getRealTime() const override;

        /**
         * @brief Gets the last frame's delta time in milliseconds
         *
         * Returns the time interval between the last two update() calls,
         * converted to milliseconds. This value includes any smoothing
         * that has been applied.
         *
         * @return Last frame's delta time in milliseconds
         *
         * @see getDeltaTime() For high-precision delta time in seconds
         * @see setFrameSmoothingPeriod() To configure delta time smoothing
         */
        u32 getTimeIntervalMilliseconds() const override;

        /**
         * @brief Gets the current real-time system time in seconds
         *
         * Returns the current system time directly from std::chrono::steady_clock
         * with high precision. This represents time since an arbitrary epoch
         * (typically system boot) and is independent of the timer's state.
         *
         * @return Current system time in seconds with sub-second precision
         *
         * @note This is a direct system time query and is not affected by
         *       timer operations like reset() or smoothing settings.
         */
        time_interval now() const override;

        /**
         * @brief Gets the current elapsed time in seconds
         *
         * Returns the total time that has elapsed since the timer was started
         * or last reset, with high precision. This is the primary time value
         * used for game logic and animations.
         *
         * @return Current elapsed time in seconds with sub-second precision
         *
         * @note This value is affected by smoothing and represents the
         *       accumulated time from all update() calls.
         *
         * @see getTimeMilliseconds() For integer millisecond precision
         */
        time_interval getTime() const override;

        /**
         * @brief Gets the last frame's delta time in seconds
         *
         * Returns the time interval between the last two update() calls
         * with high precision. This value includes smoothing if enabled
         * and is typically used for frame-rate independent animations.
         *
         * @return Last frame's delta time in seconds with sub-second precision
         *
         * @note The returned value may be smoothed to reduce jitter.
         *       The degree of smoothing depends on the configured smoothing period.
         *
         * @see getTimeIntervalMilliseconds() For integer millisecond precision
         * @see setFrameSmoothingPeriod() To configure smoothing behavior
         */
        time_interval getDeltaTime() const override;

        /**
         * @brief Sets the frame time smoothing period
         *
         * Configures the time window over which delta times are smoothed.
         * Smoothing helps reduce timing jitter by averaging delta times
         * over the specified period, providing more stable frame times.
         *
         * @param milliSeconds Smoothing period in milliseconds. Higher values
         *                     provide more smoothing but increase latency.
         *                     A value of 0 effectively disables smoothing.
         *
         * @note Smoothing introduces a small amount of latency but significantly
         *       improves timing stability for animations and physics.
         *
         * @see getFrameSmoothingPeriod() To query current smoothing period
         * @see resetSmoothing() To clear smoothing history
         */
        void setFrameSmoothingPeriod( u32 milliSeconds ) override;

        /**
         * @brief Gets the current frame time smoothing period
         *
         * Returns the time window over which delta times are currently
         * being smoothed, converted to milliseconds.
         *
         * @return Current smoothing period in milliseconds
         *
         * @see setFrameSmoothingPeriod() To configure smoothing behavior
         */
        u32 getFrameSmoothingPeriod() const override;

        /**
         * @brief Resets the smoothing algorithm state
         *
         * Clears any accumulated smoothing history, causing the smoothing
         * algorithm to restart fresh. This can be useful when there have
         * been significant timing discontinuities (e.g., after pausing).
         *
         * @note This method currently has no implementation but is provided
         *       for interface compatibility. Future versions may implement
         *       smoothing state reset functionality.
         */
        void resetSmoothing() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Sets the internal elapsed time value
         *
         * Updates the atomic storage for the total elapsed time.
         * This method uses type punning to store the floating-point
         * time value in atomic unsigned integer storage.
         *
         * @param value New elapsed time value in seconds
         *
         * @note This method is thread-safe due to atomic storage.
         * @warning Uses reinterpret_cast for type conversion - requires
         *          careful handling to maintain data integrity.
         */
        void setTime( time_interval time );

        /**
         * @brief Sets the internal delta time value
         *
         * Updates the atomic storage for the last frame's delta time.
         * This method uses type punning to store the floating-point
         * delta time value in atomic unsigned integer storage.
         *
         * @param value New delta time value in seconds
         *
         * @note This method is thread-safe due to atomic storage.
         * @warning Uses reinterpret_cast for type conversion - requires
         *          careful handling to maintain data integrity.
         */
        void setDeltaTime( time_interval deltaTime );

        /**
         * @brief Atomic storage for the current elapsed time
         *
         * Stores the total elapsed time as an atomic unsigned integer
         * for thread-safe access. The actual floating-point time value
         * is stored using type punning techniques.
         *
         * @note Access to this member should be done through getTime()
         *       and setTime() methods to ensure proper type conversion.
         */
        atomic_u32 m_time;

        /**
         * @brief Atomic storage for the current delta time
         *
         * Stores the last frame's delta time as an atomic unsigned integer
         * for thread-safe access. The actual floating-point time value
         * is stored using type punning techniques.
         *
         * @note Access to this member should be done through getDeltaTime()
         *       and setDeltaTime() methods to ensure proper type conversion.
         */
        atomic_u32 m_deltaTime;

        /**
         * @brief Type alias for the smoothing algorithm
         *
         * Defines a convenient alias for the Smooth template instantiation
         * used for frame time smoothing calculations.
         */
        using TimerSmooth = Smooth<time_interval>;

        /**
         * @brief Frame time smoothing algorithm instance
         *
         * Implements delta time smoothing to reduce timing jitter and provide
         * more stable frame times. The smoother maintains a time window of
         * recent delta time values and computes smoothed derivatives.
         *
         * @see Smooth Template class providing smoothing functionality
         * @see setFrameSmoothingPeriod() To configure smoothing parameters
         */
        TimerSmooth m_smoother;
    };
}  // namespace workphone

#endif  // TimerChrono_h__
