#ifndef __AABBOX3D_H_
#define __AABBOX3D_H_

#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Plane3.hpp>
#include <Workphone/Math/Line3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @brief Axis-aligned bounding box in 3D space.
     *
     * @tparam T the type used to represent coordinates (e.g., float or double).
     */
    template <class T>
    class WPCore_API AABB3
    {
    public:
        AABB3();
        AABB3( const AABB3<T> &other );
        AABB3( const Vector3<T> &min, const Vector3<T> &max );
        explicit AABB3( const Vector3<T> &init );
        AABB3( T minx, T miny, T minz, T maxx, T maxy, T maxz );

        bool operator==( const AABB3<T> &other ) const;
        bool operator!=( const AABB3<T> &other ) const;

        /**
         * @brief Adds a point to the bounding box, causing it to grow bigger, if the point is outside of
         * the box.
         *
         * @param p The point to add to this box.
         */
        void merge( const Vector3<T> &p );

        /**
         * @brief Adds another bounding box to this one, causing it to grow bigger if the other box is
         * outside of this one.
         *
         * @param b The other bounding box to add to this one.
         */
        void merge( const AABB3<T> &b );

        /**
         * @brief Resets the bounding box with new min and max values.
         *
         * @param x The minimum X value.
         * @param y The minimum Y value.
         * @param z The minimum Z value.
         */
        void reset( T x, T y, T z );

        /**
         * @brief Resets the bounding box with a new AABB.
         *
         * @param initValue The new AABB.
         */
        void reset( const AABB3<T> &initValue );

        /**
         * @brief Resets the bounding box with a new vector.
         *
         * @param initValue The new vector.
         */
        void reset( const Vector3<T> &initValue );

        /**
         * @brief Adds a point to the bounding box, causing it to grow bigger, if the point is outside of
         * the box.
         *
         * @param x The X coordinate of the point.
         * @param y The Y coordinate of the point.
         * @param z The Z coordinate of the point.
         */
        void merge( T x, T y, T z );

        /**
         * @brief Determines if a point is inside this box.
         * @param p The point to check.
         * @return True if the point is inside this box, false otherwise.
         */
        bool isPointInside( const Vector3<T> &p ) const;

        /**
         * @brief Determines if a point is inside this box, including the box borders.
         * @param p The point to check.
         * @return True if the point is inside this box, false otherwise.
         */
        bool isPointTotalInside( const Vector3<T> &p ) const;

        /**
         * @brief Determines if this box intersects another box.
         * @param other The other box to check.
         * @return True if there is an intersection, false otherwise.
         */
        bool intersects( const AABB3<T> &other ) const;

        /**
         * Checks if the box is fully inside the other box.
         * @param other The other box to check.
         * @return True if this box is fully inside the other box, false otherwise.
         */
        bool isFullInside( const AABB3<T> &other ) const;

        /**
         * Tests if the box intersects with a line.
         *
         * @param line The line to test intersection with.
         * @return True if there is an intersection, false if not.
         */
        bool intersectsWithLine( const Line3<T> &line ) const;

        /**
         * Tests if the box intersects with a line.
         *
         * @param linemiddle The middle point of the line.
         * @param linevect The direction vector of the line.
         * @param halflength Half the length of the line.
         * @return True if there is an intersection, false if not.
         */
        bool intersectsWithLine( const Vector3<T> &linemiddle, const Vector3<T> &linevect,
                                 T halflength ) const;

        /**
         * Classifies a relation with a plane.
         *
         * @param plane The plane to classify relation to.
         * @return ISREL3D_FRONT if the box is in front of the plane,
         *         ISREL3D_BACK if the box is back of the plane, and
         *         ISREL3D_CLIPPED if it is on both sides of the plane.
         */
        s32 classifyPlaneRelation( const Plane3<T> &plane ) const;

        /**
         * Gets the center of the bounding box.
         * @return The center of the bounding box.
         */
        Vector3<T> getCenter() const;

        /**
         * Gets the extent of the box.
         * @return The extent of the box.
         */
        Vector3<T> getExtent() const;

        /**
         * Gets half the size of the box.
         * @return Half the size of the box.
         */
        Vector3<T> getSize() const;

        /**
         * Stores all 8 edges of the box into an array.
         * @param edges Pointer to the array of 8 edges.
         */
        void getEdges( Vector3<T> *edges ) const;

        /**
         * Checks if the box is empty, which means that there is no space within the min and the max
         * edge.
         *
         * @return True if the box is empty, false otherwise.
         */
        bool isEmpty() const;

        /**
         * Repairs the box, if for example MinEdge and MaxEdge are swapped.
         */
        void repair();

        /**
         * Calculates a new interpolated bounding box.
         *
         * @param other The other box to interpolate between.
         * @param d Value between 0.0f and 1.0f.
         * @return A new interpolated bounding box.
         */
        AABB3<T> getInterpolated( const AABB3<T> &other, f32 d ) const;

        /**
         * Transforms the box according to the matrix supplied.
         * @remarks
         * By calling this method you get the axis-aligned box which
         * surrounds the transformed version of this box. Therefore, each
         * corner of the box is transformed by the matrix, and then the
         * extents are mapped back onto the axes to produce another
         * AABB. This is useful when you have a local AABB for an object
         * which is then transformed.
         * @param matrix: The transformation matrix to apply.
         * @return A new AABB representing the transformed bounding box.
         */
        AABB3<T> transform( const Matrix4<T> &matrix ) const;

        /**
         * Sets the extents of the bounding box by specifying minimum and maximum coordinates.
         * @param min: The new minimum coordinates of the box.
         * @param max: The new maximum coordinates of the box.
         */
        void setExtents( const Vector3<T> &min, const Vector3<T> &max );

        /**
         * Sets the bounding box to a null box.
         * A null box is one where the minimum and maximum bounds are equal and set to the maximum value
         * of T.
         */
        void setNull();

        /**
         * Determines whether the bounding box is a null box.
         * A null box is one where the minimum and maximum bounds are equal and set to the maximum value
         * of T.
         * @return Returns true if the bounding box is a null box, and false otherwise.
         */
        bool isNull() const;

        /**
         * Determines whether the bounding box has finite dimensions.
         * @return Returns true if the bounding box has finite dimensions, and false otherwise.
         */
        bool isFinite() const;

        /**
         * Sets the bounding box to an infinite box.
         * An infinite box is one where the minimum and maximum bounds are set to the maximum and minimum
         * values of T, respectively.
         */
        void setInfinite();

        /**
         * Determines whether the bounding box is an infinite box.
         * An infinite box is one where the minimum and maximum bounds are set to the maximum and minimum
         * values of T, respectively.
         * @return Returns true if the bounding box is an infinite box, and false otherwise.
         */
        bool isInfinite() const;

        /**
         * Determines whether the bounding box is a valid box.
         * A valid box is one where the minimum and maximum bounds are not NaN or infinity.
         * @return Returns true if the bounding box is valid, and false otherwise.
         */
        bool isValid() const;

        /**
         * Gets the minimum coordinates of the bounding box.
         * @return The minimum coordinates of the bounding box.
         */
        const Vector3<T> &getMinimum() const;

        /**
         * Sets the minimum coordinates of the bounding box.
         * @param minimum: The new minimum coordinates of the box.
         */
        void setMinimum( const Vector3<T> &minimum );

        /**
         * Gets the maximum coordinates of the bounding box.
         * @return The maximum coordinates of the bounding box.
         */
        const Vector3<T> &getMaximum() const;

        /**
         * Sets the maximum coordinates of the bounding box.
         * @param maximum: The new maximum coordinates of the box.
         */
        void setMaximum( const Vector3<T> &maximum );

        /** Gets the radius of a sphere enclosing the aabb from the outside at center mCenter.
         * @return The radius of the sphere.
         */
        T getRadius() const;

    private:
        ///< The minimum coordinates of the bounding box.
        Vector3<T> m_minimum;

        ///< The maximum coordinates of the bounding box.
        Vector3<T> m_maximum;

        ///< The extent of the bounding box. Can be Null, Finite, or Infinite.
        AabbExtent m_extent = AabbExtent::Null;
    };

    /// A typedef for an integer 3d axis aligned bounding box.
    using AABB3I = AABB3<s32>;

    /// A typedef for an float 3d axis aligned bounding box.
    using AABB3F = AABB3<f32>;

    /// A typedef for an double 3d axis aligned bounding box.
    using AABB3D = AABB3<f64>;

}  // namespace workphone

#endif
