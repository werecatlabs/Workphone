#ifndef __LinearSpline1_h__
#define __LinearSpline1_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    /**
     * @brief One-dimensional linear spline interpolation class using Hermite polynomials.
     *
     * @details LinearSpline1 provides smooth interpolation between a series of 1D points using
     * Hermite polynomial interpolation. The class supports automatic tangent calculation using
     * the Catmull-Rom approach and provides efficient evaluation of interpolated values.
     *
     * The spline uses a 4x4 coefficient matrix for Hermite polynomial evaluation, which allows
     * for smooth C1 continuous curves between control points. Tangents are calculated automatically
     * using the Catmull-Rom method unless disabled for performance reasons.
     *
     * @tparam T The numeric type for spline values (typically float or double)
     *
     * @par Thread Safety
     * This class is not thread-safe. External synchronization is required if accessed from
     * multiple threads concurrently.
     *
     * @par Usage Example
     * @code
     * LinearSpline1F spline;
     * Array<Pair<f32, f32>> points = {{0.0f, 0.0f}, {1.0f, 2.0f}, {2.0f, 1.0f}};
     * spline.setPoints(points);
     * f32 value = spline.interpolate(0.5f); // Get interpolated value at t=0.5
     * @endcode
     *
     * @since Version 1.0
     */
    template <class T>
    class WPCore_API LinearSpline1 : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * @details Initializes the spline with a Hermite polynomial coefficient matrix.
         * The coefficient matrix is set up for cubic Hermite interpolation with automatic
         * tangent calculation enabled by default.
         *
         * @par Complexity
         * O(1) - Constant time initialization
         */
        LinearSpline1();

        /**
         * @brief Virtual destructor.
         *
         * @details Safely destroys the spline object and releases any allocated resources.
         */
        ~LinearSpline1() override;

        /**
         * @brief Sets spline control points using key-value pairs.
         *
         * @details This method sets the spline control points where each pair contains
         * a time/parameter value (key) and the corresponding function value. The points
         * are stored internally and tangents are recalculated if auto-calculation is enabled.
         *
         * @param points Array of key-value pairs where first is the parameter and second is the value
         *
         * @pre The points array should be sorted by the key values for proper interpolation
         * @post The spline is ready for interpolation between the given points
         *
         * @note If auto-calculation is enabled, tangents will be recalculated automatically
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<Pair<T, T>> &points );

        /**
         * @brief Sets spline control points using separate key and value arrays.
         *
         * @details This overload allows setting control points using separate arrays for
         * parameter values (keys) and function values (points). Both arrays must have
         * the same size and corresponding indices represent paired values.
         *
         * @param keys Array of parameter values (time/input values)
         * @param points Array of function values corresponding to each key
         *
         * @pre keys.size() == points.size()
         * @pre The keys array should be sorted for proper interpolation
         * @post The spline is ready for interpolation between the given points
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<T> &keys, const Array<T> &points );

        /**
         * @brief Sets spline control points with implicit uniform parameter spacing.
         *
         * @details This method sets control points with implicit uniform parameter values.
         * The parameter values are automatically generated as 0, 1, 2, ..., n-1 where
         * n is the number of points provided.
         *
         * @param points Array of function values with implicit uniform parameter spacing
         *
         * @post The spline is ready for interpolation with parameters in range [0, points.size()-1]
         *
         * @note This is useful when you have evenly spaced data points
         *
         * @par Complexity
         * O(n) where n is the number of points
         */
        void setPoints( const Array<T> &points );

        /**
         * @brief Interpolates a value at the given parameter.
         *
         * @details Performs smooth interpolation to find the function value at parameter t.
         * The parameter is normalized across the entire spline range and the appropriate
         * segment is automatically determined.
         *
         * @param t Parameter value for interpolation (typically in range [0, 1])
         * @return Interpolated function value at parameter t
         *
         * @pre The spline must have at least 2 control points
         * @pre t should be in the valid range [0, 1] for normalized interpolation
         *
         * @note This method automatically determines which spline segment to use
         *
         * @par Complexity
         * O(1) - Constant time interpolation
         */
        T interpolate( T t );

        /**
         * @brief Interpolates a value within a specific spline segment.
         *
         * @details Performs Hermite polynomial interpolation between two specific control
         * points. This method provides more control over which segment to interpolate
         * and is more efficient when the segment is known.
         *
         * @param fromIndex Index of the starting control point for the segment
         * @param t Local parameter value within the segment (typically in range [0, 1])
         * @return Interpolated function value at the given position in the segment
         *
         * @pre fromIndex < getNumPoints() - 1
         * @pre The spline must have at least 2 control points
         * @pre t should be in range [0, 1] for interpolation within the segment
         *
         * @note t=0 returns the value at fromIndex, t=1 returns the value at fromIndex+1
         *
         * @par Complexity
         * O(1) - Constant time interpolation using precomputed coefficients
         */
        T interpolate( u32 fromIndex, T t ) const;

        /**
         * @brief Gets the number of control points in the spline.
         *
         * @return Number of control points currently stored in the spline
         *
         * @par Complexity
         * O(1) - Constant time
         */
        u32 getNumPoints() const;

        /**
         * @brief Clears all control points from the spline.
         *
         * @details Removes all control points, times, and tangent data from the spline.
         * After calling this method, the spline will be empty and require new points
         * to be set before interpolation can be performed.
         *
         * @post getNumPoints() returns 0
         * @post The spline is empty and not ready for interpolation
         *
         * @par Complexity
         * O(1) - Constant time (assuming Array::clear() is O(1))
         */
        void clear();

        /**
         * @brief Controls automatic tangent recalculation.
         *
         * @details Enables or disables automatic tangent calculation when points are modified.
         * When enabled, tangents are recalculated automatically whenever points change.
         * When disabled, you must manually call recalcTangents() after modifying points.
         *
         * @param autoCalc If true, tangents are calculated automatically when points change.
         *                  If false, you must call recalcTangents() manually for correct interpolation.
         *
         * @note Disabling auto-calculation can improve performance when adding many points,
         *       but requires manual tangent recalculation before interpolation.
         *
         * @par Performance
         * Disabling auto-calculation is recommended when setting multiple points in sequence
         *
         * @see recalcTangents()
         */
        void setAutoCalculate( bool autoCalc );

        /**
         * @brief Manually recalculates tangent vectors for all control points.
         *
         * @details Recalculates tangent vectors using the Catmull-Rom approach. This method
         * must be called manually if auto-calculation is disabled and points have been modified.
         * Tangents are essential for smooth Hermite interpolation.
         *
         * @details The tangent calculation uses the formula:
         * tangent[i] = 0.5 * (point[i+1] - point[i-1])
         *
         * Special handling is applied for endpoints:
         * - For open splines: endpoint tangents are parallel to adjacent segments
         * - For closed splines: endpoint tangents use the Catmull-Rom formula with wraparound
         *
         * @pre The spline must have at least 2 control points
         * @post All tangent vectors are updated for correct interpolation
         *
         * @note A spline is considered closed if the first and last points are identical
         *
         * @par Complexity
         * O(n) where n is the number of control points
         *
         * @see setAutoCalculate()
         */
        void recalcTangents();

    protected:
        /// @brief Flag indicating whether tangents should be automatically recalculated
        bool m_autoCalc = true;

        /// @brief Array of parameter values (time points) for each control point
        Array<T> m_times;

        /// @brief Array of 3D control points (only X component is used for 1D splines)
        Array<Vector3<T>> m_points;

        /// @brief Array of 3D tangent vectors for smooth interpolation
        Array<Vector3<T>> m_tangents;

        /// @brief 4x4 coefficient matrix for Hermite polynomial interpolation
        Matrix4<T> m_coeffs = Matrix4<T>::identity();
    };

    /// @brief Type alias for single-precision floating-point linear spline
    using LinearSpline1F = LinearSpline1<f32>;

    /// @brief Type alias for double-precision floating-point linear spline
    using LinearSpline1D = LinearSpline1<f64>;
}  // namespace workphone

#endif  // LinearSpline1
