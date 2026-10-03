#ifndef __OBBOX3D_H_
#define __OBBOX3D_H_

#include <Workphone/Math/Math.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/Matrix3.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>

namespace workphone
{
    /**
     * @brief Oriented bounding box in 3D space.
     *
     * An OBB is a 3D box that may be arbitrarily rotated around its center. It is stored as a
     * center point, a set of half extents (one per local axis) and an orientation quaternion
     * that maps the box's local axes into world space. The half extents describe a box of size
     * 2 * halfExtents centered on m_center, before the orientation is applied.
     *
     * The implementation is modelled on the OBB found in the Esoterica engine
     * (Code/Base/Math/BoundingVolumes.h/.cpp) but adapted to this engine's templated math types.
     * It only relies on the fully defined Quaternion and Vector3 primitives so that the header is
     * self-contained.
     *
     * @tparam T the type used to represent coordinates (e.g., float or double).
     */
    template <class T>
    class WPCore_API OBB3
    {
    public:
        OBB3();
        OBB3( const OBB3<T> &other );
        OBB3( const Vector3<T> &center, const Vector3<T> &halfExtents );
        OBB3( const Vector3<T> &center, const Vector3<T> &halfExtents,
              const Quaternion<T> &orientation );
        explicit OBB3( const AABB3<T> &aabb );
        OBB3( const AABB3<T> &aabb, const Transform3<T> &transform );
        OBB3( const Vector3<T> *pPoints, u32 numPoints );

        bool operator==( const OBB3<T> &other ) const;
        bool operator!=( const OBB3<T> &other ) const;

        /**
         * @brief Returns the center of the bounding box.
         * @return The center of the bounding box.
         */
        const Vector3<T> &getCenter() const;

        /**
         * @brief Returns the half extents of the bounding box (one per local axis).
         * @return The half extents of the bounding box.
         */
        const Vector3<T> &getHalfExtents() const;

        /**
         * @brief Returns the full extents (size) of the bounding box, i.e. 2 * halfExtents.
         * @return The full extents of the bounding box.
         */
        Vector3<T> getExtent() const;

        /**
         * @brief Returns the orientation of the bounding box.
         * @return The orientation of the bounding box.
         */
        const Quaternion<T> &getOrientation() const;

        /**
         * @brief Returns the i-th world-space axis of the box (a unit vector).
         * @param axisIndex The index of the axis (0 = local X, 1 = local Y, 2 = local Z).
         * @return The world-space direction of the requested local axis.
         */
        Vector3<T> getAxis( u32 axisIndex ) const;

        /**
         * @brief Sets the center of the bounding box.
         * @param center The new center of the bounding box.
         */
        void setCenter( const Vector3<T> &center );

        /**
         * @brief Sets the half extents of the bounding box.
         * @param halfExtents The new half extents of the bounding box.
         */
        void setHalfExtents( const Vector3<T> &halfExtents );

        /**
         * @brief Sets the orientation of the bounding box.
         * @param orientation The new orientation of the bounding box.
         */
        void setOrientation( const Quaternion<T> &orientation );

        /**
         * @brief Translates the bounding box by the supplied vector.
         * @param delta The vector to translate the bounding box by.
         */
        void translate( const Vector3<T> &delta );

        /**
         * @brief Rotates the bounding box by the supplied quaternion.
         * @param deltaRotation The quaternion to rotate the bounding box by.
         */
        void rotate( const Quaternion<T> &deltaRotation );

        /**
         * @brief Returns the 8 corners of the bounding box in world space.
         *
         * The caller must supply an array with room for at least 8 vectors. The corner order
         * matches the reference box used internally (see s_referenceBoxCorners).
         *
         * @param corners Pointer to the array of 8 vectors to fill.
         */
        void getCorners( Vector3<T> *corners ) const;

        /**
         * @brief Returns the axis-aligned bounding box that encloses this oriented box.
         * @return The axis-aligned bounding box enclosing this oriented box.
         */
        AABB3<T> getAABB() const;

        /**
         * @brief Determines if a point is inside this box.
         * @param point The point to check.
         * @return True if the point is inside this box, false otherwise.
         */
        bool containsPoint( const Vector3<T> &point ) const;

        /**
         * @brief Determines if this box intersects another oriented box.
         *
         * The test uses the separating axis theorem, checking the 3 local axes of each box and
         * the 9 axes formed by the cross products of pairs of local axes (15 axes in total).
         *
         * @param other The other box to check.
         * @return True if there is an intersection, false otherwise.
         */
        bool overlaps( const OBB3<T> &other ) const;

        /**
         * @brief Determines if this box intersects an axis-aligned box.
         * @param aabb The axis-aligned box to check.
         * @return True if there is an intersection, false otherwise.
         */
        bool overlaps( const AABB3<T> &aabb ) const;

        /**
         * @brief Applies a Transform3 to this box in place.
         *
         * The box is treated as a local-space box being placed by the transform: the half extents
         * are scaled component-wise, the center is rotated/scaled and translated, and the
         * orientation is composed with the transform's rotation. This keeps the result an OBB.
         *
         * @param transform The transform to apply.
         */
        void applyTransform( const Transform3<T> &transform );

        /**
         * @brief Applies a component-wise scale to the box's half extents.
         * @param scale The scale to apply to each local axis.
         */
        void applyScale( const Vector3<T> &scale );

        /**
         * @brief Returns a copy of this box transformed by the supplied transform.
         * @param transform The transform to apply.
         * @return The transformed box.
         */
        OBB3<T> getTransformed( const Transform3<T> &transform ) const;

        /**
         * @brief Determines whether the box has valid (non-negative) half extents.
         * @return True if the box is valid, false otherwise.
         */
        bool isValid() const;

        /**
         * @brief Resets the box to an empty state centered at the origin with identity orientation.
         */
        void reset();

    private:
        Vector3<T> m_center;          ///< The center of the bounding box.
        Vector3<T> m_halfExtents;     ///< The half extents of the box, one per local axis.
        Quaternion<T> m_orientation;  ///< The orientation of the box (local-to-world rotation).

        // A 2x2x2 box centered around the origin, used to enumerate the 8 corners.
        static const Vector3<T> s_referenceBoxCorners[8];
    };

    // Reference box corners (matches AABB3::getEdges ordering convention):
    //   0: (-1,-1, 1)  1: ( 1,-1, 1)  2: ( 1, 1, 1)  3: (-1, 1, 1)
    //   4: (-1,-1,-1)  5: ( 1,-1,-1)  6: ( 1, 1,-1)  7: (-1, 1,-1)

    /// A typedef for a float 3d oriented bounding box.
    using OBB3F = OBB3<f32>;

    /// A typedef for a double 3d oriented bounding box.
    using OBB3D = OBB3<f64>;

}  // namespace workphone

#endif
