#ifndef __LinearSpline2_h__
#define __LinearSpline2_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{

    /**
     * @brief A 2D linear spline interpolation class using Hermite polynomials.
     *
     * This template class provides smooth interpolation between 2D points using cubic Hermite
     * polynomials. It supports automatic tangent calculation using the Catmull-Rom approach
     * and provides various methods for setting control points.
     *
     * The spline uses a coefficient matrix based on the Hermite polynomial basis:
     * - H₀(t) = 2t³ - 3t² + 1     (basis function for first point)
     * - H₁(t) = -2t³ + 3t²        (basis function for second point)
     * - H₂(t) = t³ - 2t² + t      (basis function for first tangent)
     * - H₃(t) = t³ - t²           (basis function for second tangent)
     *
     * @tparam T The numeric type (typically float or double)
     *
     * @note This class is thread-safe for read operations but not for write operations.
     * @warning Ensure at least 2 points are provided for proper interpolation.
     *
     * @see LinearSpline1, LinearSpline3, LinearSpline4
     *
     * @par Example Usage:
     * @code
     * LinearSpline2<float> spline;
     * Array<Vector2<float>> points = {
     *     Vector2<float>(0.0f, 0.0f),
     *     Vector2<float>(1.0f, 2.0f),
     *     Vector2<float>(2.0f, 1.0f)
     * };
     * spline.setPoints(points);
     * Vector2<float> interpolated = spline.interpolate(0.5f);
     * @endcode
     */
    template <class T>
    class WPCore_API LinearSpline2 : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the Hermite polynomial coefficient matrix for interpolation.
         * The matrix is set up with the standard Hermite basis function coefficients.
         */
        LinearSpline2();

        /**
         * @brief Virtual destructor.
         *
         * Cleans up resources and ensures proper destruction of derived classes.
         */
        ~LinearSpline2() override;

        /**
         * @brief Sets spline points from paired time-value data.
         *
         * This method allows setting both time keys and corresponding 2D points
         * in a single operation using paired data.
         *
         * @param points Array of pairs containing time keys and corresponding Vector2 points
         *
         * @note If auto-calculation is enabled, tangents will be recalculated automatically.
         *
         * @par Complexity: O(n) where n is the number of points
         */
        void setPoints( const Array<Pair<T, Vector2<T>>> &points );

        /**
         * @brief Sets spline points using separate time keys and point arrays.
         *
         * This method provides separate arrays for time keys and 2D points,
         * which must have the same size.
         *
         * @param keys Array of time values (must be in ascending order for proper interpolation)
         * @param points Array of 2D points corresponding to each time key
         *
         * @pre keys.size() == points.size()
         * @note If auto-calculation is enabled, tangents will be recalculated automatically.
         *
         * @par Complexity: O(n) where n is the number of points
         */
        void setPoints( const Array<T> &keys, const Array<Vector2<T>> &points );

        /**
         * @brief Sets spline points using uniform time distribution.
         *
         * Points are distributed uniformly over the parameter range [0, 1].
         * Time keys are automatically generated as evenly spaced values.
         *
         * @param points Array of 2D points to interpolate between
         *
         * @note If auto-calculation is enabled, tangents will be recalculated automatically.
         * @note Time keys will be distributed as: 0, 1/(n-1), 2/(n-1), ..., 1
         *
         * @par Complexity: O(n) where n is the number of points
         */
        void setPoints( const Array<Vector2<T>> &points );

        /**
         * @brief Interpolates a 2D point at the given parameter value.
         *
         * Performs cubic Hermite interpolation across the entire spline.
         * The parameter t is automatically mapped to the appropriate segment.
         *
         * @param t Parameter value for interpolation (typically in range [0, 1])
         * @return Interpolated 2D point at parameter t
         *
         * @note Parameter values outside [0, 1] will be clamped to the valid range.
         * @note Requires at least 2 points to be set.
         *
         * @par Complexity: O(1)
         *
         * @par Mathematical Formula:
         * P(t) = H₀(u)·P₀ + H₁(u)·P₁ + H₂(u)·T₀ + H₃(u)·T₁
         * where u is the local parameter within the segment.
         */
        Vector2<T> interpolate( T t );

        /**
         * @brief Interpolates between two specific control points.
         *
         * Performs cubic Hermite interpolation between the point at fromIndex
         * and the next point, using their associated tangent vectors.
         *
         * @param fromIndex Index of the starting control point (0-based)
         * @param t Local parameter value within the segment [0, 1]
         * @return Interpolated 2D point between the specified control points
         *
         * @pre fromIndex < getNumPoints() - 1
         * @pre t is typically in range [0, 1] for interpolation within the segment
         *
         * @note If fromIndex is the last point, returns that point directly.
         * @note Fast paths are provided for t = 0.0 and t = 1.0.
         *
         * @par Complexity: O(1)
         */
        Vector2<T> interpolate( u32 fromIndex, T t ) const;

        /**
         * @brief Gets the number of control points in the spline.
         *
         * @return The total number of control points currently stored
         *
         * @par Complexity: O(1)
         */
        u32 getNumPoints() const;

        /**
         * @brief Clears all control points from the spline.
         *
         * Removes all points, times, and tangent data, resetting the spline
         * to an empty state.
         *
         * @post getNumPoints() == 0
         *
         * @par Complexity: O(1)
         */
        void clear();

        /**
         * @brief Controls automatic tangent calculation behavior.
         *
         * When enabled, tangents are automatically recalculated using the Catmull-Rom
         * approach whenever points are modified. When disabled, tangents must be
         * manually recalculated by calling recalcTangents().
         *
         * @param autoCalc If true, enables automatic tangent calculation on point changes.
         *                 If false, requires manual tangent recalculation.
         *
         * @note Disabling auto-calculation can improve performance when adding many points.
         * @note Remember to call recalcTangents() after batch updates when auto-calculation is disabled.
         *
         * @see recalcTangents()
         */
        void setAutoCalculate( bool autoCalc );

        /**
         * @brief Manually recalculates all tangent vectors for the spline.
         *
         * Uses the Catmull-Rom approach to calculate tangents:
         * - Interior points: tangent[i] = 0.5 * (point[i+1] - point[i-1])
         * - Endpoint tangents: calculated based on neighboring points
         * - Closed splines: endpoint tangents wrap around to maintain continuity
         *
         * @note This method is automatically called when points are modified if
         *       auto-calculation is enabled.
         * @note Must be called manually after batch updates when auto-calculation is disabled.
         * @note Requires at least 2 points for meaningful tangent calculation.
         *
         * @par Complexity: O(n) where n is the number of points
         *
         * @see setAutoCalculate()
         */
        void recalcTangents();

    protected:
        /** @brief Flag indicating whether tangents should be automatically recalculated. */
        bool m_autoCalc = true;

        /** @brief Array of time/parameter values for each control point. */
        Array<T> m_times;

        /** @brief Array of 3D control points (Z component is typically 0 for 2D splines). */
        Array<Vector3<T>> m_points;

        /** @brief Array of 3D tangent vectors for each control point. */
        Array<Vector3<T>> m_tangents;

        /**
         * @brief Hermite polynomial coefficient matrix.
         *
         * Pre-computed 4x4 matrix containing the coefficients for the cubic Hermite basis functions:
         * [ 2 -2  1  1]
         * [-3  3 -2 -1]
         * [ 0  0  1  0]
         * [ 1  0  0  0]
         */
        Matrix4<T> m_coeffs = Matrix4<T>::identity();
    };

}  // namespace workphone

#endif  // __LinearSpline2_h__
