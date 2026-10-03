/**
 * @file VectorRate.hpp
 * @brief Utilities to compute the rate of change for 3D vector events.
 *
 * This header declares the template class `VectorRate3` which keeps a short
 * history of timestamped `Vector3<T>` events and computes the rate of change
 * (derivative) over a configured time interval.
 */

#ifndef __VectorRate_h__
#define __VectorRate_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Core/Deque.hpp>
#include <Workphone/Core/Pair.hpp>

namespace workphone
{

    /**
     * @brief Compute a smoothed rate-of-change for 3D vectors.
     *
     * The template parameter `T` is the scalar type used for time and vector
     * components (e.g. `f32` or `f64`). The class stores a deque of
     * timestamped `Vector3<T>` events and computes the average rate of change
     * over the configured time interval when `calculateChange()` is called.
     *
     * Typical usage:
     * - call `setInterval()` to specify the averaging window (in the same
     *   units as the timestamps used in the `Pair<T, Vector3<T>>` events)
     * - push events into the internal history (the implementation file may
     *   provide helpers for that) and call `calculateChange()` with the
     *   newest event to get a velocity-like derivative
     */
    template <class T>
    class WPCore_API VectorRate3
    {
    public:
        /**
         * @brief Construct an empty VectorRate3 with a default interval of 0.
         */
        VectorRate3();

        /**
         * @brief Destructor.
         */
        ~VectorRate3();

        /**
         * @brief Calculate the average rate of change based on stored events.
         *
         * The argument `t` is expected to be a `Pair` containing the timestamp
         * and the corresponding `Vector3<T>` sample. The function computes the
         * average change per unit time across the stored samples that fall
         * within the configured interval and returns the resulting vector
         * representing the rate of change.
         *
         * @param t The newest timestamped vector sample.
         * @return The computed rate of change as a `Vector3<T>`.
         */
        Vector3<T> calculateChange( const Pair<T, Vector3<T>> &t );

        /**
         * @brief Get the configured averaging interval.
         *
         * @return The current interval value (same units as sample timestamps).
         */
        T getInterval() const;

        /**
         * @brief Set the averaging interval.
         *
         * The interval controls how far back in time samples are considered
         * when computing the rate. A non-positive interval (<= 0) typically
         * disables averaging and may cause immediate change calculations to
         * return zero depending on implementation.
         *
         * @param interval Interval length in the same units as sample
         * timestamps.
         */
        void setInterval( T interval );

        /**
         * @brief Clear stored history of timestamped vector events.
         *
         * After calling `clear()` the internal state is reset and the next
         * call to `calculateChange()` will only consider events added after
         * the clear.
         */
        void clear();

    protected:
        /**
         * @brief Averaging interval used when computing the rate.
         *
         * Initialized to zero by default. Units match the timestamps used in
         * the `Pair<T, Vector3<T>>` events (for example seconds).
         */
        T m_interval = T( 0.0 );

        /**
         * @brief Deque of timestamped vector samples used for rate calculation.
         *
         * Each element is a `Pair<time, Vector3<T>>` where `time` is the
         * timestamp of the sample. The deque is managed as a history buffer
         * where old samples are discarded based on `m_interval`.
         */
        Deque<Pair<T, Vector3<T>>> m_eventTimes;
    };

    /**
     * @brief Convenience typedef for single-precision vectors.
     */
    using VectorRate3F = VectorRate3<f32>;

    /**
     * @brief Convenience typedef for double-precision vectors.
     */
    using VectorRate3D = VectorRate3<f64>;

}  // namespace workphone

#endif  // __VectorRate_h__
