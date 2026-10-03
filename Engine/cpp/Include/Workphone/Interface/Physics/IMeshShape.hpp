#ifndef IPhysicsMesh_h__
#define IPhysicsMesh_h__

#include <Workphone/Interface/Physics/IPhysicsShape3.hpp>

namespace workphone
{
    namespace physics
    {

        /**
         * @brief Interface for a mesh-based physics collision shape.
         *
         * @details
         * Provides accessors for the source mesh and mesh resource used to build
         * the collision representation, as well as a processed ("clean") mesh
         * optimized for collision queries. Implementations are expected to manage
         * lifetime through the project's SmartPtr and to update internal collision
         * data when mesh resources change.
         *
         * @see IPhysicsShape3, IMesh, IMeshResource
         */
        class WPCore_API IMeshShape : public IPhysicsShape3
        {
        public:
            /**
             * @brief Virtual destructor.
             *
             * Ensures derived implementations are properly destroyed through
             * base-class pointers.
             */
            ~IMeshShape() override;

            /**
             * @brief Get the source geometry mesh used to create this shape.
             *
             * @return SmartPtr<IMesh> Smart pointer to the original mesh object.
             *         May be null if no mesh has been assigned.
             *
             * @note Ownership: returned SmartPtr increases the reference count.
             */
            virtual SmartPtr<IMesh> getMesh() const = 0;

            /**
             * @brief Get the mesh resource associated with this shape.
             *
             * @return SmartPtr<IMeshResource> Smart pointer to the mesh resource.
             *         May be null if no resource has been assigned.
             *
             * @note The resource typically contains import/asset metadata and the
             *       raw geometry used to produce the collision mesh.
             */
            virtual SmartPtr<IMeshResource> getMeshResource() const = 0;

            /**
             * @brief Set the mesh resource for this shape.
             *
             * @param meshResource SmartPtr<IMeshResource> Resource containing mesh
             *        data used to build or rebuild the collision representation.
             *        Passing a null SmartPtr clears the resource on this shape.
             *
             * @remarks Implementations may rebuild internal collision structures
             *          when a new resource is set.
             */
            virtual void setMeshResource( SmartPtr<IMeshResource> meshResource ) = 0;

            /**
             * @brief Get the processed/cleaned mesh used for collision.
             *
             * @return SmartPtr<IMesh> Smart pointer to the processed mesh
             *         (e.g. watertight, deduplicated, simplified). May be null
             *         if a processed mesh has not been generated.
             *
             * @note This mesh is expected to be optimized for physics queries,
             *       not for rendering.
             */
            virtual SmartPtr<IMesh> getCleanMesh() const = 0;

            /**
             * @brief Set the processed/cleaned mesh to be used for collision.
             *
             * @param cleanMesh SmartPtr<IMesh> The processed mesh to use. Passing
             *        a null SmartPtr clears any previously set processed mesh.
             *
             * @remarks The implementation may replace or rebuild its internal
             *          collision data based on this mesh.
             */
            virtual void setCleanMesh( SmartPtr<IMesh> cleanMesh ) = 0;

            /**
             * @brief Query whether this mesh shape should be treated as convex.
             *
             * @return true if the shape is convex; false otherwise.
             *
             * @remarks Convex meshes typically allow faster collision detection
             *          and may use different generation paths in the physics backend.
             */
            virtual bool isConvex() const = 0;

            /**
             * @brief Set whether this mesh shape should be treated as convex.
             *
             * @param convex True to treat the mesh as convex. Changing this flag
             *               may trigger rebuilding of internal collision data.
             */
            virtual void setConvex( bool convex ) = 0;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace physics
}  // namespace workphone

#endif  // IPhysicsMesh_h__
