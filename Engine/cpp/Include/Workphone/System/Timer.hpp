#ifndef __Timer_h__
#define __Timer_h__

#include <Workphone/Interface/System/ITimer.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>

namespace workphone
{
    /**
     * @brief High-precision timer implementation providing time management for game engines.
     *
     * The Timer class provides a concrete implementation of the ITimer interface, offering
     * precise time measurement and control capabilities essential for game engine operations.
     * It supports both variable and fixed timesteps, frame rate smoothing, multi-threaded
     * timing operations, and scene-based time tracking.
     *
     * Key features:
     * - High-precision time measurement using atomic operations for thread safety
     * - Variable and fixed timestep support for different update loops
     * - Delta time smoothing to reduce frame rate jitter
     * - Per-thread task timing capabilities
     * - Scene load time tracking
     * - Configurable min/max delta time clamping
     *
     * @see ITimer for interface documentation
     * @note This class is thread-safe for most operations through atomic variables
     */
    class WPCore_API Timer : public ITimer
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the timer with default values:
         * - Min/max delta times set to 0.0 (no clamping)
         * - Smoothing disabled
         * - Scene load time set to 0.0
         */
        Timer();

        /**
         * @brief Virtual destructor.
         *
         * Cleans up timer resources. Override ensures proper cleanup
         * in inheritance hierarchy.
         */
        ~Timer() override;

        /**
         * @brief Updates the timer for variable timestep operations.
         *
         * This should be called once per frame for variable timestep updates.
         * Updates current time, calculates delta time, and applies smoothing
         * if enabled.
         *
         * @see updateFixed() for fixed timestep updates
         */
        void update() override;

        /**
         * @brief Updates the timer for fixed timestep operations.
         *
         * This should be called at regular intervals for fixed timestep updates,
         * typically used for physics simulations that require deterministic timing.
         *
         * @see update() for variable timestep updates
         */
        void updateFixed() override;

        /**
         * @brief Gets the time elapsed since the scene was loaded.
         *
         * @return Time in seconds since scene load as a 64-bit floating point value
         * @see getSceneLoadTime(), setSceneLoadTime()
         */
        f64 getTimeSinceSceneLoad() override;

        /**
         * @brief Gets the scene load timestamp.
         *
         * @return The timestamp when the scene was loaded
         * @see getTimeSinceSceneLoad(), setSceneLoadTime()
         */
        time_interval getSceneLoadTime() const override;

        /**
         * @brief Sets the scene load time.
         *
         * @param t The scene load time to set
         * @see getTimeSinceSceneLoad(), getSceneLoadTime()
         */
        void setSceneLoadTime( time_interval t ) override;

        /**
         * @brief Gets the current smoothed time.
         *
         * Returns time with frame smoothing applied to reduce jitter.
         *
         * @return Smoothed time in seconds
         * @see getSmoothDeltaTime(), setSmoothDeltaTime()
         */
        f64 getSmoothTime() const override;

        /**
         * @brief Gets the current smoothed delta time.
         *
         * Returns the time between frames with smoothing applied to reduce
         * frame rate variations and provide more stable timing.
         *
         * @return Smoothed delta time in seconds
         * @see setSmoothDeltaTime(), getEnableSmoothing()
         */
        f64 getSmoothDeltaTime() const override;

        /**
         * @brief Sets the smooth delta time value.
         *
         * @param smoothDT The smooth delta time value to set in seconds
         * @see getSmoothDeltaTime(), setEnableSmoothing()
         */
        void setSmoothDeltaTime( f64 smoothDT ) override;

        /**
         * @brief Gets the current elapsed time.
         *
         * @return Current time in seconds since timer initialization
         * @see now(), getDeltaTime()
         */
        f64 getTime() const override;

        /**
         * @brief Gets the time difference between current and previous frame.
         *
         * @return Delta time in seconds
         * @see getTime(), getSmoothDeltaTime()
         */
        f64 getDeltaTime() const override;

        /**
         * @brief Gets the current high-precision timestamp.
         *
         * Provides the most accurate current time measurement available.
         *
         * @return High-precision timestamp in seconds
         * @see getTime()
         */
        f64 now() const override;

