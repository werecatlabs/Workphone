#ifndef ISubMesh_h__
#define ISubMesh_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/AABB3.hpp>

namespace workphone
{

    /**
     * @class ISubMesh
     * @brief Interface for a submesh, representing a part of a larger mesh.
     *
     * A submesh is a portion of a larger mesh that can have its own material, vertex buffer,
     * index buffer, and other properties. This interface provides methods to manipulate and
     * query submesh-specific data.
     *
     * Submeshes are commonly used in 3D graphics to:
     * - Apply different materials to different parts of a mesh
     * - Optimize rendering by grouping vertices with similar properties
     * - Support skeletal animation with bone assignments
     * - Enable level-of-detail (LOD) rendering
     *
     * @note This interface inherits from ISharedObject, enabling reference counting
     *       and automatic memory management through smart pointers.
     *
     * @see IMesh, IVertexBuffer, IIndexBuffer, IVertexBoneAssignment
     * @since 1.0
     */
    class WPCore_API ISubMesh : public ISharedObject
    {
    public:
        ISubMesh();

        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes when the submesh is destroyed.
         * All associated resources (vertex/index buffers, bone assignments) should be
         * properly released by implementations.
         */
        ~ISubMesh() override;

        /**
         * @brief Sets the material name for this submesh.
         *
         * The material name is used to associate this submesh with a specific material
         * resource that defines its visual appearance (textures, shaders, etc.).
         *
         * @param materialName The name/identifier of the material to use for this submesh.
         *                     Should correspond to a material loaded in the material manager.
         *
         * @note Setting an empty string will remove the material association.
         * @note The material must be loaded separately before rendering.
         */
        virtual void setMaterialName( const String &materialName ) = 0;

        /**
         * @brief Gets the material name used by this submesh.
         *
         * @return The name/identifier of the material currently associated with this submesh.
         *         Returns an empty string if no material is assigned.
         */
        virtual String getMaterialName() const = 0;

        /**
         * @brief Sets the vertex buffer for this submesh.
         *
         * The vertex buffer contains per-vertex data such as positions, normals, texture coordinates,
         * and other vertex attributes. This buffer defines the geometry vertices for this submesh.
         *
         * @param vertexBuffer A smart pointer to the vertex buffer to use. Can be nullptr to remove
         *                     the current vertex buffer association.
         *
         * @note When using shared vertices (getUseSharedVertices() == true), this buffer may
         *       reference the parent mesh's shared vertex data.
         * @note The vertex buffer should remain valid for the lifetime of the submesh.
         */
        virtual void setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer ) = 0;

        /**
         * @brief Gets the vertex buffer used by this submesh.
         *
         * @return A smart pointer to the vertex buffer containing this submesh's vertex data.
         *         May return nullptr if no vertex buffer is currently assigned.
         */
        virtual SmartPtr<IVertexBuffer> getVertexBuffer() const = 0;

        /**
         * @brief Sets the index buffer for this submesh.
         *
         * The index buffer defines how vertices are connected to form triangles, lines, or points.
         * It contains indices into the vertex buffer that specify the primitive topology.
         *
         * @param indexBuffer A smart pointer to the index buffer to use. Can be nullptr for
         *                    non-indexed rendering (direct vertex array rendering).
         *
         * @note The indices should be valid for the current vertex buffer size.
         * @note The index buffer should remain valid for the lifetime of the submesh.
         */
        virtual void setIndexBuffer( SmartPtr<IIndexBuffer> indexBuffer ) = 0;

        /**
         * @brief Gets the index buffer used by this submesh.
         *
         * @return A smart pointer to the index buffer defining this submesh's primitive topology.
         *         May return nullptr if the submesh uses non-indexed rendering.
         */
        virtual SmartPtr<IIndexBuffer> getIndexBuffer() const = 0;

        /**
         * @brief Updates the axis-aligned bounding box (AABB) for this submesh.
         *
         * This method recalculates the AABB based on the current vertex data in the vertex buffer.
         * The AABB encompasses all vertices and is used for frustum culling, collision detection,
         * and spatial queries.
         *
         * @note This operation can be expensive for large vertex buffers as it requires
         *       examining all vertex positions.
         * @note Should be called after modifying vertex positions to maintain accuracy.
         */
        virtual void updateAABB() = 0;

        /**
         * @brief Gets the axis-aligned bounding box (AABB) of this submesh.
         *
         * The AABB represents the smallest box aligned with the coordinate axes that
         * completely contains all vertices of this submesh.
         *
         * @return The AABB of the submesh in local coordinate space.
         *
         * @note The returned AABB may not be current if vertex data has been modified
         *       since the last call to updateAABB().
         */
        virtual AABB3<real_Num> getAABB() const = 0;

        /**
         * @brief Sets the axis-aligned bounding box (AABB) for this submesh.
         *
         * Manually sets the AABB without recalculating from vertex data. This can be
         * useful for optimization or when the AABB is known from external sources.
         *
         * @param aabb The new AABB to set for this submesh.
         *
         * @warning Setting an incorrect AABB may cause rendering or culling issues.
         *          Ensure the provided AABB accurately represents the vertex extents.
         */
        virtual void setAABB( const AABB3<real_Num> &aabb ) = 0;

