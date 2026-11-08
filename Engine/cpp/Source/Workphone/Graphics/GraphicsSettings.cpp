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
        s32 maximumTextureSize = 0;
        profile.getPropertyValue( "quality.texture", m_textureUploadPolicy.quality );
        profile.getPropertyValue( "quality.maxTextureSize", maximumTextureSize );
        m_textureUploadPolicy.quality = std::clamp( m_textureUploadPolicy.quality, 0, 3 );
        m_textureUploadPolicy.maximum = static_cast<u32>( std::clamp( maximumTextureSize, 0, 16384 ) );
        m_validationLayers = profile.getPropertyAsBool( "graphics.validationLayers" );
        m_allowRendererFallback = !profile.hasProperty( "graphics.allowRendererFallback" ) ||
                                  profile.getPropertyAsBool( "graphics.allowRendererFallback" );
        const auto preset = profile.getPropertyAsInt( "quality.preset", 4 );
        if( preset >= 0 && preset < 4 )
        {
            constexpr s32 shadowQuality[] = { 0, 2, 3, 4 };
            constexpr s32 shadowResolution[] = { 0, 1, 2, 2 };
            m_shadowQuality = shadowQuality[preset];
            m_shadowResolution = shadowResolution[preset];
        }
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

    std::optional<u32> GraphicsSettings::getAnisotropyOverride() const
    {
        return m_anisotropyOverride;
    }

    std::optional<u32> GraphicsSettings::getTextureFilteringOverride() const
    {
        return m_textureFilteringOverride;
    }

    std::optional<s32> GraphicsSettings::getLightingMode() const
    {
        return m_hasLightingMode ? std::optional{ m_lightingMode } : std::nullopt;
    }

    bool GraphicsSettings::getLightingDisabled() const
    {
        return m_lightingDisabled;
    }

    bool GraphicsSettings::getReflectionProbes() const
    {
        return m_reflectionProbes;
    }

    bool GraphicsSettings::getDynamicBatching() const
    {
        return m_dynamicBatching;
    }

    bool GraphicsSettings::getStaticBatching() const
    {
        return m_staticBatching;
    }

    bool GraphicsSettings::getAllowRendererFallback() const
    {
        return m_allowRendererFallback;
    }

    bool GraphicsSettings::getValidationLayers() const
    {
        return m_validationLayers;
    }

    f32 GraphicsSettings::getShadowDistance() const
    {
        return m_shadowDistance;
    }

    s32 GraphicsSettings::getShadowResolution() const
    {
        return m_shadowResolution;
    }

    s32 GraphicsSettings::getShadowQuality() const
    {
        return m_shadowQuality;
    }

    s32 GraphicsSettings::getParticleQuality() const
    {
        return m_particleQuality;
    }

    TextureUploadPolicy GraphicsSettings::getTextureUploadPolicy() const
    {
        return m_textureUploadPolicy;
    }

    s32 GraphicsSettings::getPresentMode() const
    {
        return m_presentMode;
    }

    s32 GraphicsSettings::getShaderModel() const
    {
        return m_shaderModel;
    }

    s32 GraphicsSettings::getShaderLanguage() const
    {
        return m_shaderLanguage;
    }

    s32 GraphicsSettings::getRendererMode() const
    {
        return m_rendererMode;
    }

    const String &GraphicsSettings::getRendererApi() const
    {
        return m_rendererApi;
    }

}  // namespace workphone::render