        /**
         * @brief Gets the frame smoothing time interval.
         *
         * @return Frame smoothing time interval
         * @see setFrameSmoothingTime()
         */
        time_interval getFrameSmoothingTime() const override;

        /**
         * @brief Sets the frame smoothing time interval.
         *
         * @param value The frame smoothing time interval to set
         * @see getFrameSmoothingTime()
         */
        void setFrameSmoothingTime( time_interval value ) override;

        /**
         * @brief Gets the tick count for the main thread.
         *
         * @return Number of timer updates since initialization
         * @see getTickCount(TaskId)
         */
        u32 getTickCount() override;

        /**
         * @brief Gets the tick count for a specific task/thread.
         *
         * @param task The thread task to query
         * @return Number of timer updates for the specified task
         * @see getTickCount()
         */
        u32 getTickCount( TaskId task ) override;

        /**
         * @brief Checks if the timer uses a steady clock.
         *
         * A steady clock is not affected by system clock adjustments.
         *
         * @return True if using a steady clock, false otherwise
         */
        bool isSteady() const override;

        /**
         * @brief Resets the timer to its initial state.
         *
         * Clears all accumulated timing data and restarts from zero.
         *
         * @see reset(f64)
         */
        void reset() override;

        /**
         * @brief Resets the timer with a specific start time.
         *
         * @param t The start time to set in seconds
         * @see reset()
         */
        void reset( f64 t ) override;

        /**
         * @brief Gets the minimum allowed delta time.
         *
         * Delta times below this value will be clamped to prevent
         * issues with very small time steps.
         *
         * @return Minimum delta time in seconds
         * @see setMinDeltaTime(), getMaxDeltaTime()
         */
        f64 getMinDeltaTime() const;

        /**
         * @brief Sets the minimum allowed delta time.
         *
         * @param value Minimum delta time in seconds
         * @see getMinDeltaTime(), setMaxDeltaTime()
         */
        void setMinDeltaTime( f64 value );

        /**
         * @brief Gets the maximum allowed delta time.
         *
         * Delta times above this value will be clamped to prevent
         * issues with very large time steps (e.g., when debugging).
         *
         * @return Maximum delta time in seconds
         * @see setMaxDeltaTime(), getMinDeltaTime()
         */
        f64 getMaxDeltaTime() const;

        /**
         * @brief Sets the maximum allowed delta time.
         *
         * @param value Maximum delta time in seconds
         * @see getMaxDeltaTime(), setMinDeltaTime()
         */
        void setMaxDeltaTime( f64 value );

        /**
         * @brief Gets the derived fixed time value.
         *
         * This is a virtual method that can be overridden to provide
         * custom fixed time calculations.
         *
         * @return Derived fixed time in seconds
         * @see getFixedTime(), setFixedTime()
         */
        virtual f64 getDerivedFixedTime() const;

        /**
         * @brief Gets the current fixed timestep time.
         *
         * @return Fixed time in seconds
         * @see getFixedTime(u32), setFixedTime()
         */
        f64 getFixedTime() const override;

        /**
         * @brief Gets the fixed timestep time for a specific task.
         *
         * @param task Task identifier
         * @return Fixed time in seconds for the specified task
         * @see getFixedTime(), setFixedTime()
         */
        f64 getFixedTime( u32 task ) const override;

        /**
         * @brief Gets the current fixed time with high precision.
         *
         * @return Current fixed time timestamp in seconds
         * @see getFixedTimeNow(u32), getFixedTime()
         */
        virtual f64 getFixedTimeNow() const;

        /**
         * @brief Gets the current fixed time for a specific task.
         *
         * @param task Task identifier
         * @return Current fixed time timestamp in seconds for the task
         * @see getFixedTimeNow(), getFixedTime()
         */
        virtual f64 getFixedTimeNow( u32 task ) const;

        /**
         * @brief Sets the fixed time value.
         *
         * @param value Fixed time value to set in seconds
         * @see getFixedTime(), getDerivedFixedTime()
         */
        virtual void setFixedTime( f64 value );

        /**
         * @brief Gets the fixed time interval for a specific task.
         *
         * @param task The task to query
         * @return Fixed time interval in seconds
         * @see setFixedTimeInterval(TaskId, f64)
         */
        f64 getFixedTimeInterval( TaskId task ) const override;

