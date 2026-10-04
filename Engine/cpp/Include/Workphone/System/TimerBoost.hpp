#ifndef TimerBoost_h__
#define TimerBoost_h__

#include <Workphone/System/Timer.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/Array.hpp>
#include <boost/chrono/chrono.hpp>

namespace workphone
{

    /**
     * @brief Timer implementation based on boost::chrono::steady_clock.
     *
     * TimerBoost provides a thread-safe, high-precision timer implementation using the boost::chrono
     * library. It supports multiple concurrent tasks/threads with separate timing data for each, frame
     * smoothing, fixed time steps, and various time measurement functionalities.
     *
     * The timer maintains separate timing states for different TaskId types, making it suitable
     * for multi-threaded applications where different subsystems (rendering, physics, etc.) need
     * independent timing information.
     *
     * Key features:
     * - Thread-safe operation with atomic variables
     * - Per-task timing state management
     * - Frame smoothing for stable delta times
     * - Fixed timestep support for deterministic updates
     * - High-precision timing using boost::chrono::steady_clock
     *
     * @see Timer
     * @see TaskId
     */
    class WPCore_API TimerBoost : public Timer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the timer with default values and allocates arrays for all TaskId types.
         * Sets the start point to the current steady clock time.
         */
        TimerBoost();

        /**
         * @brief Destructor.
         *
         * Cleans up resources and calls unload().
         */
        ~TimerBoost() override;

        /**
         * @brief Updates the timer using current system time.
         *
         * Calculates delta time since last update and updates timing values for the current task.
         * Optionally applies frame smoothing if enabled. Also updates fixed time and tick count.
         *
         * @copydetails Timer::update
         */
        void update() override;

        /**
         * @brief Updates the timer with a specified delta time.
         *
         * Manual update mode that bypasses system time calculation and uses the provided delta time.
         * Useful for deterministic timing or when delta time is calculated externally.
         *
         * @param dt Delta time in seconds to advance the timer by
         */
        void update( f64 dt );

        /**
         * @brief Updates the fixed timestep counter.
         *
         * Advances the fixed time by the configured fixed time interval for the current task.
         * Updates the fixed time point reference for accurate fixed time calculations.
         *
         * @copydetails Timer::updateFixed
         */
        void updateFixed() override;

        /**
         * @brief Gets the current time for the current task.
         *
         * Returns the accumulated time since timer start for the calling thread's task.
         * This is the smoothed time if smoothing is enabled, otherwise the raw time.
         *
         * @return Current time in seconds
         * @copydetails Timer::getTime
         */
        f64 getTime() const override;

        /**
         * @brief Gets the delta time for the current task.
         *
         * Returns the time elapsed since the last update() call for the calling thread's task.
         * This may be smoothed delta time if frame smoothing is enabled.
         *
         * @return Delta time in seconds since last update
         * @copydetails Timer::getDeltaTime
         */
        f64 getDeltaTime() const override;

        /**
         * @brief Gets the current absolute time since timer creation.
         *
         * Returns the wall clock time elapsed since the timer was constructed,
         * regardless of resets or task-specific timing states.
         *
         * @return Absolute time in seconds since timer creation
         * @copydetails Timer::now
         */
        f64 now() const override;

        /**
         * @brief Gets the frame smoothing time window.
         *
         * Returns the time window (in seconds) used for frame time smoothing calculations.
         * Frame smoothing averages delta times over this period to reduce timing jitter.
         *
         * @return Frame smoothing time window in seconds
         * @copydetails Timer::getFrameSmoothingTime
         */
        time_interval getFrameSmoothingTime() const override;

        /**
         * @brief Sets the frame smoothing time window.
         *
         * Configures the time window used for frame smoothing. A larger window provides
         * more stable but less responsive delta times. Set to 0 to disable smoothing.
         *
         * @param value Frame smoothing time window in seconds
         * @copydetails Timer::setFrameSmoothingTime
         */
        void setFrameSmoothingTime( time_interval value ) override;

        /**
         * @brief Gets the update tick count for the current task.
         *
         * Returns the number of times update() has been called for the current task.
         *
         * @return Number of update ticks
         */
        u32 getTickCount() override;

