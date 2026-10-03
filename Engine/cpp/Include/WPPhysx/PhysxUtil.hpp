#ifndef PhysxUtil_h__
#define PhysxUtil_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Quaternion.hpp>
#include <Workphone/Math/AABB3.hpp>
#include <Workphone/Math/Transform3.hpp>
#include "PxQuat.h"
#include "PxMat44.h"
#include "PxMat33.h"
#include "PxVec3.h"
#include "PxBounds3.h"

namespace workphone
{
    namespace physics
    {
        /**
         * @brief Utility functions for converting between PhysX types and Workphone math types
         *
         * This class centralizes conversions and small helper comparisons used when bridging
         * the Workphone math primitives (Vector3, Quaternion, AABB3, Transform3) with
         * PhysX types (PxVec3, PxQuat, PxBounds3, PxTransform, PxMat44).
         *
         * All conversion functions perform direct component-wise translations between types.
         * Equality helpers perform approximate, component-wise comparisons using an epsilon
         * tolerance to account for floating-point imprecision.
         */
        class PhysxUtil
        {
        public:
            /**
             * @brief Compare two PhysX vectors for approximate equality.
             *
             * Compares each component of the two vectors and returns true if the absolute
             * difference for every component is less than or equal to @p epsilon.
             *
             * @param v1 First PhysX vector.
             * @param v2 Second PhysX vector.
             * @param epsilon Tolerance used for component-wise comparison. Default is 1e-4.
             * @return true if vectors are approximately equal within the provided epsilon,
             *         false otherwise.
             */
            static bool equals( const physx::PxVec3 &v1, const physx::PxVec3 &v2,
                                physics_Num epsilon = 0.0001f );

            /**
             * @brief Compare two PhysX quaternions for approximate equality.
             *
             * Performs a component-wise comparison of the quaternion coefficients using the
             * specified @p epsilon. Note: this function compares components directly and
             * does not attempt to canonicalize quaternions (e.g. q and -q represent the
             * same rotation but may not compare equal component-wise).
             *
             * @param q1 First PhysX quaternion.
             * @param q2 Second PhysX quaternion.
             * @param epsilon Tolerance used for component-wise comparison. Default is 1e-4.
             * @return true if quaternions are approximately equal within the provided epsilon,
             *         false otherwise.
             */
            static bool equals( const physx::PxQuat &q1, const physx::PxQuat &q2,
                                physics_Num epsilon = 0.0001f );

            /**
             * @brief Compare two PhysX transforms for approximate equality.
             *
             * Compares both translation and rotation components of the transforms using the
             * provided @p epsilon for component-wise comparisons.
             *
             * @param t1 First PhysX transform.
             * @param t2 Second PhysX transform.
             * @param epsilon Tolerance used for component-wise comparison. Default is 1e-4.
             * @return true if transforms are approximately equal within the provided epsilon,
             *         false otherwise.
             */
            static bool equals( const physx::PxTransform &t1, const physx::PxTransform &t2,
                                physics_Num epsilon = 0.0001f );

            /**
             * @brief Convert a PhysX vector to a Workphone Vector3.
             *
             * The conversion preserves component ordering (x, y, z).
             *
             * @param vec3 PhysX vector to convert.
             * @return Converted Workphone::Vector3 instance.
             */
            static Vector3<physics_Num> toFB( const physx::PxVec3 &vec3 );

            /**
             * @brief Convert a PhysX quaternion to a Workphone Quaternion.
             *
             * The conversion preserves quaternion components (x, y, z, w).
             *
             * @param q PhysX quaternion to convert.
             * @return Converted Workphone::Quaternion instance.
             */
            static Quaternion<physics_Num> toFB( const physx::PxQuat &q );

            /**
             * @brief Convert a PhysX axis-aligned bounding box to a Workphone AABB3.
             *
             * Maps PhysX bounds (min, max) directly to Workphone's AABB representation.
             *
             * @param b PhysX bounds to convert.
             * @return Converted Workphone::AABB3 instance.
             */
            static AABB3<physics_Num> toFB( const physx::PxBounds3 &b );

            /**
             * @brief Convert a PhysX transform to a Workphone Transform3.
             *
             * Converts both translation and rotation components of the PhysX transform.
             *
             * @param t PhysX transform to convert.
             * @return Converted Workphone::Transform3 instance.
             */
            static Transform3<physics_Num> toFB( const physx::PxTransform &t );

            /**
             * @brief Convert a Workphone Vector3 to a PhysX PxVec3.
             *
             * Preserves component ordering (x, y, z).
             *
             * @param vec3 Workphone vector to convert.
             * @return Converted physx::PxVec3.
             */
            static physx::PxVec3 toPx( const Vector3<physics_Num> &vec3 );

            /**
             * @brief Convert a Workphone Quaternion to a PhysX PxQuat.
             *
             * Preserves quaternion components (x, y, z, w).
             *
             * @param q Workphone quaternion to convert.
             * @return Converted physx::PxQuat.
             */
            static physx::PxQuat toPx( const Quaternion<physics_Num> &q );

            /**
             * @brief Convert a Workphone AABB3 to a PhysX PxBounds3.
             *
             * Maps Workphone AABB min/max to PhysX bounds.
             *
             * @param b Workphone AABB to convert.
             * @return Converted physx::PxBounds3.
             */
            static physx::PxBounds3 toPx( const AABB3<physics_Num> &b );

            /**
             * @brief Convert a Workphone Transform3 to a PhysX PxTransform.
             *
             * Converts both position and orientation.
             *
             * @param t Workphone transform to convert.
             * @return Converted physx::PxTransform.
             */
            static physx::PxTransform toPx( const Transform3<physics_Num> &t );

            /**
             * @brief Build a PhysX 4x4 matrix from a position and orientation.
             *
             * Creates a right-handed 4x4 transformation matrix (physx::PxMat44) that
             * represents the rotation given by @p rot and translation given by @p pos.
             * The returned matrix is suitable for use where PhysX expects a 4x4 transform.
             *
             * @param pos Translation component.
             * @param rot Rotation component as a Workphone quaternion.
             * @return physx::PxMat44 representing the composed transform.
             */
            static physx::PxMat44 toPx( const Vector3<physics_Num>    &pos,
                                        const Quaternion<physics_Num> &rot );
        };
    } // end namespace physics
} // namespace workphone

#endif // PhysxUtil_h__
