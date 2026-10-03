#ifndef CGraphicsMesh_h__
#define CGraphicsMesh_h__

#include <Workphone/Graphics/GraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IGraphicsMesh.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @class GraphicsMesh
         * @brief Represents a graphics mesh object that manages mesh data, materials, and animation for
         * rendering.
         *
         * This class provides an interface for setting and retrieving mesh materials, mesh names,
         * skeletons, and animation controllers. It supports hardware animation and vertex processing
         * checks. The mesh can have a main material and multiple sub-materials.
         *
         * @note Inherits from GraphicsObject<IGraphicsMesh>.
         */
        class WPCore_API GraphicsMesh : public GraphicsObject<IGraphicsMesh>
        {
        public:
            /**
             * @brief Constructs a new GraphicsMesh object.
             */
            GraphicsMesh();

            /**
             * @brief Destroys the GraphicsMesh object.
             */
            ~GraphicsMesh() override;

            /**
             * @brief Sets the material name for the mesh or a sub-mesh.
             * @param materialName The name of the material to set.
             * @param index The sub-mesh index. If -1, sets the main material name.
             */
            void setMaterialName( const String &materialName, s32 index = -1 ) override;

            /**
             * @brief Gets the material name for the mesh or a sub-mesh.
             * @param index The sub-mesh index. If -1, gets the main material name.
             * @return The material name as a String.
             */
            String getMaterialName( s32 index = -1 ) const override;

            /**
             * @brief Sets the material for the mesh or a sub-mesh.
             * @param material The material to set.
             * @param index The sub-mesh index. If -1, sets the main material.
             */
            void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) override;

            /**
             * @brief Gets the material for the mesh or a sub-mesh.
             * @param index The sub-mesh index. If -1, gets the main material.
             * @return A smart pointer to the material.
             */
            SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const override;

            /**
             * @brief Enables or disables hardware animation for the mesh.
             * @param enabled True to enable hardware animation, false to disable.
             */
            void setHardwareAnimationEnabled( bool enabled ) override;

            /**
             * @brief Checks and updates the vertex processing method for the mesh.
             */
            void checkVertexProcessing() override;

            /**
             * @brief Gets the animation controller associated with the mesh.
             * @return A smart pointer to the animation controller.
             */
            SmartPtr<IAnimationController> getAnimationController() override;

            /**
             * @brief Gets the name of the mesh.
             * @return The mesh name as a String.
             */
            String getMeshName() const override;

            /**
             * @brief Sets the name of the mesh.
             * @param meshName The name to set for the mesh.
             */
            void setMeshName( const String &meshName ) override;

            ProgressiveMeshOptions getProgressiveMeshOptions() const override;

            void setProgressiveMeshOptions( const ProgressiveMeshOptions &options ) override;

            /**
             * @brief Gets the skeleton associated with the mesh.
             * @return A smart pointer to the graphics skeleton.
             */
            SmartPtr<IGraphicsSkeleton> getSkeleton() const override;

            /**
             * @brief Sets the skeleton for the mesh.
             * @param skeleton A smart pointer to the graphics skeleton to associate with the mesh.
             */
            void setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            ///< Animation controller for the mesh.
            AtomicSmartPtr<IAnimationController> m_animationController;

            ///< The main material for the mesh.
            AtomicSmartPtr<IMaterial> m_material;

            ///< The skeleton associated with the mesh.
            AtomicSmartPtr<IGraphicsSkeleton> m_skeleton;

            ///< Flag indicating if hardware animation is enabled.
            bool m_hardwareAnimationEnabled = false;

            String m_meshName;      ///< The name of the mesh.
            String m_materialName;  ///< The main material name for the mesh.

            AtomicObject<ProgressiveMeshOptions> m_progressiveMeshOptions;

            ConcurrentArray<String> m_subMaterialNames;           ///< Material names for sub-meshes.
            ConcurrentArray<SmartPtr<IMaterial>> m_subMaterials;  ///< Materials for sub-meshes.
        };
    }  // namespace render
}  // namespace workphone

#endif  // CGraphicsMesh_h__
