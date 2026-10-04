#ifndef TimerMT_h__
#define TimerMT_h__

#include <Workphone/System/Timer.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Atomics/AtomicFloat.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/Array.hpp>
#include <cmath>
#include <chrono>

namespace workphone
{

    /**
     * @class TimerMT
     * @brief Thread-safe timer based on std::chrono::steady_clock.
     *
     * This class provides a multi-threaded, high-resolution timer with support for
     * frame smoothing, fixed time steps, and per-task timing. It is designed to be
     * used in environments where accurate and thread-safe timing is required.
     *
     * @note All methods are thread-safe unless otherwise specified.
     */
    class WPCore_API TimerMT : public Timer
    {
    public:
        /**
         * @brief Constructs a new TimerMT object.
         */
        TimerMT();

        /**
         * @brief Destroys the TimerMT object.
         */
        ~TimerMT() override;

        void load( SmartPtr<ISharedObject> data ) override;
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Updates the timer based on the current time.
         *
         * This should be called once per frame to update internal timing values.
         */
        void update() override;

        /**
         * @brief Updates the timer with a specified delta time.
         *
         * @param dt The time in seconds since the last update.
         */
        void update( f64 dt );

        /**
         * @brief Updates the timer using a fixed time step.
         *
         * This is typically used for physics or logic updates that require a fixed interval.
         */
        void updateFixed() override;

        /**
         * @brief Gets the current time in seconds since the timer started.
         *
         * @return The current time in seconds.
         */
        f64 getTime() const override;

        /**
         * @brief Gets the time elapsed since the last update.
         *
         * @return The delta time in seconds.
         */
        f64 getDeltaTime() const override;

        /**
         * @brief Gets the current time in seconds (alias for getTime).
         *
         * @return The current time in seconds.
         */
        f64 now() const override;

        /**
         * @brief Gets the frame smoothing time interval.
         *
         * @return The frame smoothing time interval.
         */
        time_interval getFrameSmoothingTime() const override;

        /**
         * @brief Sets the frame smoothing time interval.
         *
         * @param frameSmoothingTime The new frame smoothing time interval.
         */
        void setFrameSmoothingTime( time_interval frameSmoothingTime ) override;

        /**
         * @brief Gets the tick count for the current task.
         *
         * @return The tick count.
         */
        u32 getTickCount() override;

        /**
         * @brief Gets the tick count for a specific task.
         *
         * @param task The task for which to get the tick count.
         * @return The tick count for the specified task.
         */
        u32 getTickCount( TaskId task ) override;

        /**
         * @brief Resets the timer to zero.
         */
        void reset() override;

        /**
         * @brief Resets the timer to a specific time value.
         *
         * @param t The time value to reset to.
         */
        void reset( f64 t ) override;

        /**
         * @brief Checks if the timer uses a steady clock.
         *
         * @return True if the timer is steady, false otherwise.
         */
        bool isSteady() const override;

        /**
         * @brief Gets the derived fixed time value.
         *
         * @return The derived fixed time in seconds.
         */
        f64 getDerivedFixedTime() const override;

        /**
         * @brief Gets the fixed time value for the current task.
         *
         * @return The fixed time in seconds.
         */
        f64 getFixedTime() const override;

        /**
         * @brief Gets the fixed time value for a specific task.
         *
         * @param task The task for which to get the fixed time.
         * @return The fixed time in seconds for the specified task.
         */
        f64 getFixedTime( u32 task ) const override;

        /**
         * @brief Gets the current fixed time (now) for the current task.
         *
         * @return The current fixed time in seconds.
         */
        f64 getFixedTimeNow() const override;

        /**
         * @brief Gets the current fixed time (now) for a specific task.
         *
         * @param task The task for which to get the fixed time now.
         * @return The current fixed time in seconds for the specified task.
         */
        f64 getFixedTimeNow( u32 task ) const override;

