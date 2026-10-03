#ifndef ClawSky_h__
#define ClawSky_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/Graphics/ISky.hpp>

namespace workphone
{
    namespace render
    {
        class WPGraphics_API ClawSky : public ISky
        {
        public:
            ClawSky();
            ~ClawSky() override;

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

            WP_CLASS_REGISTER_DECL;

        protected:
            Array<SmartPtr<ITexture>> m_textures;
            SmartPtr<IMaterial> m_material;
            SmartPtr<IGraphicsScene> m_scene;
            bool m_visible = true;
            f32 m_distance = 1000.0f;
        };
    }  // namespace render
}  // namespace workphone

#endif  // ClawSky_h__
