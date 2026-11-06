#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Graphics/GraphicsSettings.hpp>
#include <Workphone/Graphics/ProjectGraphicsProviders.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>
#include <algorithm>
#include <utility>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, GraphicsSettings, Director );

    GraphicsSettings::GraphicsSettings() = default;

    GraphicsSettings::GraphicsSettings( SmartPtr<Properties> playerProfile ) :
        m_reflectionProbes( true ),
        m_playerProfile( std::move( playerProfile ) )
    {
        if( !m_playerProfile )
        {
            return;
        }

        const auto &profile = *m_playerProfile;
        m_rendererApi = projectRendererApi( m_playerProfile );
        m_rendererMode = profile.getPropertyAsInt( "graphics.renderer.mode" );
        m_shaderLanguage = profile.getPropertyAsInt( "graphics.shaderLanguage" );
        m_shaderModel = profile.getPropertyAsInt( "graphics.shaderModel" );
        m_presentMode = profile.getPropertyAsInt( "graphics.presentMode" );
        m_particleQuality = std::clamp( profile.getPropertyAsInt( "quality.particles" ), 0, 3 );
        m_validationLayers = profile.getPropertyAsBool( "graphics.validationLayers" );
        m_allowRendererFallback = !profile.hasProperty( "graphics.allowRendererFallback" ) ||
                                  profile.getPropertyAsBool( "graphics.allowRendererFallback" );
        profile.getPropertyValue( "graphics.shadowQuality", m_shadowQuality );
        profile.getPropertyValue( "graphics.shadowResolution", m_shadowResolution );
        profile.getPropertyValue( "graphics.shadowDistance", m_shadowDistance );
        profile.getPropertyValue( "graphics.reflectionProbes", m_reflectionProbes );
        profile.getPropertyValue( "performance.staticBatching", m_staticBatching );
        profile.getPropertyValue( "performance.dynamicBatching", m_dynamicBatching );

        s32 value = 0;
        if( profile.getPropertyValue( "graphics.textureFiltering", value ) )
        {
            setTextureFiltering( value );
        }
        if( profile.getPropertyValue( "graphics.anisotropy", value ) )
        {
            setAnisotropy( value );
        }
        m_hasLightingMode = profile.getPropertyValue( "graphics.lightingMode", m_lightingMode );
        if( m_hasLightingMode )
        {
            // The UV override historically parses the serialized value.
            m_lightingDisabled = profile.getPropertyAsInt( "graphics.lightingMode" ) == 0;
        }
    }

    GraphicsSettings::~GraphicsSettings() = default;

    auto GraphicsSettings::getCreateWindow() const -> bool
    {
        return m_createWindow;
    }

    void GraphicsSettings::setCreateWindow( bool createWindow )
    {
        m_createWindow = createWindow;
    }

    auto GraphicsSettings::getShowDialog() const -> bool
    {
        return m_showDialog;
    }

    void GraphicsSettings::setShowDialog( bool showDialog )
    {
        m_showDialog = showDialog;
    }

    void GraphicsSettings::setCreateRenderUI( bool createRenderUI )
    {
        m_createRenderUI = createRenderUI;
    }

    bool GraphicsSettings::getCreateRenderUI() const
    {
        return m_createRenderUI;
    }

    bool GraphicsSettings::getVSync() const
    {
        return m_vsync;
    }

    void GraphicsSettings::setVSync( bool enabled )
    {
        m_vsync = enabled;
    }

    void GraphicsSettings::setAnisotropy( s32 anisotropy )
    {
        m_anisotropy = anisotropy;
        m_anisotropyOverride = static_cast<u32>( std::clamp( anisotropy, 1, 16 ) );
    }

    s32 GraphicsSettings::getAnisotropy() const
    {
        return m_anisotropy;
    }

    void GraphicsSettings::setTextureFiltering( s32 textureFiltering )
    {
        m_textureFiltering = textureFiltering;
        m_textureFilteringOverride = static_cast<u32>( std::clamp( textureFiltering, 0, 2 ) );
    }

    s32 GraphicsSettings::getTextureFiltering() const
    {
        return m_textureFiltering;
    }

    SmartPtr<Properties> GraphicsSettings::getPlayerProfile() const
    {
        return m_playerProfile;
    }

}  // namespace workphone::render
