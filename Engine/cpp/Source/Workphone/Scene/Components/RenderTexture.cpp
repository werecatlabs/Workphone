#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IGraphicsScene.hpp>
#include <Workphone/Interface/Graphics/IRenderTexture.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone::scene
{
    WP_CLASS_REGISTER_DERIVED( workphone::scene, RenderTexture, Component );

    // Static property names
    const String RenderTexture::widthStr = String( "width" );
    const String RenderTexture::heightStr = String( "height" );
    const String RenderTexture::formatStr = String( "format" );
    const String RenderTexture::textureNameStr = String( "textureName" );
    const String RenderTexture::autoUpdateStr = String( "autoUpdate" );
    const String RenderTexture::updateStr = String( "Update" );

    RenderTexture::RenderTexture() :
        m_width( 512 ),
        m_height( 512 ),
        m_format( PixelFormat::PF_R8G8B8A8 ),
        m_autoUpdate( true )
    {
    }

    RenderTexture::~RenderTexture()
    {
    }

    void RenderTexture::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            Component::load( data );

            createRenderTexture();

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RenderTexture::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            destroyRenderTexture();

            m_renderTexture = nullptr;
            m_texture = nullptr;

            Component::unload( data );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    Array<SmartPtr<ISharedObject>> RenderTexture::getChildObjects() const
    {
        auto childObjects = Component::getChildObjects();
        childObjects.push_back( m_renderTexture );
        childObjects.push_back( m_texture );
        return childObjects;
    }

    SmartPtr<Properties> RenderTexture::getProperties() const
    {
        auto properties = Component::getProperties();

        if( properties )
        {
            properties->setProperty( widthStr, m_width );
            properties->setProperty( heightStr, m_height );
            properties->setProperty( formatStr, static_cast<s32>( m_format ) );
            properties->setProperty( textureNameStr, m_textureName );
            properties->setProperty( autoUpdateStr, m_autoUpdate );
            properties->setButtonPressed( updateStr, false );
        }

        return properties;
    }

    void RenderTexture::setProperties( SmartPtr<Properties> properties )
    {
        Component::setProperties( properties );

        if( !properties )
            return;

        auto oldWidth = m_width;
        auto oldHeight = m_height;
        auto oldFormat = m_format;
        auto oldTextureName = m_textureName;
        auto oldAutoUpdate = m_autoUpdate;

        properties->getPropertyValue( widthStr, m_width );
        properties->getPropertyValue( heightStr, m_height );

        // Defensive: Ensure minimum dimensions
        if( m_width == 0 )
        {
            WP_LOG( "RenderTexture: Width cannot be 0. Resetting to 512." );
            m_width = 512;
        }
        if( m_height == 0 )
        {
            WP_LOG( "RenderTexture: Height cannot be 0. Resetting to 512." );
            m_height = 512;
        }

        s32 format = static_cast<s32>( m_format );
        properties->getPropertyValue( formatStr, format );
        m_format = static_cast<PixelFormat>( format );

        properties->getPropertyValue( textureNameStr, m_textureName );
        properties->getPropertyValue( autoUpdateStr, m_autoUpdate );

        if( oldTextureName != m_textureName && m_texture )
        {
            m_texture->setName( m_textureName );
        }

        if( oldAutoUpdate != m_autoUpdate && m_renderTexture )
        {
            m_renderTexture->setAutoUpdated( m_autoUpdate );
        }

        // Check if we need to recreate the render texture
        if( oldWidth != m_width || oldHeight != m_height || oldFormat != m_format )
        {
            if( getLoadingState() == LoadingState::Loaded )
            {
                try
                {
                    destroyRenderTexture();
                    createRenderTexture();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }

        if( properties->isButtonPressed( updateStr ) )
        {
            update();
        }
    }

    void RenderTexture::update()
    {
        if( m_renderTexture )
        {
            m_renderTexture->update();
        }
    }

    SmartPtr<render::IRenderTexture> RenderTexture::getRenderTexture() const
    {
        return m_renderTexture;
    }

    void RenderTexture::setRenderTexture( SmartPtr<render::IRenderTexture> renderTexture )
    {
        m_renderTexture = renderTexture;
    }

    SmartPtr<render::ITexture> RenderTexture::getTexture() const
    {
        return m_texture;
    }

    void RenderTexture::setTexture( SmartPtr<render::ITexture> texture )
    {
        m_texture = texture;
    }

    u32 RenderTexture::getWidth() const
    {
        return m_width;
    }

    void RenderTexture::setWidth( u32 width )
    {
        if( width == 0 )
        {
            WP_LOG( "RenderTexture: Width cannot be 0." );
            return;
        }

        if( m_width != width )
        {
            m_width = width;

            if( getLoadingState() == LoadingState::Loaded )
            {
                try
                {
                    destroyRenderTexture();
                    createRenderTexture();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }
    }

    u32 RenderTexture::getHeight() const
    {
        return m_height;
    }

    void RenderTexture::setHeight( u32 height )
    {
        if( height == 0 )
        {
            WP_LOG( "RenderTexture: Height cannot be 0." );
            return;
        }

        if( m_height != height )
        {
            m_height = height;

            if( getLoadingState() == LoadingState::Loaded )
            {
                try
                {
                    destroyRenderTexture();
                    createRenderTexture();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }
    }

    PixelFormat RenderTexture::getFormat() const
    {
        return m_format;
    }

    void RenderTexture::setFormat( PixelFormat format )
    {
        if( m_format != format )
        {
            m_format = format;

            if( getLoadingState() == LoadingState::Loaded )
            {
                try
                {
                    destroyRenderTexture();
                    createRenderTexture();
                }
                catch( std::exception &e )
                {
                    WP_LOG_EXCEPTION( e );
                }
            }
        }
    }

    String RenderTexture::getTextureName() const
    {
        return m_textureName;
    }

    void RenderTexture::setTextureName( const String &textureName )
    {
        m_textureName = textureName;
        if( m_texture && !textureName.empty() )
        {
            m_texture->setName( textureName );
        }
    }

    bool RenderTexture::getAutoUpdate() const
    {
        return m_autoUpdate;
    }

    void RenderTexture::setAutoUpdate( bool autoUpdate )
    {
        m_autoUpdate = autoUpdate;
        if( m_renderTexture )
        {
            m_renderTexture->setAutoUpdated( autoUpdate );
        }
    }

    void RenderTexture::createRenderTexture()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            if( !graphicsSystem )
            {
                WP_LOG( "RenderTexture: Graphics system not available." );
                return;
            }

            auto textureManager = graphicsSystem->getTextureManager();
            if( !textureManager )
            {
                WP_LOG( "RenderTexture: Texture manager not available." );
                return;
            }

            m_texture = textureManager->createRenderTexture();
            if( m_texture )
            {
                if( !m_textureName.empty() )
                {
                    m_texture->setName( m_textureName );
                }

                m_texture->setSize(
                    Vector2I( static_cast<s32>( m_width ), static_cast<s32>( m_height ) ) );

                m_renderTexture = workphone::dynamic_pointer_cast<render::IRenderTexture>(
                    m_texture->getRenderTarget() );
                if( m_renderTexture )
                {
                    m_renderTexture->setSize(
                        Vector2I( static_cast<s32>( m_width ), static_cast<s32>( m_height ) ) );
                    m_renderTexture->setAutoUpdated( m_autoUpdate );
                }
                else
                {
                    WP_LOG( "RenderTexture: Texture did not provide a render target." );
                }
            }
            else
            {
                WP_LOG( "RenderTexture: Failed to create render-target texture." );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void RenderTexture::destroyRenderTexture()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            if( !applicationManager )
                return;

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                if( auto textureManager = graphicsSystem->getTextureManager() )
                {
                    textureManager->destroyRenderTexture( m_texture );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        m_renderTexture = nullptr;
        m_texture = nullptr;
    }

}  // namespace workphone::scene
