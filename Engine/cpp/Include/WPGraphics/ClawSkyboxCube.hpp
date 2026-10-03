#ifndef ClawSkyboxCube_h__
#define ClawSkyboxCube_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ISkyboxCube.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include "workphone_graphics_skybox_cube.h"

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawSkyboxCube
         * @brief ClawHammer implementation of a cube skybox.
         *
         * Wraps the C89 @c wp_skybox_cube API and implements the engine
         * @c ISkyboxCube interface. It tracks the skybox material, scene,
         * visibility and distance, and forwards these to the native skybox
         * object when the required scene and camera natives are available.
         */
        class WPGraphics_API ClawSkyboxCube : public ISkyboxCube
        {
        public:
            ClawSkyboxCube();
            ~ClawSkyboxCube() override;

            void update() override;

            void setTexture( SmartPtr<ITexture> texture, u32 layerIdx = 0 ) override;
            void setTexture( const String &fileName, u32 layerIdx = 0 ) override;
            Array<SmartPtr<ITexture>> getTextures() const override;
            void setTextures( const Array<SmartPtr<ITexture>> &textures ) override;
            String getTextureName( u32 layerIdx = 0 ) const override;
            SmartPtr<ITexture> getTexture( u32 layerIdx = 0 ) const override;
            void setMaterial( SmartPtr<IMaterial> material ) override;
            SmartPtr<IMaterial> getMaterial() const override;
            SmartPtr<IGraphicsScene> getScene() const override;
            void setScene( SmartPtr<IGraphicsScene> scene ) override;
            bool isVisible() const override;
            void setVisible( bool visible ) override;
            f32 getDistance() const override;
            void setDistance( f32 distance ) override;
            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Returns the native C89 cube skybox handle.
             */
            wp_skybox_cube *getNativeSkybox() const;

            /**
             * @brief Internal access to the underlying native object pointer.
             */
            void _getObject( void **ppObject ) const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Reloads the native skybox using the current scene, camera
             *        material, visibility and distance.
             */
            void updateNative();

            wp_skybox_cube *m_skybox;              ///< Native cube skybox handle.
            Array<SmartPtr<ITexture>> m_textures;  ///< Stored texture layers.
            SmartPtr<IMaterial> m_material;        ///< Assigned material.
            SmartPtr<IGraphicsScene> m_scene;      ///< Assigned graphics scene.
            bool m_visible = true;                 ///< Visibility flag.
            f32 m_distance = 5000.0f;              ///< Skybox cube size / distance.
            bool m_loaded = false;                 ///< Whether the native object is loaded.
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawSkyboxCube_h__
