#ifndef _WP_RotationalSpline3_H
#define _WP_RotationalSpline3_H

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @brief A 3D rotational spline class for smooth quaternion interpolation.
     *
     * This class interpolates orientations (rotations) along a spline using
     * derivatives of quaternions. It implements the ShoeMake (1987) approach
     * for quaternion spline interpolation, providing smooth rotational motion
     * between control points.
     *
     * The spline uses spherical quadrangle interpolation (SQUAD) to ensure
     * smooth transitions between quaternion control points while maintaining
     * the shortest rotation path when desired.
     *
     * @tparam T The numeric type for quaternion components (typically float or double).
     *
     * @note This class automatically calculates tangents for smooth interpolation
     *       unless explicitly disabled for performance reasons.
     *
     * @see Quaternion, ISharedObject
     */
    template <class T>
    class WPCore_API RotationalSpline3 : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty rotational spline with automatic tangent calculation enabled.
         */
        RotationalSpline3();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~RotationalSpline3() override;

        /**
         * @brief Adds a control point to the end of the spline.
         *
         * The new quaternion control point is appended to the spline.
         * If automatic calculation is enabled, tangents are recalculated.
         *
         * @param p The quaternion control point to add.
         *
         * @note The quaternion should be normalized for proper interpolation.
         *
         * @see setAutoCalculate(), recalcTangents()
         */
        void addPoint( const Quaternion<T> &p );

        /**
         * @brief Gets a specific control point of the spline.
         *
         * @param index The zero-based index of the control point to retrieve.
         *
         * @return A const reference to the quaternion at the specified index.
         *
         * @pre index must be less than getNumPoints().
         *
         * @throw Assertion failure if index is out of bounds in debug builds.
         *
         * @see getNumPoints()
         */
        const Quaternion<T> &getPoint( u16 index ) const;

        /**
         * @brief Gets the number of control points in the spline.
         *
         * @return The total number of quaternion control points.
         *
         * @note At least 2 points are required for interpolation.
         */
        unsigned short getNumPoints() const;

        /**
         * @brief Clears all control points in the spline.
         *
         * Removes all quaternion control points and their associated tangents,
         * resetting the spline to an empty state.
         */
        void clear();

        /**
         * @brief Updates a single control point in the spline.
         *
         * Modifies an existing control point at the specified index.
         * If automatic calculation is enabled, tangents are recalculated.
         *
         * @param index The zero-based index of the point to update.
         * @param value The new quaternion value for the control point.
         *
         * @pre The point at index must already exist in the spline.
         * @pre index must be less than getNumPoints().
         *
         * @throw Assertion failure if index is out of bounds in debug builds.
         *
         * @note The quaternion should be normalized for proper interpolation.
         *
         * @see setAutoCalculate(), recalcTangents()
         */
        void updatePoint( u16 index, const Quaternion<T> &value );

        /**
         * @brief Returns an interpolated quaternion based on a parametric value over the whole spline.
         *
         * Given a parametric value t between 0 and 1 representing the distance along the
         * entire length of the spline, this method returns a smoothly interpolated quaternion.
         *
         * @param t Parametric value in the range [0, 1] where:
         *          - 0.0 corresponds to the first control point
         *          - 1.0 corresponds to the last control point
         * @param useShortestPath If true, rotation takes the shortest possible path.
         *                        If false, rotation may take a longer path to avoid flipping.
         *
         * @return The interpolated quaternion at parameter t.
         *
         * @pre The spline must contain at least 2 control points.
         * @pre t should be in the range [0, 1] for normal interpolation.
         *
         * @note Values of t outside [0, 1] will extrapolate beyond the spline endpoints.
         *
         * @see interpolate(unsigned int, f32, bool)
         */
        Quaternion<T> interpolate( f32 t, bool useShortestPath = true );

        /**
         * @brief Interpolates a single segment of the spline given a parametric value.
         *
         * Performs interpolation between two adjacent control points using the
         * spherical quadrangle (SQUAD) interpolation method.
         *
         * @param fromIndex The index of the starting control point (treated as t=0).
         *                  The point at fromIndex + 1 is treated as t=1.
         * @param t Parametric value in the range [0, 1] for the segment.
         * @param useShortestPath If true, rotation takes the shortest possible path.
         *                        If false, rotation may take a longer path to avoid flipping.
         *
         * @return The interpolated quaternion at parameter t within the specified segment.
         *
         * @pre fromIndex must be less than getNumPoints() - 1.
         * @pre The spline must contain at least 2 control points.
         *
         * @note If fromIndex + 1 equals getNumPoints(), returns the control point at fromIndex.
         * @note For t=0.0, returns the control point at fromIndex.
         * @note For t=1.0, returns the control point at fromIndex + 1.
         *
         * @see interpolate(f32, bool)
         */
        Quaternion<T> interpolate( u32 fromIndex, f32 t, bool useShortestPath = true );

        /**
         * @brief Enables or disables automatic tangent calculation.
         *
         * Controls whether the spline automatically recalculates tangents whenever
         * control points are added or modified. Disabling this can improve performance
         * when adding many points at once.
         *
         * @param autoCalc If true, tangents are automatically calculated whenever a point changes.
         *                 If false, you must manually call recalcTangents() when needed.
         *
         * @note When disabled, remember to call recalcTangents() after finishing
         *       all point modifications to ensure proper interpolation.
         *
         * @see recalcTangents(), addPoint(), updatePoint()
         */
        void setAutoCalculate( bool autoCalc );

        /**
         * @brief Manually recalculates the tangents associated with this spline.
         *
         * Uses the ShoeMake (1987) approach to calculate tangent quaternions at each
         * control point. These tangents are used by the SQUAD interpolation algorithm
         * to ensure smooth transitions between control points.
         *
         * The algorithm handles both open and closed splines:
         * - For open splines, endpoint tangents are calculated using neighboring points
         * - For closed splines (first point equals last point), tangents wrap around
         *
         * @note This method is automatically called when adding or updating points
         *       if automatic calculation is enabled.
         * @note Must be called manually if setAutoCalculate(false) has been used.
         * @note Requires at least 2 control points to calculate tangents.
         *
         * @see setAutoCalculate()
         */
        void recalcTangents( void );

        /**
         * @brief Gets the current shortest route setting.
         *
         * @return true if the spline uses the shortest rotation path by default.
         *         false if it may use longer paths to avoid quaternion flipping.
         *
         * @see setUseShortestRoute()
         */
        bool getUseShortestRoute() const;

        /**
         * @brief Sets whether to use the shortest rotation path by default.
         *
         * This setting affects the default behavior of interpolation methods when
         * the useShortestPath parameter is not explicitly specified.
         *
         * @param useShortestRoute If true, rotations will take the shortest possible path.
         *                         If false, rotations may take longer paths to avoid flipping.
         *
         * @note This setting does not override explicit useShortestPath parameters
         *       in interpolation method calls.
         *
         * @see getUseShortestRoute(), interpolate()
         */
        void setUseShortestRoute( bool useShortestRoute );

    protected:
        /**
         * @brief Array of quaternion control points defining the spline path.
         *
         * These points represent the orientations that the spline passes through.
         * Each quaternion should be normalized for proper interpolation.
         */
        Array<Quaternion<T>> m_points;

        /**
         * @brief Array of tangent quaternions for smooth interpolation.
         *
         * These tangents are calculated using the ShoeMake algorithm and are
         * used by the SQUAD interpolation method to ensure smooth transitions
         * between control points.
         */
        Array<Quaternion<T>> m_tangents;

        /**
         * @brief Flag controlling automatic tangent calculation.
         *
         * When true, tangents are automatically recalculated whenever control
         * points are added or modified. When false, tangents must be manually
         * recalculated using recalcTangents().
         */
        bool m_autoCalc = true;

        /**
         * @brief Default setting for shortest rotation path.
         *
         * Determines whether rotations should take the shortest possible path
         * when the useShortestPath parameter is not explicitly specified in
         * interpolation methods.
         */
        bool m_useShortestRoute = true;
    };

    /**
     * @brief Type alias for RotationalSpline3 using single-precision floating point.
     */
    using RotationalSpline3F = RotationalSpline3<f32>;

    /**
     * @brief Type alias for RotationalSpline3 using double-precision floating point.
     */
    using RotationalSpline3D = RotationalSpline3<f64>;
}  // namespace workphone

#endif
