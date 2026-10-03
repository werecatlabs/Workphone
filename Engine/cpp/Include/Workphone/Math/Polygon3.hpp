#ifndef __WP_Polygon3__H
#define __WP_Polygon3__H

#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Triangle3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief A 3D polygon class that stores an array of 3D points.
     *
     * @tparam T The numeric type of the polygon's coordinates.
     */
    template <class T>
    class WPCore_API Polygon3
    {
    public:
        /**
         * @brief Default constructor.
         */
        Polygon3();

        /**
         * @brief Copy constructor.
         *
         * @param polygon The polygon to copy.
         */
        Polygon3( const Polygon3 &polygon );

        /**
         * @brief Constructor that takes an array of 3D points.
         *
         * @param points An array of 3D points.
         */
        explicit Polygon3( const Array<Vector3<T>> &points );

        /**
         * @brief Destructor.
         */
        ~Polygon3();

        /**
         * @brief Adds a 3D point to the polygon.
         *
         * @param point The 3D point to add.
         */
        void addPoint( const Vector3<T> &point );

        /**
         * @brief Adds an array of 3D points to the polygon.
         *
         * @param points An array of 3D points to add.
         */
        void addPoints( const Array<Vector3<T>> &points );

        /**
         * @brief Sets the array of 3D points for the polygon.
         *
         * @param points An array of 3D points.
         */
        void setPoints( const Array<Vector3<T>> &points );

        /**
         * @brief Removes a 3D point from the polygon.
         *
         * @param i The index of the point to remove.
         *
         * @return `true` if the point was removed, `false` if the index was invalid.
         */
        bool removePoint( u32 i );

        /**
         * @brief Gets the 3D point at the specified index.
         *
         * @param i The index of the point to get.
         *
         * @return The 3D point at the specified index.
         */
        Vector3<T> getPoint( u32 i ) const;

        /**
         * @brief Gets the number of points in the polygon.
         *
         * @return The number of points in the polygon.
         */
        u32 getNumPoints() const;

        /**
         * @brief Gets an array of triangles that make up the polygon, projected onto the specified
         * plane.
         *
         * @param plane The plane to project the polygon onto.
         * @param triangles An array to store the resulting triangles.
         *
         * @return `true` if the operation was successful, `false` if the polygon is not a simple
         * polygon.
         */
        bool getTriangles( const Plane3<T> &plane, Array<Triangle3<T>> &triangles );

        /**
         * @brief Gets the center of the polygon.
         *
         * @return The center of the polygon.
         */
        Vector3<T> getCenter() const;

        /**
         * @brief Gets the array of 3D points for the polygon.
         *
         * @return The array of 3D points for the polygon.
         */
        Array<Vector3<T>> getPoints() const;

    private:
        /**
         * @brief Determines if the triangle (u,v,w) can be snipped.
         *
         * @param contour The contour of the polygon.
         * @param u The index of the first vertex of the triangle.
         * @param v The index of the second vertex of the triangle.
         * @param w The index of the third vertex of the triangle.
         * @param n The number of vertices in the contour.
         * @param V An array to store indices of vertices that are inside the triangle.
         * @return true if the triangle can be snipped, false otherwise.
         */
        bool snip( const Array<Vector2<T>> &contour, s32 u, s32 v, s32 w, s32 n, s32 *V );

        /**
         * @brief Calculates the area of a 2D polygon represented by its contour.
         *
         * @param contour The contour of the polygon.
         * @return The area of the polygon.
         */
        T area( const Array<Vector2<T>> &contour );

        /**
         * @brief Determines if a point is inside a triangle specified by its vertices.
         *
         * @param Ax The x-coordinate of the first vertex.
         * @param Ay The y-coordinate of the first vertex.
         * @param Bx The x-coordinate of the second vertex.
         * @param By The y-coordinate of the second vertex.
         * @param Cx The x-coordinate of the third vertex.
         * @param Cy The y-coordinate of the third vertex.
         * @param Px The x-coordinate of the point to check.
         * @param Py The y-coordinate of the point to check.
         * @return true if the point is inside the triangle, false otherwise.
         */
        bool insideTriangle( T Ax, T Ay, T Bx, T By, T Cx, T Cy, T Px, T Py );

        /**
         * @brief The points in the polygon.
         */
        Array<Vector3<T>> m_points;
    };

    /**
     * @brief Typedef for a 3D polygon with single-precision floating point coordinates.
     */
    using Polygon3F = Polygon3<f32>;

    /**
     * @brief Typedef for a 3D polygon with double-precision floating point coordinates.
     */
    using Polygon3D = Polygon3<f64>;

}  // namespace workphone

#endif
