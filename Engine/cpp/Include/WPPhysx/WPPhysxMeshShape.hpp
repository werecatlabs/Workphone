#ifndef WPPhysxMeshShape_h__
#define WPPhysxMeshShape_h__

#include <WPPhysx/WPPhysxPrerequisites.hpp>
#include <Workphone/Physics/MeshShape.hpp>
#include <Workphone/Core/Array.hpp>
#include <WPPhysx/WPPhysxShape.hpp>

namespace workphone
{
    namespace physics
    {
        /**
         * @brief PhysX implementation of a MeshShape.
         *
         * This class adapts Workphone's MeshShape interface to PhysX primitives,
         * handling mesh data extraction, cooking (PhysX mesh preparation), shape
         * creation and runtime actor management (static/dynamic).
         *
         * It owns mesh data buffers used to build PhysX geometry and exposes
         * helpers for transforms and caching.
         */
        class PhysxMeshShape : public PhysxShape<MeshShape>
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal members to safe defaults. Does not create
             * PhysX objects — call \c createShape() after providing valid mesh
             * data or resource.
             */
            PhysxMeshShape();

            /**
             * @brief Virtual destructor.
             *
             * Ensures PhysX objects and owned resources are cleaned up when the
             * object is destroyed.
             */
            ~PhysxMeshShape() override;

            /**
             * @copydoc PhysxShape<IMeshShape>::load
             *
             * Loads mesh-related data from a serialized/shared object. This
             * typically populates internal mesh buffers or sets the mesh
             * resource.
             *
             * @param data Shared data object containing serialized mesh/state.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc PhysxShape<IMeshShape>::unload
             *
             * Releases or clears mesh-specific data previously loaded by
             * \c load.
             *
             * @param data Shared data object to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Set the axis-aligned bounding box used for the mesh.
             *
             * The AABB is used to compute transforms, extents and can be used to
             * optimize cooking/caching decisions.
             *
             * @param box Axis-aligned bounding box in local space.
             */
            void setAABB( const AABB3<physics_Num> &box );

            /**
             * @brief Get the current axis-aligned bounding box.
             *
             * @return Current AABB in local space.
             */
            AABB3<physics_Num> getAABB() const;

            /**
             * @brief Convert a local transform into the world transform for the collider.
             *
             * Applies the internal collider transform to the supplied transform
             * to produce the transform used by PhysX for the mesh shape.
             *
             * @param t Input local transform.
             * @return Resulting world-space transform for the collider.
             */
            Transform3<physics_Num> getWorldTransform( const Transform3<physics_Num> &t );

            /**
             * @brief Convert an engine transform to a PhysX PxTransform suitable for mesh geometry.
             *
             * @param t Input transform in engine representation.
             * @return Equivalent PhysX transform for mesh geometry placement.
             */
            physx::PxTransform getMeshTransform( const Transform3<physics_Num> &t );

            /**
             * @brief Get the path used to cache cooked mesh data for a sub-mesh.
             *
             * Returns a filesystem path where the cooked PhysX mesh for the
             * given sub-mesh should be stored or looked up.
             *
             * @param subMeshIndex Index of the sub-mesh.
             * @return Cache file path as a string.
             */
            String getMeshCachePath( u32 subMeshIndex ) const;

            /**
             * @brief Create PhysX mesh geometry from the current mesh data.
             *
             * This will use the stored vertices/indices (and possibly cooked data)
             * to produce PhysX geometry objects ready for shape creation.
             */
            void createMeshGeometry();

            /**
             * @brief Create the PhysX shape and attach it to the appropriate actor.
             *
             * Overrides base class behavior to ensure mesh-specific geometry is
             * used when creating the PhysX shape.
             */
            void createShape() override;

            /**
             * @brief Get the mesh resource currently assigned to this shape.
             *
             * @return Smart pointer to the mesh resource or nullptr if none set.
             */
            SmartPtr<IMeshResource> getMeshResource() const override;

