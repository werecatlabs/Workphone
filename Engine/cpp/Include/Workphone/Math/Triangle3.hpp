#ifndef __WP_TRIANGLE_3D_H_
#define __WP_TRIANGLE_3D_H_

#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Line3.hpp>
#include <Workphone/Math/Plane3.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{
    /**
     * @class Triangle3
     * @brief A template class representing a 3D triangle, useful for collision detection and geometric
     * operations.
     *
     * This class provides various methods to perform operations such as checking if a point is inside
     * the triangle, finding the closest point on the triangle, calculating intersections with lines, and
     * more.
     *
     * @tparam T The type of the coordinates (e.g., float, double).
     */
    template <class T>
    class WPCore_API Triangle3
    {
    public:
        /**
         * @brief Default constructor. Initializes the triangle with all vertices set to zero.
         */
        Triangle3();

        /**
         * @brief Constructor that initializes the triangle with the given vertices.
         *
         * @param v1 The first vertex of the triangle.
         * @param v2 The second vertex of the triangle.
         * @param v3 The third vertex of the triangle.
         */
        Triangle3( Vector3<T> v1, Vector3<T> v2, Vector3<T> v3 );

        /**
         * @brief Checks if the triangle is completely inside a given axis-aligned bounding box (AABB).
         *
         * @param box The bounding box to check against.
         * @return True if the triangle is entirely within the bounding box, false otherwise.
         */
        bool isTotalInsideBox( const AABB3<T> &box ) const;

        /**
         * @brief Equality operator.
         *
         * @param other The triangle to compare with.
         * @return True if all vertices of the triangles are equal, false otherwise.
         */
        bool operator==( const Triangle3<T> &other ) const;

        /**
         * @brief Inequality operator.
         *
         * @param other The triangle to compare with.
         * @return True if any vertex of the triangles is different, false otherwise.
         */
        bool operator!=( const Triangle3<T> &other ) const;

        /**
         * @brief Finds the closest point on the triangle to a given point on the same plane.
         *
         * @param p The point on the same plane as the triangle.
         * @return The closest point on the triangle.
         */
        Vector3<T> closestPointOnTriangle( const Vector3<T> &p ) const;

        /**
         * @brief Checks if a point is inside the triangle.
         *
         * Assumes the point is already on the same plane as the triangle.
         *
         * @param p The point to test.
         * @return True if the point is inside the triangle, false otherwise.
         */
        bool isPointInside( const Vector3<T> &p ) const;

        /**
         * @brief Checks if a point is inside the triangle using a faster algorithm.
         *
         * Assumes the point is already on the same plane as the triangle.
         *
         * @param p The point to test.
         * @return True if the point is inside the triangle, false otherwise.
         */
        bool isPointInsideFast( const Vector3<T> &p ) const;

        /**
         * @brief Checks if two points are on the same side of a line defined by two other points.
         *
         * @param p1 The first point to test.
         * @param p2 The second point to test.
         * @param a The first point defining the line.
         * @param b The second point defining the line.
         * @return True if both points are on the same side of the line, false otherwise.
         */
        bool isOnSameSide( const Vector3<T> &p1, const Vector3<T> &p2, const Vector3<T> &a,
                           const Vector3<T> &b ) const;

        /**
         * @brief Calculates the intersection of the triangle with a limited 3D line segment.
         *
         * @param line The line segment to intersect with.
         * @param outIntersection The intersection point, if any.
         * @return True if there is an intersection, false otherwise.
         */
        bool getIntersectionWithLimitedLine( const Line3<T> &line, Vector3<T> &outIntersection ) const;

        /**
         * @brief Calculates the intersection of the triangle with an infinite 3D line.
         *
         * @param linePoint A point on the line.
         * @param lineVect The direction vector of the line.
         * @param outIntersection The intersection point, if any.
         * @return True if there is an intersection, false otherwise.
         */
        bool getIntersectionWithLine( const Vector3<T> &linePoint, const Vector3<T> &lineVect,
                                      Vector3<T> &outIntersection ) const;

        /**
         * @brief Calculates the intersection of a 3D line with the plane of the triangle.
         *
         * @param linePoint A point on the line.
         * @param lineVect The direction vector of the line.
         * @param outIntersection The intersection point, if any.
         * @return True if there is an intersection, false otherwise.
         */
        bool getIntersectionOfPlaneWithLine( const Vector3<T> &linePoint, const Vector3<T> &lineVect,
                                             Vector3<T> &outIntersection ) const;

        /**
         * @brief Calculates the normal vector of the triangle.
         *
         * @note The returned normal is not normalized.
         *
         * @return The normal vector of the triangle.
         */
        Vector3<T> getNormal() const;

        /**
         * @brief Determines if the triangle is front-facing relative to a given look direction.
         *
         * @param lookDirection The direction to check against.
         * @return True if the triangle is front-facing, false otherwise.
         */
        bool isFrontFacing( const Vector3<T> &lookDirection ) const;

        /**
         * @brief Retrieves the plane on which the triangle lies.
         *
         * @return The plane of the triangle.
         */
        Plane3<T> getPlane() const;

        /**
         * @brief Calculates the area of the triangle.
         *
         * @return The area of the triangle.
         */
        T getArea() const;

        /**
         * @brief Sets the vertices of the triangle.
         *
         * @param a The first vertex.
         * @param b The second vertex.
         * @param c The third vertex.
         */
        void set( const Vector3<T> &a, const Vector3<T> &b, const Vector3<T> &c );

    protected:
        Vector3<T> pointA;  ///< The first vertex of the triangle.
        Vector3<T> pointB;  ///< The second vertex of the triangle.
        Vector3<T> pointC;  ///< The third vertex of the triangle.
    };

    /// Typedef for an integer 3D triangle.
    using Triangle3I = Triangle3<s32>;

    /// Typedef for a 3D triangle with single-precision floating-point coordinates.
    using Triangle3F = Triangle3<f32>;

    /// Typedef for a 3D triangle with double-precision floating-point coordinates.
    using Triangle3D = Triangle3<f64>;

}  // namespace workphone

#endif
