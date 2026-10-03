#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    using BlendMode = render::IGraphicsCamera::CompositeBlendMode;
    using CompositeLayer = render::IGraphicsCamera::CompositeLayer;
    using PostProcessSettings = render::IGraphicsCamera::PostProcessSettings;

    class RecordingCompositeCamera : public render::GraphicsCamera
    {
    public:
        SmartPtr<render::IGraphicsObject> clone( const String & ) const override
        {
            return nullptr;
        }

        PostProcessSettings getPostProcessSettings() const override
        {
            return postProcessSettings;
        }

        void setPostProcessSettings( const PostProcessSettings &settings ) override
        {
            postProcessSettings = settings;
            ++postProcessSetCount;
        }

        Array<CompositeLayer> getCompositeLayers() const override
        {
            return compositeLayers;
        }

        void setCompositeLayers( const Array<CompositeLayer> &layers ) override
        {
            compositeLayers = layers;
            ++compositeSetCount;
        }

        PostProcessSettings postProcessSettings;
        Array<CompositeLayer> compositeLayers;
        u32 postProcessSetCount = 0;
        u32 compositeSetCount = 0;
    };

    SmartPtr<scene::RenderTexture> makeLayer( SmartPtr<render::ITexture> texture )
    {
        auto layer = workphone::make_ptr<scene::RenderTexture>();
        layer->setTexture( texture );
        return layer;
    }

    void checkPostProcessSettings( const PostProcessSettings &actual,
                                   const PostProcessSettings &expected )
    {
        BOOST_CHECK_EQUAL( actual.enabled, expected.enabled );
        BOOST_CHECK_EQUAL( actual.fxaa, expected.fxaa );
        BOOST_CHECK_EQUAL( actual.bloom, expected.bloom );
        BOOST_CHECK_EQUAL( actual.workspace, expected.workspace );
        BOOST_CHECK_CLOSE( actual.exposure, expected.exposure, 0.001f );
        BOOST_CHECK_CLOSE( actual.gamma, expected.gamma, 0.001f );
        BOOST_CHECK_CLOSE( actual.contrast, expected.contrast, 0.001f );
        BOOST_CHECK_CLOSE( actual.saturation, expected.saturation, 0.001f );
        BOOST_CHECK_CLOSE( actual.bloomIntensity, expected.bloomIntensity, 0.001f );
        BOOST_CHECK_CLOSE( actual.bloomThreshold, expected.bloomThreshold, 0.001f );
        BOOST_CHECK_CLOSE( actual.vignette, expected.vignette, 0.001f );
    }

    PostProcessSettings makeNonDefaultPostProcessSettings()
    {
        PostProcessSettings settings;
        settings.enabled = true;
        settings.fxaa = true;
        settings.bloom = true;
        settings.exposure = 1.25f;
        settings.gamma = 1.9f;
        settings.contrast = 1.15f;
        settings.saturation = 0.85f;
        settings.bloomIntensity = 2.5f;
        settings.bloomThreshold = 0.65f;
        settings.vignette = 0.3f;
        settings.workspace = "UnitTests/Camera/PostProcessWorkspace";
        return settings;
    }

    String layerPropertyName( u32 index, const String &suffix )
    {
        return Camera::s_compositeLayerPrefix + StringUtil::toString( index ) + suffix;
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentCameraCompositeTests )

