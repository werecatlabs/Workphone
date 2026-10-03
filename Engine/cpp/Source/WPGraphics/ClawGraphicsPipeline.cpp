#include <WPGraphics/WPClawHammerPCH.hpp>
#include <WPGraphics/ClawGraphicsPipeline.hpp>
#include <WPGraphics/ClawScene.hpp>
#include <WPGraphics/ClawUtil.hpp>
#include <Workphone/Workphone.hpp>
#include <workphone.h>
#include <workphone_graphics_pipeline.h>
#include <workphone_graphics_renderer.h>
#include <workphone_graphics_renderer_software.h>
#include <algorithm>
#include <cstring>

namespace workphone::render
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, ClawGraphicsPipeline, GraphicsPipeline );

    ClawGraphicsPipeline::ClawGraphicsPipeline() = default;

    ClawGraphicsPipeline::~ClawGraphicsPipeline()
    {
        shutdown();
    }

    bool ClawGraphicsPipeline::initialize( s32 width, s32 height )
    {
        if( width <= 0 || height <= 0 )
        {
            WP_LOG_ERROR( "WPGraphics/Pipeline: invalid dimensions " + std::to_string( width ) + "x" + std::to_string( height ) + "; both must be positive." );
            return false;
        }

        if( isInitialized() )
        {
            if( getWidth() == width && getHeight() == height )
            {
                return true;
            }
            shutdown();
        }

        WP_LOG_INFO( "WPGraphics/Pipeline: initializing CPU post-processing at " + std::to_string( width ) + "x" + std::to_string( height ) +
                     "; quality=" + std::to_string( static_cast<s32>( getQualityLevel() ) ) );
        if( !GraphicsPipeline::initialize( width, height ) )
        {
            WP_LOG_ERROR( "WPGraphics/Pipeline: base pipeline initialization failed." );
            return false;
        }

        m_nativePipeline =
            wp_pipeline_create( width, height, static_cast<wp_pipeline_quality>( getQualityLevel() ) );
        if( !m_nativePipeline )
        {
            WP_LOG_ERROR( "WPGraphics/Pipeline: native effect or framebuffer allocation failed at " + std::to_string( width ) + "x" + std::to_string( height ) );
            GraphicsPipeline::shutdown();
            return false;
        }

        m_rgbaOutput.resize( static_cast<size_t>( width ) * height * 4u, 0.0f );
        m_hasPreviousViewProjection = false;
        applyNativeSettings();
        WP_LOG_INFO( "WPGraphics/Pipeline: ready; TAA=" + std::to_string( isTAAEnabled() ) +
                     ", AO=" + std::to_string( isGTAOEnabled() ) + ", SSR=" + std::to_string( isSSREnabled() ) +
                     ", bloom=" + std::to_string( isBloomEnabled() ) + ", exposure=" + std::to_string( isExposureEnabled() ) +
                     ", DOF=" + std::to_string( isDOFEnabled() ) + ", motion blur=" + std::to_string( isMotionBlurEnabled() ) +
                     ", contact shadows=" + std::to_string( isContactShadowsEnabled() ) + ", CSM=" + std::to_string( isCSMEnabled() ) );
        return true;
    }

    void ClawGraphicsPipeline::shutdown()
    {
        if( m_nativePipeline )
        {
            wp_pipeline_destroy( m_nativePipeline );
            m_nativePipeline = nullptr;
        }
        m_rgbaOutput.clear();
        m_hdrInput.clear();
        m_normalInput.clear();
        m_depthInput.clear();
        m_velocityInput.clear();
        m_ldrPresentation.clear();
        m_hasFrameInputs = false;
        m_hasPreviousViewProjection = false;
        GraphicsPipeline::shutdown();
    }

    void ClawGraphicsPipeline::beginFrame()
    {
        GraphicsPipeline::beginFrame();
    }

    void ClawGraphicsPipeline::endFrame()
    {
        GraphicsPipeline::endFrame();
    }

    void ClawGraphicsPipeline::render()
    {
        GraphicsPipeline::render();
        if( !m_nativePipeline || !isInitialized() || !getCamera() || !m_hasFrameInputs )
        {
            return;
        }

        const auto &view = getViewMatrix();
        const auto &projection = getProjectionMatrix();
        const auto viewProjection = projection * view;
        const auto inverseViewProjection = viewProjection.inverse();
        const auto nativeView = ClawUtil::toNativeMatrix( view );
        const auto nativeProjection = ClawUtil::toNativeMatrix( projection );
        const auto nativeCurrent = ClawUtil::toNativeMatrix( viewProjection );
        const auto nativePrevious =
            ClawUtil::toNativeMatrix( m_hasPreviousViewProjection ? m_previousViewProjection : viewProjection );
        const auto nativeInverse = ClawUtil::toNativeMatrix( inverseViewProjection );

        wp_pipeline_set_view_proj( m_nativePipeline, &nativeCurrent, &nativePrevious, &nativeInverse );
        wp_pipeline_set_frame_inputs( m_nativePipeline, m_hdrInput.data(), m_normalInput.data(),
                                 m_depthInput.data(), m_velocityInput.data() );

        void *nativeScene = nullptr;
        if( auto clawScene = dynamic_cast<ClawScene *>( getScene() ) )
        {
            nativeScene = clawScene->getNativeScene();
        }
        wp_pipeline_render_frame( m_nativePipeline, nativeScene, &nativeView, &nativeProjection, nullptr, 0,
                             nullptr );

        s32 width = 0;
        s32 height = 0;
        const auto *rgb = wp_pipeline_get_output( m_nativePipeline, &width, &height );
        if( rgb && width > 0 && height > 0 )
        {
            const auto pixelCount = static_cast<size_t>( width ) * height;
            m_rgbaOutput.resize( pixelCount * 4u );
            for( size_t pixel = 0; pixel < pixelCount; ++pixel )
            {
                m_rgbaOutput[pixel * 4u + 0u] = rgb[pixel * 3u + 0u];
                m_rgbaOutput[pixel * 4u + 1u] = rgb[pixel * 3u + 1u];
                m_rgbaOutput[pixel * 4u + 2u] = rgb[pixel * 3u + 2u];
                m_rgbaOutput[pixel * 4u + 3u] = 1.0f;
            }
        }

        m_previousViewProjection = viewProjection;
        m_hasPreviousViewProjection = true;
        m_hasFrameInputs = false;
    }

    bool ClawGraphicsPipeline::captureSoftwareFrame( wp_renderer *renderer )
    {
        auto software = renderer ? wp_renderer_get_software( renderer ) : nullptr;
        if( !software || !isInitialized() )
            return false;

        const auto width = wp_renderer_software_get_width( software );
        const auto height = wp_renderer_software_get_height( software );
        const auto *pixels =
            static_cast<const wp_u8 *>( wp_renderer_software_get_framebuffer( software ) );
        const auto *depth = wp_renderer_software_get_depth_buffer( software );
        if( !pixels || !depth || width != getWidth() || height != getHeight() )
            return false;

        const auto format = wp_renderer_software_get_pixel_format( software );
        const size_t pixelCount = static_cast<size_t>( width ) * height;
        const size_t stride =
            format == WORKPHONE_PIXEL_FORMAT_RGB8 || format == WORKPHONE_PIXEL_FORMAT_BGR8 ? 3u : 4u;
        m_hdrInput.resize( pixelCount * 3u );
        m_normalInput.resize( pixelCount * 4u );
        m_depthInput.resize( pixelCount );
        m_velocityInput.resize( pixelCount * 2u, 0.0f );

        for( size_t i = 0; i < pixelCount; ++i )
        {
            const bool bgra =
                format == WORKPHONE_PIXEL_FORMAT_BGRA8 || format == WORKPHONE_PIXEL_FORMAT_BGR8;
            const auto r = pixels[i * stride + ( bgra ? 2u : 0u )] / 255.0f;
            const auto g = pixels[i * stride + 1u] / 255.0f;
            const auto b = pixels[i * stride + ( bgra ? 0u : 2u )] / 255.0f;
            m_hdrInput[i * 3u + 0u] = wp_hdr_srgb_to_linear( r );
            m_hdrInput[i * 3u + 1u] = wp_hdr_srgb_to_linear( g );
            m_hdrInput[i * 3u + 2u] = wp_hdr_srgb_to_linear( b );
            m_normalInput[i * 4u + 0u] = 0.5f;
            m_normalInput[i * 4u + 1u] = 0.5f;
            m_normalInput[i * 4u + 2u] = 0.0f;  // No normal/coverage attachment is available.
            m_normalInput[i * 4u + 3u] = 1.0f;
            m_depthInput[i] = depth[i];
        }
        m_hasFrameInputs = true;
        return true;
    }

    bool ClawGraphicsPipeline::presentSoftwareFrame( wp_renderer *renderer ) const
    {
        auto software = renderer ? wp_renderer_get_software( renderer ) : nullptr;
        if( !software || m_rgbaOutput.empty() )
            return false;
        const auto format = wp_renderer_software_get_pixel_format( software );
        const size_t pixelCount = static_cast<size_t>( getWidth() ) * getHeight();
        const size_t stride =
            format == WORKPHONE_PIXEL_FORMAT_RGB8 || format == WORKPHONE_PIXEL_FORMAT_BGR8 ? 3u : 4u;
        m_ldrPresentation.resize( pixelCount * stride );
        for( size_t i = 0; i < pixelCount; ++i )
        {
            const bool bgra =
                format == WORKPHONE_PIXEL_FORMAT_BGRA8 || format == WORKPHONE_PIXEL_FORMAT_BGR8;
            const auto r =
                static_cast<u8>( std::clamp( m_rgbaOutput[i * 4u], 0.0f, 1.0f ) * 255.0f + 0.5f );
            const auto g =
                static_cast<u8>( std::clamp( m_rgbaOutput[i * 4u + 1u], 0.0f, 1.0f ) * 255.0f + 0.5f );
            const auto b =
                static_cast<u8>( std::clamp( m_rgbaOutput[i * 4u + 2u], 0.0f, 1.0f ) * 255.0f + 0.5f );
            m_ldrPresentation[i * stride + ( bgra ? 2u : 0u )] = r;
            m_ldrPresentation[i * stride + 1u] = g;
            m_ldrPresentation[i * stride + ( bgra ? 0u : 2u )] = b;
            if( stride == 4u )
                m_ldrPresentation[i * stride + 3u] = 255u;
        }
        return wp_renderer_software_set_framebuffer( software, m_ldrPresentation.data(), format ) != 0;
    }

    void ClawGraphicsPipeline::setQualityLevel( QualityLevel level )
    {
        GraphicsPipeline::setQualityLevel( level );
        applyNativeSettings();
    }

    void ClawGraphicsPipeline::setTaaSettings( const TaaSettings &settings )
    {
        GraphicsPipeline::setTaaSettings( settings );
        if( m_nativePipeline )
            wp_pipeline_set_taa_params( m_nativePipeline, settings.m_feedback );
    }

    void ClawGraphicsPipeline::setTaoaSettings( const TaoaSettings &settings )
    {
        GraphicsPipeline::setTaoaSettings( settings );
        if( m_nativePipeline )
            wp_pipeline_set_ao_params( m_nativePipeline, settings.m_radius, settings.m_intensity );
    }

    void ClawGraphicsPipeline::setHdrSettings( const HdrSettings &settings )
    {
        GraphicsPipeline::setHdrSettings( settings );
        if( m_nativePipeline )
            wp_pipeline_set_bloom_params( m_nativePipeline, settings.m_bloomThreshold,
                                     settings.m_bloomIntensity );
    }

    void ClawGraphicsPipeline::setCsmSettings( const CsmSettings &settings )
    {
        GraphicsPipeline::setCsmSettings( settings );
        if( m_nativePipeline )
            wp_pipeline_set_shadow_quality( m_nativePipeline, settings.m_cascadeCount,
                                       settings.m_shadowMapSize );
    }