        /**
         * @brief Gets the update tick count for a specific task.
         *
         * Returns the number of times update() has been called for the specified task.
         *
         * @param task The task to get tick count for
         * @return Number of update ticks for the specified task
         */
        u32 getTickCount( TaskId task ) override;

        /**
         * @brief Resets the timer to initial state.
         *
         * Resets all timing values to zero and sets the start point to current time.
         * Preserves smoothed time relationships but resets tick counters.
         *
         * @copydetails Timer::reset
         */
        void reset() override;

        /**
         * @brief Resets the timer with a specific time offset.
         *
         * Resets all timing values and sets all tasks to the specified time value.
         * Useful for synchronized starts or resuming from a specific time point.
         *
         * @param t Time value in seconds to set as the new starting time
         */
        void reset( f64 t ) override;

        /**
         * @brief Checks if the underlying clock is steady.
         *
         * Returns whether the boost::chrono::steady_clock provides monotonic time
         * that is not affected by system clock adjustments.
         *
         * @return true if clock is steady (monotonic), false otherwise
         * @copydetails Timer::isSteady
         */
        bool isSteady() const override;

        /**
         * @brief Gets the derived fixed time based on tick count.
         *
         * Calculates fixed time as tick_count * fixed_time_interval.
         * This provides a perfectly regular timestep based on update frequency.
         *
         * @return Derived fixed time in seconds
         */
        f64 getDerivedFixedTime() const override;

        /**
         * @brief Gets the fixed time for the current task.
         *
         * Returns the accumulated fixed timestep time for the current task.
         * This advances by fixed intervals on each updateFixed() call.
         *
         * @return Fixed time in seconds
         * @copydetails Timer::getFixedTime
         */
        f64 getFixedTime() const override;

        /**
         * @brief Gets the fixed time for a specific task.
         *
         * Returns the accumulated fixed timestep time for the specified task.
         *
         * @param task Task index to get fixed time for
         * @return Fixed time in seconds for the specified task
         */
        f64 getFixedTime( u32 task ) const override;

        /**
         * @brief Gets the current interpolated fixed time.
         *
         * Returns fixed time plus interpolation based on time since last fixed update.
         * Provides smooth time progression between fixed timestep updates.
         *
         * @return Interpolated fixed time in seconds
         */
        f64 getFixedTimeNow() const override;

        /**
         * @brief Gets the current interpolated fixed time for a specific task.
         *
         * Returns fixed time plus interpolation for the specified task.
         *
         * @param task Task index to get interpolated fixed time for
         * @return Interpolated fixed time in seconds
         */
        f64 getFixedTimeNow( u32 task ) const override;

        /**
         * @brief Sets the fixed time for the current task.
         *
         * Manually sets the fixed time value, useful for synchronization or resuming.
         *
         * @param value Fixed time value in seconds
         */
        void setFixedTime( f64 value ) override;

        /**
         * @brief Gets the fixed time interval for a specific task.
         *
         * Returns the time step used for fixed timestep updates.
         *
         * @param task Task to get fixed time interval for
         * @return Fixed time interval in seconds
         */
        f64 getFixedTimeInterval( TaskId task ) const override;

        /**
         * @brief Gets the fixed time interval for the current task.
         *
         * Returns the time step used for fixed timestep updates.
         *
         * @return Fixed time interval in seconds
         */
        f64 getFixedTimeInterval() const override;

        /**
         * @brief Sets the fixed time interval for the current task.
         *
         * Configures the time step for fixed timestep updates. Common values
         * are 1/60 (16.67ms) for 60Hz updates or 1/120 (8.33ms) for 120Hz.
         *
         * @param value Fixed time interval in seconds
         */
        void setFixedTimeInterval( f64 value ) override;

        /**
         * @brief Sets the fixed time interval for a specific task.
         *
         * Configures the time step for fixed timestep updates for the specified task.
         *
         * @param task Task to set fixed time interval for
         * @param value Fixed time interval in seconds
         */
        void setFixedTimeInterval( TaskId task, f64 timeStep ) override;

