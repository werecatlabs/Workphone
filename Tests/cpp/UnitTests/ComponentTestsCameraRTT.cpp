#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <Workphone/Graphics/RenderTexture.hpp>
#include <Workphone/Graphics/Texture.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class RecordingTargetCamera : public render::GraphicsCamera
    {
    public:
        SmartPtr<render::IGraphicsObject> clone( const String & ) const override
        {
            return nullptr;
        }

        SmartPtr<render::ITexture> getTargetTexture() const override
        {
            return targetTexture;
        }

        void setTargetTexture( SmartPtr<render::ITexture> texture ) override
        {
            targetTexture = texture;
            ++targetSetCount;
        }

        SmartPtr<render::ITexture> targetTexture;
        u32 targetSetCount = 0;
    };

    class RecordingRenderTarget : public render::RenderTexture
    {
    public:
        void update() override
        {
            ++updateCount;
        }

        void setAutoUpdated( bool value ) override
        {
            autoUpdated = value;
            ++autoUpdateSetCount;
        }

        bool autoUpdated = true;
        u32 updateCount = 0;
        u32 autoUpdateSetCount = 0;
    };

    template <class T>
    void checkProperty( const SmartPtr<Properties> &properties, const String &name, const T &expected )
    {
        BOOST_REQUIRE_MESSAGE( properties->hasProperty( name ),
                               "Missing render-texture property: " << name );
        T actual{};
        BOOST_REQUIRE( properties->getPropertyValue( name, actual ) );
        BOOST_CHECK( actual == expected );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentCameraRenderTextureTests )

BOOST_AUTO_TEST_CASE( render_texture_default_contract_and_property_keys_are_stable )
{
    TestGuard guard;

    auto renderTexture = workphone::make_ptr<scene::RenderTexture>();
    BOOST_REQUIRE( renderTexture );

    BOOST_CHECK( !renderTexture->isLoaded() );
    BOOST_CHECK_EQUAL( renderTexture->getWidth(), 512u );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 512u );
    BOOST_CHECK( renderTexture->getFormat() == PixelFormat::PF_R8G8B8A8 );
    BOOST_CHECK( renderTexture->getTextureName().empty() );
    BOOST_CHECK( renderTexture->getAutoUpdate() );
    BOOST_CHECK( !renderTexture->getRenderTexture() );
    BOOST_CHECK( !renderTexture->getTexture() );

    BOOST_CHECK_EQUAL( scene::RenderTexture::widthStr, "width" );
    BOOST_CHECK_EQUAL( scene::RenderTexture::heightStr, "height" );
    BOOST_CHECK_EQUAL( scene::RenderTexture::formatStr, "format" );
    BOOST_CHECK_EQUAL( scene::RenderTexture::textureNameStr, "textureName" );
    BOOST_CHECK_EQUAL( scene::RenderTexture::autoUpdateStr, "autoUpdate" );
    BOOST_CHECK_EQUAL( scene::RenderTexture::updateStr, "Update" );
    BOOST_CHECK_EQUAL( Camera::s_outputRenderTextureStr, "outputRenderTexture" );
}

BOOST_AUTO_TEST_CASE( render_texture_configuration_round_trips_and_rejects_zero_dimensions )
{
    TestGuard guard;

    auto renderTexture = workphone::make_ptr<scene::RenderTexture>();
    BOOST_REQUIRE( renderTexture );

    renderTexture->setWidth( 2048u );
    renderTexture->setHeight( 1024u );
    renderTexture->setFormat( PixelFormat::PF_FLOAT16_RGBA );
    renderTexture->setTextureName( "UnitTests.Camera.HDR" );
    renderTexture->setAutoUpdate( false );

    BOOST_CHECK_EQUAL( renderTexture->getWidth(), 2048u );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 1024u );
    BOOST_CHECK( renderTexture->getFormat() == PixelFormat::PF_FLOAT16_RGBA );
    BOOST_CHECK_EQUAL( renderTexture->getTextureName(), "UnitTests.Camera.HDR" );
    BOOST_CHECK( !renderTexture->getAutoUpdate() );

    renderTexture->setWidth( 0u );
    renderTexture->setHeight( 0u );
    BOOST_CHECK_EQUAL( renderTexture->getWidth(), 2048u );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 1024u );

    renderTexture->setWidth( std::numeric_limits<u32>::max() );
    renderTexture->setHeight( 1u );
    BOOST_CHECK_EQUAL( renderTexture->getWidth(), std::numeric_limits<u32>::max() );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 1u );
}

