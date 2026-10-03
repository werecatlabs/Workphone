#ifndef __WP_Polygon2__H
#define __WP_Polygon2__H

#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Line2.hpp>
#include <Workphone/Math/Triangle3.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief A 2D polygon.
     */
    template <class T>
    class WPCore_API Polygon2
    {
    public:
        /**
         * @brief Creates a new `Polygon2` with the specified number of points.
         *
         * @param capacity The number of points to reserve.
         */
        explicit Polygon2( u32 capacity = 8 );

        /**
         * @brief Creates a copy of the specified `Polygon2`.
         *
         * @param polygon The polygon to copy.
         */
        Polygon2( const Polygon2 &polygon );

        /**
         * @brief Creates a new `Polygon2` from the specified points.
         *
         * @param points The points of the polygon.
         */
        explicit Polygon2( const Array<Vector2<T>> &points );

        /** Destructor. */
        ~Polygon2();

        /**
         * @brief Determines whether this `Polygon2` is equal to another `Polygon2`.
         *
         * @param other The other `Polygon2`.
         * @return `true` if the two polygons are equal, `false` otherwise.
         */
        bool operator==( const Polygon2<T> &other ) const;

        /**
         * @brief Determines whether this `Polygon2` is not equal to another `Polygon2`.
         *
         * @param other The other `Polygon2`.
         * @return `true` if the two polygons are not equal, `false` otherwise.
         */
        bool operator!=( const Polygon2<T> &other ) const;

        /**
         * @brief Adds a point to this `Polygon2`.
         *
         * @param point The point to add.
         */
        void addPoint( const Vector2<T> &point );

        /**
         * @brief Adds multiple points to this `Polygon2`.
         *
         * @param points The points to add.
         */
        void addPoints( const Array<Vector2<T>> &points );

        /**
         * @brief Sets the points of this `Polygon2`.
         *
         * @param points The new points of the polygon.
         */
        void setPoints( const Array<Vector2<T>> &points );

        /**
         * @brief Removes the point at the specified index from this `Polygon2`.
         *
         * @param i The index of the point to remove.
         * @return `true` if the point was removed, `false` otherwise.
         */
        bool removePoint( u32 i );

        /**
         * @brief Sets this `Polygon2` to a box with the specified minimum and maximum points.
         *
         * @param minPoint The minimum point.
         * @param maxPoint The maximum point.
         */
        void setAsBox( const Vector2<T> &minPoint, const Vector2<T> &maxPoint );

        /**
         * @brief Calculates the center of this `Polygon2`.
         *
         * @return The center of the polygon.
         */
        Vector2<T> getCenter() const;

        /**
         * @brief Calculates a scaled version of this `Polygon2`.
         *
         * @param scale The scale factor.
         * @return The scaled polygon.
         */
        Polygon2 getScalePolygon( T scale ) const;

        /**
         * @brief Calculates a version of this `Polygon2` offset by the specified amount.
         *
         * @param offset The offset.
         * @return The offset polygon.
         */
        Polygon2 getOffsetPolygon( T offset ) const;

        /**
         * @brief Returns an array of all the polygon vertices.
         *
         * @return An array of all the polygon vertices.
         */
        Array<Vector2<T>> getPoints() const;

        /**
         * @brief Returns the vertex at the given index.
         *
         * @param i The index of the vertex.
         * @return The vertex at the given index.
         */
        Vector2<T> getPoint( u32 i ) const;

        /**
         * @brief Returns the number of points.
         */
        u32 getNumPoints() const;

        /**
         * @brief Returns the signed area of the polygon.
         *
         * @return If the area is positive, the vertices are ordered in counter-clockwise direction.
         * If the area is negative, the vertices are ordered in clockwise direction.
         */
        T getArea() const;

        /**
         * @brief Flips the order of the polygon vertices.
         */
        void changePointOrdering();

        /**
         * @brief Subdivides the polygon into smaller ones that have an area in the given range.
         *
         * @param minArea The minimum area allowed for a polygon.
         * @param maxArea The maximum area allowed for a polygon.
         * @param resultPolygons The array to store the resulting polygons.
         */
        void subDivide( T minArea, T maxArea, Array<Polygon2<T>> &resultPolygons );

        /**
         * @brief Subdivides the polygon into smaller ones that have an area in the given range.
         *
         * @param minArea The minimum area allowed for a polygon.
         * @param maxArea The maximum area allowed for a polygon.
         * @param resultPolygons The array to store the resulting polygons.
         */
        void subDivide( const Vector2<T> &minArea, const Vector2<T> &maxArea,
                        Array<Polygon2<T>> &resultPolygons );

        /**
         * @brief Determines if the given point is inside the polygon.
         *
         * @param point The point to test.
         * @return True if the point is inside the polygon, false otherwise.
         */
        bool isPointInside( const Vector2<T> &point ) const;

        /**
         * @brief Determines if the polygon is completely inside the given polygon.
         *
         * @param polygon The polygon to test.
         * @return True if the polygon is completely inside the given polygon, false otherwise.
         */
        bool isInside( const Polygon2 &polygon ) const;

        /**
         * @brief Determines if the polygon intersects the given polygon.
         *
         * @param polygon The polygon to test.
         * @return True if the polygon intersects the given polygon, false otherwise.
         */
        bool intersects( const Polygon2 &polygon ) const;

    private:
        /**
         * @brief Computes the minimum and maximum intervals of the given polygon on the specified axis.
         *
         * @param polygon The polygon to compute the intervals of.
         * @param axis The axis to project the polygon onto.
         * @param min The computed minimum interval.
         * @param max The computed maximum interval.
         */
        void getPolygonIntervals( const Polygon2<T> &polygon, const Vector2<T> &axis, T &min,
                                  T &max ) const;

        /**
         * @brief Computes the distance between two intervals.
         *
         * @param minA The minimum value of the first interval.
         * @param maxA The maximum value of the first interval.
         * @param minB The minimum value of the second interval.
         * @param maxB The maximum value of the second interval.
         *
         * @return The computed distance.
         */
        T intervalDistance( T minA, T maxA, T minB, T maxB ) const;

        /**
         * @brief Flips a tangent vector by 90 degrees.
         *
         * @param edgePoint0 The first point of the edge to flip the tangent of.
         * @param edgePoint1 The second point of the edge to flip the tangent of.
         * @param edgeTangent The tangent vector to flip.
         */
        void flipTangent( const Vector2<T> &edgePoint0, const Vector2<T> &edgePoint1,
                          Vector2<T> &edgeTangent ) const;

        /**
         * @brief Finds the edge of this polygon that is on the opposite side of a given point.
         *
         * @param edgePoint0 The first point of the edge to check.
         * @param edgePoint1 The second point of the edge to check.
         * @param pointOnEdge The point on the edge to check.
         * @param collidingPoint The point on the opposite edge of this polygon that intersects the ray
         * from the given point.
         *
         * @return The index of the edge that is opposite to the given point.
         */
        u32 findOppositeEdge( const Vector2<T> &edgePoint0, const Vector2<T> &edgePoint1,
                              const Vector2<T> &pointOnEdge, Vector2<T> &collidingPoint );

        /**
         * @brief Subdivides this polygon into smaller polygons.
         *
         * @param level The current level of the subdivision.
         * @param minArea The minimum area of a resulting polygon.
         * @param maxArea The maximum area of a resulting polygon.
         * @param resultPolygons The resulting sub-polygons.
         */
        void recursiveSubdivideInLots( s32 level, T minArea, T maxArea,
                                       Array<Polygon2<T>> &resultPolygons );

        /**
         * @brief Subdivides this polygon into smaller polygons.
         *
         * @param level The current level of the subdivision.
         * @param minArea The minimum area of a resulting polygon.
         * @param maxArea The maximum area of a resulting polygon.
         * @param resultPolygons The resulting sub-polygons.
         */
        void recursiveSubdivideInLots( s32 level, const Vector2<T> &minArea, const Vector2<T> &maxArea,
                                       Array<Polygon2<T>> &resultPolygons );

        /**
         * @brief Searches for edges in this polygon that intersect another polygon.
         *
         * @param i1 The index of the first edge that intersects another polygon.
         * @param l1 The location of the intersection along the first edge.
         * @param i2 The index of the second edge that intersects another polygon.
         * @param l2 The location of the intersection along the second edge.
         */
        void polygonSearchEdges( s32 &i1, T &l1, s32 &i2, T &l2 );

        /**
         * @brief Computes the length of the 2D edge specified by the given index.
         * @param i The index of the 2D edge to measure.
         * @return The length of the specified 2D edge.
         */
        T polygonEdgeLength2D( s32 i );

        Array<Vector2<T>> m_points; /**< The 2D points that make up the polygon. */
    };

    /**
     * @brief Typedef for a 2D polygon with single-precision floating point coordinates.
     */
    using Polygon2F = Polygon2<f32>;

    /**
     * @brief Typedef for a 2D polygon with double-precision floating point coordinates.
     */
    using Polygon2D = Polygon2<f64>;

}  // namespace workphone

#endif  // __WP_Polygon2__H