        /**
         * @brief Sets the timer's absolute start time.
         *
         * Adjusts the internal start point to make the timer appear to have
         * started at the specified time in the past.
         *
         * @param time Start time offset in seconds
         */
        void setStartTime( f64 time ) override;

        /**
         * @brief Gets the smoothed time for the current task.
         *
         * Returns time value that has been processed through frame smoothing
         * to reduce jitter and provide more stable timing.
         *
         * @return Smoothed time in seconds
         */
        f64 getSmoothTime() const override;

        /**
         * @brief Gets the smoothed delta time for the current task.
         *
         * Returns delta time that has been averaged over the smoothing window
         * to provide stable frame timing despite system load variations.
         *
         * @return Smoothed delta time in seconds
         */
        f64 getSmoothDeltaTime() const override;

        /**
         * @brief Sets the smoothed delta time manually.
         *
         * Manually overrides the smoothed delta time value and updates smooth time accordingly.
         *
         * @param smoothDT Smoothed delta time value in seconds
         */
        void setSmoothDeltaTime( f64 smoothDT ) override;

        /**
         * @brief Gets the time for a specific task.
         *
         * Returns the current accumulated time for the specified task.
         *
         * @param task Task to get time for
         * @return Current time in seconds for the specified task
         */
        f64 getTime( TaskId task ) const override;

        /**
         * @brief Gets the previous frame time for a specific task.
         *
         * Returns the time value from the previous update for the specified task.
         * Useful for calculating velocities or detecting time reversals.
         *
         * @param task Task to get previous time for
         * @return Previous frame time in seconds
         */
        f64 getPreviousTime( TaskId task ) const override;

        /**
         * @brief Gets the delta time for a specific task.
         *
         * Returns the time elapsed since last update for the specified task.
         *
         * @param task Task to get delta time for
         * @return Delta time in seconds for the specified task
         */
        f64 getDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the smoothed time for a specific task.
         *
         * Manually sets the smoothed time value for the specified task.
         *
         * @param task Task to set smoothed time for
         * @param t Smoothed time value in seconds
         */
        void setSmoothTime( TaskId task, f64 t );

        /**
         * @brief Gets the smoothed time for a specific task.
         *
         * Returns the frame-smoothed time value for the specified task.
         *
         * @param task Task to get smoothed time for
         * @return Smoothed time in seconds
         */
        f64 getSmoothTime( TaskId task ) const;

        /**
         * @brief Gets the previous smoothed time for a specific task.
         *
         * Returns the smoothed time value from the previous frame for the specified task.
         *
         * @param task Task to get previous smoothed time for
         * @return Previous smoothed time in seconds
         */
        f64 getPrevSmoothTime( TaskId task ) const;

        /**
         * @brief Gets the smoothed delta time for a specific task.
         *
         * Returns the frame-smoothed delta time for the specified task.
         *
         * @param task Task to get smoothed delta time for
         * @return Smoothed delta time in seconds
         */
        f64 getSmoothDeltaTime( TaskId task ) const override;

        /**
         * @brief Gets the maximum allowed delta time for a specific task.
         *
         * Returns the upper limit for delta time values. Large delta times
         * (e.g., from pausing/debugging) can be clamped to this value.
         *
         * @param task Task to get maximum delta time for
         * @return Maximum delta time in seconds
         */
        f64 getMaxDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the maximum allowed delta time for a specific task.
         *
         * Configures the upper limit for delta time values to prevent
         * instability from large time steps.
         *
         * @param task Task to set maximum delta time for
         * @param t Maximum delta time in seconds
         */
        void setMaxDeltaTime( TaskId task, f64 t ) override;

        /**
         * @brief Gets the minimum allowed delta time for a specific task.
         *
         * Returns the lower limit for delta time values to prevent
         * issues with very small or zero time steps.
         *
         * @param task Task to get minimum delta time for
         * @return Minimum delta time in seconds
         */
        f64 getMinDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the minimum allowed delta time for a specific task.
         *
         * Configures the lower limit for delta time values.
         *
         * @param task Task to set minimum delta time for
         * @param t Minimum delta time in seconds
         */
        void setMinDeltaTime( TaskId task, f64 t ) override;

