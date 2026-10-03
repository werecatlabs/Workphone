#ifndef __ITimer_h__
#define __ITimer_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Thread/Thread.hpp>

namespace workphone
{

    /**
     * @brief Interface for a timer class.
     *
     * This class defines an interface for a timer, including functions for updating the timer,
     * retrieving and setting the current time and time intervals, getting the system time,
     * and resetting the timer.
     */
    class WPCore_API ITimer : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        ~ITimer() override;

        /**
         * @brief Updates the timer with a fixed time interval.
         * This function should be called on a fixed interval in order to update the timer.
         */
        virtual void updateFixed() = 0;

        /**
         * @brief Gets the time since level load in seconds.
         * @return A time interval value.
         */
        virtual time_interval getTimeSinceSceneLoad() = 0;

        /**
         * @brief Gets the time since level load in seconds.
         * @return A time interval value.
         */
        virtual time_interval getSceneLoadTime() const = 0;

        /**
         * @brief Sets the time since level load in seconds.
         * @param t A time interval value.
         */
        virtual void setSceneLoadTime( time_interval t ) = 0;

        /**
         * @brief Gets the current smooth time in seconds.
         * @return A double value.
         */
        virtual time_interval getSmoothTime() const = 0;

        /**
         * @brief Gets the current smooth delta time in seconds.
         * @return A double value.
         */
        virtual time_interval getSmoothDeltaTime() const = 0;

        /**
         * @brief Sets the smoothing period for delta time.
         * @param smoothDT The smoothing period in milliseconds.
         */
        virtual void setSmoothDeltaTime( double smoothDT ) = 0;

        /**
         * @brief Returns the current virtual time in milliseconds.
         *
         * This value starts with 0 and can be manipulated using setTime(), stopTimer(),
         * startTimer(), etc. This value depends on the set speed of the timer if the timer
         * is stopped, etc. If you need the system time, use getRealTime().
         * @return A 32-bit unsigned integer value.
         */
        virtual u32 getTimeMilliseconds() const = 0;

        /**
         * @brief Gets the system time in milliseconds.
         * @return A 32-bit unsigned integer value.
         */
        virtual u32 getRealTime() const = 0;

        /**
         * @brief Returns the time interval in milliseconds between ticks.
         * @return A 32-bit unsigned integer value.
         */
        virtual u32 getTimeIntervalMilliseconds() const = 0;

        /**
         * @brief Gets the current time in seconds.
         * @return A time interval value.
         */
        virtual time_interval now() const = 0;

        /**
         * @brief Gets the current time in seconds.
         * @return A time interval value.
         */
        virtual time_interval getTime() const = 0;

        /**
         * @brief Gets the current delta time in seconds.
         * @return A double value.
         */
        virtual time_interval getDeltaTime() const = 0;

        /**
         * @brief Sets the frame smoothing period.
         * @param milliSeconds The smoothing period in milliseconds.
         */
        virtual void setFrameSmoothingPeriod( u32 milliSeconds ) = 0;

        /**
         * @brief Gets the frame smoothing period.
         * @return The frame smoothing period in milliseconds as an unsigned 32-bit integer.
         */
        virtual u32 getFrameSmoothingPeriod() const = 0;

        /**
         * @brief Resets the frame smoothing.
         * This function resets the frame smoothing, clearing any accumulated values.
         */
        virtual void resetSmoothing() = 0;

        /**
         * @brief Gets the tick count for the specified task.
         * @return The tick count as an unsigned 32-bit integer.
         */
        virtual u32 getTickCount() = 0;

        /**
         * @brief Gets the tick count for the specified task.
         * @param task The task to get the tick count for.
         * @return The tick count as an unsigned 32-bit integer.
         */
        virtual u32 getTickCount( TaskId task ) = 0;

        /**
         * @brief Gets the fixed time interval for the specified task.
         * @param task The task to get the fixed time interval for.
         * @return The fixed time interval as a double-precision floating point value.
         */
        virtual f64 getFixedTimeInterval( TaskId task ) const = 0;

        /**
         * @brief Gets the fixed time interval.
         * @return The fixed time interval as a double-precision floating point value.
         */
        virtual f64 getFixedTimeInterval() const = 0;

