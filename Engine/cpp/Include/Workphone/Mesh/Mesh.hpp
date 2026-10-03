#ifndef __FBMesh__H
#define __FBMesh__H

#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{

    /**
     * @brief Implementation of a 3D mesh supporting multiple submeshes, skeletal animation, and vertex
     * animations.
     *
     * The Mesh class provides a concrete implementation of the IMesh interface, representing
     * 3D geometry data that can be rendered by the graphics system. It supports:
     * - Multiple submeshes with different materials
     * - Shared vertex data for memory efficiency
     * - Skeletal animation with bone assignments
     * - Vertex animations and morph targets
     * - Level-of-detail (LOD) support
     * - Bounding volume calculations
     *
     * The mesh can operate in two vertex data modes:
     * - Shared vertex data: All submeshes reference a common vertex buffer
     * - Individual vertex data: Each submesh maintains its own vertex buffer
     *
     * @par Thread Safety
     * This class is thread-safe for read operations when properly locked.
     * Write operations require external synchronization or use of the built-in
     * mutex through lock()/unlock() methods.
     *
     * @par Performance Considerations
     * - Use shared vertex data when submeshes differ only in materials or indices
     * - Call updateAABB() after modifying vertex positions for accurate culling
     * - Consider LOD levels for distance-based performance optimization
     *
     * @see IMesh, ISubMesh, IVertexBuffer, ISkeleton
     * @since 1.0
     * @author Workphone Engine Team
     */
    class WPCore_API Mesh : public IMesh
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates an empty mesh with no submeshes, animations, or skeletal data.
         * The mesh is initialized with default bounding values and shared vertex
         * data disabled.
         *
         * @post The mesh is in a valid, empty state
         * @post getNumSubMeshes() returns 0
         * @post getHasSharedVertexData() returns false
         * @post getBoundingSphereRadius() returns 0.0
         */
        Mesh();

        /**
         * @brief Virtual destructor.
         *
         * Properly releases all mesh resources including submeshes, vertex buffers,
         * animations, skeleton data, and bone assignments. The destructor ensures
         * proper cleanup of all smart pointer references.
         *
         * @note All associated submeshes and animations are automatically released
         */
        ~Mesh() override;

        /**
         * @brief Unloads the mesh and releases its resources.
         *
         * This method is called during the resource cleanup process to release
         * all mesh data including submeshes, vertex buffers, animations, and
         * skeletal information.
         *
         * @param data Optional shared object data for the unload operation.
         *             Can be null for standard unload operations.
         *
         * @post All mesh resources are released
         * @post The mesh is in an unloaded state
         *
         * @see ISharedObject::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /** @copydoc IMesh::addSubMesh */
        void addSubMesh( SmartPtr<ISubMesh> subMesh ) override;

        /** @copydoc IMesh::removeSubMesh */
        void removeSubMesh( SmartPtr<ISubMesh> subMesh ) override;

        /** @copydoc IMesh::removeAllSubMeshes */
        void removeAllSubMeshes() override;

        /** @copydoc IMesh::getSubMeshes */
        Array<SmartPtr<ISubMesh>> getSubMeshes() const override;

        /**
         * @brief Retrieves a submesh by its index.
         *
         * @param index The zero-based index of the submesh to retrieve.
         * @return A smart pointer to the submesh at the specified index.
         *
         * @pre index must be less than getNumSubMeshes()
         * @throw std::out_of_range if index is out of bounds
         *
         * @see getSubMeshes(), getNumSubMeshes()
         */
        SmartPtr<ISubMesh> getSubMesh( u32 index ) const override;

        /**
         * @brief Gets the total number of submeshes in the mesh.
         *
         * @return The number of submeshes currently contained in this mesh.
         *
         * @note This is a constant-time operation
         * @see getSubMesh(), getSubMeshes()
         */
        u32 getNumSubMeshes() const override;

        /** @copydoc IMesh::updateAABB */
        void updateAABB( bool forceSubMeshUpdate = false ) override;

        /** @copydoc IMesh::getAABB */
        AABB3<real_Num> getAABB() const override;

        /** @copydoc IMesh::setAABB */
        void setAABB( const AABB3<real_Num> &aabb ) override;

        /** @copydoc IMesh::setBoundingSphereRadius */
        void setBoundingSphereRadius( real_Num radius ) override;

        /** @copydoc IMesh::getBoundingSphereRadius */
        real_Num getBoundingSphereRadius() const override;

        /** @copydoc IMesh::clone */
        SmartPtr<IMesh> clone() const override;

        /** @copydoc IMesh::compare */
        bool compare( SmartPtr<IMesh> other ) const override;

        /** @copydoc IMesh::getAnimationInterface */
        SmartPtr<IAnimationInterface> getAnimationInterface() const override;

        /** @copydoc IMesh::setAnimationInterface */
        void setAnimationInterface( SmartPtr<IAnimationInterface> animationInterface ) override;

        /** @copydoc IMesh::getHasSharedVertexData */
        bool getHasSharedVertexData() const override;

        /** @copydoc IMesh::setHasSharedVertexData */
        void setHasSharedVertexData( bool hasSharedVertexData ) override;

        /**
         * @brief Retrieves the shared vertex buffer.
         *
         * Returns the vertex buffer that is shared among submeshes when
         * shared vertex data mode is enabled.
         *
         * @return A smart pointer to the shared vertex buffer, or null if
         *         no shared vertex buffer is set.
         *
         * @see setSharedVertexBuffer(), getHasSharedVertexData()
         */
        SmartPtr<IVertexBuffer> getSharedVertexBuffer() const override;

        /**
         * @brief Sets the shared vertex buffer.
         *
         * Assigns a vertex buffer to be shared among submeshes. This is only
         * effective when shared vertex data mode is enabled.
         *
         * @param sharedVertexBuffer A smart pointer to the vertex buffer to set as shared.
         *                          Can be null to remove the current shared buffer.
         *
         * @post getSharedVertexBuffer() returns the specified buffer
         *
         * @see getSharedVertexBuffer(), setHasSharedVertexData()
         */
        void setSharedVertexBuffer( SmartPtr<IVertexBuffer> sharedVertexBuffer ) override;

        /** @copydoc IMesh::hasSkeleton */
        bool hasSkeleton() const override;

        /**
         * @brief Sets whether the mesh has skeletal animation support.
         *
         * Enables or disables skeletal animation capabilities for this mesh.
         * When enabled, the mesh can be deformed using bone transformations.
         *
         * @param hasSkeleton True to enable skeletal animation, false to disable it.
         *
         * @post hasSkeleton() returns the specified value
         *
         * @see hasSkeleton(), setSkeleton()
         */
        void setHasSkeleton( bool hasSkeleton );

        /** @copydoc IMesh::getSkeletonName */
        String getSkeletonName() const override;

        /**
         * @brief Sets the name identifier of the skeleton.
         *
         * Associates a skeleton name with this mesh for skeletal animation.
         * The name can be used to load or reference skeletal data.
         *
         * @param skeletonName The name identifier of the skeleton.
         *
         * @post getSkeletonName() returns the specified name
         *
         * @see getSkeletonName(), setSkeleton()
         */
        void setSkeletonName( const String &skeletonName );

        /**
         * @brief Retrieves the skeleton associated with the mesh.
         *
         * @return A smart pointer to the skeleton object, or null if no
         *         skeleton is associated with this mesh.
         *
         * @see setSkeleton(), hasSkeleton()
         */
        SmartPtr<ISkeleton> getSkeleton() const override;

        /** @copydoc IMesh::setSkeleton */
        void setSkeleton( SmartPtr<ISkeleton> skeleton ) override;

        /** @copydoc IMesh::addBoneAssignment */
        void addBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) override;

        /** @copydoc IMesh::removeBoneAssignment */
        void removeBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) override;

        /** @copydoc IMesh::getBoneAssignments */
        Array<SmartPtr<IVertexBoneAssignment>> getBoneAssignments() const override;

        /** @copydoc IMesh::getNumLodLevels */
        u32 getNumLodLevels() const override;

        /** @copydoc IMesh::isEdgeListBuilt */
        bool isEdgeListBuilt() const override;

        /** @copydoc IMesh::hasVertexAnimation */
        bool hasVertexAnimation() const override;

        /** @copydoc IMesh::getNumAnimations */
        u32 getNumAnimations() const override;

        /** @copydoc IMesh::getAnimation */
        SmartPtr<IAnimation> getAnimation( u32 index ) const override;

        /** @copydoc IMesh::getAnimationByName */
        SmartPtr<IAnimation> getAnimationByName( const String &name ) const override;

        /** @copydoc IMesh::createAnimation */
        SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) override;

        /** @copydoc IMesh::removeAnimation */
        void removeAnimation( SmartPtr<IAnimation> animation ) override;

        /** @copydoc IMesh::removeAllAnimations */
        void removeAllAnimations() override;

        /**
         * @brief Locks the mesh for thread-safe operations.
         *
         * Acquires the internal mutex to ensure thread-safe access to mesh data.
         * This should be called before performing read operations from multiple threads.
         *
         * @post The mesh is locked for the current thread
         *
         * @note Always pair with unlock() to avoid deadlocks
         * @see unlock(), isValid()
         */
        void lock() override;

        /**
         * @brief Attempts to lock the mesh for thread-safe operations.
         *
         * Tries to acquire the internal mutex without blocking. If the mutex is already
         * locked by another thread, this method returns false.
         *
         * @return True if the mesh was successfully locked, false if it is already locked
         *
         * @note Always pair with unlock() if lock() succeeds to avoid deadlocks
         * @see lock(), unlock(), isValid()
         */
        bool try_lock() override;

        /**
         * @brief Unlocks the mesh after thread-safe operations.
         *
         * Releases the internal mutex to allow other threads to access mesh data.
         * This should be called after completing thread-safe operations.
         *
         * @pre The mesh must be locked by the current thread
         * @post The mesh is unlocked
         *
         * @note Always call after lock() to avoid deadlocks
         * @see lock(), isValid()
         */
        void unlock() override;

        /**
         * @brief Checks if the mesh is in a valid state.
         *
         * Validates the internal state of the mesh including submesh consistency,
         * vertex buffer validity, and animation data integrity.
         *
         * @return True if the mesh is valid and can be used for rendering,
         *         false if there are validation errors.
         *
         * @note This method is thread-safe and can be called without locking
         * @see lock(), unlock()
         */
        bool isValid() const override;

        /** @copydoc IMesh::createPose */
        SmartPtr<IMeshPose> createPose( u16 target, const String &name = String() ) override;

        /** @copydoc IMesh::getNumPoses */
        u32 getNumPoses() const override;

        /** @copydoc IMesh::getPose */
        SmartPtr<IMeshPose> getPose( u32 index ) const override;

        /** @copydoc IMesh::getPoseByName */
        SmartPtr<IMeshPose> getPoseByName( const String &name ) const override;

        /** @copydoc IMesh::addPose */
        void addPose( SmartPtr<IMeshPose> pose ) override;

        /** @copydoc IMesh::removePose */
        void removePose( SmartPtr<IMeshPose> pose ) override;

        /** @copydoc IMesh::removeAllPoses */
        void removeAllPoses() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** The skeleton used for bone-based animation. Can be null if no skeletal animation is used. */
        SmartPtr<ISkeleton> m_skeleton;

        /** The vertex buffer shared among submeshes when shared vertex data mode is enabled. */
        SmartPtr<IVertexBuffer> m_sharedVertexBuffer;

        /** The animation interface for vertex-level animations such as morph targets. */
        SmartPtr<IAnimationInterface> m_animationInterface;

        /** The axis-aligned bounding box encompassing all mesh geometry in local space. */
        AABB3<real_Num> m_aabb;

        /** The radius of the bounding sphere used for distance-based calculations. */
        real_Num m_boundingSphereRadius = 0.0f;

        /** Flag indicating whether vertex data is shared between submeshes for memory efficiency. */
        bool m_hasSharedVertexData = false;

        /** Flag indicating whether the mesh supports skeletal animation. */
        bool m_hasSkeleton = false;

        /** The name identifier of the skeleton used for animation. */
        FixedString<128> m_skeletonName;

        /** Collection of submeshes that make up this mesh. Each submesh can have different materials. */
        Array<SmartPtr<ISubMesh>> m_subMeshes;

        /** Collection of animations (vertex animations, morph targets) associated with this mesh. */
        Array<SmartPtr<IAnimation>> m_animations;

        /** Collection of vertex bone assignments for skeletal animation vertex weights. */
        Array<SmartPtr<IVertexBoneAssignment>> m_boneAssignments;

        /** Collection of poses (morph targets) associated with this mesh. */
        Array<SmartPtr<IMeshPose>> m_poses;
    };
}  // namespace workphone

#endif
