#ifndef __FBSubMesh__H
#define __FBSubMesh__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>

namespace workphone
{
    /**
     * @class SubMesh
     * @brief Implementation of a sub-mesh within a larger mesh structure.
     *
     * A SubMesh represents a portion of a 3D mesh that shares a single material and
     * a set of vertex and index buffers. This allows for efficient rendering of
     * complex objects by breaking them down into smaller, material-specific parts.
     */
    class WPCore_API SubMesh : public ISubMesh
    {
    public:
        /**
         * @brief Constructs a new SubMesh instance.
         */
        SubMesh();

        /**
         * @brief Destroys the SubMesh instance.
         */
        ~SubMesh() override;

        /**
         * @brief Loads the sub-mesh data from a shared object.
         * @param data Smart pointer to the shared object containing the mesh data.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads the sub-mesh data.
         * @param data Smart pointer to the shared object from which the data was loaded.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Sets the name of the material associated with this sub-mesh.
         * @param materialName The name of the material.
         */
        void setMaterialName( const String &materialName ) override;

        /**
         * @brief Retrieves the name of the material associated with this sub-mesh.
         * @return The material name as a String.
         */
        String getMaterialName() const override;

        /**
         * @brief Sets the vertex buffer for the sub-mesh.
         * @param vertexBuffer Smart pointer to the vertex buffer.
         */
        void setVertexBuffer( SmartPtr<IVertexBuffer> vertexBuffer ) override;

        /**
         * @brief Retrieves the vertex buffer used by the sub-mesh.
         * @return Smart pointer to the vertex buffer.
         */
        SmartPtr<IVertexBuffer> getVertexBuffer() const override;

        /**
         * @brief Sets the index buffer for the sub-mesh.
         * @param indexBuffer Smart pointer to the index buffer.
         */
        void setIndexBuffer( SmartPtr<IIndexBuffer> indexBuffer ) override;

        /**
         * @brief Retrieves the index buffer used by the sub-mesh.
         * @return Smart pointer to the index buffer.
         */
        SmartPtr<IIndexBuffer> getIndexBuffer() const override;

        /**
         * @brief Recalculates the Axis-Aligned Bounding Box (AABB) based on the vertex buffer.
         */
        void updateAABB() override;

        /**
         * @brief Retrieves the current AABB of the sub-mesh.
         * @return The AABB of the sub-mesh.
         */
        AABB3<real_Num> getAABB() const override;

        /**
         * @brief Sets the AABB of the sub-mesh.
         * @param aabb The AABB to set.
         */
        void setAABB( const AABB3<real_Num> &aabb ) override;

        /**
         * @brief Creates a deep copy of the sub-mesh.
         * @return A smart pointer to the cloned sub-mesh.
         */
        SmartPtr<ISubMesh> clone() const override;

        /**
         * @brief Checks if the sub-mesh uses shared vertices.
         * @return True if shared vertices are used, otherwise false.
         */
        bool getUseSharedVertices() const override;

        /**
         * @brief Sets whether the sub-mesh should use shared vertices.
         * @param useSharedVertices True to use shared vertices, false otherwise.
         */
        void setUseSharedVertices( bool useSharedVertices ) override;

        /**
         * @brief Compares this sub-mesh with another sub-mesh for equality.
         * @param other Smart pointer to the other sub-mesh.
         * @return True if they are considered equal, otherwise false.
         */
        bool compare( SmartPtr<ISubMesh> other ) const override;

        /**
         * @brief Adds a bone assignment for skeletal animation.
         * @param vba Smart pointer to the vertex bone assignment.
         */
        void addBoneAssignment( SmartPtr<IVertexBoneAssignment> vba ) override;

        /**
         * @brief Removes a bone assignment for skeletal animation.
         * @param vba Smart pointer to the vertex bone assignment to remove.
         */
        void removeBoneAssignment( SmartPtr<IVertexBoneAssignment> vba ) override;

        /**
         * @brief Retrieves all bone assignments for the sub-mesh.
         * @return An array of smart pointers to vertex bone assignments.
         */
        Array<SmartPtr<IVertexBoneAssignment>> getBoneAssignments() const override;

        /**
         * @brief Retrieves the current render operation type (e.g., Triangle List).
         * @return The render operation type.
         */
        RenderOperationType getRenderOperationType() const override;

        /**
         * @brief Sets the render operation type for the sub-mesh.
         * @param renderOperationType The desired render operation type.
         */
        void setRenderOperationType( RenderOperationType renderOperationType ) override;

        /**
         * @brief Validates if the sub-mesh is correctly configured and usable.
         * @return True if the sub-mesh is valid, otherwise false.
         */
        bool isValid() const override;

        void lock() override;
        bool try_lock() override;
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        /** @brief Pointer to the vertex buffer. */
        SmartPtr<IVertexBuffer> m_vertexBuffer;

        /** @brief Pointer to the index buffer. */
        SmartPtr<IIndexBuffer> m_indexBuffer;

        /** @brief The axis-aligned bounding box of the sub-mesh. */
        AABB3<real_Num> m_aabb;

        /** @brief The render operation type, defaults to Triangle List. */
        RenderOperationType m_renderOperationType = RenderOperationType::OT_TRIANGLE_LIST;

        /** @brief Flag indicating if shared vertices are used. */
        bool m_useSharedVertices = false;

        /** @brief The name of the material associated with this sub-mesh. */
        FixedString<128> m_materialName;

        /** @brief List of bone assignments for skeletal animation. */
        Array<SmartPtr<IVertexBoneAssignment>> m_boneAssignments;
    };
}  // namespace workphone

#endif