BOOST_AUTO_TEST_CASE( render_texture_properties_serialize_and_restore_the_complete_contract )
{
    TestGuard guard;

    auto source = workphone::make_ptr<scene::RenderTexture>();
    auto restored = workphone::make_ptr<scene::RenderTexture>();
    BOOST_REQUIRE( source );
    BOOST_REQUIRE( restored );

    source->setWidth( 1600u );
    source->setHeight( 900u );
    source->setFormat( PixelFormat::PF_B8G8R8A8 );
    source->setTextureName( "UnitTests.Camera.Output" );
    source->setAutoUpdate( false );

    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );
    checkProperty( properties, scene::RenderTexture::widthStr, 1600u );
    checkProperty( properties, scene::RenderTexture::heightStr, 900u );
    checkProperty( properties, scene::RenderTexture::formatStr,
                   static_cast<s32>( PixelFormat::PF_B8G8R8A8 ) );
    checkProperty( properties, scene::RenderTexture::textureNameStr,
                   String( "UnitTests.Camera.Output" ) );
    checkProperty( properties, scene::RenderTexture::autoUpdateStr, false );
    BOOST_CHECK( properties->hasProperty( scene::RenderTexture::updateStr ) );
    BOOST_CHECK( !properties->isButtonPressed( scene::RenderTexture::updateStr ) );

    restored->setProperties( properties );
    BOOST_CHECK_EQUAL( restored->getWidth(), 1600u );
    BOOST_CHECK_EQUAL( restored->getHeight(), 900u );
    BOOST_CHECK( restored->getFormat() == PixelFormat::PF_B8G8R8A8 );
    BOOST_CHECK_EQUAL( restored->getTextureName(), "UnitTests.Camera.Output" );
    BOOST_CHECK( !restored->getAutoUpdate() );
}

BOOST_AUTO_TEST_CASE( render_texture_partial_and_zero_properties_are_handled_defensively )
{
    TestGuard guard;

    auto renderTexture = workphone::make_ptr<scene::RenderTexture>();
    BOOST_REQUIRE( renderTexture );
    renderTexture->setWidth( 800u );
    renderTexture->setHeight( 600u );
    renderTexture->setTextureName( "PreservedName" );
    renderTexture->setAutoUpdate( false );

    auto partial = workphone::make_ptr<Properties>();
    partial->setProperty( scene::RenderTexture::widthStr, 320u );
    renderTexture->setProperties( partial );
    BOOST_CHECK_EQUAL( renderTexture->getWidth(), 320u );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 600u );
    BOOST_CHECK_EQUAL( renderTexture->getTextureName(), "PreservedName" );
    BOOST_CHECK( !renderTexture->getAutoUpdate() );

    auto invalid = workphone::make_ptr<Properties>();
    invalid->setProperty( scene::RenderTexture::widthStr, 0u );
    invalid->setProperty( scene::RenderTexture::heightStr, 0u );
    renderTexture->setProperties( invalid );
    BOOST_CHECK_EQUAL( renderTexture->getWidth(), 512u );
    BOOST_CHECK_EQUAL( renderTexture->getHeight(), 512u );
}