        /**
         * @brief Creates a deep copy of this submesh.
         *
         * Creates a new submesh instance with identical properties, including:
         * - Material name
         * - Vertex and index buffer references (may be shared or deep copied)
         * - AABB
         * - Render operation type
         * - Bone assignments
         *
         * @return A smart pointer to the newly created submesh clone.
         *
         * @note The exact cloning behavior (shared vs deep copy of buffers) is
         *       implementation-dependent.
         * @note Bone assignments are typically deep copied to maintain independence.
         */
        virtual SmartPtr<ISubMesh> clone() const = 0;

        /**
         * @brief Checks whether this submesh uses shared vertices.
         *
         * When true, this submesh references vertex data from its parent mesh's shared
         * vertex buffer. When false, it uses its own dedicated vertex buffer.
         *
         * @return True if shared vertices are used, false if the submesh has its own vertex data.
         *
         * @note Shared vertices can reduce memory usage but limit per-submesh vertex modifications.
         */
        virtual bool getUseSharedVertices() const = 0;

        /**
         * @brief Sets whether this submesh uses shared vertices.
         *
         * Controls whether this submesh should use the parent mesh's shared vertex buffer
         * or maintain its own separate vertex data.
         *
         * @param useSharedVertices True to use shared vertices from the parent mesh,
         *                          false to use a dedicated vertex buffer for this submesh.
         *
         * @note Changing this setting may require updating vertex buffer references.
         * @note When switching to shared vertices, ensure the parent mesh has valid shared data.
         */
        virtual void setUseSharedVertices( bool useSharedVertices ) = 0;

        /**
         * @brief Compares this submesh with another submesh for equality.
         *
         * Performs a comprehensive comparison including:
         * - Material names
         * - Vertex and index buffer contents
         * - AABB values
         * - Render operation types
         * - Bone assignments
         *
         * @param other A smart pointer to the submesh to compare with.
         *              Can be nullptr, which will return false.
         *
         * @return True if the submeshes are considered equal, false otherwise.
         *
         * @note The comparison may be expensive for large meshes as it may compare buffer contents.
         * @note Exact comparison behavior is implementation-dependent.
         */
        virtual bool compare( SmartPtr<ISubMesh> other ) const = 0;

        /**
         * @brief Gets the render operation type for this submesh.
         *
         * The render operation type defines how the vertex/index data should be interpreted
         * during rendering (e.g., triangles, lines, points, triangle strips).
         *
         * @return The current render operation type for this submesh.
         *
         * @see RenderOperationType
         */
        virtual RenderOperationType getRenderOperationType() const = 0;

        /**
         * @brief Sets the render operation type for this submesh.
         *
         * Specifies how the graphics system should interpret the vertex and index data
         * when rendering this submesh.
         *
         * @param renderOperationType The render operation type to set.
         *                            Common values include triangle lists, triangle strips,
         *                            line lists, and point lists.
         *
         * @note The render operation type must be compatible with the index buffer format.
         * @see RenderOperationType
         */
        virtual void setRenderOperationType( RenderOperationType renderOperationType ) = 0;

        /**
         * @brief Adds a bone assignment to this submesh.
         *
         * Bone assignments define how vertices in this submesh are influenced by bones
         * in a skeletal animation system. Each assignment typically specifies a vertex
         * index, bone index, and weight value.
         *
         * @param boneAssignment A smart pointer to the bone assignment to add.
         *                       Should not be nullptr.
         *
         * @note Multiple bone assignments can affect the same vertex for smooth deformation.
         * @note Bone assignments are essential for skeletal animation and skinning.
         * @see IVertexBoneAssignment
         */
        virtual void addBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) = 0;

        /**
         * @brief Removes a bone assignment from this submesh.
         *
         * Removes the specified bone assignment from this submesh's collection.
         * The assignment is identified by object equality, not by its properties.
         *
         * @param boneAssignment A smart pointer to the bone assignment to remove.
         *                       Should match an existing assignment object.
         *
         * @note If the assignment is not found, this operation has no effect.
         * @note Removing bone assignments may affect skeletal animation for affected vertices.
         * @see IVertexBoneAssignment
         */
        virtual void removeBoneAssignment( SmartPtr<IVertexBoneAssignment> boneAssignment ) = 0;

        /**
         * @brief Gets all bone assignments for this submesh.
         *
         * Returns a collection of all bone assignments currently associated with this submesh.
         * These assignments define how bones influence vertices during skeletal animation.
         *
         * @return An array of smart pointers to bone assignments.
         *
         * @note The returned array may be empty if no bone assignments exist.
         * @note Modifying the returned array does not affect the submesh's assignments.
         * @see IVertexBoneAssignment
         */
        virtual Array<SmartPtr<IVertexBoneAssignment>> getBoneAssignments() const = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // ISubMesh_h__