        /**
         * @brief Sets the fixed time value for the current task.
         *
         * @param value The new fixed time value in seconds.
         */
        void setFixedTime( f64 value ) override;

        /**
         * @brief Gets the fixed time interval for a specific task.
         *
         * @param task The task for which to get the fixed time interval.
         * @return The fixed time interval in seconds.
         */
        f64 getFixedTimeInterval( TaskId task ) const override;

        /**
         * @brief Gets the fixed time interval for the current task.
         *
         * @return The fixed time interval in seconds.
         */
        f64 getFixedTimeInterval() const override;

        /**
         * @brief Sets the fixed time interval for the current task.
         *
         * @param value The new fixed time interval in seconds.
         */
        void setFixedTimeInterval( f64 value ) override;

        /**
         * @brief Sets the fixed time interval for a specific task.
         *
         * @param task The task for which to set the fixed time interval.
         * @param value The new fixed time interval in seconds.
         */
        void setFixedTimeInterval( TaskId task, f64 value ) override;

        /**
         * @brief Sets the start time for the timer.
         *
         * @param time The start time in seconds.
         */
        void setStartTime( f64 time ) override;

        /**
         * @brief Gets the current time in seconds with higher precision (128-bit).
         *
         * @return The current time in seconds (128-bit precision).
         */
        f64 now128() const;

        /**
         * @brief Gets the smoothed time value for the current task.
         *
         * @return The smoothed time in seconds.
         */
        f64 getSmoothTime() const override;

        /**
         * @brief Gets the smoothed delta time for the current task.
         *
         * @return The smoothed delta time in seconds.
         */
        f64 getSmoothDeltaTime() const override;

        /**
         * @brief Sets the smoothed delta time for the current task.
         *
         * @param smoothDT The new smoothed delta time in seconds.
         */
        void setSmoothDeltaTime( f64 smoothDT ) override;

        /**
         * @brief Gets the time value for a specific task.
         *
         * @param task The task for which to get the time.
         * @return The time in seconds for the specified task.
         */
        f64 getTime( TaskId task ) const override;

        /**
         * @brief Gets the previous time value for a specific task.
         *
         * @param task The task for which to get the previous time.
         * @return The previous time in seconds for the specified task.
         */
        f64 getPreviousTime( TaskId task ) const override;

        /**
         * @brief Gets the delta time for a specific task.
         *
         * @param task The task for which to get the delta time.
         * @return The delta time in seconds for the specified task.
         */
        f64 getDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the smoothed time for a specific task.
         *
         * @param task The task for which to set the smoothed time.
         * @param t The new smoothed time in seconds.
         */
        void setSmoothTime( TaskId task, f64 t );

        /**
         * @brief Gets the smoothed time for a specific task.
         *
         * @param task The task for which to get the smoothed time.
         * @return The smoothed time in seconds for the specified task.
         */
        f64 getSmoothTime( TaskId task ) const;

        /**
         * @brief Gets the previous smoothed time for a specific task.
         *
         * @param task The task for which to get the previous smoothed time.
         * @return The previous smoothed time in seconds for the specified task.
         */
        f64 getPrevSmoothTime( TaskId task ) const;

        /**
         * @brief Gets the smoothed delta time for a specific task.
         *
         * @param task The task for which to get the smoothed delta time.
         * @return The smoothed delta time in seconds for the specified task.
         */
        f64 getSmoothDeltaTime( TaskId task ) const override;

        /**
         * @brief Gets the maximum delta time for a specific task.
         *
         * @param task The task for which to get the maximum delta time.
         * @return The maximum delta time in seconds for the specified task.
         */
        f64 getMaxDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the maximum delta time for a specific task.
         *
         * @param task The task for which to set the maximum delta time.
         * @param t The new maximum delta time in seconds.
         */
        void setMaxDeltaTime( TaskId task, f64 t ) override;

