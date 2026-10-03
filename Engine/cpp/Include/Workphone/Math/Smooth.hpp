#ifndef Smooth_h__
#define Smooth_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Pair.hpp>

namespace workphone
{
    /**
     * @brief A template class for smoothing numeric values over a time interval.
     *
     * The Smooth class maintains a sliding window of timestamped values and computes
     * smoothed derivatives by calculating the rate of change over the specified time interval.
     * This is useful for reducing noise in time-series data such as sensor readings,
     * animation values, or physics calculations.
     *
     * The smoothing algorithm works by:
     * 1. Storing timestamped values in a deque
     * 2. Removing values older than the smooth interval
     * 3. Computing the rate of change between the oldest and newest values
     *
     * @tparam T The numeric type of the values to be smoothed (e.g., float, double, Vector3)
     *
     * @note The type T must support:
     *       - Default construction T()
     *       - Subtraction operator (T - T)
     *       - Division by integer (T / size_t)
     *       - Assignment from time_interval
     *
     * @warning This class is not thread-safe. External synchronization is required
     *          for concurrent access.
     *
     * @par Usage Example:
     * @code
     * Smooth<float> velocitySmooth;
     * velocitySmooth.setSmoothInterval(1.0f); // 1 second smoothing window
     *
     * // Add timestamped values
     * float smoothedVelocity = velocitySmooth.getValue({currentTime, currentPosition});
     * @endcode
     */
    template <class T>
    class WPCore_API Smooth : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates a Smooth instance with a default smoothing interval of 1.0 time units.
         * The internal value storage is initially empty.
         */
        Smooth();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of resources when the object is destroyed
         * through a base class pointer.
         */
        ~Smooth() override;

        /**
         * @brief Adds a new timestamped value and returns the smoothed derivative.
         *
         * This method adds the provided timestamped value to the internal buffer,
         * removes values that fall outside the smoothing interval, and computes
         * the rate of change (derivative) over the remaining time window.
         *
         * The smoothed value represents the average rate of change per time unit
         * across all values within the smoothing interval.
         *
         * @param value A pair containing the timestamp and the associated value
         * @return The smoothed derivative (rate of change). Returns T() if insufficient data.
         *
         * @note On the first call, this method returns T() (default-constructed value)
         *       since at least two values are required to compute a derivative.
         *
         * @note Values are automatically pruned to maintain only those within the
         *       current smoothing interval, keeping memory usage bounded.
         */
        T getValue( const Pair<time_interval, T> &value );

        /**
         * @brief Gets the current smoothing time interval.
         *
         * @return The time interval over which smoothing is performed
         */
        time_interval getSmoothInterval() const;

        /**
         * @brief Sets the smoothing time interval.
         *
         * Changes the time window used for smoothing calculations. Values older
         * than this interval will be discarded on the next call to getValue().
         *
         * @param smoothInterval The new smoothing interval. Must be positive.
         *
         * @note Changing the interval does not immediately affect stored values.
         *       The change takes effect on the next call to getValue().
         *
         * @warning Setting a very small interval may result in insufficient data
         *          for meaningful smoothing.
         */
        void setSmoothInterval( time_interval smoothInterval );

    protected:
        /**
         * @brief The time interval over which smoothing is performed.
         *
         * Values older than this interval are discarded from the smoothing calculation.
         * Default value is T(1.0).
         */
        time_interval m_smoothInterval = T( 1.0 );

        /**
         * @brief Storage for timestamped values within the smoothing window.
         *
         * This deque maintains values in chronological order, with the oldest
         * values at the front and newest at the back. Values are automatically
         * pruned to stay within the smoothing interval.
         */
        Deque<Pair<time_interval, T>> m_values;
    };

}  // namespace workphone

#endif  // Smooth_h__
