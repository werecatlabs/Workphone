#ifndef LinearSpline4_h__
#define LinearSpline4_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector4.hpp>

namespace workphone
{
    /**
     * @brief A template class for performing linear interpolation on 4D vectors.
     *
     * LinearSpline4 provides functionality for linear interpolation between multiple 4D points.
     * The interpolation parameter t is normalized to the range [0, 1], where 0 corresponds
     * to the first point and 1 corresponds to the last point. The class automatically
     * divides the parameter space into equal segments based on the number of points.
     *
     * @tparam T The numeric type for the vector components (e.g., float, double)
     *
     * @note This class requires at least 2 points to perform interpolation.
     * @note The class inherits from ISharedObject for memory management.
     *
     * @see Vector4
     * @see ISharedObject
     */
    template <class T>
    class WPCore_API LinearSpline4 : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty LinearSpline4 with no control points.
         */
        LinearSpline4();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup when the object is destroyed.
         */
        ~LinearSpline4() override;

        /**
         * @brief Performs linear interpolation at parameter t.
         *
         * Interpolates between the control points using the normalized parameter t.
         * The parameter t is clamped to the valid range and mapped to the appropriate
         * segment between consecutive points.
         *
         * @param t The interpolation parameter in range [0, 1]
         *          - t = 0 returns the first point
         *          - t = 1 returns the last point
         *          - Values between 0 and 1 return interpolated positions
         *
         * @return Vector4<T> The interpolated 4D vector at parameter t
         *
         * @throws std::out_of_range If fewer than 2 points are available for interpolation
         *
         * @note If t exceeds 1.0, the last point is returned
         * @note The interpolation uses equal spacing between all control points
         */
        Vector4<T> interpolate( T t );

        /**
         * @brief Gets the number of control points in the spline.
         *
         * @return u32 The current number of control points
         */
        u32 getNumPoints() const;

        /**
         * @brief Adds a new control point to the spline.
         *
         * Appends a new 4D point to the end of the control points list.
         * The point will be used in subsequent interpolation calculations.
         *
         * @param point The 4D vector to add as a control point
         */
        void addPoint( const Vector4<T> &point );

        /**
         * @brief Gets a copy of all control points.
         *
         * @return Array<Vector4<T>> A copy of the control points array
         */
        Array<Vector4<T>> getPoints() const;

        /**
         * @brief Sets the control points for the spline.
         *
         * Replaces all existing control points with the provided array.
         *
         * @param points The new array of control points to use
         */
        void setPoints( const Array<Vector4<T>> &points );

    protected:
        /**
         * @brief Internal storage for the control points.
         *
         * Contains all the 4D vectors that define the spline's shape.
         */
        Array<Vector4<T>> m_points;
    };

    /**
     * @brief Type alias for LinearSpline4 using 32-bit floating point values.
     */
    using LinearSpline4F = LinearSpline4<f32>;

    /**
     * @brief Type alias for LinearSpline4 using 64-bit floating point values.
     */
    using LinearSpline4D = LinearSpline4<f64>;
}  // namespace workphone

#endif  // LinearSpline4_h__
