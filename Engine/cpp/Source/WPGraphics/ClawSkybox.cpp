#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawSkybox.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawSkybox, ISkybox );

        ClawSkybox::ClawSkybox() : m_skybox( wp_skybox_create() )
        {
            if( !m_skybox )
            {
                WP_LOG_ERROR( "ClawSkybox::ClawSkybox: failed to create native skybox." );
                return;
            }

            updateNative();
        }

        ClawSkybox::~ClawSkybox()
        {
            if( m_skybox )
            {
                wp_skybox_destroy( m_skybox );
                m_skybox = nullptr;
            }
        }

        void ClawSkybox::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
        {
            if( layerIdx >= m_textures.size() )
                m_textures.resize( layerIdx + 1 );
            m_textures[layerIdx] = texture;

            if( !m_skybox )
                return;

            if( layerIdx >= WP_SKYBOX_MAX_TEXTURES )
                return;

            if( texture )
            {
                auto name = static_cast<const wp_c8 *>(
                    static_cast<const void *>( texture->getName().c_str() ) );
                wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( layerIdx ), name );
            }
            else
            {
                wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( layerIdx ), nullptr );
            }
        }

        void ClawSkybox::setTexture( const String &fileName, u32 layerIdx )
        {
            if( layerIdx >= m_textures.size() )
                m_textures.resize( layerIdx + 1 );
            m_textures[layerIdx] = nullptr;

            if( !m_skybox )
                return;

            if( layerIdx >= WP_SKYBOX_MAX_TEXTURES )
                return;

            auto name = static_cast<const wp_c8 *>( static_cast<const void *>( fileName.c_str() ) );
            wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( layerIdx ), name );
        }

        Array<SmartPtr<ITexture>> ClawSkybox::getTextures() const
        {
            return m_textures;
        }

        void ClawSkybox::setTextures( const Array<SmartPtr<ITexture>> &textures )
        {
            m_textures = textures;

            if( !m_skybox )
                return;

            const u32 count = static_cast<u32>( m_textures.size() ) < WP_SKYBOX_MAX_TEXTURES
                                  ? static_cast<u32>( m_textures.size() )
                                  : WP_SKYBOX_MAX_TEXTURES;

            for( u32 i = 0; i < count; ++i )
            {
                if( m_textures[i] )
                {
                    auto name = static_cast<const wp_c8 *>(
                        static_cast<const void *>( m_textures[i]->getName().c_str() ) );
                    wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( i ), name );
                }
                else
                {
                    wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( i ), nullptr );
                }
            }

            for( u32 i = count; i < WP_SKYBOX_MAX_TEXTURES; ++i )
            {
                wp_skybox_set_texture_name( m_skybox, static_cast<wp_s32>( i ), nullptr );
            }
        }

        String ClawSkybox::getTextureName( u32 layerIdx ) const
        {
            if( !m_skybox || layerIdx >= WP_SKYBOX_MAX_TEXTURES )
                return StringUtil::EmptyString;

            const wp_c8 *name = wp_skybox_get_texture_name( m_skybox, static_cast<wp_s32>( layerIdx ) );
            if( !name || !name[0] )
                return StringUtil::EmptyString;

            return String( reinterpret_cast<const c8 *>( name ) );
        }

        SmartPtr<ITexture> ClawSkybox::getTexture( u32 layerIdx ) const
        {
            return ( layerIdx < m_textures.size() ) ? m_textures[layerIdx] : nullptr;
        }

        void ClawSkybox::setMaterial( SmartPtr<IMaterial> material )
        {
            m_material = material;
            updateNative();
        }

        SmartPtr<IMaterial> ClawSkybox::getMaterial() const
        {
            return m_material;
        }

        SmartPtr<IGraphicsScene> ClawSkybox::getScene() const
        {
            return m_scene;
        }

        void ClawSkybox::setScene( SmartPtr<IGraphicsScene> scene )
        {
            m_scene = scene;
            updateNative();
        }

        bool ClawSkybox::isVisible() const
        {
            return m_visible;
        }

        void ClawSkybox::setVisible( bool visible )
        {
            m_visible = visible;
            updateNative();
        }

        f32 ClawSkybox::getDistance() const
        {
            return m_distance;
        }

        void ClawSkybox::setDistance( f32 distance )
        {
            m_distance = distance;
            updateNative();
        }

        bool ClawSkybox::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawSkybox::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        wp_skybox *ClawSkybox::getNativeSkybox() const
        {
            return m_skybox;
        }

        void ClawSkybox::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_skybox;
            }
        }

        void ClawSkybox::updateNative()
        {
            if( !m_skybox )
                return;

            wp_skybox_set_enabled( m_skybox, m_visible ? 1 : 0 );
            wp_skybox_set_distance( m_skybox, m_distance );
        }
    }  // namespace render
}  // namespace workphone
