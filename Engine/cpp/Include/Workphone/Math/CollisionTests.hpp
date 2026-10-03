#ifndef CollisionTests_h__
#define CollisionTests_h__

#include <Workphone/Math/AABB2.hpp>
#include <Workphone/Math/Cylinder3.hpp>
#include <Workphone/Math/Line2.hpp>
#include <Workphone/Math/Sphere3.hpp>

namespace workphone
{
    /**
     * @brief A template class providing static methods for collision detection between various geometric
     * primitives.
     *
     * The CollisionTests class contains static methods for performing intersection tests between
     * different geometric shapes such as lines, axis-aligned bounding boxes, and spheres.
     * All methods return boolean values indicating whether the tested objects intersect.
     *
     * @tparam T The numeric type used for geometric calculations (typically float or double).
     *
     * @note This class is designed to be used with floating-point types for geometric calculations.
     * @note All methods are static and do not require instantiation of the class.
     *
     * @see AABB2, Line2, Sphere3, AABB3
     *
     * @author Workphone Math Library
     */
    template <class T>
    class WPCore_API CollisionTests
    {
    public:
        /**
         * @brief Tests for intersection between a 2D line segment and a 2D axis-aligned bounding box.
         *
         * This method performs a line-box intersection test using the Cohen-Sutherland-like approach.
         * It projects both the line segment and the bounding box onto the X and Y axes and checks
         * for overlap in both dimensions.
         *
         * @param line The 2D line segment to test for intersection.
         * @param box The 2D axis-aligned bounding box to test against.
         * @return true if the line segment intersects with the bounding box, false otherwise.
         *
         * @note This method handles degenerate cases where the line is vertical or horizontal.
         * @note The intersection test is inclusive of the bounding box edges.
         */
        static bool test( const Line2<T> &line, const AABB2<T> &box );

        /**
         * @brief Tests for intersection between two 3D spheres.
         *
         * This method performs a sphere-sphere intersection test by comparing the distance
         * between sphere centers with the sum of their radii. The test uses squared distances
         * to avoid expensive square root calculations.
         *
         * @param a The first sphere to test.
         * @param b The second sphere to test.
         * @return true if the spheres intersect or touch, false otherwise.
         *
         * @note Spheres are considered intersecting if they touch at exactly one point.
         * @note This method uses squared distance calculations for better performance.
         */
        static bool test( const Sphere3<T> &a, const Sphere3<T> &b );

        /**
         * @brief Tests for intersection between two 3D axis-aligned bounding boxes.
         *
         * This method performs an AABB-AABB intersection test using the separating axis theorem.
         * It checks for overlap in all three dimensions (X, Y, Z). If there is overlap in all
         * three dimensions, the boxes intersect.
         *
         * @param a The first 3D axis-aligned bounding box to test.
         * @param b The second 3D axis-aligned bounding box to test.
         * @return true if the bounding boxes intersect, false otherwise.
         *
         * @note Bounding boxes are considered intersecting if they touch at any point or edge.
         * @note This is an efficient O(1) operation that requires only simple comparisons.
         */
        static bool test( const AABB3<T> &a, const AABB3<T> &b );

        /**
         * @brief Tests for intersection between two 2D axis-aligned bounding boxes.
         *
         * This method performs an AABB-AABB intersection test using the separating axis theorem.
         * It checks for overlap in both dimensions (X, Y). If there is overlap in both dimensions,
         * the boxes intersect.
         *
         * @param a The first 2D axis-aligned bounding box to test.
         * @param b The second 2D axis-aligned bounding box to test.
         * @return true if the bounding boxes intersect, false otherwise.
         *
         * @note Bounding boxes are considered intersecting if they touch at any point or edge.
         * @note This is an efficient O(1) operation that requires only simple comparisons.
         */
        static bool test( const AABB2<T> &a, const AABB2<T> &b );

        /**
         * @brief Tests for intersection between a 3D cylinder and a 3D axis-aligned bounding box.
         *
         * This method performs a cylinder-AABB intersection test by checking if the cylinder's
         * axis intersects with the bounding box and if the cylinder's radius is within the bounds
         * of the AABB in the XY plane.
         *
         * @param a The cylinder to test for intersection.
         * @param b The axis-aligned bounding box to test against.
         * @return true if the cylinder intersects with the bounding box, false otherwise.
         *
         * @note This method assumes that the cylinder is oriented along its axis and does not handle
         *       arbitrary orientations.
         */
        static bool test( const Cylinder3<T> &a, const AABB3<T> &b );
    };

    /**
     * @brief Type alias for CollisionTests using 32-bit floating-point precision.
     *
     * This alias provides a convenient way to use CollisionTests with single-precision
     * floating-point numbers (float/f32).
     *
     * @see CollisionTests, CollisionTestsD
     */
    using CollisionTestsF = CollisionTests<f32>;

    /**
     * @brief Type alias for CollisionTests using 64-bit floating-point precision.
     *
     * This alias provides a convenient way to use CollisionTests with double-precision
     * floating-point numbers (double/f64).
     *
     * @see CollisionTests, CollisionTestsF
     */
    using CollisionTestsD = CollisionTests<f64>;
}  // namespace workphone

#endif  // CollisionTests_h__
