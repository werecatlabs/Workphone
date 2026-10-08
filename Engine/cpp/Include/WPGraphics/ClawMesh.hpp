#ifndef ClawMesh_h__
#define ClawMesh_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <workphone_graphics_skinning.h>
#include <Workphone/Graphics/GraphicsMesh.hpp>
#include <Workphone/Core/FixedString.hpp>
#include <Workphone/Atomics/AtomicFixedString.hpp>
#include "workphone_graphics_mesh.h"

struct wp_graphics_object;

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawMesh
         * @brief Implementation of a graphics mesh that bridges Workphone's GraphicsMesh with the native Claw graphics system.
         *
         * This class manages the lifecycle and properties of a native wp_graphics_mesh and its associated
         * render object, providing an interface for loading, unloading, and configuring mesh properties
         * such as materials, visibility, and animations.
         */
        class WPGraphics_API ClawMesh : public GraphicsMesh
        {
        public:
            ClawMesh();
            ~ClawMesh() override;

            /** Loads mesh data from a shared object. */
            void load( SmartPtr<ISharedObject> data ) override;

            /** Rebuilds native geometry while keeping the render object attached. */
            void reload( SmartPtr<ISharedObject> data ) override;

            /** Unloads the currently loaded mesh data. */
            void unload( SmartPtr<ISharedObject> data ) override;

            /** Returns the pointer to the underlying native Claw mesh object. */
            wp_graphics_mesh *getNativeMesh() const;

            /** Binds the scene-owned C render object used for native submission. */
            void bindNativeRenderObject( wp_graphics_object *object );

            /** Returns the native render object associated with this mesh. */
            wp_graphics_object *getNativeRenderObject() const;

            /** Calculates and returns the local axis-aligned bounding box (AABB) of the mesh. */
            AABB3<real_Num> getLocalAABB() const override;

            /** Sets the material name for the specified index. Defaults to the first material if index is -1. */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /** Retrieves the material name for the specified index. */
            String getMaterialName( s32 index = -1 ) const override;

            /** Assigns a material object to the specified index. */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /** Retrieves the material object for the specified index. */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            /** Enables or disables hardware-accelerated animation. */
            void setHardwareAnimationEnabled( bool enabled ) override;

            /** Validates and updates vertex processing settings for the current hardware state. */
            void checkVertexProcessing() override;

            /** Returns the animation controller associated with this mesh. */
            SmartPtr<IAnimationController> getAnimationController() override;

            /** Sets whether the mesh is visible during rendering. */
            void setVisible( bool visible ) override;

            /** Checks if the mesh is currently visible. */
            bool isVisible() const override;

            /** Configures whether the mesh casts shadows into the scene. */
            void setCastShadows( bool castShadows ) override;
            bool getCastShadows() const override;

            /** Configures whether the mesh receives shadows from other objects. */
            void setReceiveShadows( bool receiveShadows ) override;
            bool getReceiveShadows() const override;

            /** Sets specific visibility flags for filtering during the render pass. */
            void setVisibilityFlags( u32 flags ) override;

            /** Returns the identifier name of the mesh. */
            String getMeshName() const override;

            /** Sets the identifier name of the mesh. */
            void setMeshName( const String &meshName ) override;

            /** Retrieves the current progressive mesh (LOD) options. */
            ProgressiveMeshOptions getProgressiveMeshOptions() const override;

            /** Configures the progressive mesh (LOD) options. */
            void setProgressiveMeshOptions( const ProgressiveMeshOptions &options ) override;

            /** Returns the skeleton used for skeletal animation. */
            SmartPtr<IGraphicsSkeleton> getSkeleton() const override;

            /** Assigns a skeleton for skeletal animation. */
            void setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton ) override;

            /** Initial CPU deformation path; call from the mesh-owning render thread.
             * Joint palettes are model-space current_joint * inverse_bind. */
            bool setSkinningData( const Array<wp_skin_vertex> &vertices );
            bool applySkinningPalette( const Array<wp_mat4f> &palette );
            bool hasSkinningData() const;
            /** Replace indexed triangle geometry and refresh bounds/GPU data. Render thread only. */
            bool updateGeometry( const Array<wp_graphics_mesh_vertex_pntc> &vertices,
                                 const Array<u32> &indices );

            /** Creates a deep copy of the mesh object. */
            SmartPtr<IGraphicsObject> clone(
                const String &name = StringUtil::EmptyString ) const override;

            /** Internal helper to retrieve the raw underlying object pointer. */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<wp_skin_vertex> m_skinVertices;
            Array<wp_skin_result> m_skinOutput;
            wp_graphics_mesh *m_mesh = nullptr;  ///< Pointer to the native Claw mesh structure.
            wp_graphics_object *m_renderObject =
                nullptr;                   ///< Pointer to the native render object used for submission.
            atomic_bool m_visible = true;  ///< Atomic flag indicating the visibility state of the mesh.
            atomic_bool m_castShadows = true;
            atomic_bool m_receiveShadows = true;
            atomic_fixed_string<WP_MAX_PATH> m_meshName;  ///< Name of the mesh resource.
            ProgressiveMeshOptions m_options;  ///< Configuration for progressive mesh / LOD levels.
        };

    }  // namespace render
}  // namespace workphone

#endif  // ClawMesh_h__