        /**
         * @brief Gets the global fixed time interval.
         *
         * @return Fixed time interval in seconds
         * @see setFixedTimeInterval(f64)
         */
        f64 getFixedTimeInterval() const override;

        /**
         * @brief Sets the global fixed time interval.
         *
         * @param value Fixed time interval in seconds
         * @see getFixedTimeInterval()
         */
        void setFixedTimeInterval( f64 value ) override;

        /**
         * @brief Sets the fixed time interval for a specific task.
         *
         * @param task The task to configure
         * @param value Fixed time interval in seconds
         * @see getFixedTimeInterval(TaskId)
         */
        void setFixedTimeInterval( TaskId task, f64 value ) override;

        /**
         * @brief Gets the current time for a specific task.
         *
         * @param task The task to query
         * @return Current time in seconds for the task
         * @see getPreviousTime(TaskId), getDeltaTime(TaskId)
         */
        f64 getTime( TaskId task ) const override;

        /**
         * @brief Gets the previous frame time for a specific task.
         *
         * @param task The task to query
         * @return Previous time in seconds for the task
         * @see getTime(TaskId), getDeltaTime(TaskId)
         */
        f64 getPreviousTime( TaskId task ) const override;

        /**
         * @brief Gets the delta time for a specific task.
         *
         * @param task The task to query
         * @return Delta time in seconds for the task
         * @see getTime(TaskId), getSmoothDeltaTime(TaskId)
         */
        f64 getDeltaTime( TaskId task ) const override;

        /**
         * @brief Gets the smoothed delta time for a specific task.
         *
         * @param task The task to query
         * @return Smoothed delta time in seconds for the task
         * @see getDeltaTime(TaskId), getSmoothDeltaTime()
         */
        f64 getSmoothDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the timer start time.
         *
         * This virtual method allows customization of the timer's start time.
         *
         * @param time Start time in seconds
         * @see reset(), reset(f64)
         */
        virtual void setStartTime( f64 time );

        /**
         * @brief Checks if frame smoothing is enabled.
         *
         * @return True if smoothing is enabled, false otherwise
         * @see setEnableSmoothing(), getSmoothDeltaTime()
         */
        bool getEnableSmoothing() const;

        /**
         * @brief Enables or disables frame smoothing.
         *
         * When enabled, delta times are smoothed to reduce frame rate jitter.
         *
         * @param value True to enable smoothing, false to disable
         * @see getEnableSmoothing(), setSmoothDeltaTime()
         */
        void setEnableSmoothing( bool value );

        /**
         * @brief Gets the maximum delta time for a specific task.
         *
         * @param task The task to query
         * @return Maximum delta time in seconds for the task
         * @see setMaxDeltaTime(TaskId, f64), getMinDeltaTime(TaskId)
         */
        virtual f64 getMaxDeltaTime( TaskId task ) const;

        /**
         * @brief Sets the maximum delta time for a specific task.
         *
         * @param task The task to configure
         * @param t Maximum delta time in seconds
         * @see getMaxDeltaTime(TaskId), setMinDeltaTime(TaskId, f64)
         */
        virtual void setMaxDeltaTime( TaskId task, f64 t );

        /**
         * @brief Gets the minimum delta time for a specific task.
         *
         * @param task The task to query
         * @return Minimum delta time in seconds for the task
         * @see setMinDeltaTime(TaskId, f64), getMaxDeltaTime(TaskId)
         */
        virtual f64 getMinDeltaTime( TaskId task ) const;

        /**
         * @brief Sets the minimum delta time for a specific task.
         *
         * @param task The task to configure
         * @param t Minimum delta time in seconds
         * @see getMinDeltaTime(TaskId), setMaxDeltaTime(TaskId, f64)
         */
        virtual void setMinDeltaTime( TaskId task, f64 t );

        /**
         * @brief Gets the start offset time.
         *
         * @return Start offset in seconds
         * @see setStartOffset(f64)
         */
        f64 getStartOffset() const override;

        /**
         * @brief Sets the start offset time.
         *
         * @param value Start offset in seconds
         * @see getStartOffset()
         */
        void setStartOffset( f64 value ) override;