        /**
         * @brief Gets the start time offset for the current task.
         *
         * Returns the time offset applied at timer start or reset.
         *
         * @return Start time offset in seconds
         */
        f64 getStartOffset() const override;

        /**
         * @brief Sets the start time offset for the current task.
         *
         * Configures a time offset that is applied to timer calculations.
         *
         * @param value Start time offset in seconds
         */
        void setStartOffset( f64 value ) override;

        /**
         * @brief Gets the start time offset for a specific task.
         *
         * Returns the time offset for the specified task.
         *
         * @param task Task to get start offset for
         * @return Start time offset in seconds
         */
        f64 getStartOffset( TaskId task ) const override;

        /**
         * @brief Sets the start time offset for a specific task.
         *
         * Configures a time offset for the specified task.
         *
         * @param task Task to set start offset for
         * @param value Start time offset in seconds
         */
        void setStartOffset( TaskId task, f64 value ) override;

        /**
         * @brief Gets the fixed time offset for a specific task.
         *
         * Returns the offset applied to fixed timestep calculations.
         *
         * @param task Task to get fixed offset for
         * @return Fixed time offset in seconds
         */
        f64 getFixedOffset( TaskId task ) const override;

        /**
         * @brief Sets the fixed time offset for a specific task.
         *
         * Configures an offset for fixed timestep calculations.
         *
         * @param task Task to set fixed offset for
         * @param offset Fixed time offset in seconds
         */
        void setFixedOffset( TaskId task, f64 offset ) override;

        /**
         * @brief Gets the accumulated time for a specific task.
         *
         * Returns the total accumulated time that has been added through
         * update calls for the specified task.
         *
         * @param task Task to get accumulated time for
         * @return Accumulated time in seconds
         */
        f64 getAccumulated( TaskId task ) const override;

        /**
         * @brief Sets the accumulated time for a specific task.
         *
         * Manually sets the accumulated time value for the specified task.
         *
         * @param task Task to set accumulated time for
         * @param value Accumulated time value in seconds
         */
        void setAccumulated( TaskId task, f64 value ) override;

        /**
         * @brief Adds to the accumulated time for a specific task.
         *
         * Adds the specified time value to the accumulated time for the task.
         *
         * @param task Task to add accumulated time for
         * @param value Time value to add in seconds
         */
        void addAccumulated( TaskId task, f64 value ) override;

        /// Class registration macro for object serialization and reflection
        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Calculates smoothed frame time using event history.
         *
         * Analyzes recent frame times within the smoothing window to calculate
         * an average delta time, reducing timing jitter and spikes.
         *
         * @param fNow Current time in seconds
         * @param times Reference to the deque storing recent frame times
         * @return Smoothed delta time in seconds
         */
        f64 calculateEventTime( f64 fNow, Deque<f64> &times );

        /// Accumulated time values per task (thread-safe)
        Array<atomic_f64> m_accumulated;

        /// Start time offsets per task (thread-safe)
        Array<atomic_f64> m_startOffset;

        /// Fixed timestep offsets per task (thread-safe)
        Array<atomic_f64> m_fixedOffset;

        /// Frame smoothing time windows per task (thread-safe)
        Array<atomic_f64> m_frameSmoothingTime;

        /// Fixed timestep reference time points per task
        Array<boost::chrono::steady_clock::time_point> m_fixedTimePoints;

        /// Fixed timestep accumulated times per task (thread-safe)
        Array<atomic_f64> m_fixedTime;

        /// Fixed timestep intervals per task (thread-safe)
        Array<atomic_f64> m_fixedTimeInterval;

        /// Previous frame times per task (thread-safe)
        Array<atomic_f64> m_prevTime;

        /// Current times per task (thread-safe)
        Array<atomic_f64> m_time;

        /// Delta times per task (thread-safe)
        Array<atomic_f64> m_deltaTime;

        /// Smoothed times per task (thread-safe)
        Array<atomic_f64> m_smoothTime;

        /// Previous smoothed times per task (thread-safe)
        Array<atomic_f64> m_prevSmoothTime;

        /// Smoothed delta times per task (thread-safe)
        Array<atomic_f64> m_smoothDeltaTime;

