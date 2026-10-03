#ifndef Plane3_h__
#define Plane3_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/WorkphoneEnums.hpp>
#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    /**
     * @brief Template class representing a 3D plane with intersection and classification utilities.
     *
     * The plane is defined by a normal vector and a distance from the origin.
     * Provides methods for intersection tests with lines and other planes, point classification,
     * and plane construction from points or vectors.
     *
     * @tparam T Numeric type (e.g., float, double, int).
     */
    template <class T>
    class WPCore_API Plane3
    {
    public:
        /** @name Constructors */
        ///@{
        /**
         * @brief Default constructor. Initializes the plane with a normal of (0,1,0) and passing through
         * the origin.
         */
        Plane3();

        /**
         * @brief Constructs a plane from a point and a normal vector.
         * @param MPoint A point on the plane.
         * @param Normal The normal vector of the plane.
         */
        Plane3( const Vector3<T> &MPoint, const Vector3<T> &Normal );

        /**
         * @brief Constructs a plane from a point and a normal, using raw values.
         * @param px X coordinate of a point on the plane.
         * @param py Y coordinate of a point on the plane.
         * @param pz Z coordinate of a point on the plane.
         * @param nx X component of the normal vector.
         * @param ny Y component of the normal vector.
         * @param nz Z component of the normal vector.
         */
        Plane3( T px, T py, T pz, T nx, T ny, T nz );

        /**
         * @brief Copy constructor.
         * @param other The plane to copy.
         */
        Plane3( const Plane3<T> &other );

        /**
         * @brief Constructs a plane from three points.
         * @param point1 First point on the plane.
         * @param point2 Second point on the plane.
         * @param point3 Third point on the plane.
         */
        Plane3( const Vector3<T> &point1, const Vector3<T> &point2, const Vector3<T> &point3 );
        ///@}

        /** @name Operators */
        ///@{
        /**
         * @brief Checks if two planes are equal (same normal and distance).
         * @param other The plane to compare.
         * @return True if planes are equal, false otherwise.
         */
        bool operator==( const Plane3<T> &other ) const;

        /**
         * @brief Checks if two planes are not equal.
         * @param other The plane to compare.
         * @return True if planes are not equal, false otherwise.
         */
        bool operator!=( const Plane3<T> &other ) const;
        ///@}

        /** @name Plane Setup */
        ///@{
        /**
         * @brief Sets the plane from a point and a normal vector.
         * @param point A point on the plane.
         * @param nvector The normal vector.
         */
        void setPlane( const Vector3<T> &point, const Vector3<T> &nvector );

        /**
         * @brief Sets the plane from a normal vector and a distance from the origin.
         * @param nvect The normal vector.
         * @param d The distance from the origin.
         */
        void setPlane( const Vector3<T> &nvect, T d );

        /**
         * @brief Sets the plane from three points.
         * @param point1 First point on the plane.
         * @param point2 Second point on the plane.
         * @param point3 Third point on the plane.
         */
        void setPlane( const Vector3<T> &point1, const Vector3<T> &point2, const Vector3<T> &point3 );
        ///@}

        /** @name Intersection and Classification */
        ///@{
        /**
         * @brief Computes the intersection point of the plane with a line.
         * @param linePoint A point on the line.
         * @param lineVect The direction vector of the line.
         * @param outIntersection Output parameter for the intersection point.
         * @return True if intersection exists, false otherwise.
         */
        bool getIntersectionWithLine( const Vector3<T> &linePoint, const Vector3<T> &lineVect,
                                      Vector3<T> &outIntersection ) const;

        /**
         * @brief Computes the relative position (t) of the intersection point along a line segment.
         *
         * Only valid if intersection is known to exist.
         * @param linePoint1 Start point of the line segment.
         * @param linePoint2 End point of the line segment.
         * @return The interpolation factor t in [0,1] where the intersection occurs.
         */
        f32 getKnownIntersectionWithLine( const Vector3<T> &linePoint1,
                                          const Vector3<T> &linePoint2 ) const;

        /**
         * @brief Computes the intersection point of the plane with a line segment.
         * @param linePoint1 Start point of the segment.
         * @param linePoint2 End point of the segment.
         * @param outIntersection Output parameter for the intersection point.
         * @return True if intersection exists within the segment, false otherwise.
         */
        bool getIntersectionWithLimitedLine( const Vector3<T> &linePoint1, const Vector3<T> &linePoint2,
                                             Vector3<T> &outIntersection ) const;

        /**
         * @brief Classifies the relation of a point to the plane.
         * @param point The point to classify.
         * @return ISREL3D_FRONT if in front, ISREL3D_BACK if behind, ISREL3D_PLANAR if on the plane.
         */
        s32 classifyPointRelation( const Vector3<T> &point ) const;

        /**
         * @brief Recalculates the plane's distance from the origin using a new member point.
         * @param MPoint A point on the plane.
         */
        void recalculateD( const Vector3<T> &MPoint );

        /**
         * @brief Returns a member point of the plane (point closest to the origin).
         * @return The member point.
         */
        Vector3<T> getMemberPoint() const;

        /**
         * @brief Checks if this plane intersects with another plane.
         * @param other The other plane.
         * @return True if the planes intersect, false otherwise.
         */
        bool existsInterSection( const Plane3<T> &other ) const;

        /**
         * @brief Computes the intersection line of this plane with another plane.
         * @param other The other plane.
         * @param outLinePoint Output parameter for a point on the intersection line.
         * @param outLineVect Output parameter for the direction of the intersection line.
         * @return True if intersection exists, false otherwise.
         */
        bool getIntersectionWithPlane( const Plane3<T> &other, Vector3<T> &outLinePoint,
                                       Vector3<T> &outLineVect ) const;

        /**
         * @brief Computes the intersection point of this plane with two other planes.
         * @param o1 The first other plane.
         * @param o2 The second other plane.
         * @param outPoint Output parameter for the intersection point.
         * @return True if intersection exists, false otherwise.
         */
        bool getIntersectionWithPlanes( const Plane3<T> &o1, const Plane3<T> &o2,
                                        Vector3<T> &outPoint ) const;

        /**
         * @brief Checks if the plane is front-facing relative to a given direction.
         * @param lookDirection The direction to test.
         * @return True if the plane is front-facing, false if back-facing.
         * @note The normal must be normalized for this to be correct.
         */
        bool isFrontFacing( const Vector3<T> &lookDirection ) const;

        /**
         * @brief Computes the signed distance from the plane to a point.
         * @param point The point to measure from.
         * @return The signed distance.
         * @note The normal must be normalized for this to be correct.
         */
        T getDistance( const Vector3<T> &point ) const;

        /**
         * @brief Classifies the side of an axis-aligned box relative to the plane.
         * @param centre The center of the box.
         * @param halfSize The half-size extents of the box.
         * @return PlaneSide::NEGATIVE_SIDE, PlaneSide::POSITIVE_SIDE, or PlaneSide::BOTH_SIDE.
         */
        PlaneSide getSide( const Vector3<T> &centre, const Vector3<T> &halfSize ) const;
        ///@}

        /** @name Accessors and Mutators */
        ///@{
        /**
         * @brief Gets the normal vector of the plane.
         * @return The normal vector.
         */
        Vector3<T> getNormal() const;

        /**
         * @brief Sets the normal vector of the plane.
         * @param normal The new normal vector.
         */
        void setNormal( const Vector3<T> &normal );

        /**
         * @brief Gets the distance from the origin to the plane.
         * @return The distance.
         */
        T getDistance() const;

        /**
         * @brief Sets the distance from the origin to the plane.
         * @param distance The new distance.
         */
        void setDistance( T distance );
        ///@}

    private:
        Vector3<T> m_normal;  //!< The normal vector of the plane.
        T m_distance;         //!< The distance from the origin to the plane.
    };

    //! Typedef for a 3D plane with integer components.
    using Plane3I = Plane3<s32>;

    //! Typedef for a 3D plane with single-precision floating-point components.
    using Plane3F = Plane3<f32>;

    //! Typedef for a 3D plane with double-precision floating-point components.
    using Plane3D = Plane3<f64>;

}  // namespace workphone

#endif  // Plane3_h__