#define WP_CLAW_TOGGLE( method, baseMethod, nativeMethod )     \
    void ClawGraphicsPipeline::method( bool enabled )          \
    {                                                          \
        GraphicsPipeline::baseMethod( enabled );               \
        if( m_nativePipeline )                                 \
            nativeMethod( m_nativePipeline, enabled ? 1 : 0 ); \
    }

    WP_CLAW_TOGGLE( enableTAA, enableTAA, wp_pipeline_enable_taa )
    WP_CLAW_TOGGLE( enableGTAO, enableGTAO, wp_pipeline_enable_ao )
    WP_CLAW_TOGGLE( enableSSR, enableSSR, wp_pipeline_enable_ssr )
    WP_CLAW_TOGGLE( enableContactShadows, enableContactShadows, wp_pipeline_enable_contact_shadows )
    WP_CLAW_TOGGLE( enableDOF, enableDOF, wp_pipeline_enable_dof )
    WP_CLAW_TOGGLE( enableBloom, enableBloom, wp_pipeline_enable_bloom )
    WP_CLAW_TOGGLE( enableExposure, enableExposure, wp_pipeline_enable_exposure )
    WP_CLAW_TOGGLE( enableMotionBlur, enableMotionBlur, wp_pipeline_enable_motion_blur )
    WP_CLAW_TOGGLE( enableCSM, enableCSM, wp_pipeline_enable_shadows )