BOOST_AUTO_TEST_CASE( camera_post_processing_and_compositing_defaults_are_deterministic )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( camera );

    const auto settings = camera->getPostProcessSettings();
    BOOST_CHECK( !settings.enabled );
    BOOST_CHECK( !settings.fxaa );
    BOOST_CHECK( !settings.bloom );
    BOOST_CHECK_EQUAL( settings.exposure, 0.0f );
    BOOST_CHECK_EQUAL( settings.gamma, 2.2f );
    BOOST_CHECK_EQUAL( settings.contrast, 1.0f );
    BOOST_CHECK_EQUAL( settings.saturation, 1.0f );
    BOOST_CHECK_EQUAL( settings.bloomIntensity, 0.0f );
    BOOST_CHECK_EQUAL( settings.bloomThreshold, 1.0f );
    BOOST_CHECK_EQUAL( settings.vignette, 0.0f );
    BOOST_CHECK( settings.workspace.empty() );
    BOOST_CHECK( camera->getCompositeLayers().empty() );

    BOOST_CHECK_EQUAL( Camera::s_postProcessEnabledStr, "postProcessing.enabled" );
    BOOST_CHECK_EQUAL( Camera::s_postProcessWorkspaceStr, "postProcessing.workspace" );
    BOOST_CHECK_EQUAL( Camera::s_fxaaStr, "postProcessing.fxaa" );
    BOOST_CHECK_EQUAL( Camera::s_bloomStr, "postProcessing.bloom" );
    BOOST_CHECK_EQUAL( Camera::s_exposureStr, "postProcessing.exposure" );
    BOOST_CHECK_EQUAL( Camera::s_gammaStr, "postProcessing.gamma" );
    BOOST_CHECK_EQUAL( Camera::s_contrastStr, "postProcessing.contrast" );
    BOOST_CHECK_EQUAL( Camera::s_saturationStr, "postProcessing.saturation" );
    BOOST_CHECK_EQUAL( Camera::s_bloomIntensityStr, "postProcessing.bloomIntensity" );
    BOOST_CHECK_EQUAL( Camera::s_bloomThresholdStr, "postProcessing.bloomThreshold" );
    BOOST_CHECK_EQUAL( Camera::s_vignetteStr, "postProcessing.vignette" );
    BOOST_CHECK_EQUAL( Camera::s_compositeLayerPrefix, "compositing.layer" );

    camera->unload( nullptr );
    camera = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_post_process_settings_round_trip_and_propagate_to_render_camera )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingCompositeCamera>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( renderCamera );
    component->setCamera( renderCamera );

    const auto expected = makeNonDefaultPostProcessSettings();
    component->setPostProcessSettings( expected );

    checkPostProcessSettings( component->getPostProcessSettings(), expected );
    checkPostProcessSettings( renderCamera->postProcessSettings, expected );
    BOOST_CHECK_EQUAL( renderCamera->postProcessSetCount, 1u );

    auto reset = PostProcessSettings{};
    component->setPostProcessSettings( reset );
    checkPostProcessSettings( component->getPostProcessSettings(), reset );
    checkPostProcessSettings( renderCamera->postProcessSettings, reset );
    BOOST_CHECK_EQUAL( renderCamera->postProcessSetCount, 2u );

    renderCamera->unload( nullptr );
    component->unload( nullptr );

    renderCamera = nullptr;
    component = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_post_process_properties_serialize_and_restore_every_field )
{
    TestGuard guard;

    auto source = workphone::make_ptr<Camera>();
    auto restored = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( source );
    BOOST_REQUIRE( restored );

    const auto expected = makeNonDefaultPostProcessSettings();
    source->setPostProcessSettings( expected );
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );

    bool boolValue = false;
    f32 floatValue = 0.0f;
    String stringValue;
    BOOST_REQUIRE( properties->getPropertyValue( Camera::s_postProcessEnabledStr, boolValue ) );
    BOOST_CHECK( boolValue );
    BOOST_REQUIRE( properties->getPropertyValue( Camera::s_fxaaStr, boolValue ) );
    BOOST_CHECK( boolValue );
    BOOST_REQUIRE( properties->getPropertyValue( Camera::s_bloomStr, boolValue ) );
    BOOST_CHECK( boolValue );
    BOOST_REQUIRE( properties->getPropertyValue( Camera::s_postProcessWorkspaceStr, stringValue ) );
    BOOST_CHECK_EQUAL( stringValue, expected.workspace );

    const Pair<String, f32> floatProperties[] = {
        { Camera::s_exposureStr, expected.exposure },
        { Camera::s_gammaStr, expected.gamma },
        { Camera::s_contrastStr, expected.contrast },
        { Camera::s_saturationStr, expected.saturation },
        { Camera::s_bloomIntensityStr, expected.bloomIntensity },
        { Camera::s_bloomThresholdStr, expected.bloomThreshold },
        { Camera::s_vignetteStr, expected.vignette }
    };
    for( const auto &entry : floatProperties )
    {
        BOOST_REQUIRE( properties->getPropertyValue( entry.first, floatValue ) );
        BOOST_CHECK_CLOSE( floatValue, entry.second, 0.001f );
    }

    restored->setProperties( properties );
    checkPostProcessSettings( restored->getPostProcessSettings(), expected );

    source->unload( nullptr );
    restored->unload( nullptr );

    source = nullptr;
    restored = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_layers_are_emitted_in_slot_order_with_all_metadata )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto texture1 = workphone::make_ptr<render::Texture>();
    auto texture3 = workphone::make_ptr<render::Texture>();
    auto layer1 = makeLayer( texture1 );
    auto layer3 = makeLayer( texture3 );
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( texture1 );
    BOOST_REQUIRE( texture3 );
    BOOST_REQUIRE( layer1 );
    BOOST_REQUIRE( layer3 );

    component->setCompositeRenderTexture( 3u, layer3, BlendMode::Screen, 0.8f, false );
    component->setCompositeRenderTexture( 1u, layer1, BlendMode::Add, 0.35f, true );

    const auto layers = component->getCompositeLayers();
    BOOST_REQUIRE_EQUAL( layers.size(), 2u );
    BOOST_CHECK( layers[0].texture.get() == texture1.get() );
    BOOST_CHECK( layers[0].blendMode == BlendMode::Add );
    BOOST_CHECK_CLOSE( layers[0].opacity, 0.35f, 0.001f );
    BOOST_CHECK( layers[0].enabled );
    BOOST_CHECK( layers[1].texture.get() == texture3.get() );
    BOOST_CHECK( layers[1].blendMode == BlendMode::Screen );
    BOOST_CHECK_CLOSE( layers[1].opacity, 0.8f, 0.001f );
    BOOST_CHECK( !layers[1].enabled );

    component->unload( nullptr );
    texture1->unload( nullptr );
    texture3->unload( nullptr );

    component = nullptr;
    texture1 = nullptr;
    texture3 = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_opacity_is_clamped_and_invalid_slots_are_ignored )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto texture0 = workphone::make_ptr<render::Texture>();
    auto texture1 = workphone::make_ptr<render::Texture>();
    auto layer0 = makeLayer( texture0 );
    auto layer1 = makeLayer( texture1 );
    BOOST_REQUIRE( component );

    component->setCompositeRenderTexture( 0u, layer0, BlendMode::Replace, -10.0f );
    component->setCompositeRenderTexture( 1u, layer1, BlendMode::Multiply, 10.0f );
    component->setCompositeRenderTexture( 4u, layer1, BlendMode::Screen, 0.5f );
    component->setCompositeRenderTexture( std::numeric_limits<u32>::max(), layer1, BlendMode::Alpha,
                                          0.5f );

    const auto layers = component->getCompositeLayers();
    BOOST_REQUIRE_EQUAL( layers.size(), 2u );
    BOOST_CHECK_EQUAL( layers[0].opacity, 0.0f );
    BOOST_CHECK_EQUAL( layers[1].opacity, 1.0f );
    BOOST_CHECK( layers[0].blendMode == BlendMode::Replace );
    BOOST_CHECK( layers[1].blendMode == BlendMode::Multiply );

    component->unload( nullptr );
    texture0->unload( nullptr );
    texture1->unload( nullptr );

    component = nullptr;
    texture0 = nullptr;
    texture1 = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_supports_every_blend_mode )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto texture = workphone::make_ptr<render::Texture>();
    auto layer = makeLayer( texture );
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( layer );

    const BlendMode modes[] = { BlendMode::Replace, BlendMode::Alpha, BlendMode::Add,
                                BlendMode::Multiply, BlendMode::Screen };
    for( const auto mode : modes )
    {
        component->setCompositeRenderTexture( 0u, layer, mode, 1.0f );
        const auto layers = component->getCompositeLayers();
        BOOST_REQUIRE_EQUAL( layers.size(), 1u );
        BOOST_CHECK( layers[0].blendMode == mode );
    }

    component->unload( nullptr );
    texture->unload( nullptr );

    component = nullptr;
    texture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_layer_properties_round_trip_and_expose_editor_metadata )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.sceneManager );
    BOOST_REQUIRE( guard.scene );

    auto source = workphone::make_ptr<Camera>();
    auto restored = workphone::make_ptr<Camera>();
    auto texture = workphone::make_ptr<render::Texture>();
    auto renderTexture = makeLayer( texture );
    BOOST_REQUIRE( source );
    BOOST_REQUIRE( restored );
    BOOST_REQUIRE( renderTexture );

    // Component references are serialized by UUID and resolved through the current scene.
    auto renderTextureActor = guard.sceneManager->createActor();
    BOOST_REQUIRE( renderTextureActor );
    renderTextureActor->addComponentInstance( renderTexture );
    guard.scene->addActor( renderTextureActor );

    source->setCompositeRenderTexture( 2u, renderTexture, BlendMode::Multiply, 0.42f, true );
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );

    const auto textureProperty = layerPropertyName( 2u, ".renderTexture" );
    const auto blendProperty = layerPropertyName( 2u, ".blendMode" );
    const auto opacityProperty = layerPropertyName( 2u, ".opacity" );
    const auto enabledProperty = layerPropertyName( 2u, ".enabled" );
    BOOST_CHECK( properties->hasProperty( textureProperty ) );
    BOOST_CHECK( properties->hasProperty( blendProperty ) );
    BOOST_CHECK( properties->hasProperty( opacityProperty ) );
    BOOST_CHECK( properties->hasProperty( enabledProperty ) );
    BOOST_CHECK_EQUAL( properties->getPropertyObject( textureProperty ).getAttribute( "resourceType" ),
                       "RenderTexture" );

    s32 blendMode = -1;
    f32 opacity = 0.0f;
    bool enabled = false;
    BOOST_REQUIRE( properties->getPropertyValue( blendProperty, blendMode ) );
    BOOST_REQUIRE( properties->getPropertyValue( opacityProperty, opacity ) );
    BOOST_REQUIRE( properties->getPropertyValue( enabledProperty, enabled ) );
    BOOST_CHECK_EQUAL( blendMode, static_cast<s32>( BlendMode::Multiply ) );
    BOOST_CHECK_CLOSE( opacity, 0.42f, 0.001f );
    BOOST_CHECK( enabled );

    restored->setProperties( properties );
    const auto layers = restored->getCompositeLayers();
    BOOST_REQUIRE_EQUAL( layers.size(), 1u );
    BOOST_CHECK( layers[0].texture.get() == texture.get() );
    BOOST_CHECK( layers[0].blendMode == BlendMode::Multiply );
    BOOST_CHECK_CLOSE( layers[0].opacity, 0.42f, 0.001f );
    BOOST_CHECK( layers[0].enabled );

    source->unload( nullptr );
    restored->unload( nullptr );
    texture->unload( nullptr );

    source = nullptr;
    restored = nullptr;
    texture = nullptr;
    renderTexture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_changes_propagate_and_transform_update_repairs_stale_state )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingCompositeCamera>();
    auto firstTexture = workphone::make_ptr<render::Texture>();
    auto replacementTexture = workphone::make_ptr<render::Texture>();
    auto renderTexture = makeLayer( firstTexture );
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( renderCamera );
    BOOST_REQUIRE( renderTexture );

    component->setCamera( renderCamera );
    component->setCompositeRenderTexture( 0u, renderTexture, BlendMode::Alpha, 0.75f );
    BOOST_REQUIRE_EQUAL( renderCamera->compositeLayers.size(), 1u );
    BOOST_CHECK( renderCamera->compositeLayers[0].texture.get() == firstTexture.get() );
    BOOST_CHECK_EQUAL( renderCamera->compositeSetCount, 1u );

    renderTexture->setTexture( replacementTexture );
    component->updateTransform();
    BOOST_REQUIRE_EQUAL( renderCamera->compositeLayers.size(), 1u );
    BOOST_CHECK( renderCamera->compositeLayers[0].texture.get() == replacementTexture.get() );
    BOOST_CHECK_EQUAL( renderCamera->compositeSetCount, 2u );

    component->updateTransform();
    BOOST_CHECK_EQUAL( renderCamera->compositeSetCount, 2u );

    component->unload( nullptr );
    renderCamera->unload( nullptr );
    firstTexture->unload( nullptr );
    replacementTexture->unload( nullptr );

    component = nullptr;
    renderCamera = nullptr;
    firstTexture = nullptr;
    replacementTexture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_composite_uses_weak_component_references_and_allows_slot_clearing )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto texture = workphone::make_ptr<render::Texture>();
    BOOST_REQUIRE( component );

    /*
    {
        auto renderTexture = makeLayer( texture );
        component->setCompositeRenderTexture( 0u, renderTexture, BlendMode::Alpha, 1.0f );
        BOOST_REQUIRE_EQUAL( component->getCompositeLayers().size(), 1u );
        component->setCompositeRenderTexture( 0u, nullptr, BlendMode::Alpha, 1.0f );
        BOOST_CHECK( component->getCompositeLayers().empty() );
    }

    {
        auto renderTexture = makeLayer( texture );
        component->setCompositeRenderTexture( 1u, renderTexture, BlendMode::Add, 1.0f );
        BOOST_REQUIRE_EQUAL( component->getCompositeLayers().size(), 1u );
    }*/

    BOOST_CHECK( component->getCompositeLayers().empty() );

    component->unload( nullptr );
    texture->unload( nullptr );
    component = nullptr;
    texture = nullptr;    
}

BOOST_AUTO_TEST_SUITE_END()