        /**
         * @brief Sets the fixed time interval.
         * @param fixedTimeInterval The fixed time interval as a double-precision floating point value.
         */
        virtual void setFixedTimeInterval( f64 fixedTimeInterval ) = 0;

        /**
         * @brief Sets the fixed time interval for the specified task.
         * @param task The task to set the fixed time interval for.
         * @param fixedTimeInterval The fixed time interval as a double-precision floating point value.
         */
        virtual void setFixedTimeInterval( TaskId task, f64 fixedTimeInterval ) = 0;

        /**
         * @brief Gets the time for the specified task.
         * @param task The task to get the time for.
         * @return The time as a double-precision floating point value.
         */
        virtual f64 getTime( TaskId task ) const = 0;

        /**
         * @brief Gets the previous time for the specified task.
         * @param task The task to get the previous time for.
         * @return The previous time as a double-precision floating point value.
         */
        virtual f64 getPreviousTime( TaskId task ) const = 0;

        /**
         * @brief Gets the delta time for the specified task.
         * @param task The task to get the delta time for.
         * @return The delta time as a double-precision floating point value.
         */
        virtual f64 getDeltaTime( TaskId task ) const = 0;

        /**
         * @brief Gets the smooth delta time for the specified task.
         * @param task The task to get the smooth delta time for.
         * @return The smooth delta time as a double-precision floating point value.
         */
        virtual f64 getSmoothDeltaTime( TaskId task ) const = 0;

        /**
         * @brief Gets the start offset.
         * @return The start offset as a double-precision floating point value.
         */
        virtual f64 getStartOffset() const = 0;

        /**
         * @brief Sets the start offset.
         * @param startOffset The start offset as a double-precision floating point value.
         */
        virtual void setStartOffset( f64 startOffset ) = 0;

        /**
         * @brief Gets the start offset for the specified task.
         * @param task The task to get the start offset for.
         * @return The start offset as a double-precision floating point value.
         */
        virtual f64 getStartOffset( TaskId task ) const = 0;

        /**
         * @brief Sets the start offset for the specified task.
         * @param task The task to set the start offset for.
         * @param startOffset The start offset as a double-precision floating point value.
         */
        virtual void setStartOffset( TaskId task, f64 startOffset ) = 0;

        /**
         * @brief Checks if the timer is steady.
         * @return True if the timer is steady, false otherwise.
         */
        virtual bool isSteady() const = 0;

        /**
         * @brief Resets the timer.
         * This function resets the timer, clearing any accumulated values and setting the start time to
         * the current time.
         */
        virtual void reset() = 0;

        /**
         * @brief Resets the timer with a specified start time.
         * @param t The start time as a double-precision floating point value.
         */
        virtual void reset( f64 t ) = 0;

        /**
         * @brief Gets the fixed time.
         * @return The fixed time as a double-precision floating point value.
         */
        virtual f64 getFixedTime() const = 0;

        /**
         * @brief Gets the fixed time for the specified task.
         * @param task The task to get the fixed time for.
         * @return The fixed time as a double-precision floating point value.
         */
        virtual f64 getFixedTime( u32 task ) const = 0;

        /**
         * @brief Gets the fixed offset for the specified task.
         * @param task The task to get the fixed offset for.
         * @return The fixed offset as a double-precision floating point value.
         */
        virtual f64 getFixedOffset( TaskId task ) const = 0;

        /**
         * @brief Sets the fixed offset for the specified task.
         * @param task The task to set the fixed offset for.
         * @param offset The fixed offset as a double-precision floating point value.
         */
        virtual void setFixedOffset( TaskId task, f64 offset ) = 0;

        /** Get frame smoothing time.
         * @return The frame smoothing time as a time interval value.
         */
        virtual time_interval getFrameSmoothingTime() const = 0;

        /** Set frame smoothing time.
         * @param frameSmoothingTime The frame smoothing time as a time interval value.
         */
        virtual void setFrameSmoothingTime( time_interval frameSmoothingTime ) = 0;

        WP_CLASS_REGISTER_DECL;
    };
}  // namespace workphone

#endif  // FBTimer_h__
