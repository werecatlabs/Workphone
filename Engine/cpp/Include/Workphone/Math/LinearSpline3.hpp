#ifndef LinearSpline3_h__
#define LinearSpline3_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Pair.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix4.hpp>

namespace workphone
{
    /**
     * @brief A 3D linear spline class for smooth interpolation through control points in 3D space.
     *
     * LinearSpline3 provides smooth interpolation between a series of 3D points using Hermite
     * polynomial interpolation. It supports both parameterized and non-parameterized point sets,
     * and can automatically calculate tangent vectors for smooth transitions between points.
     *
     * The spline uses Catmull-Rom tangent calculation for smooth curves and supports both
     * closed and open splines. It can operate in automatic tangent calculation mode or
     * manual mode for performance optimization when adding many points.
     *
     * @tparam T The numeric type for coordinates (typically float or double).
     */
    template <class T>
    class WPCore_API LinearSpline3 : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes the spline with Hermite polynomial coefficients and enables
         * automatic tangent calculation by default.
         */
        LinearSpline3();

        /**
         * @brief Virtual destructor.
         */
        ~LinearSpline3() override;

        /**
         * @brief Adds a single point to the spline.
         * @param point The 3D position of the control point to add.
         */
        void addPoint( const Vector3<T> &point );

        /**
         * @brief Updates an existing point in the spline.
         * @param index The index of the point to update.
         * @param point The new 3D position for the control point.
         */
        void updatePoint( u32 index, const Vector3<T> &point );

        /**
         * @brief Sets the spline points using time-value pairs.
         *
         * This method allows you to specify both the parameter values (times) and
         * the corresponding 3D positions for each control point. This is useful when
         * you need non-uniform spacing between points.
         *
         * @param points Array of pairs where first element is the parameter value
         *               and second is the 3D position.
         */
        void setPoints( const Array<Pair<T, Vector3<T>>> &points );

        /**
         * @brief Sets the spline points using separate arrays for parameters and positions.
         *
         * This method allows you to specify parameter values and 3D positions separately.
         * Both arrays must have the same size.
         *
         * @param keys Array of parameter values for each point.
         * @param points Array of 3D positions corresponding to each parameter value.
         */
        void setPoints( const Array<T> &keys, const Array<Vector3<T>> &points );

        /**
         * @brief Sets the spline points using only 3D positions.
         *
         * When using this method, the parameter values are implicitly distributed
         * uniformly from 0 to 1 across all points.
         *
         * @param points Array of 3D positions for the control points.
         */
        void setPoints( const Array<Vector3<T>> &points );

        /**
         * @brief Interpolates a position on the spline at the given parameter value.
         *
         * This method automatically determines which segment the parameter falls into
         * and performs interpolation within that segment.
         *
         * @param t Parameter value for interpolation. Should be in range [0, 1] for
         *          uniform parameterization, or within the range of your parameter
         *          values for non-uniform parameterization.
         * @return Interpolated 3D position on the spline.
         */
        Vector3<T> interpolate( T t ) const;

        /**
         * @brief Interpolates a position within a specific spline segment.
         *
         * This method performs interpolation between two consecutive control points
         * using the specified segment index and local parameter value.
         *
         * @param fromIndex Index of the starting control point for the segment.
         *                  Must be less than the total number of points minus one.
         * @param t Local parameter value within the segment, typically in range [0, 1].
         * @return Interpolated 3D position within the specified segment.
         */
        Vector3<T> interpolate( u32 fromIndex, T t ) const;

        /**
         * @brief Gets the number of control points in the spline.
         *
         * @return The number of control points currently stored in the spline.
         */
        u32 getNumPoints() const;

        /**
         * @brief Clears all the points in the spline.
         *
         * Removes all control points, parameter values, and computed tangents from the spline.
         * After calling this method, the spline will be empty and ready for new points.
         */
        void clear();

        /**
         * @brief Controls automatic tangent calculation when points are modified.
         *
         * The spline calculates tangents at each point automatically based on the input points.
         * Normally it does this every time a point changes. However, if you have a lot of points
         * to add in one go, you probably don't want to incur this overhead and would prefer to
         * defer the calculation until you are finished setting all the points. You can do this
         * by calling this method with a parameter of 'false'. Just remember to manually call
         * the recalcTangents method when you are done.
         *
         * @param autoCalc If true, tangents are calculated automatically whenever a point changes.
         *                 If false, you must call recalcTangents() to recalculate them manually
         *                 when it best suits your needs.
         */
        void setAutoCalculate( bool autoCalc );

        /**
         * @brief Recalculates the tangents associated with this spline.
         *
         * If you tell the spline not to update on demand by calling setAutoCalculate(false)
         * then you must call this after completing your updates to the spline points.
         *
         * This method uses the Catmull-Rom approach for tangent calculation:
         * - tangent[i] = 0.5 * (point[i+1] - point[i-1])
         * - For endpoints, special handling is applied based on whether the spline is closed or open
         * - For closed splines, the tangent wraps around to use points from the other end
         * - For open splines, endpoint tangents are calculated using adjacent points
         */
        void recalcTangents();

        /**
         * @brief Gets a control point at the specified index.
         *
         * @param index Index of the control point to retrieve. Must be less than getNumPoints().
         * @return The 3D position of the control point at the specified index.
         */
        Vector3<T> getPoint( u32 index ) const;

    protected:
        /** @brief Flag indicating whether tangents should be automatically recalculated when points
         * change. */
        bool m_autoCalc = true;

        /** @brief Array of parameter values for each control point. */
        Array<T> m_times;

        /** @brief Array of 3D positions for each control point. */
        Array<Vector3<T>> m_points;

        /** @brief Array of computed tangent vectors for each control point. */
        Array<Vector3<T>> m_tangents;

        /**
         * @brief Matrix of Hermite polynomial coefficients used for interpolation.
         *
         * This matrix contains the coefficients for the Hermite basis functions:
         * - Row 0: [2, -2, 1, 1]   - Coefficients for t^3 terms
         * - Row 1: [-3, 3, -2, -1] - Coefficients for t^2 terms
         * - Row 2: [0, 0, 1, 0]    - Coefficients for t^1 terms
         * - Row 3: [1, 0, 0, 0]    - Coefficients for t^0 terms
         */
        Matrix4<T> m_coeffs = Matrix4<T>::identity();
    };

    /** @brief Convenience typedef for single-precision floating point splines. */
    using LinearSpline3F = LinearSpline3<f32>;

    /** @brief Convenience typedef for double-precision floating point splines. */
    using LinearSpline3D = LinearSpline3<f64>;
}  // namespace workphone

#endif  // LinearSpline3_h__