#undef WP_CLAW_TOGGLE

    const f32 *ClawGraphicsPipeline::getOutputBuffer( s32 *outWidth, s32 *outHeight ) const
    {
        if( outWidth )
            *outWidth = getWidth();
        if( outHeight )
            *outHeight = getHeight();
        return m_rgbaOutput.empty() ? nullptr : m_rgbaOutput.data();
    }

    const f32 *ClawGraphicsPipeline::getGBufferNormal() const
    {
        return m_nativePipeline ? wp_pipeline_get_gbuffer_normal( m_nativePipeline ) : nullptr;
    }

    const f32 *ClawGraphicsPipeline::getGBufferDepth() const
    {
        return m_nativePipeline ? wp_pipeline_get_gbuffer_depth( m_nativePipeline ) : nullptr;
    }

    const f32 *ClawGraphicsPipeline::getGBufferVelocity() const
    {
        return m_nativePipeline ? wp_pipeline_get_gbuffer_velocity( m_nativePipeline ) : nullptr;
    }

    void ClawGraphicsPipeline::setDebugView( DebugView view )
    {
        GraphicsPipeline::setDebugView( view );
        if( m_nativePipeline )
            wp_pipeline_set_debug_view( m_nativePipeline, ClawUtil::toNativeDebugView( view ) );
    }

    void ClawGraphicsPipeline::applyNativeSettings()
    {
        if( !m_nativePipeline )
            return;

        enableTAA( isTAAEnabled() );
        enableGTAO( isGTAOEnabled() );
        enableSSR( isSSREnabled() );
        enableContactShadows( isContactShadowsEnabled() );
        enableDOF( isDOFEnabled() );
        enableBloom( isBloomEnabled() );
        enableExposure( isExposureEnabled() );
        enableMotionBlur( isMotionBlurEnabled() );
        enableCSM( isCSMEnabled() );
        setTaaSettings( getTaaSettings() );
        setTaoaSettings( getTaoaSettings() );
        setHdrSettings( getHdrSettings() );
        setCsmSettings( getCsmSettings() );
        setDebugView( getDebugView() );
    }

}  // namespace workphone::render