        /**
         * @brief Gets the minimum delta time for a specific task.
         *
         * @param task The task for which to get the minimum delta time.
         * @return The minimum delta time in seconds for the specified task.
         */
        f64 getMinDeltaTime( TaskId task ) const override;

        /**
         * @brief Sets the minimum delta time for a specific task.
         *
         * @param task The task for which to set the minimum delta time.
         * @param t The new minimum delta time in seconds.
         */
        void setMinDeltaTime( TaskId task, f64 t ) override;

        /**
         * @brief Gets the start offset for the current task.
         *
         * @return The start offset in seconds.
         */
        f64 getStartOffset() const override;

        /**
         * @brief Sets the start offset for the current task.
         *
         * @param value The new start offset in seconds.
         */
        void setStartOffset( f64 value ) override;

        /**
         * @brief Gets the start offset for a specific task.
         *
         * @param task The task for which to get the start offset.
         * @return The start offset in seconds for the specified task.
         */
        f64 getStartOffset( TaskId task ) const override;

        /**
         * @brief Sets the start offset for a specific task.
         *
         * @param task The task for which to set the start offset.
         * @param value The new start offset in seconds.
         */
        void setStartOffset( TaskId task, f64 value ) override;

        /**
         * @brief Gets the fixed offset for a specific task.
         *
         * @param task The task for which to get the fixed offset.
         * @return The fixed offset in seconds for the specified task.
         */
        f64 getFixedOffset( TaskId task ) const override;

        /**
         * @brief Sets the fixed offset for a specific task.
         *
         * @param task The task for which to set the fixed offset.
         * @param offset The new fixed offset in seconds.
         */
        void setFixedOffset( TaskId task, f64 offset ) override;

        /**
         * @brief Gets the accumulated time for a specific task.
         *
         * @param task The task for which to get the accumulated time.
         * @return The accumulated time in seconds for the specified task.
         */
        f64 getAccumulated( TaskId task ) const override;

        /**
         * @brief Sets the accumulated time for a specific task.
         *
         * @param task The task for which to set the accumulated time.
         * @param value The new accumulated time in seconds.
         */
        void setAccumulated( TaskId task, f64 value ) override;

        /**
         * @brief Adds to the accumulated time for a specific task.
         *
         * @param task The task for which to add to the accumulated time.
         * @param value The value to add in seconds.
         */
        void addAccumulated( TaskId task, f64 value ) override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /**
         * @brief Calculates the average event time from a queue of time points.
         *
         * @param fNow The current time.
         * @param times The queue of previous event times.
         * @return The calculated event time.
         */
        f64 calculateEventTime( f64 fNow, Deque<f64> &times );

        /// Accumulated time for each task (thread-safe).
        Array<atomic_f64> m_accumulated;
        /// Start offset for each task (thread-safe).
        Array<atomic_f64> m_startOffset;
        /// Fixed offset for each task (thread-safe).
        Array<atomic_f64> m_fixedOffset;

        /// Frame smoothing time for each task (thread-safe).
        Array<atomic_f64> m_frameSmoothingTime;

        /// Fixed time points for each task.
        Array<std::chrono::steady_clock::time_point> m_fixedTimePoints;
        /// Fixed time for each task (thread-safe).
        Array<atomic_f64> m_fixedTime;
        /// Fixed time interval for each task (thread-safe).
        Array<atomic_f64> m_fixedTimeInterval;

        /// Previous time for each task (thread-safe).
        Array<atomic_f64> m_prevTime;
        /// Current time for each task (thread-safe).
        Array<atomic_f64> m_time;
        /// Delta time for each task (thread-safe).
        Array<atomic_f64> m_deltaTime;

        /// Smoothed time for each task (thread-safe).
        Array<atomic_f64> m_smoothTime;
        /// Previous smoothed time for each task (thread-safe).
        Array<atomic_f64> m_prevSmoothTime;
        /// Smoothed delta time for each task (thread-safe).
        Array<atomic_f64> m_smoothDeltaTime;