BOOST_AUTO_TEST_CASE( render_texture_forwards_name_auto_update_and_manual_update_to_resources )
{
    TestGuard guard;

    auto component = workphone::make_ptr<scene::RenderTexture>();
    auto texture = workphone::make_ptr<render::Texture>();
    auto target = workphone::make_ptr<RecordingRenderTarget>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( texture );
    BOOST_REQUIRE( target );

    component->setTexture( texture );
    component->setRenderTexture( target );
    component->setTextureName( "UnitTests.Camera.NamedTarget" );
    component->setAutoUpdate( false );
    component->update();
    component->update();

    BOOST_CHECK_EQUAL( texture->getName(), "UnitTests.Camera.NamedTarget" );
    BOOST_CHECK( !target->autoUpdated );
    BOOST_CHECK_EQUAL( target->autoUpdateSetCount, 1u );
    BOOST_CHECK_EQUAL( target->updateCount, 2u );

    component->setAutoUpdate( true );
    BOOST_CHECK( target->autoUpdated );
    BOOST_CHECK_EQUAL( target->autoUpdateSetCount, 2u );

    auto updateProperties = workphone::make_ptr<Properties>();
    updateProperties->setButtonPressed( scene::RenderTexture::updateStr, true );
    component->setProperties( updateProperties );
    BOOST_CHECK_EQUAL( target->updateCount, 3u );
}

BOOST_AUTO_TEST_CASE( render_texture_child_objects_and_unload_release_runtime_resources )
{
    TestGuard guard;

    auto component = workphone::make_ptr<scene::RenderTexture>();
    auto texture = workphone::make_ptr<render::Texture>();
    auto target = workphone::make_ptr<RecordingRenderTarget>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( texture );
    BOOST_REQUIRE( target );

    component->setTexture( texture );
    component->setRenderTexture( target );
    const auto children = component->getChildObjects();
    auto hasTexture = false;
    auto hasTarget = false;
    for( const auto &child : children )
    {
        hasTexture = hasTexture || child.get() == texture.get();
        hasTarget = hasTarget || child.get() == target.get();
    }
    BOOST_CHECK( hasTexture );
    BOOST_CHECK( hasTarget );

    // Avoid handing a synthetic texture to a real backend texture manager during teardown.
    component->setTexture( nullptr );
    component->unload( nullptr );
    BOOST_CHECK( !component->getTexture() );
    BOOST_CHECK( !component->getRenderTexture() );
    BOOST_CHECK( !component->isLoaded() );

    component->unload( nullptr );
    BOOST_CHECK( !component->getTexture() );
    BOOST_CHECK( !component->getRenderTexture() );
}