        /**
         * @brief Gets the start offset for a specific task.
         *
         * @param task The task to query
         * @return Start offset in seconds for the task
         * @see setStartOffset(TaskId, f64)
         */
        f64 getStartOffset( TaskId task ) const override;

        /**
         * @brief Sets the start offset for a specific task.
         *
         * @param task The task to configure
         * @param value Start offset in seconds
         * @see getStartOffset(TaskId)
         */
        void setStartOffset( TaskId task, f64 value ) override;

        /**
         * @brief Gets the fixed offset for a specific task.
         *
         * @param task The task to query
         * @return Fixed offset in seconds for the task
         * @see setFixedOffset(TaskId, f64)
         */
        f64 getFixedOffset( TaskId task ) const override;

        /**
         * @brief Sets the fixed offset for a specific task.
         *
         * @param task The task to configure
         * @param offset Fixed offset in seconds
         * @see getFixedOffset(TaskId)
         */
        void setFixedOffset( TaskId task, f64 offset ) override;

        /**
         * @brief Gets the accumulated time for a specific task.
         *
         * Accumulated time represents the total time that has been processed
         * by the task, useful for fixed timestep calculations.
         *
         * @param task The task to query
         * @return Accumulated time in seconds for the task
         * @see setAccumulated(TaskId, f64), addAccumulated(TaskId, f64)
         */
        virtual f64 getAccumulated( TaskId task ) const;

        /**
         * @brief Sets the accumulated time for a specific task.
         *
         * @param task The task to configure
         * @param value Accumulated time in seconds
         * @see getAccumulated(TaskId), addAccumulated(TaskId, f64)
         */
        virtual void setAccumulated( TaskId task, f64 value );

        /**
         * @brief Adds to the accumulated time for a specific task.
         *
         * @param task The task to modify
         * @param value Time to add in seconds
         * @see getAccumulated(TaskId), setAccumulated(TaskId, f64)
         */
        virtual void addAccumulated( TaskId task, f64 value );

        /**
         * @brief Gets the current time in milliseconds.
         *
         * @return Current time in milliseconds
         * @see getRealTime(), getTimeIntervalMilliseconds()
         */
        u32 getTimeMilliseconds() const override;

        /**
         * @brief Gets the real (system) time in milliseconds.
         *
         * This returns the actual system time, unaffected by timer manipulations.
         *
         * @return Real time in milliseconds
         * @see getTimeMilliseconds()
         */
        u32 getRealTime() const override;

        /**
         * @brief Gets the time interval in milliseconds.
         *
         * @return Time interval between updates in milliseconds
         * @see getTimeMilliseconds(), getDeltaTime()
         */
        u32 getTimeIntervalMilliseconds() const override;

        /**
         * @brief Sets the frame smoothing period.
         *
         * @param milliSeconds Smoothing period in milliseconds
         * @see getFrameSmoothingPeriod(), setFrameSmoothingTime()
         */
        void setFrameSmoothingPeriod( u32 milliSeconds ) override;

        /**
         * @brief Gets the frame smoothing period.
         *
         * @return Smoothing period in milliseconds
         * @see setFrameSmoothingPeriod(), getFrameSmoothingTime()
         */
        u32 getFrameSmoothingPeriod() const override;

        /**
         * @brief Resets all smoothing calculations.
         *
         * Clears smoothing history and starts fresh calculations.
         *
         * @see setEnableSmoothing(), getSmoothDeltaTime()
         */
        void resetSmoothing() override;

        /// Registration macro for the workphone framework
        WP_CLASS_REGISTER_DECL;

    protected:
        /// Minimum allowed delta time in seconds (0.0 = no minimum)
        f64 m_minDeltaTime = 0.0;

        /// Maximum allowed delta time in seconds (0.0 = no maximum)
        f64 m_maxDeltaTime = 0.0;

        /// Thread-safe flag indicating if frame smoothing is enabled
        atomic_bool m_enableSmoothing = false;

        /// Thread-safe storage for time since scene/level load
        atomic_f64 m_timeSinceLevelLoad = 0.0;
    };
}  // namespace workphone

#endif  // Timer_h__
