#ifndef __IMesh_h__
#define __IMesh_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    /**
     * @brief Interface for a mesh object representing 3D geometry.
     *
     * A mesh is a collection of vertices, edges, and faces that defines the shape of a 3D object.
     * This interface provides methods to manage submeshes, bounding volumes, skeletal animations,
     * and other geometric properties. The mesh can contain multiple submeshes with different materials,
     * shared vertex data for memory efficiency, and skeletal animation data for character animation.
     *
     * @par Threading
     * This interface is not thread-safe. External synchronization is required when accessing
     * from multiple threads.
     *
     * @par Memory Management
     * The mesh uses smart pointers for automatic memory management of submeshes, vertex buffers,
     * and animation data.
     *
     * @see ISubMesh, IVertexBuffer, IAnimation
     * @since 1.0
     * @author Workphone Engine Team
     */
    class WPCore_API IMesh : public ISharedObject
    {
    public:
        IMesh();

        /**
         * @brief Virtual destructor.
         *
         * Properly destroys the mesh object and releases all associated resources including
         * submeshes, vertex buffers, and animation data.
         */
        ~IMesh() override;

        //--------------------------------------------------------------------------
        // Submesh Management
        //--------------------------------------------------------------------------

        /**
         * @brief Adds a submesh to the mesh.
         *
         * Submeshes allow a single mesh to use multiple materials and rendering techniques.
         * Each submesh can have its own vertex buffer or share vertex data with the parent mesh.
         *
         * @param subMesh A smart pointer to the submesh to be added. Must not be null.
         *
         * @pre subMesh must be a valid, non-null smart pointer
         * @post The submesh is added to the mesh's submesh collection
         * @post getNumSubMeshes() returns the increased count
         *
         * @note The mesh takes ownership of the submesh
         * @see removeSubMesh(), getSubMeshes()
         */
        virtual void addSubMesh( SmartPtr<ISubMesh> subMesh ) = 0;

        /**
         * @brief Removes a submesh from the mesh.
         *
         * Removes the specified submesh from the mesh's collection. If the submesh
         * is not found in the collection, this operation has no effect.
         *
         * @param subMesh A smart pointer to the submesh to be removed.
         *
         * @pre subMesh must be a valid smart pointer
         * @post If found, the submesh is removed from the mesh's submesh collection
         * @post getNumSubMeshes() returns the decreased count (if submesh was found)
         *
         * @see addSubMesh(), removeAllSubMeshes()
         */
        virtual void removeSubMesh( SmartPtr<ISubMesh> subMesh ) = 0;

        /**
         * @brief Removes all submeshes from the mesh.
         *
         * Clears the entire submesh collection, effectively making this an empty mesh.
         * All submesh resources are properly released.
         *
         * @post getNumSubMeshes() returns 0
         * @post getSubMeshes() returns an empty array
         *
         * @see removeSubMesh(), addSubMesh()
         */
        virtual void removeAllSubMeshes() = 0;

        /**
         * @brief Retrieves all submeshes in the mesh.
         *
         * Returns a copy of the submesh collection. Modifications to the returned
         * array do not affect the mesh's internal submesh collection.
         *
         * @return An array of smart pointers to the submeshes. May be empty if no submeshes exist.
         *
         * @note The returned array is a copy, not a reference to internal data
         * @see getSubMesh(), getNumSubMeshes()
         */
        virtual Array<SmartPtr<ISubMesh>> getSubMeshes() const = 0;

        /**
         * @brief Retrieves a specific submesh by index.
         *
         * @param index The zero-based index of the submesh to retrieve.
         * @return A smart pointer to the submesh at the specified index.
         *
         * @pre index must be less than getNumSubMeshes()
         * @throw std::out_of_range if index is invalid (implementation dependent)
         *
         * @see getSubMeshes(), getNumSubMeshes()
         */
        virtual SmartPtr<ISubMesh> getSubMesh( u32 index ) const = 0;

        /**
         * @brief Gets the number of submeshes in the mesh.
         *
         * @return The number of submeshes currently contained in this mesh.
         *
         * @note This count includes all submeshes, regardless of their state or validity
         * @see getSubMeshes(), getSubMesh()
         */
        virtual u32 getNumSubMeshes() const = 0;

        //--------------------------------------------------------------------------
        // Bounding Volume Management
        //--------------------------------------------------------------------------

        /**
         * @brief Updates the axis-aligned bounding box (AABB) of the mesh.
         *
         * Recalculates the mesh's bounding box based on current vertex positions.
         * This should be called after modifying vertex data to ensure accurate culling
         * and spatial queries.
         *
         * @param forceSubMeshUpdate If true, forces an update of all submeshes' bounding boxes
         *                          before calculating the mesh AABB. If false, uses cached
         *                          submesh bounds for better performance.
         *
         * @post The mesh's AABB reflects the current geometry bounds
         * @post If forceSubMeshUpdate is true, all submesh AABBs are also updated
         *
         * @note Call this after modifying vertex positions for accurate bounds
         * @see getAABB(), setAABB()
         */
        virtual void updateAABB( bool forceSubMeshUpdate = false ) = 0;

        /**
         * @brief Retrieves the axis-aligned bounding box (AABB) of the mesh.
         *
         * The AABB encompasses all vertices in all submeshes and is used for
         * frustum culling, collision detection, and spatial partitioning.
         *
         * @return The AABB of the mesh in local coordinate space.
         *
         * @note The returned AABB may be outdated if vertex data has been modified
         *       without calling updateAABB()
         * @see updateAABB(), setAABB()
         */
        virtual AABB3<real_Num> getAABB() const = 0;

        /**
         * @brief Sets the axis-aligned bounding box (AABB) of the mesh.
         *
         * Manually overrides the mesh's bounding box. Use with caution as this
         * bypasses automatic bounds calculation and may lead to incorrect culling
         * if the provided AABB doesn't accurately represent the geometry.
         *
         * @param aabb The new AABB to set for this mesh.
         *
         * @post getAABB() returns the specified AABB
         *
         * @warning Setting an incorrect AABB may cause rendering artifacts or
         *          incorrect collision detection
         * @see getAABB(), updateAABB()
         */
        virtual void setAABB( const AABB3<real_Num> &aabb ) = 0;

        /**
         * @brief Sets the radius of the bounding sphere.
         *
         * The bounding sphere radius is used for distance-based operations such as
         * level-of-detail calculations and broad-phase collision detection.
         *
         * @param radius The radius of the bounding sphere in local units. Must be non-negative.
         *
         * @pre radius >= 0
         * @post getBoundingSphereRadius() returns the specified radius
         *
         * @see getBoundingSphereRadius()
         */
        virtual void setBoundingSphereRadius( real_Num radius ) = 0;

        /**
         * @brief Gets the radius of the bounding sphere.
         *
         * @return The radius of the bounding sphere in local units.
         *
         * @see setBoundingSphereRadius()
         */
        virtual real_Num getBoundingSphereRadius() const = 0;

        //--------------------------------------------------------------------------
        // Mesh Operations
        //--------------------------------------------------------------------------

        /**
         * @brief Creates a deep copy of the mesh.
         *
         * Creates a complete copy of the mesh including all submeshes, vertex data,
         * animation data, and properties. The cloned mesh is independent of the original.
         *
         * @return A smart pointer to the cloned mesh with identical geometry and properties.
         *
         * @post The returned mesh is a complete, independent copy
         * @post Modifications to the clone do not affect the original mesh
         *
         * @note This is a potentially expensive operation for large meshes
         * @see compare()
         */
        virtual SmartPtr<IMesh> clone() const = 0;

        /**
         * @brief Compares the mesh with another mesh for equality.
         *
         * Performs a deep comparison of mesh properties including geometry, materials,
         * animation data, and other attributes.
         *
         * @param other A smart pointer to the other mesh to compare with.
         * @return True if the meshes are identical in all aspects, false otherwise.
         *
         * @note This comparison includes vertex data, indices, materials, and animations
         * @note Null pointers are handled gracefully (returns false)
         * @see clone()
         */
        virtual bool compare( SmartPtr<IMesh> other ) const = 0;

        //--------------------------------------------------------------------------
        // Animation Interface
        //--------------------------------------------------------------------------

        /**
         * @brief Retrieves the animation interface for the mesh.
         *
         * The animation interface provides access to mesh-level animation capabilities
         * such as morph targets and vertex animations.
         *
         * @return A smart pointer to the animation interface, or null if no animation
         *         interface is associated with this mesh.
         *
         * @see setAnimationInterface(), hasVertexAnimation()
         */
        virtual SmartPtr<IAnimationInterface> getAnimationInterface() const = 0;

        /**
         * @brief Sets the animation interface for the mesh.
         *
         * Associates an animation interface with this mesh to enable vertex-level
         * animations such as morph targets.
         *
         * @param animationInterface A smart pointer to the animation interface to set.
         *                          Can be null to remove the current interface.
         *
         * @post getAnimationInterface() returns the specified interface
         *
         * @see getAnimationInterface(), hasVertexAnimation()
         */
        virtual void setAnimationInterface( SmartPtr<IAnimationInterface> animationInterface ) = 0;

        //--------------------------------------------------------------------------
        // Shared Vertex Data
        //--------------------------------------------------------------------------

        /**
         * @brief Checks if the mesh has shared vertex data.
         *
         * Shared vertex data allows multiple submeshes to reference the same vertex buffer,
         * reducing memory usage when submeshes differ only in index data or materials.
         *
         * @return True if the mesh uses shared vertex data, false if each submesh
         *         has its own vertex data.
         *
         * @see setHasSharedVertexData(), getSharedVertexBuffer()
         */
        virtual bool getHasSharedVertexData() const = 0;

        /**
         * @brief Sets whether the mesh has shared vertex data.
         *
         * Enables or disables shared vertex data mode. When enabled, submeshes can
         * reference a common vertex buffer instead of maintaining separate vertex data.
         *
         * @param hasSharedVertexData True to enable shared vertex data, false to disable it.
         *
         * @post getHasSharedVertexData() returns the specified value
         *
         * @note Changing this setting may require reorganizing vertex data
         * @see getHasSharedVertexData(), setSharedVertexBuffer()
         */
        virtual void setHasSharedVertexData( bool hasSharedVertexData ) = 0;

        /**
         * @brief Retrieves the shared vertex buffer of the mesh.
         *
         * Returns the vertex buffer that is shared among submeshes when shared
         * vertex data mode is enabled.
         *
         * @return A smart pointer to the shared vertex buffer, or null if no shared
         *         vertex buffer is set or shared vertex data is disabled.
         *
         * @see setSharedVertexBuffer(), getHasSharedVertexData()
         */
        virtual SmartPtr<IVertexBuffer> getSharedVertexBuffer() const = 0;

        /**
         * @brief Sets the shared vertex buffer of the mesh.
         *
         * Assigns a vertex buffer to be shared among submeshes. This is only effective
         * when shared vertex data mode is enabled.
         *
         * @param sharedVertexBuffer A smart pointer to the vertex buffer to set as shared.
         *                          Can be null to remove the current shared buffer.
         *
         * @post getSharedVertexBuffer() returns the specified buffer
         *
         * @note Setting a shared vertex buffer without enabling shared vertex data
         *       has no effect on rendering
         * @see getSharedVertexBuffer(), setHasSharedVertexData()
         */
        virtual void setSharedVertexBuffer( SmartPtr<IVertexBuffer> sharedVertexBuffer ) = 0;

        //--------------------------------------------------------------------------
        // Skeletal Animation
        //--------------------------------------------------------------------------

        /**
         * @brief Checks if the mesh has a skeleton for bone-based animation.
         *
         * Skeletal animation uses a hierarchy of bones to deform the mesh vertices,
         * commonly used for character animation.
         *
         * @return True if the mesh has an associated skeleton, false otherwise.
         *
         * @see getSkeletonName()
         */
        virtual bool hasSkeleton() const = 0;

        /**
         * @brief Retrieves the name of the skeleton used by the mesh.
         *
         * The skeleton name can be used to load or reference the skeletal data
         * for bone-based animation.
         *
         * @return The name identifier of the skeleton, or an empty string if no
         *         skeleton is associated with this mesh.
         *
         * @see hasSkeleton()
         */
        virtual String getSkeletonName() const = 0;

        /**
         * @brief Gets the skeleton associated with the mesh.
         *
         * Returns a smart pointer to the skeleton object that defines the bone hierarchy
         * and animation data for this mesh. If no skeleton is associated, returns null.
         *
         * @return A smart pointer to the skeleton, or null if no skeleton is set.
         *
         * @see setSkeleton(), hasSkeleton()
         */
        virtual SmartPtr<ISkeleton> getSkeleton() const = 0;

        /**
         * @brief Sets the skeleton for the mesh.
         *
         * Associates a skeleton with this mesh to enable skeletal animation.
         * The skeleton must be loaded and valid before setting it.
         *
         * @param skeleton A smart pointer to the skeleton to set. Can be null to
         *                 remove the current skeleton.
         *
         * @post getSkeleton() returns the specified skeleton
         *
         * @see getSkeleton(), hasSkeleton()
         */
        virtual void setSkeleton( SmartPtr<ISkeleton> skeleton ) = 0;

        /**
         * @brief Adds a bone assignment to the mesh.
         *
         * Bone assignments define how vertices are influenced by skeletal bones,
         * specifying weights and bone indices for skinning calculations.
         *
         * @param boneAssignment A smart pointer to the vertex bone assignment to add.
         *
         * @pre boneAssignment must be a valid, non-null smart pointer
         * @post The bone assignment is added to the mesh's bone assignment collection
         *
         * @see removeBoneAssignment()
         */
        virtual void addBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) = 0;

        /**
         * @brief Removes a bone assignment from the mesh.
         *
         * Removes the specified bone assignment from the mesh's collection.
         *
         * @param boneAssignment A smart pointer to the bone assignment to remove.
         *
         * @see addBoneAssignment()
         */
        virtual void removeBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) = 0;

        /**
         * @brief Gets all bone assignments for this mesh.
         *
         * Returns a collection of all bone assignments currently associated with this mesh.
         * These assignments define how bones influence vertices during skeletal animation.
         *
         * @return An array of smart pointers to bone assignments.
         *
         * @note The returned array may be empty if no bone assignments exist.
         * @note Modifying the returned array does not affect the mesh's assignments.
         * @see IVertexBoneAssignment
         */
        virtual Array<SmartPtr<IVertexBoneAssignment>> getBoneAssignments() const = 0;

        //--------------------------------------------------------------------------
        // Level of Detail (LOD)
        //--------------------------------------------------------------------------

        /**
         * @brief Gets the number of level-of-detail (LOD) levels for the mesh.
         *
         * LOD levels provide multiple representations of the same mesh at different
         * levels of detail, allowing for performance optimization based on distance
         * from the camera.
         *
         * @return The number of LOD levels available for this mesh. A return value
         *         of 0 indicates no LOD support, 1 indicates the base mesh only.
         *
         * @note LOD level 0 is typically the highest detail version
         */
        virtual u32 getNumLodLevels() const = 0;

        //--------------------------------------------------------------------------
        // Edge and Vertex Animation
        //--------------------------------------------------------------------------

        /**
         * @brief Checks if the edge list for the mesh has been built.
         *
         * Edge lists are used for advanced rendering techniques such as silhouette
         * detection, shadow volume generation, and wireframe rendering.
         *
         * @return True if the edge list has been computed and is available,
         *         false if it needs to be built or is not supported.
         *
         * @note Edge list generation can be computationally expensive
         */
        virtual bool isEdgeListBuilt() const = 0;

        /**
         * @brief Checks if the mesh has vertex animation data.
         *
         * Vertex animation (also known as morph target animation) deforms the mesh
         * by interpolating between different vertex positions, commonly used for
         * facial animation or simple deformations.
         *
         * @return True if the mesh contains vertex animation data, false otherwise.
         *
         * @see getAnimationInterface(), getNumAnimations()
         */
        virtual bool hasVertexAnimation() const = 0;

        //--------------------------------------------------------------------------
        // Animation Management
        //--------------------------------------------------------------------------

        /**
         * @brief Gets the number of animations associated with the mesh.
         *
         * @return The total number of animations (vertex animations, morph targets, etc.)
         *         available for this mesh.
         *
         * @see getAnimation(), getAnimationByName(), hasVertexAnimation()
         */
        virtual u32 getNumAnimations() const = 0;

        /**
         * @brief Retrieves an animation by index.
         *
         * @param index The zero-based index of the animation to retrieve.
         * @return A smart pointer to the animation at the specified index.
         *
         * @pre index must be less than getNumAnimations()
         * @throw std::out_of_range if index is invalid (implementation dependent)
         *
         * @see getNumAnimations(), getAnimationByName()
         */
        virtual SmartPtr<IAnimation> getAnimation( u32 index ) const = 0;

        /**
         * @brief Retrieves an animation by name.
         *
         * @param name The name of the animation to retrieve.
         * @return A smart pointer to the named animation, or null if no animation
         *         with the specified name exists.
         *
         * @see getAnimation(), createAnimation()
         */
        virtual SmartPtr<IAnimation> getAnimationByName( const String &name ) const = 0;

        /**
         * @brief Creates a new animation for the mesh.
         *
         * Creates and adds a new animation with the specified name and duration.
         * The animation can then be populated with keyframes and tracks.
         *
         * @param name The unique name for the new animation.
         * @param length The duration of the animation in seconds.
         * @return A smart pointer to the newly created animation.
         *
         * @pre name should be unique within this mesh's animation collection
         * @pre length should be positive
         * @post getNumAnimations() returns the increased count
         * @post The animation can be retrieved using getAnimationByName(name)
         *
         * @see removeAnimation(), getAnimationByName()
         */
        virtual SmartPtr<IAnimation> createAnimation( const String &name, f32 length ) = 0;

        /**
         * @brief Removes an animation from the mesh.
         *
         * Removes the specified animation from the mesh's animation collection
         * and releases its resources.
         *
         * @param animation A smart pointer to the animation to remove.
         *
         * @post If found, the animation is removed from the collection
         * @post getNumAnimations() returns the decreased count (if animation was found)
         *
         * @see createAnimation(), removeAllAnimations()
         */
        virtual void removeAnimation( SmartPtr<IAnimation> animation ) = 0;

        /**
         * @brief Removes all animations from the mesh.
         *
         * Clears the entire animation collection and releases all animation resources.
         *
         * @post getNumAnimations() returns 0
         * @post All animation data is released
         *
         * @see removeAnimation(), createAnimation()
         */
        virtual void removeAllAnimations() = 0;

        //--------------------------------------------------------------------------
        // Pose Management
        //--------------------------------------------------------------------------

        /**
         * @brief Creates a new pose for the mesh.
         * @param target The target index for the pose (submesh or vertex buffer index).
         * @param name The name of the pose.
         * @return A smart pointer to the newly created pose.
         */
        virtual SmartPtr<IMeshPose> createPose( u16 target, const String &name = String() ) = 0;

        /**
         * @brief Gets the number of poses in the mesh.
         * @return The number of poses.
         */
        virtual u32 getNumPoses() const = 0;

        /**
         * @brief Gets a pose by index.
         * @param index The index of the pose to retrieve.
         * @return A smart pointer to the pose at the specified index.
         */
        virtual SmartPtr<IMeshPose> getPose( u32 index ) const = 0;

        /**
         * @brief Gets a pose by name.
         * @param name The name of the pose to retrieve.
         * @return A smart pointer to the named pose, or null if not found.
         */
        virtual SmartPtr<IMeshPose> getPoseByName( const String &name ) const = 0;

        /**
         * @brief Adds a pose to the mesh.
         * @param pose The pose to add.
         */
        virtual void addPose( SmartPtr<IMeshPose> pose ) = 0;

        /**
         * @brief Removes a pose from the mesh.
         * @param pose The pose to remove.
         */
        virtual void removePose( SmartPtr<IMeshPose> pose ) = 0;

        /**
         * @brief Removes all poses from the mesh.
         */
        virtual void removeAllPoses() = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IMesh_h__
