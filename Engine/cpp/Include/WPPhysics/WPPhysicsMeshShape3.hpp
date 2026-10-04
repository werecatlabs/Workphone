#ifndef WPPHYSICSMESHSHAPE3_HPP
#define WPPHYSICSMESHSHAPE3_HPP

#include <WPPhysics/WPPhysicsShape3Adapter.hpp>
#include <Workphone/Interface/Physics/IMeshShape.hpp>

namespace workphone::physics
{
    /**
     * @class WPPhysicsMeshShape3
     * @brief Implementation of a mesh-based physics collider.
     *
     * This class manages physics shapes generated from meshes, supporting both
     * convex and concave configurations and handling the transition from mesh resources
     * to physics engine data.
     */
    class WPPhysicsMeshShape3 : public WPPhysicsShape3Adapter<IMeshShape>
    {
    public:
        WPPhysicsMeshShape3();

        /** @brief Loads the shape data from a shared object. */
        void load( SmartPtr<ISharedObject> data ) override;

        /** @brief Gets the mesh used for the physics shape. */
        SmartPtr<IMesh> getMesh() const override;

        /** @brief Gets the mesh resource associated with this shape. */
        SmartPtr<IMeshResource> getMeshResource() const override;

        /** @brief Sets the mesh resource for this shape. */
        void setMeshResource( SmartPtr<IMeshResource> meshResource ) override;

        /** @brief Gets the original, unmodified mesh. */
        SmartPtr<IMesh> getCleanMesh() const override;

        /** @brief Sets the original unmodified mesh. */
        void setCleanMesh( SmartPtr<IMesh> cleanMesh ) override;

        /** @brief Returns true if the mesh is treated as a convex hull. */
        bool isConvex() const override;

        /** @brief Sets whether the mesh should be treated as convex. */
        void setConvex( bool convex ) override;

        /** @brief Creates a deep copy of the mesh shape. */
        SmartPtr<IPhysicsShape3> clone() override;

    protected:
        /** @brief Internal method to rebuild the internal vertex and index buffers. */
        void rebuildMeshData();

        AtomicSmartPtr<IMeshResource> m_meshResource;  ///< Resource providing the mesh data.
        AtomicSmartPtr<IMesh> m_cleanMesh;             ///< The original mesh before any processing.
        Array<wp_f32> m_vertices;                      ///< Vertex buffer used by the physics engine.
        Array<wp_u32> m_indices;                       ///< Index buffer used by the physics engine.
        atomic_bool m_convex = false;                  ///< Flag indicating if the shape is convex.
    };
}  // namespace workphone::physics

#endif