            /**
             * @brief Assign a mesh resource to this mesh shape.
             *
             * Setting a mesh resource does not automatically create PhysX geometry;
             * call \c createMeshGeometry() or \c createShape() after setting.
             *
             * @param meshResource Mesh resource to use.
             */
            void setMeshResource( SmartPtr<IMeshResource> meshResource ) override;

            /**
             * @brief Retrieve the engine mesh instance used by this shape.
             *
             * This may return the original mesh or a processed/cleaned version.
             *
             * @return Smart pointer to the mesh.
             */
            SmartPtr<IMesh> getMesh() const override;

            /**
             * @brief Get a 'clean' (processed/validated) mesh used for collision.
             *
             * Some meshes must be cleaned (duplicate vertices removed, indexing
             * converted) before cooking. This accessor returns that mesh if set.
             *
             * @return Smart pointer to the clean mesh instance.
             */
            SmartPtr<IMesh> getCleanMesh() const override;

            /**
             * @brief Set a processed (clean) mesh to be used for collision cooking.
             *
             * @param cleanMesh Mesh prepared for use by the physics cooker.
             */
            void setCleanMesh( SmartPtr<IMesh> cleanMesh ) override;

            /**
             * @brief Get the original mesh resource path (if available).
             *
             * @return Path to the mesh asset as a string.
             */
            String getMeshPath() const;

            /**
             * @brief Check whether the mesh shape has valid data and PhysX objects.
             *
             * @return True if the mesh shape is ready to be used by the physics scene.
             */
            bool isValid() const override;

            /**
             * @brief Get the PhysX output stream used for custom cooking.
             *
             * This pointer is not owned by the class; the caller is responsible
             * for the lifetime of the pointed object.
             *
             * @return Pointer to a PxOutputStream or nullptr.
             */
            physx::PxOutputStream *getOutputStream() const;

            /**
             * @brief Set an external PhysX output stream to control mesh cooking output.
             *
             * The provided stream will be used when the class writes cooked mesh
             * data to disk or other sinks.
             *
             * @param outputStream Pointer to a PxOutputStream (not owned).
             */
            void setOutputStream( physx::PxOutputStream *outputStream );

            /**
             * @brief Handle incoming state-change messages.
             *
             * Processes messages related to mesh/resource/state updates and returns
             * true if the message was handled and caused internal changes that may
             * require recreation of geometry or properties updates.
             *
             * @param message State message payload.
             * @return True if handled.
             */
            bool handleStateChanged( const SmartPtr<IStateMessage> &message ) override;

            /**
             * @brief Handle a full state object update.
             *
             * @param state New state object to consume.
             * @return True if handled.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Set the index buffer for a sub-mesh.
             *
             * @param subMesh Index of the sub-mesh to update.
             * @param indices Index array (32-bit indices).
             */
            void setIndices( s32 subMesh, const Array<u32> &indices );

            /**
             * @brief Get the number of sub-meshes available in the current mesh.
             *
             * @return Number of sub-meshes.
             */
            s32 getNumSubMeshes() const;

            /**
             * @brief Access mutable shared vertex buffer.
             *
             * @return Reference to the array of shared vertices.
             */
            Array<Vector3<physics_Num>> &getVertices();

            /**
             * @brief Access const shared vertex buffer.
             *
             * @return Const reference to the vertex array.
             */
            const Array<Vector3<physics_Num>> &getVertices() const;

            /**
             * @brief Replace the shared vertex buffer.
             *
             * @param vertices New vertex array to set.
             */
            void setVertices( const Array<Vector3<physics_Num>> &vertices );

            /**
             * @brief Get the index buffer for the given sub-mesh as 32-bit indices.
             *
             * @param subMeshIdx Index of the sub-mesh.
             * @return Reference to the index array for the sub-mesh.
             */
            Array<u32> &getIndices( s32 subMeshIdx );

            /**
             * @brief Get the transform applied to a given sub-mesh.
             *
             * Sub-mesh transforms allow non-uniform placement of sub-meshes within a resource.
             *
             * @param subMeshIdx Sub-mesh index.
             * @return Transform of the sub-mesh in local space.
             */
            Transform3<physics_Num> getSubMeshTransform( s32 subMeshIdx );

