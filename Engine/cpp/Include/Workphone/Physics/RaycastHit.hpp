#ifndef CRaycastHit_h__
#define CRaycastHit_h__

#include <Workphone/Interface/Physics/IRaycastHit.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Container for information produced by a physics raycast.
         *
         * This class implements IRaycastHit and stores details about a single
         * intersection between a ray and the physics scene (collider, point,
         * normal, texture coordinates, distance, etc.).
         */
        class WPCore_API RaycastHit : public IRaycastHit
        {
        public:
            /**
             * @brief Construct an empty RaycastHit with default values.
             */
            RaycastHit();

            /**
             * @brief Virtual destructor.
             */
            ~RaycastHit() override;

            /**
             * @brief Get the collider shape that was hit.
             * @return Smart pointer to the IPhysicsShape3 that was intersected,
             *         or null if no collider was hit.
             */
            SmartPtr<IPhysicsShape3> getCollider() const override;

            /**
             * @brief Set the collider shape that was hit.
             * @param shape Smart pointer to the collider shape.
             */
            void setCollider( SmartPtr<IPhysicsShape3> shape ) override;

            /**
             * @brief Get the rigid body associated with the hit, if any.
             * @return Smart pointer to the IRigidBody3 instance, or null if none.
             */
            SmartPtr<IRigidBody3> getRigidBody() const override;

            /**
             * @brief Set the rigid body associated with the hit.
             * @param rigidBody Smart pointer to the rigid body.
             */
            void setRigidBody( SmartPtr<IRigidBody3> rigidBody ) override;

            /**
             * @brief Get barycentric coordinates of the hit on a triangle.
             * @return 2D barycentric coordinate vector (u, v). For triangle hits,
             *         the third barycentric coordinate can be derived as (1 - u - v).
             */
            Vector2<real_Num> getBarycentricCoordinate() const override;

            /**
             * @brief Set barycentric coordinates of the hit on a triangle.
             * @param barycentricCoordinate 2D barycentric coordinate (u, v).
             */
            void setBarycentricCoordinate( const Vector2<real_Num> &barycentricCoordinate ) override;

            /**
             * @brief Get the lightmap UV coordinate at the hit point.
             * @return 2D lightmap coordinate.
             */
            Vector2<real_Num> getLightmapCoord() const override;

            /**
             * @brief Set the lightmap UV coordinate at the hit point.
             * @param lightmapCoord 2D lightmap UV coordinate.
             */
            void setLightmapCoord( const Vector2<real_Num> &lightmapCoord ) override;

            /**
             * @brief Get the surface normal at the hit location.
             * @return Normal vector in world space (should be normalized).
             */
            Vector3<real_Num> getNormal() const override;

            /**
             * @brief Set the surface normal at the hit location.
             * @param normal Surface normal vector in world space.
             */
            void setNormal( const Vector3<real_Num> &normal ) override;

            /**
             * @brief Get the world-space position of the hit.
             * @return 3D point where the ray intersected the collider.
             */
            Vector3<real_Num> getPoint() const override;

            /**
             * @brief Set the world-space position of the hit.
             * @param point 3D intersection point.
             */
            void setPoint( const Vector3<real_Num> &point ) override;

            /**
             * @brief Get the primary texture coordinates at the hit.
             * @return 2D texture coordinate (UV).
             */
            Vector2<real_Num> getTextureCoord() const override;

            /**
             * @brief Set the primary texture coordinates at the hit.
             * @param textureCoord 2D texture coordinate (UV).
             */
            void setTextureCoord( const Vector2<real_Num> &textureCoord ) override;

            /**
             * @brief Get the secondary texture coordinates at the hit (if available).
             * @return 2D secondary texture coordinate (UV2).
             */
            Vector2<real_Num> getTextureCoord2() const override;

            /**
             * @brief Set the secondary texture coordinates at the hit.
             * @param textureCoord2 2D secondary texture coordinate (UV2).
             */
            void setTextureCoord2( const Vector2<real_Num> &textureCoord2 ) override;

            /**
             * @brief Get distance from the ray origin to the hit point.
             * @return Distance along the ray to the intersection point.
             */
            real_Num getDistance() const override;

            /**
             * @brief Set distance from the ray origin to the hit point.
             * @param distance Distance value to set.
             */
            void setDistance( real_Num distance ) override;

            /**
             * @brief Get the index of the triangle that was hit (if the collider is a mesh).
             * @return Triangle index in the mesh, or -1 if not applicable.
             */
            s32 getTriangleIndex() const override;

            /**
             * @brief Set the triangle index that was hit.
             * @param triangleIndex Index of the triangle in the mesh.
             */
            void setTriangleIndex( s32 triangleIndex ) override;

            /**
             * @brief Get the collision mask describing which collision layers were involved.
             * @return Collision mask bitfield.
             */
            u32 getCollisionMask() const override;

            /**
             * @brief Set the collision mask for this hit.
             * @param collisionMask Collision mask bitfield.
             */
            void setCollisionMask( u32 collisionMask ) override;

            /**
             * @brief Whether static objects were considered for this raycast.
             * @return True if static objects were included; false otherwise.
             */
            bool getCheckStatic() const override;

            /**
             * @brief Enable or disable checking static objects for this hit.
             * @param checkStatic True to include static objects in the raycast.
             */
            void setCheckStatic( bool checkStatic ) override;

            /**
             * @brief Whether dynamic objects were considered for this raycast.
             * @return True if dynamic objects were included; false otherwise.
             */
            bool getCheckDynamic() const override;

            /**
             * @brief Enable or disable checking dynamic objects for this hit.
             * @param checkDynamic True to include dynamic objects in the raycast.
             */
            void setCheckDynamic( bool checkDynamic ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /** Collider shape that was hit (may be null). */
            SmartPtr<IPhysicsShape3> m_collider;

            /** Rigid body associated with the hit (may be null). */
            SmartPtr<IRigidBody3> m_rigidbody;

            /** Barycentric coordinates on the struck triangle (u, v). */
            Vector2<real_Num> m_barycentricCoordinate;

            /** Lightmap UV coordinates at the hit point. */
            Vector2<real_Num> m_lightmapCoord;

            /** Surface normal at the intersection point (world space). */
            Vector3<real_Num> m_normal;

            /** World-space hit position. */
            Vector3<real_Num> m_point;

            /** Primary texture UV at hit. */
            Vector2<real_Num> m_textureCoord;

            /** Secondary texture UV at hit (UV2). */
            Vector2<real_Num> m_textureCoord2;

            /** Distance from ray origin to hit point. Default = 0.0. */
            real_Num m_distance = static_cast<real_Num>( 0.0 );

            /** Index of the triangle that was hit (mesh hit). Default = 0. */
            s32 m_triangleIndex = 0;

            /** Collision mask bitfield for layers involved in the hit. Default = 0. */
            u32 m_collisionMask = 0;

            /** If true, static geometry was considered by the raycast. Default = true. */
            bool m_checkStatic = true;

            /** If true, dynamic bodies were considered by the raycast. Default = true. */
            bool m_checkDynamic = true;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // CRaycastHit_h__
