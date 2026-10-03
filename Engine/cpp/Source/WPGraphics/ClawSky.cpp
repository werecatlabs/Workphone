#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawSky.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawSky, ISky );

        ClawSky::ClawSky()
        {
        }

        ClawSky::~ClawSky()
        {
        }

        void ClawSky::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
        {
            if( layerIdx >= m_textures.size() )
                m_textures.resize( layerIdx + 1 );
            m_textures[layerIdx] = texture;
        }

        void ClawSky::setTexture( const String &fileName, u32 layerIdx )
        {
            // Texture loading not fully implemented
        }

        Array<SmartPtr<ITexture>> ClawSky::getTextures() const
        {
            return m_textures;
        }

        void ClawSky::setTextures( const Array<SmartPtr<ITexture>> &textures )
        {
            m_textures = textures;
        }

        String ClawSky::getTextureName( u32 layerIdx ) const
        {
            return StringUtil::EmptyString;
        }

        SmartPtr<ITexture> ClawSky::getTexture( u32 layerIdx ) const
        {
            return ( layerIdx < m_textures.size() ) ? m_textures[layerIdx] : nullptr;
        }

        void ClawSky::setMaterial( SmartPtr<IMaterial> material )
        {
            m_material = material;
        }

        SmartPtr<IMaterial> ClawSky::getMaterial() const
        {
            return m_material;
        }

        SmartPtr<IGraphicsScene> ClawSky::getScene() const
        {
            return m_scene;
        }

        void ClawSky::setScene( SmartPtr<IGraphicsScene> scene )
        {
            m_scene = scene;
        }

        bool ClawSky::isVisible() const
        {
            return m_visible;
        }

        void ClawSky::setVisible( bool visible )
        {
            m_visible = visible;
        }

        f32 ClawSky::getDistance() const
        {
            return m_distance;
        }

        void ClawSky::setDistance( f32 distance )
        {
            m_distance = distance;
        }

        bool ClawSky::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawSky::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }
    }  // namespace render
}  // namespace workphone