        /// Tick count for each task (thread-safe).
        Array<atomic_u32> m_ticks;

        /// Minimum delta time for each task (thread-safe).
        Array<atomic_f64> m_minDeltaTime;

        /// Maximum delta time for each task (thread-safe).
        Array<atomic_f64> m_maxDeltaTime;

        /// The starting time point for the timer.
        std::chrono::steady_clock::time_point m_startPoint;

        /// The current time point for the timer.
        std::chrono::steady_clock::time_point m_currentPoint;

        /// Indicates whether the timer has started.
        atomic_bool m_started = false;

        /// Queue of event times for each task, used for smoothing.
        using EventTimesQueue = Deque<f64>;
        Array<EventTimesQueue> m_eventTimes;
    };

    // Inline implementations with improved comments

    /**
     * @brief Gets the delta time for the current task.
     * @return The delta time in seconds.
     */
    inline f64 TimerMT::getDeltaTime() const
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        WP_ASSERT( task >= 0 );
        WP_ASSERT( static_cast<size_t>( task ) < m_deltaTime.size() );
        return m_deltaTime[task];
    }

    /**
     * @brief Gets the current time for the current task.
     * @return The current time in seconds.
     */
    inline f64 TimerMT::getTime() const
    {
        auto task = static_cast<s32>( Thread::getCurrentTask() );
        WP_ASSERT( task >= 0 );
        WP_ASSERT( static_cast<size_t>( task ) < m_time.size() );
        return m_time[task];
    }

    /**
     * @brief Gets the current time for a specific task.
     * @param task The task for which to get the time.
     * @return The current time in seconds for the specified task.
     */
    inline f64 TimerMT::getTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_time.size() );
        return m_time[iTask];
    }

    /**
     * @brief Gets the previous time for a specific task.
     * @param task The task for which to get the previous time.
     * @return The previous time in seconds for the specified task.
     */
    inline f64 TimerMT::getPreviousTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_prevTime.size() );
        return m_prevTime[iTask];
    }

    /**
     * @brief Gets the delta time for a specific task.
     * @param task The task for which to get the delta time.
     * @return The delta time in seconds for the specified task.
     */
    inline f64 TimerMT::getDeltaTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_deltaTime.size() );
        return m_deltaTime[iTask];
    }

    /**
     * @brief Sets the smoothed time for a specific task.
     * @param task The task for which to set the smoothed time.
     * @param t The new smoothed time in seconds.
     */
    inline void TimerMT::setSmoothTime( TaskId task, const f64 t )
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_smoothTime.size() );
        WP_ASSERT( std::isfinite( t ) );
        WP_ASSERT( t >= 0.0 );
        m_smoothTime[iTask] = t;
    }

    /**
     * @brief Gets the smoothed time for a specific task.
     * @param task The task for which to get the smoothed time.
     * @return The smoothed time in seconds for the specified task.
     */
    inline f64 TimerMT::getSmoothTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_smoothTime.size() );
        return m_smoothTime[iTask];
    }

    /**
     * @brief Gets the previous smoothed time for a specific task.
     * @param task The task for which to get the previous smoothed time.
     * @return The previous smoothed time in seconds for the specified task.
     */
    inline f64 TimerMT::getPrevSmoothTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_prevSmoothTime.size() );
        return m_prevSmoothTime[iTask];
    }

    /**
     * @brief Gets the smoothed delta time for a specific task.
     * @param task The task for which to get the smoothed delta time.
     * @return The smoothed delta time in seconds for the specified task.
     */
    inline f64 TimerMT::getSmoothDeltaTime( TaskId task ) const
    {
        const auto iTask = static_cast<s32>( task );
        WP_ASSERT( iTask >= 0 );
        WP_ASSERT( static_cast<size_t>( iTask ) < m_smoothDeltaTime.size() );
        return m_smoothDeltaTime[iTask];
    }

}  // namespace workphone

#endif  // TimerMT_h__
