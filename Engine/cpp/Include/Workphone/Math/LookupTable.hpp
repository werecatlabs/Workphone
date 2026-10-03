#ifndef LookupTable_h__
#define LookupTable_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Math.hpp>

namespace workphone
{
    /**
     * @brief Generic lookup table with linear interpolation for key-value pairs.
     *
     * @details LookupTable provides efficient storage and linear interpolation of key-value
     * pairs. It supports three different ways to set control points and performs linear
     * interpolation between adjacent points to find intermediate values.
     *
     * The lookup table stores data as an array of key-value pairs where the first element
     * is the lookup key (input parameter) and the second element is the corresponding value.
     * Linear interpolation is used to calculate values between stored data points.
     *
     * @tparam T The numeric type for both keys and values (typically float or double)
     *
     * @par Thread Safety
     * This class is not thread-safe. External synchronization is required if accessed from
     * multiple threads concurrently.
     *
     * @par Usage Example
     * @code
     * LookupTableF table;
     * Array<Pair<f32, f32>> points = {{0.0f, 1.0f}, {1.0f, 3.0f}, {2.0f, 2.0f}};
     * table.setPoints(points);
     * f32 value = table.interpolate(0.5f); // Returns 2.0f (linear interpolation)
     * @endcode
     *
     * @since Version 1.0
     */
    template <class T>
    class WPCore_API LookupTable : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @details Initializes an empty lookup table with no data points.
         *
         * @par Complexity
         * O(1) - Constant time initialization
         */
        LookupTable();

        /**
         * @brief Virtual destructor.
         *
         * @details Safely destroys the lookup table object and releases any allocated resources.
         */
        ~LookupTable() override;

        /**
         * @brief Sets lookup table data using key-value pairs.
         *
         * @details This method sets the lookup table data where each pair contains
         * a key (input parameter) and the corresponding value. The data is stored
         * internally and can be used for interpolation.
         *
         * @param points Array of key-value pairs where first is the lookup key and second is the value
         *
         * @pre The points array should be sorted by key values for proper interpolation behavior
         * @post The lookup table is ready for interpolation between the given points
         *
         * @note For best results, ensure keys are in ascending order
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<Pair<T, T>> &points );

        /**
         * @brief Sets lookup table data using separate key and value arrays.
         *
         * @details This overload allows setting lookup table data using separate arrays for
         * keys and values. Both arrays must have the same size and corresponding indices
         * represent paired key-value relationships.
         *
         * @param keys Array of lookup keys (input parameters)
         * @param points Array of values corresponding to each key
         *
         * @pre keys.size() == points.size()
         * @pre The keys array should be sorted in ascending order for proper interpolation
         * @post The lookup table is ready for interpolation between the given points
         *
         * @note The method uses the minimum of both array sizes if they differ
         *
         * @par Complexity
         * O(n) where n is min(keys.size(), points.size())
         */
        void setPoints( const Array<T> &keys, const Array<T> &points );

        /**
         * @brief Sets lookup table data with implicit sequential keys.
         *
         * @details This method sets lookup table data with implicit sequential key values.
         * The key values are automatically generated as 0, 1, 2, ..., n-1 where
         * n is the number of points provided.
         *
         * @param points Array of values with implicit sequential keys starting from 0
         *
         * @post The lookup table is ready for interpolation with keys in range [0, points.size()-1]
         *
         * @note This is useful when you have evenly indexed data points
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<T> &points );

        /**
         * @brief Performs linear interpolation to find the value at the given key.
         *
         * @details Performs linear interpolation to find the value corresponding to the
         * specified key. The method searches for the appropriate key range and linearly
         * interpolates between the surrounding data points. If the key is outside the
         * stored range, the nearest endpoint value is returned.
         *
         * The interpolation formula used is:
         * result = value0 + (value1 - value0) * ((key - key0) / (key1 - key0))
         *
         * @param t The lookup key for which to find the interpolated value
         * @return Interpolated value at the specified key
         *
         * @pre The lookup table must have at least 1 data point
         *
         * @note If t is less than the first key, the first value is returned
         * @note If t is greater than the last key, the last value is returned
         * @note If only one data point exists, that value is always returned
         *
         * @par Complexity
         * O(n) where n is the number of data points (linear search for key range)
         */
        T interpolate( T t );

    private:
        /// @brief Array of key-value pairs storing the lookup table data
        Array<Pair<T, T>> m_points;
    };

    /// @brief Type alias for single-precision floating-point lookup table
    using LookupTableI = LookupTable<s32>;

    /// @brief Type alias for single-precision floating-point lookup table
    using LookupTableF = LookupTable<f32>;

    /// @brief Type alias for double-precision floating-point lookup table
    using LookupTableD = LookupTable<f64>;

}  // namespace workphone

#endif  // LookupTable_h__
