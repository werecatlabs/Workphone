#include "WPGraphics/WPClawHammerPCH.hpp"
#include <WPGraphics/ClawSkyboxCube.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone
{
    namespace render
    {
        WP_CLASS_REGISTER_DERIVED( workphone::render, ClawSkyboxCube, ISkyboxCube );

        ClawSkyboxCube::ClawSkyboxCube() : m_skybox( wp_skybox_cube_create() )
        {
            if( !m_skybox )
            {
                WP_LOG_ERROR( "ClawSkyboxCube::ClawSkyboxCube: failed to create native skybox cube." );
            }
        }

        ClawSkyboxCube::~ClawSkyboxCube()
        {
            if( m_skybox )
            {
                wp_skybox_cube_unload( m_skybox );
                wp_skybox_cube_destroy( m_skybox );
                m_skybox = nullptr;
            }
        }

        void ClawSkyboxCube::update()
        {
            // Resolve the current camera each frame, including editor camera switches.
            updateNative();
        }

        void ClawSkyboxCube::setTexture( SmartPtr<ITexture> texture, u32 layerIdx )
        {
            if( layerIdx >= m_textures.size() )
                m_textures.resize( layerIdx + 1 );
            m_textures[layerIdx] = texture;
        }

        void ClawSkyboxCube::setTexture( const String &fileName, u32 layerIdx )
        {
            // Texture loading not fully implemented.
        }

        Array<SmartPtr<ITexture>> ClawSkyboxCube::getTextures() const
        {
            return m_textures;
        }

        void ClawSkyboxCube::setTextures( const Array<SmartPtr<ITexture>> &textures )
        {
            m_textures = textures;
        }

        String ClawSkyboxCube::getTextureName( u32 layerIdx ) const
        {
            return StringUtil::EmptyString;
        }

        SmartPtr<ITexture> ClawSkyboxCube::getTexture( u32 layerIdx ) const
        {
            return ( layerIdx < m_textures.size() ) ? m_textures[layerIdx] : nullptr;
        }

        void ClawSkyboxCube::setMaterial( SmartPtr<IMaterial> material )
        {
            m_material = material;
            updateNative();
        }

        SmartPtr<IMaterial> ClawSkyboxCube::getMaterial() const
        {
            return m_material;
        }

        SmartPtr<IGraphicsScene> ClawSkyboxCube::getScene() const
        {
            return m_scene;
        }

        void ClawSkyboxCube::setScene( SmartPtr<IGraphicsScene> scene )
        {
            m_scene = scene;
            updateNative();
        }

        bool ClawSkyboxCube::isVisible() const
        {
            return m_visible;
        }

        void ClawSkyboxCube::setVisible( bool visible )
        {
            m_visible = visible;
            updateNative();
        }

        f32 ClawSkyboxCube::getDistance() const
        {
            return m_distance;
        }

        void ClawSkyboxCube::setDistance( f32 distance )
        {
            m_distance = distance;
            updateNative();
        }

        bool ClawSkyboxCube::handleStateMessage( const SmartPtr<IStateMessage> &message )
        {
            return false;
        }

        bool ClawSkyboxCube::handleStateChanged( SmartPtr<IState> &state )
        {
            return false;
        }

        wp_skybox_cube *ClawSkyboxCube::getNativeSkybox() const
        {
            return m_skybox;
        }

        void ClawSkyboxCube::_getObject( void **ppObject ) const
        {
            if( ppObject )
            {
                *ppObject = m_skybox;
            }
        }

        void ClawSkyboxCube::updateNative()
        {
            if( !m_skybox )
                return;

            if( !m_scene )
            {
                if( m_loaded )
                {
                    wp_skybox_cube_unload( m_skybox );
                    m_loaded = false;
                }
                return;
            }

            void *sceneNative = nullptr;
            m_scene->_getObject( &sceneNative );

            SmartPtr<IGraphicsCamera> camera = m_scene->getActiveCamera();
            if( !camera )
                camera = m_scene->getDefaultCamera();

            void *cameraNative = nullptr;
            if( camera )
            {
                camera->_getObject( &cameraNative );
            }

            if( !sceneNative || !cameraNative )
            {
                if( m_loaded )
                {
                    wp_skybox_cube_unload( m_skybox );
                    m_loaded = false;
                }
                return;
            }

            wp_skybox_cube_load( m_skybox, sceneNative, cameraNative, m_distance );
            m_loaded = true;

            wp_graphics_material *nativeMaterial = nullptr;
            if( m_material )
            {
                void *object = nullptr;
                m_material->_getObject( &object );
                nativeMaterial = static_cast<wp_graphics_material *>( object );
            }

            wp_skybox_cube_apply_material( m_skybox, nativeMaterial );
            wp_skybox_cube_update( m_skybox );
        }
    }  // namespace render
}  // namespace workphone
