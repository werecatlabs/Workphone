#ifndef ClawSkybox_h__
#define ClawSkybox_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ISkybox.hpp>
#include "workphone_graphics_skybox.h"

namespace workphone
{
    namespace render
    {
        /**
         * @class ClawSkybox
         * @brief ClawHammer implementation of a six-sided skybox.
         *
         * Wraps the C89 @c wp_skybox API and implements the engine
         * @c ISkybox interface. It tracks the skybox material, scene,
         * visibility and distance, and forwards these to the native skybox
         * object.
         */
        class WPGraphics_API ClawSkybox : public ISkybox
        {
        public:
            ClawSkybox();
            ~ClawSkybox() override;

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
             * @brief Returns the native C89 skybox handle.
             */
            wp_skybox *getNativeSkybox() const;

            /**
             * @brief Internal access to the underlying native object pointer.
             */
            void _getObject( void **ppObject ) const;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Synchronises the stored C++ state into the native skybox.
             */
            void updateNative();

            wp_skybox *m_skybox;                   ///< Native skybox handle.
            Array<SmartPtr<ITexture>> m_textures;  ///< Stored texture layers.
            SmartPtr<IMaterial> m_material;        ///< Assigned material.
            SmartPtr<IGraphicsScene> m_scene;      ///< Assigned graphics scene.
            bool m_visible = true;                 ///< Visibility flag.
            f32 m_distance = 1000.0f;              ///< Skybox distance.
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawSkybox_h__
