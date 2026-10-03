#ifndef DynamicMesh_h__
#define DynamicMesh_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IDynamicMesh.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @file DynamicMesh.hpp
         * @brief Runtime wrapper for a mesh/sub-mesh pair that can be changed at runtime.
         *
         * The DynamicMesh class implements the IDynamicMesh interface and provides
         * setters/getters for an `IMesh` and an `ISubMesh`. It is intended for use
         * with dynamic geometry that may be replaced or updated during the application's
         * lifetime (for example: streaming, procedural generation, or LOD swapping).
         *
         * Ownership/semantics:
         * - The class stores `SmartPtr` references to the mesh and sub-mesh. Callers
         *   should observe the SmartPtr semantics used across the codebase.
         * - The `setDirty` flag allows external code to mark the mesh as changed so
         *   rendering or resource-management subsystems can react accordingly.
         */
        class WPCore_API DynamicMesh : public GraphicsObject<IDynamicMesh>
        {
        public:
            /**
             * @brief Construct an empty DynamicMesh.
             *
             * By default no mesh or sub-mesh is set and the dirty flag is false.
             */
            DynamicMesh();

            /**
             * @brief Virtual destructor.
             *
             * Ensures correct cleanup when deleted through base pointers.
             */
            ~DynamicMesh() override;

            /**
             * @brief Set the mesh reference for this DynamicMesh.
             * @param mesh SmartPtr to an IMesh instance to use. May be null to clear.
             *
             * Replaces the current mesh reference. If the mesh changes, callers may
             * want to call `setDirty(true)` to notify downstream systems.
             */
            void setMesh( SmartPtr<IMesh> mesh ) override;

            /**
             * @brief Get the currently assigned mesh.
             * @return SmartPtr<IMesh> current mesh reference (may be null).
             *
             * Returned SmartPtr shares ownership according to `SmartPtr` semantics.
             */
            SmartPtr<IMesh> getMesh() const override;

            /**
             * @brief Set the sub-mesh reference for this DynamicMesh.
             * @param subMesh SmartPtr to an ISubMesh instance to use. May be null to clear.
             *
             * Sub-meshes are typically used to reference a subset of geometry/materials
             * from a parent `IMesh`. Replacing the sub-mesh does not modify the parent mesh.
             */
            void setSubMesh( SmartPtr<ISubMesh> subMesh ) override;

            /**
             * @brief Get the currently assigned sub-mesh.
             * @return SmartPtr<ISubMesh> current sub-mesh reference (may be null).
             */
            SmartPtr<ISubMesh> getSubMesh() const override;

            /**
             * @brief Mark the dynamic mesh as dirty or clean.
             * @param dirty true to mark as dirty (changed), false to mark as clean.
             *
             * The dirty flag is a simple hint for resource management or rendering
             * subsystems to re-upload or otherwise handle the changed geometry.
             */
            void setDirty( bool dirty ) override;

            WP_CLASS_REGISTER_DECL; /**< Reflection/registration macro used by the engine. */

        private:
            SmartPtr<IMesh> m_mesh;       /**< Owned/shared pointer to the current mesh. */
            SmartPtr<ISubMesh> m_subMesh; /**< Owned/shared pointer to the current sub-mesh. */
            bool m_dirty = false; /**< True when the mesh has been modified and needs handling. */
        };
    }  // namespace render
}  // namespace workphone

#endif  // DynamicMesh_h__
