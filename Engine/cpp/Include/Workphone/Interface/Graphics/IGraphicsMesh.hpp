#ifndef _IGraphicsMesh_H
#define _IGraphicsMesh_H

#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Interface/Graphics/IGraphicsObject.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Interface for a graphics mesh object.
         *
         * This interface provides functionality for managing 3D mesh objects in the graphics system.
         * It supports material assignment, skeletal animation, hardware acceleration, and mesh
         * properties. Meshes can be rendered with different materials per submesh and can cast shadows.
         */
        class WPCore_API IGraphicsMesh : public IGraphicsObject
        {
        public:
            /** @brief Hash identifier for the render queue property. */
            static const hash_type RENDER_QUEUE_HASH;

            /** @brief Hash identifier for the visibility flags property. */
            static const hash_type VISIBILITY_FLAGS_HASH;

            /** @brief Property string identifier for mesh name. */
            static const String meshNamePropertyStr;

            /** @brief Property string identifier for shadow casting flag. */
            static const String castShadowsPropertyStr;

            IGraphicsMesh();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of derived classes.
             */
            ~IGraphicsMesh() override;

            /**
             * Sets the name of the material to be used for rendering.
             *
             * @param materialName The name of the material.
             * @param index The index of the submesh (default value is -1 for all submeshes).
             */
            virtual void setMaterialName( const String &materialName, s32 index = -1 ) = 0;

            /**
             * Gets the name of the material used by the mesh.
             *
             * @param index The index of the submesh (default value is -1 for all submeshes).
             *
             * @return The name of the material.
             */
            virtual String getMaterialName( s32 index = -1 ) const = 0;

            /**
             * Sets the material to be used for rendering.
             *
             * @param material The material to use.
             * @param index The index of the submesh (default value is -1 for all submeshes).
             */
            virtual void setMaterial( SmartPtr<IMaterial> material, s32 index = -1 ) = 0;

            /**
             * Gets the material used by the mesh.
             *
             * @param index The index of the submesh (default value is -1 for all submeshes).
             *
             * @return The material.
             */
            virtual SmartPtr<IMaterial> getMaterial( s32 index = -1 ) const = 0;

            /**
             * Sets whether hardware animation is enabled for the mesh.
             *
             * @param enabled `true` to enable hardware animation, `false` to disable it.
             */
            virtual void setHardwareAnimationEnabled( bool enabled ) = 0;

            /**
             * Checks whether the processing is done in hardware or software for the mesh.
             */
            virtual void checkVertexProcessing() = 0;

            /**
             * Gets an animation controller for this mesh.
             *
             * @return The animation controller.
             */
            virtual SmartPtr<IAnimationController> getAnimationController() = 0;

            /**
             * Gets the name of the mesh.
             *
             * @return The name of the mesh.
             */
            virtual String getMeshName() const = 0;

            /**
             * Sets the name of the mesh.
             *
             * @param meshName The name of the mesh.
             */
            virtual void setMeshName( const String &meshName ) = 0;

            /** Gets the progressive mesh settings used to build this render mesh. */
            virtual ProgressiveMeshOptions getProgressiveMeshOptions() const = 0;

            /** Sets the progressive mesh settings used on the next load or reload. */
            virtual void setProgressiveMeshOptions( const ProgressiveMeshOptions &options ) = 0;

            /** Gets the mesh skeleton. */
            virtual SmartPtr<IGraphicsSkeleton> getSkeleton() const = 0;

            /** Sets the mesh skeleton. */
            virtual void setSkeleton( SmartPtr<IGraphicsSkeleton> skeleton ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace render
}  // namespace workphone

#endif
