#ifndef __FBMesh_CollisionMesh_h__
#define __FBMesh_CollisionMesh_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    /**
     * @file CollisionMesh.hpp
     * @brief Collision mesh representation and raycast utilities.
     *
     * Provides a lightweight collision representation of a mesh composed of
     * one or more collision sub-meshes. The class supports loading/unloading
     * as a shared object and provides ray casting helper methods that
     * accumulate intersection hits.
     */
    class WPCore_API CollisionMesh : public ISharedObject
    {
    public:
        /**
         * @brief Data describing a single ray-mesh hit.
         *
         * Contains the distance from the ray origin to the intersection point.
         * Additional contact information can be added if needed in the future.
         */
        class HitData
        {
        public:
            /**
             * @brief Default construct a HitData with zero distance.
             */
            HitData();

            /**
             * @brief Construct a HitData with an initial hit distance.
             * @param distance The distance from the ray origin to the hit point.
             */
            explicit HitData( f32 distance );

            /**
             * @brief Get the hit distance.
             * @return The distance from the ray origin to the intersection.
             */
            f32 getHitDistance() const;

            /**
             * @brief Set the hit distance.
             * @param hitDistance The distance from the ray origin to the intersection.
             */
            void setHitDistance( f32 hitDistance );

        protected:
            /// Distance along the ray to the hit point (world units).
            f32 m_hitDistance = 0.0f;
        };

        /**
         * @brief Default constructor.
         *
         * Creates an empty collision mesh instance. Sub-meshes can be populated
         * by calling load or by other initialization code.
         */
        CollisionMesh();

        /**
         * @brief Construct a collision mesh from a graphics mesh and a transform.
         * @param mesh Smart pointer to the source IMesh used to build collision data.
         * @param transform Transform applied to the mesh vertices when computing collisions.
         *
         * This constructor creates a collision representation for the provided
         * mesh using the supplied transform. The transform is typically the
         * object's world transform.
         */
        CollisionMesh( const SmartPtr<IMesh> &mesh, const Matrix4<real_Num> &transform );

        /**
         * @brief Virtual destructor.
         */
        ~CollisionMesh() override;

        /**
         * @brief Load collision mesh data from a shared object.
         * @copydoc ISharedObject::load
         *
         * Implementations should populate internal sub-mesh structures and any
         * GPU/physics resources required for collision queries.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload and release resources associated with this collision mesh.
         * @copydoc ISharedObject::unload
         *
         * Implementations should release any resources allocated during load.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Perform a ray cast against the collision mesh.
         * @param origin Ray origin in world space.
         * @param dir Ray direction in world space.
         * @param hits Output array that will be populated with intersection distances.
         *             Distances are measured from @p origin along @p dir.
         * @return True if one or more intersections were found, false otherwise.
         *
         * The method appends hit distances to @p hits. The direction vector does
         * not have to be normalized; callers should be aware that returned
         * distances are in the same units as the input vectors.
         */
        bool rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir, Array<f32> &hits );

        /**
         * @brief Perform a ray cast against the collision mesh and return detailed hit data.
         * @param origin Ray origin in world space.
         * @param dir Ray direction in world space.
         * @param hits Output array that will be populated with HitData entries.
         * @return True if one or more intersections were found, false otherwise.
         *
         * Each HitData entry contains at least the hit distance. The array is
         * appended with hits discovered during the query.
         */
        bool rayCast( const Vector3<real_Num> &origin, const Vector3<real_Num> &dir,
                      Array<HitData> &hits );

        WP_CLASS_REGISTER_DECL;

    private:
        /// Per-submesh collision data used to perform fine-grained queries.
        ConcurrentArray<SmartPtr<CollisionSubMesh>> m_subMeshes;
    };
}  // namespace workphone

#endif  // CollisionMesh_h__
