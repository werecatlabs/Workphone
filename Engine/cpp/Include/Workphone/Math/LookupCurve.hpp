#ifndef LookupCurve_h__
#define LookupCurve_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{
    /**
     * @brief Generic lookup curve with smooth interpolation for key-value pairs.
     *
     * @details LookupCurve provides efficient storage and smooth curve interpolation of
     * key-value pairs. Unlike LookupTable which uses linear interpolation, LookupCurve
     * provides smoother transitions between control points using cubic interpolation.
     *
     * The lookup curve stores data as an array of key-value pairs where the first element
     * is the lookup key (input parameter) and the second element is the corresponding value.
     * Cubic interpolation is used to calculate smooth values between stored data points.
     *
     * @tparam T The numeric type for both keys and values (typically float or double)
     *
     * @par Thread Safety
     * This class is not thread-safe. External synchronization is required if accessed from
     * multiple threads concurrently.
     *
     * @par Usage Example
     * @code
     * LookupCurveF curve;
     * Array<Pair<f32, f32>> points = {{0.0f, 1.0f}, {1.0f, 3.0f}, {2.0f, 2.0f}};
     * curve.setPoints(points);
     * f32 value = curve.interpolate(0.5f); // Returns smoothly interpolated value
     * @endcode
     *
     * @since Version 1.0
     */
    template <class T>
    class WPCore_API LookupCurve : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @details Initializes an empty lookup curve with no data points.
         *
         * @par Complexity
         * O(1) - Constant time initialization
         */
        LookupCurve();

        /**
         * @brief Virtual destructor.
         *
         * @details Safely destroys the lookup curve object and releases any allocated resources.
         */
        ~LookupCurve() override;

        /**
         * @brief Sets lookup curve data using key-value pairs.
         *
         * @details This method sets the lookup curve data where each pair contains
         * a key (input parameter) and the corresponding value. The data is stored
         * internally and can be used for smooth curve interpolation.
         *
         * @param points Array of key-value pairs where first is the lookup key and second is the value
         *
         * @pre The points array should be sorted by key values for proper interpolation behavior
         * @post The lookup curve is ready for interpolation between the given points
         *
         * @note For best results, ensure keys are in ascending order
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<Pair<T, T>> &points );

        /**
         * @brief Sets lookup curve data using separate key and value arrays.
         *
         * @details This overload allows setting lookup curve data using separate arrays for
         * keys and values. Both arrays must have the same size and corresponding indices
         * represent paired key-value relationships.
         *
         * @param keys Array of lookup keys (input parameters)
         * @param points Array of values corresponding to each key
         *
         * @pre keys.size() == points.size()
         * @pre The keys array should be sorted in ascending order for proper interpolation
         * @post The lookup curve is ready for interpolation between the given points
         *
         * @note The method uses the minimum of both array sizes if they differ
         *
         * @par Complexity
         * O(n) where n is min(keys.size(), points.size())
         */
        void setPoints( const Array<T> &keys, const Array<T> &points );

        /**
         * @brief Sets lookup curve data with implicit sequential keys.
         *
         * @details This method sets lookup curve data with implicit sequential key values.
         * The key values are automatically generated as 0, 1, 2, ..., n-1 where
         * n is the number of points provided.
         *
         * @param points Array of values with implicit sequential keys starting from 0
         *
         * @post The lookup curve is ready for interpolation with keys in range [0, points.size()-1]
         *
         * @note This is useful when you have evenly indexed data points
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<T> &points );

        /**
         * @brief Performs smooth curve interpolation to find the value at the given key.
         *
         * @details Performs cubic (Catmull-Rom) interpolation to find the value corresponding
         * to the specified key. The method searches for the appropriate key range and smoothly
         * interpolates between the surrounding data points using neighboring control points.
         * If the key is outside the stored range, the nearest endpoint value is returned.
         *
         * @param t The lookup key for which to find the interpolated value
         * @return Smoothly interpolated value at the specified key
         *
         * @pre The lookup curve must have at least 1 data point
         *
         * @note If t is less than the first key, the first value is returned
         * @note If t is greater than the last key, the last value is returned
         * @note If only one data point exists, that value is always returned
         * @note With 2 points, linear interpolation is used; with 3+, cubic interpolation
         *
         * @par Complexity
         * O(n) where n is the number of data points (linear search for key range)
         */
        T interpolate( T t );

    protected:
        /// @brief Array of key-value pairs storing the lookup curve data
        Array<Pair<real_Num, real_Num>> m_points;
    };

    /// @brief Type alias for 32-bit integer lookup curve
    using LookupCurveI = LookupCurve<s32>;

    /// @brief Type alias for single-precision floating-point lookup curve
    using LookupCurveF = LookupCurve<f32>;

    /// @brief Type alias for double-precision floating-point lookup curve
    using LookupCurveD = LookupCurve<f64>;

}  // namespace workphone

#endif  // LookupCurve_h__