        /// Update tick counters per task (thread-safe)
        Array<atomic_u32> m_ticks;

        /// Minimum allowed delta times per task (thread-safe)
        Array<atomic_f64> m_minDeltaTime;

        /// Maximum allowed delta times per task (thread-safe)
        Array<atomic_f64> m_maxDeltaTime;

        /// Timer start reference point
        boost::chrono::steady_clock::time_point m_startPoint;

        /// Current time reference point
        boost::chrono::steady_clock::time_point m_currentPoint;

        /// Flag indicating if timer has been started (thread-safe)
        atomic_bool m_bStarted = false;

        /// Type alias for event time history queues
        using EventTimesQueue = Deque<f64>;

        /// Event time history for frame smoothing per task
        Array<EventTimesQueue> m_eventTimes;
    };

    /**
     * @brief Inline getter for delta time of current task.
     *
     * High-performance inline implementation that retrieves delta time
     * for the calling thread's current task without function call overhead.
     *
     * @return Delta time in seconds for current task
     */
    inline f64 TimerBoost::getDeltaTime() const
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_deltaTime[task];
    }

    /**
     * @brief Inline getter for current time of current task.
     *
     * High-performance inline implementation that retrieves current time
     * for the calling thread's current task without function call overhead.
     *
     * @return Current time in seconds for current task
     */
    inline f64 TimerBoost::getTime() const
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        return m_time[task];
    }

    /**
     * @brief Inline getter for current time of specific task.
     *
     * High-performance inline implementation for retrieving time of any task.
     *
     * @param task Task to get time for
     * @return Current time in seconds for specified task
     */
    inline f64 TimerBoost::getTime( TaskId task ) const
    {
        return m_time[static_cast<s32>( task )];
    }

    /**
     * @brief Inline getter for previous time of specific task.
     *
     * High-performance inline implementation for retrieving previous frame time.
     *
     * @param task Task to get previous time for
     * @return Previous frame time in seconds for specified task
     */
    inline f64 TimerBoost::getPreviousTime( TaskId task ) const
    {
        return m_prevTime[static_cast<s32>( task )];
    }

    /**
     * @brief Inline getter for delta time of specific task.
     *
     * High-performance inline implementation for retrieving delta time of any task.
     *
     * @param task Task to get delta time for
     * @return Delta time in seconds for specified task
     */
    inline f64 TimerBoost::getDeltaTime( TaskId task ) const
    {
        return m_deltaTime[static_cast<s32>( task )];
    }

    /**
     * @brief Inline setter for smoothed time of specific task.
     *
     * High-performance inline implementation for setting smoothed time.
     *
     * @param task Task to set smoothed time for
     * @param t Smoothed time value in seconds
     */
    inline void TimerBoost::setSmoothTime( TaskId task, const f64 t )
    {
        m_smoothTime[static_cast<s32>( task )] = t;
    }

    /**
     * @brief Inline getter for smoothed time of specific task.
     *
     * High-performance inline implementation for retrieving smoothed time.
     *
     * @param task Task to get smoothed time for
     * @return Smoothed time in seconds for specified task
     */
    inline f64 TimerBoost::getSmoothTime( TaskId task ) const
    {
        return m_smoothTime[static_cast<s32>( task )];
    }

    /**
     * @brief Inline getter for previous smoothed time of specific task.
     *
     * High-performance inline implementation for retrieving previous smoothed time.
     *
     * @param task Task to get previous smoothed time for
     * @return Previous smoothed time in seconds for specified task
     */
    inline f64 TimerBoost::getPrevSmoothTime( TaskId task ) const
    {
        return m_prevSmoothTime[static_cast<s32>( task )];
    }

    /**
     * @brief Inline getter for smoothed delta time of specific task.
     *
     * High-performance inline implementation for retrieving smoothed delta time.
     *
     * @param task Task to get smoothed delta time for
     * @return Smoothed delta time in seconds for specified task
     */
    inline f64 TimerBoost::getSmoothDeltaTime( TaskId task ) const
    {
        return m_smoothDeltaTime[static_cast<s32>( task )];
    }

}  // namespace workphone

#endif  // TimerBoost_h__