            /**
             * @brief Cook the current mesh into PhysX-ready binary format.
             *
             * This uses PhysX cooking APIs to produce optimized collision meshes.
             * The output may be written to disk or cached in memory depending on settings.
             */
            void cookMesh();

            /**
             * @brief Cook the mesh and write cooked data to the provided path.
             *
             * @param path Filesystem path to store cooked mesh data.
             */
            void cookMesh( const String &path );

            /**
             * @brief Convert internal 32-bit indices into multiple 16-bit index buffers when possible.
             *
             * PhysX cooking may require 16-bit indices for certain mesh formats; this
             * helper returns indices split per sub-mesh and narrowed to 16-bit where appropriate.
             *
             * @return A 2D array of 16-bit index buffers (per sub-mesh).
             */
            Array<Array<u16>> getIndicesAs2dArray() const;

            /**
             * @brief Gather child objects (for serialization / editor integration).
             *
             * Overrides base to return mesh-related child objects such as mesh resources.
             *
             * @return Array of shared object pointers representing children.
             */
            Array<SmartPtr<ISharedObject>> getChildObjects() const override;

            /**
             * @brief Retrieve properties describing this mesh shape.
             *
             * Properties include mesh path, extents, transforms and other metadata.
             *
             * @return Smart pointer to a Properties object.
             */
            SmartPtr<Properties> getProperties() const override;

            /**
             * @brief Apply properties to this mesh shape.
             *
             * Used when loading or applying editor changes.
             *
             * @param properties Properties to apply.
             */
            void setProperties( SmartPtr<Properties> properties ) override;

            /**
             * @brief Read raw mesh data from the assigned mesh resource.
             *
             * Populates internal vertex/index buffers and per-submesh transforms.
             */
            void readMeshData();

            /**
             * @brief Build a PhysX-compatible sub-mesh from an index buffer and transform.
             *
             * This takes the provided 16-bit index buffer and per-submesh transform
             * and constructs the vertex/index sets used by the PhysX cooker or geometry builder.
             *
             * @param subMeshIdx Index of the sub-mesh to build.
             * @param indexBuffer 16-bit index buffer for the sub-mesh.
             * @param t Local transform applied to that sub-mesh.
             */
            void buildSubMesh( s32 subMeshIdx, const Array<u16> &indexBuffer,
                               const Transform3<physics_Num> &t );

            /**
             * @brief Create an associated runtime state object used by the engine.
             *
             * Overrides base to include mesh-specific state fields.
             */
            void createStateObject() override;

            /// Optional output stream used during PhysX cooking (not owned).
            physx::PxOutputStream *m_outputStream = nullptr;

            /// PhysX dynamic actor (if the mesh is simulated as dynamic). Not owned here.
            physx::PxRigidDynamic *m_dynamicActor = nullptr;

            /// PhysX static actor (if the mesh is static). Not owned here.
            physx::PxRigidStatic *m_staticActor = nullptr;

            /// Local transform applied to the collider relative to the object transform.
            Transform3<physics_Num> m_colliderTransform;

            /// Extents of the collider AABB (default is unit extents).
            Vector3<physics_Num> m_extents = Vector3<physics_Num>::unit();

            /// Mesh resource backing this shape (asset/resource handle).
            AtomicSmartPtr<IMeshResource> m_meshResource;

            /// Cleaned mesh prepared for collision (atomic smart pointer for thread safety).
            AtomicSmartPtr<IMesh> m_cleanMesh;

            /// Shared vertex buffer (flattened across sub-meshes when applicable).
            Array<Vector3<physics_Num>> m_sharedVertices;

            /// Per-submesh vertex arrays (after applying submesh-specific transforms if needed).
            Array<Array<Vector3<physics_Num>>> m_vertices;

            /// Per-submesh index buffers (32-bit indices).
            Array<Array<u32>> m_indices;

            /// Per-submesh local transforms.
            Array<Transform3<physics_Num>> m_transforms;
        };

    } // end namespace physics
} // namespace workphone

#endif // WPPhysxMeshShape_h__