BOOST_AUTO_TEST_CASE( camera_output_render_texture_binds_immediately_and_can_be_cleared )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingTargetCamera>();
    auto output = workphone::make_ptr<scene::RenderTexture>();
    auto texture = workphone::make_ptr<render::Texture>();
    BOOST_REQUIRE( camera );
    BOOST_REQUIRE( renderCamera );
    BOOST_REQUIRE( output );
    BOOST_REQUIRE( texture );

    output->setTexture( texture );
    camera->setCamera( renderCamera );
    camera->setOutputRenderTexture( output );

    BOOST_CHECK( camera->getOutputRenderTexture().get() == output.get() );
    BOOST_CHECK( camera->getTargetTexture().get() == texture.get() );
    BOOST_CHECK( renderCamera->targetTexture.get() == texture.get() );
    BOOST_CHECK_EQUAL( renderCamera->targetSetCount, 1u );

    // Detach the backend double before clearing so the component does not try to create a
    // default-window viewport as part of switching away from an off-screen target.
    camera->setCamera( nullptr );
    camera->setOutputRenderTexture( nullptr );
    BOOST_CHECK( !camera->getOutputRenderTexture() );
    BOOST_CHECK( !camera->getTargetTexture() );
    BOOST_CHECK_EQUAL( renderCamera->targetSetCount, 1u );

    camera->unload( nullptr );
    renderCamera->unload( nullptr );
    output->unload( nullptr );
    texture->unload( nullptr );

    camera = nullptr;
    renderCamera = nullptr;
    output = nullptr;
    texture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_output_target_is_resynchronized_after_deferred_texture_creation )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingTargetCamera>();
    auto output = workphone::make_ptr<scene::RenderTexture>();
    auto texture = workphone::make_ptr<render::Texture>();
    BOOST_REQUIRE( camera );
    BOOST_REQUIRE( renderCamera );
    BOOST_REQUIRE( output );
    BOOST_REQUIRE( texture );

    camera->setOutputRenderTexture( output );
    camera->setCamera( renderCamera );
    BOOST_CHECK( !camera->getTargetTexture() );

    output->setTexture( texture );
    camera->updateTransform();
    BOOST_CHECK( camera->getTargetTexture().get() == texture.get() );
    BOOST_CHECK( renderCamera->targetTexture.get() == texture.get() );
    BOOST_CHECK_EQUAL( renderCamera->targetSetCount, 1u );

    camera->updateTransform();
    BOOST_CHECK_EQUAL( renderCamera->targetSetCount, 1u );

    camera->unload( nullptr );
    renderCamera->unload( nullptr );
    output->unload( nullptr );
    texture->unload( nullptr );

    camera = nullptr;
    renderCamera = nullptr;
    output = nullptr;
    texture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_output_render_texture_properties_round_trip_with_resource_metadata )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.sceneManager );
    BOOST_REQUIRE( guard.scene );

    auto source = workphone::make_ptr<Camera>();
    auto restored = workphone::make_ptr<Camera>();
    auto output = workphone::make_ptr<scene::RenderTexture>();
    auto texture = workphone::make_ptr<render::Texture>();
    BOOST_REQUIRE( source );
    BOOST_REQUIRE( restored );
    BOOST_REQUIRE( output );
    BOOST_REQUIRE( texture );

    // Component references are serialized by UUID and resolved through the current scene.
    auto outputActor = guard.sceneManager->createActor();
    BOOST_REQUIRE( outputActor );
    outputActor->addComponentInstance( output );
    guard.scene->addActor( outputActor );

    output->setTexture( texture );
    source->setOutputRenderTexture( output );
    auto properties = source->getProperties();
    BOOST_REQUIRE( properties );
    BOOST_CHECK( properties->hasProperty( Camera::s_outputRenderTextureStr ) );
    BOOST_CHECK_EQUAL(
        properties->getPropertyObject( Camera::s_outputRenderTextureStr ).getAttribute( "resourceType" ),
        "RenderTexture" );

    SmartPtr<IComponent> serializedComponent;
    BOOST_REQUIRE(
        properties->getPropertyValue( Camera::s_outputRenderTextureStr, serializedComponent ) );
    BOOST_CHECK( serializedComponent.get() == output.get() );

    restored->setProperties( properties );
    BOOST_CHECK( restored->getOutputRenderTexture().get() == output.get() );
    BOOST_CHECK( restored->getTargetTexture().get() == texture.get() );

    source->unload( nullptr );
    restored->unload( nullptr );
    output->unload( nullptr );
    texture->unload( nullptr );

    source = nullptr;
    restored = nullptr;
    output = nullptr;
    texture = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_output_render_texture_strongly_referenced )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( camera );

    {
        auto output = workphone::make_ptr<scene::RenderTexture>();
        camera->setOutputRenderTexture( output );
        BOOST_CHECK( camera->getOutputRenderTexture().get() == output.get() );
    }

    // Camera maintains strong reference - RenderTexture stays valid after scope ends
    BOOST_CHECK( camera->getOutputRenderTexture() );

    // Clear the reference explicitly to release it
    camera->setOutputRenderTexture( nullptr );
    BOOST_CHECK( !camera->getOutputRenderTexture() );

    camera->unload( nullptr );
    camera = nullptr;
}

BOOST_AUTO_TEST_SUITE_END()
