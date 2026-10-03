#ifndef IRaycastHit_h__
#define IRaycastHit_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Vector2.hpp>
#include <Workphone/Math/Vector3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface describing the result of a single raycast hit.
         *
         * This interface represents the data produced by a physics raycast:
         * the shape/rigid body hit, contact point and normal, texture and
         * lightmap coordinates, barycentric coordinates (for triangle hits),
         * hit distance, triangle index and collision mask used, and flags
         * indicating whether static/dynamic objects were considered.
         *
         * Implementations should provide storage and accessors for all fields
         * required to fully describe the contact produced by a raycast.
         */
        class WPCore_API IRaycastHit : public ISharedObject
        {
        public:
            /** @brief Virtual destructor. */
            ~IRaycastHit() override;

            /**
             * @brief Get the collider (shape) that was hit by the ray.
             * @return SmartPtr to the shape that was hit, or null if none.
             */
            virtual SmartPtr<IPhysicsShape3> getCollider() const = 0;

            /**
             * @brief Set the collider (shape) that was hit by the ray.
             * @param shape SmartPtr to the shape that was hit.
             */
            virtual void setCollider( SmartPtr<IPhysicsShape3> shape ) = 0;

            /**
             * @brief Get the rigid body associated with the hit (if any).
             * @return SmartPtr to the rigid body, or null if none.
             */
            virtual SmartPtr<IRigidBody3> getRigidBody() const = 0;

            /**
             * @brief Set the rigid body associated with the hit.
             * @param rigidBody SmartPtr to the rigid body that was hit.
             */
            virtual void setRigidBody( SmartPtr<IRigidBody3> rigidBody ) = 0;

            /**
             * @brief Get barycentric coordinates for triangle hits.
             *
             * When the hit occurred on a triangle mesh, the barycentric
             * coordinates describe the location inside the triangle.
             *
             * @return Barycentric coordinates (u,v).
             */
            virtual Vector2<real_Num> getBarycentricCoordinate() const = 0;

            /**
             * @brief Set barycentric coordinates for triangle hits.
             * @param barycentricCoordinate The barycentric coordinates (u,v).
             */
            virtual void setBarycentricCoordinate( const Vector2<real_Num> &barycentricCoordinate ) = 0;

            /**
             * @brief Get the lightmap UV coordinates at the hit point.
             * @return Lightmap UV coordinates.
             */
            virtual Vector2<real_Num> getLightmapCoord() const = 0;

            /**
             * @brief Set the lightmap UV coordinates at the hit point.
             * @param lightmapCoord Lightmap UV coordinates.
             */
            virtual void setLightmapCoord( const Vector2<real_Num> &lightmapCoord ) = 0;

            /**
             * @brief Get the world-space surface normal at the hit point.
             * @return Surface normal vector (normalized expected).
             */
            virtual Vector3<real_Num> getNormal() const = 0;

            /**
             * @brief Set the world-space surface normal at the hit point.
             * @param normal Surface normal vector.
             */
            virtual void setNormal( const Vector3<real_Num> &normal ) = 0;

            /**
             * @brief Get the world-space position of the hit point.
             * @return Hit position.
             */
            virtual Vector3<real_Num> getPoint() const = 0;

            /**
             * @brief Set the world-space position of the hit point.
             * @param point Hit position.
             */
            virtual void setPoint( const Vector3<real_Num> &point ) = 0;

            /**
             * @brief Get the primary texture UV coordinates at the hit point.
             * @return Primary texture UV coordinates.
             */
            virtual Vector2<real_Num> getTextureCoord() const = 0;

            /**
             * @brief Set the primary texture UV coordinates at the hit point.
             * @param textureCoord Primary texture UV coordinates.
             */
            virtual void setTextureCoord( const Vector2<real_Num> &textureCoord ) = 0;

            /**
             * @brief Get the secondary texture UV coordinates at the hit point.
             * @return Secondary texture UV coordinates.
             */
            virtual Vector2<real_Num> getTextureCoord2() const = 0;

            /**
             * @brief Set the secondary texture UV coordinates at the hit point.
             * @param textureCoord2 Secondary texture UV coordinates.
             */
            virtual void setTextureCoord2( const Vector2<real_Num> &textureCoord2 ) = 0;

            /**
             * @brief Get the distance from the ray origin to the hit point.
             * @return Distance along the ray to the hit.
             */
            virtual real_Num getDistance() const = 0;

            /**
             * @brief Set the distance from the ray origin to the hit point.
             * @param distance Distance along the ray to the hit.
             */
            virtual void setDistance( real_Num distance ) = 0;

            /**
             * @brief Get the triangle index of the hit on a triangle mesh.
             * @return Triangle index, or -1 if not applicable.
             */
            virtual s32 getTriangleIndex() const = 0;

            /**
             * @brief Set the triangle index of the hit on a triangle mesh.
             * @param triangleIndex Triangle index where the hit occurred.
             */
            virtual void setTriangleIndex( s32 triangleIndex ) = 0;

            /**
             * @brief Get the collision mask used for the raycast.
             * @return Collision mask bits.
             */
            virtual u32 getCollisionMask() const = 0;

            /**
             * @brief Set the collision mask used for the raycast.
             * @param collisionMask Collision mask bits.
             */
            virtual void setCollisionMask( u32 collisionMask ) = 0;

            /**
             * @brief Whether the raycast should consider static objects.
             * @return True if static objects were checked / considered.
             */
            virtual bool getCheckStatic() const = 0;

            /**
             * @brief Set whether the raycast should consider static objects.
             * @param checkStatic True to include static objects.
             */
            virtual void setCheckStatic( bool checkStatic ) = 0;

            /**
             * @brief Whether the raycast should consider dynamic objects.
             * @return True if dynamic objects were checked / considered.
             */
            virtual bool getCheckDynamic() const = 0;

            /**
             * @brief Set whether the raycast should consider dynamic objects.
             * @param checkDynamic True to include dynamic objects.
             */
            virtual void setCheckDynamic( bool checkDynamic ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IRaycastHit_h__
