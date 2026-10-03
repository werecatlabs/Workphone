#include "UnitTests.hpp"
#include "TestGuard.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Graphics/GraphicsCamera.hpp>
#include <Workphone/Graphics/Viewport.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    class RecordingCamera : public render::GraphicsCamera
    {
    public:
        SmartPtr<render::IGraphicsObject> clone( const String & ) const override
        {
            return nullptr;
        }

        void setFOVy( f32 value ) override
        {
            fov = value;
            ++fovSetCount;
        }

        f32 getFOVy() const override
        {
            return fov;
        }

        void setNearClipDistance( f32 value ) override
        {
            nearClip = value;
            ++nearClipSetCount;
        }

        f32 getNearClipDistance() const override
        {
            return nearClip;
        }

        void setFarClipDistance( f32 value ) override
        {
            farClip = value;
            ++farClipSetCount;
        }

        f32 getFarClipDistance() const override
        {
            return farClip;
        }

        Ray3<real_Num> getRay( f32 screenX, f32 screenY ) const override
        {
            lastRayCoordinates = Vector2<real_Num>( screenX, screenY );
            ++rayQueryCount;
            return ray;
        }

        bool isObjectVisible( const AABB3<real_Num> & ) const override
        {
            ++visibilityQueryCount;
            return objectVisible;
        }

        f32 fov = 0.0f;
        f32 nearClip = 0.0f;
        f32 farClip = 0.0f;
        u32 fovSetCount = 0;
        u32 nearClipSetCount = 0;
        u32 farClipSetCount = 0;
        mutable Vector2<real_Num> lastRayCoordinates = Vector2<real_Num>::zero();
        mutable u32 rayQueryCount = 0;
        mutable u32 visibilityQueryCount = 0;
        bool objectVisible = false;
        Ray3<real_Num> ray =
            Ray3<real_Num>( Vector3<real_Num>( 1.0, 2.0, 3.0 ), Vector3<real_Num>( 0.0, 0.0, -1.0 ) );
    };

    class RecordingViewport : public render::Viewport
    {
    public:
        void _getObject( void **object ) const override
        {
            if( object )
            {
                *object = nullptr;
            }
        }

        void setBackgroundColour( const ColourF &value ) override
        {
            backgroundColour = value;
            ++backgroundSetCount;
        }

        void setZOrder( s32 value ) override
        {
            zOrder = value;
            ++zOrderSetCount;
        }

        void setOverlaysEnabled( bool value ) override
        {
            overlaysEnabled = value;
            ++overlaysSetCount;
        }

        void setAutoUpdated( bool value ) override
        {
            autoUpdated = value;
            ++autoUpdatedSetCount;
        }

        void setShadowsEnabled( bool value ) override
        {
            shadowsEnabled = value;
            ++shadowsSetCount;
        }

        void setVisibilityMask( u32 value ) override
        {
            visibilityMask = value;
            ++visibilityMaskSetCount;
        }

        void setEnableUI( bool value ) override
        {
            enableUI = value;
            ++enableUISetCount;
        }

        void setEnableSceneRender( bool value ) override
        {
            enableSceneRender = value;
            ++enableSceneRenderSetCount;
        }

        void setClearEveryFrame( bool value, u32 buffers ) override
        {
            clearEveryFrame = value;
            clearBuffers = buffers;
            ++clearSetCount;
        }

        ColourF backgroundColour;
        s32 zOrder = 0;
        u32 visibilityMask = 0;
        u32 clearBuffers = 0;
        bool overlaysEnabled = false;
        bool autoUpdated = false;
        bool shadowsEnabled = false;
        bool enableUI = false;
        bool enableSceneRender = false;
        bool clearEveryFrame = false;
        u32 backgroundSetCount = 0;
        u32 zOrderSetCount = 0;
        u32 overlaysSetCount = 0;
        u32 autoUpdatedSetCount = 0;
        u32 shadowsSetCount = 0;
        u32 visibilityMaskSetCount = 0;
        u32 enableUISetCount = 0;
        u32 enableSceneRenderSetCount = 0;
        u32 clearSetCount = 0;
    };

    template <class T>
    void requireProperty( const SmartPtr<Properties> &properties, const String &name, const T &expected )
    {
        BOOST_REQUIRE_MESSAGE( properties->hasProperty( name ), "Missing camera property: " << name );
        T actual{};
        BOOST_REQUIRE( properties->getPropertyValue( name, actual ) );
        BOOST_CHECK( actual == expected );
    }

    void checkColour( const ColourF &actual, const ColourF &expected )
    {
        BOOST_CHECK_CLOSE( actual.r, expected.r, 0.001f );
        BOOST_CHECK_CLOSE( actual.g, expected.g, 0.001f );
        BOOST_CHECK_CLOSE( actual.b, expected.b, 0.001f );
        BOOST_CHECK_CLOSE( actual.a, expected.a, 0.001f );
    }
}  // namespace

BOOST_AUTO_TEST_SUITE( ComponentCameraTests )

BOOST_AUTO_TEST_CASE( camera_default_contract_and_property_keys_are_stable )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( camera );

    BOOST_CHECK( !camera->isLoaded() );
    BOOST_CHECK( !camera->isActive() );
    BOOST_CHECK_EQUAL( camera->getZOrder(), 0u );
    BOOST_CHECK_EQUAL( camera->getFOV(), 45.0f );
    BOOST_CHECK_EQUAL( camera->getNearClipDistance(), 0.1f );
    BOOST_CHECK_EQUAL( camera->getFarClipDistance(), 1000.0f );
    BOOST_CHECK_EQUAL( camera->getOrthoWindowWidth(), 0.0f );
    BOOST_CHECK_EQUAL( camera->getOrthoWindowHeight(), 0.0f );
    BOOST_CHECK( !camera->getEnableShadows() );
    BOOST_CHECK( camera->getEnableSceneRender() );
    BOOST_CHECK( camera->getEnableUI() );
    BOOST_CHECK( camera->getClearEveryFrame() );
    BOOST_CHECK( camera->getOverlaysEnabled() );
    BOOST_CHECK( camera->getAutoUpdated() );
    BOOST_CHECK( !camera->getCamera() );
    BOOST_CHECK( !camera->getNode() );
    BOOST_CHECK( !camera->getViewport() );
    BOOST_CHECK( !camera->getTargetTexture() );

    BOOST_CHECK_EQUAL( Camera::s_zOrderStr, "zOrder" );
    BOOST_CHECK_EQUAL( Camera::s_isActiveStr, "isActive" );
    BOOST_CHECK_EQUAL( Camera::s_enableShadowsStr, "enableShadows" );
    BOOST_CHECK_EQUAL( Camera::s_visibilityMaskStr, "visibilityMask" );
    BOOST_CHECK_EQUAL( Camera::s_viewportBackgroundColourStr, "viewportBackgroundColour" );
    BOOST_CHECK_EQUAL( Camera::s_enableSceneRenderStr, "enableSceneRender" );
    BOOST_CHECK_EQUAL( Camera::s_enableUIStr, "enableUI" );
    BOOST_CHECK_EQUAL( Camera::s_clearEveryFrameStr, "clearEveryFrame" );
    BOOST_CHECK_EQUAL( Camera::s_overlaysEnabledStr, "overlaysEnabled" );
    BOOST_CHECK_EQUAL( Camera::s_autoUpdatedStr, "autoUpdated" );
    BOOST_CHECK_EQUAL( Camera::s_fovStr, "fov" );
    BOOST_CHECK_EQUAL( Camera::s_nearClipStr, "nearClip" );
    BOOST_CHECK_EQUAL( Camera::s_farClipStr, "farClip" );
    BOOST_CHECK_EQUAL( Camera::s_orthoWidthStr, "orthoWidth" );
    BOOST_CHECK_EQUAL( Camera::s_orthoHeightStr, "orthoHeight" );

    camera->unload( nullptr );
    camera = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_scalar_configuration_round_trips_without_runtime_objects )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( camera );

    const ColourF background( 0.15f, 0.25f, 0.35f, 0.45f );
    camera->setZOrder( 73u );
    camera->setEnableShadows( true );
    camera->setFOV( 87.5f );
    camera->setNearClipDistance( 0.025f );
    camera->setFarClipDistance( 25000.0f );
    camera->setOrthoWindowWidth( 1920.0f );
    camera->setOrthoWindowHeight( 1080.0f );
    camera->setViewportBackgroundColour( background );
    camera->setEnableSceneRender( false );
    camera->setEnableUI( false );
    camera->setClearEveryFrame( false );
    camera->setOverlaysEnabled( false );
    camera->setAutoUpdated( false );

    BOOST_CHECK_EQUAL( camera->getZOrder(), 73u );
    BOOST_CHECK( camera->getEnableShadows() );
    BOOST_CHECK_EQUAL( camera->getFOV(), 87.5f );
    BOOST_CHECK_EQUAL( camera->getNearClipDistance(), 0.025f );
    BOOST_CHECK_EQUAL( camera->getFarClipDistance(), 25000.0f );
    BOOST_CHECK_EQUAL( camera->getOrthoWindowWidth(), 1920.0f );
    BOOST_CHECK_EQUAL( camera->getOrthoWindowHeight(), 1080.0f );
    checkColour( camera->getViewportBackgroundColour(), background );
    BOOST_CHECK( !camera->getEnableSceneRender() );
    BOOST_CHECK( !camera->getEnableUI() );
    BOOST_CHECK( !camera->getClearEveryFrame() );
    BOOST_CHECK( !camera->getOverlaysEnabled() );
    BOOST_CHECK( !camera->getAutoUpdated() );

    camera->unload( nullptr );
    camera = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_properties_serialize_the_complete_core_contract )
{
    TestGuard guard;

    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( camera );

    const ColourF background( 0.2f, 0.4f, 0.6f, 0.8f );
    camera->setZOrder( 19u );
    camera->setEnableShadows( true );
    camera->setFOV( 61.0f );
    camera->setNearClipDistance( 0.2f );
    camera->setFarClipDistance( 4096.0f );
    camera->setOrthoWindowWidth( 24.0f );
    camera->setOrthoWindowHeight( 12.0f );
    camera->setViewportBackgroundColour( background );
    camera->setEnableSceneRender( false );
    camera->setEnableUI( false );
    camera->setClearEveryFrame( false );
    camera->setOverlaysEnabled( false );
    camera->setAutoUpdated( false );

    auto properties = camera->getProperties();
    BOOST_REQUIRE( properties );

    requireProperty( properties, Camera::s_zOrderStr, 19u );
    requireProperty( properties, Camera::s_isActiveStr, false );
    requireProperty( properties, Camera::s_enableShadowsStr, true );
    requireProperty( properties, Camera::s_visibilityMaskStr, std::numeric_limits<u32>::max() );
    requireProperty( properties, Camera::s_viewportBackgroundColourStr, background );
    requireProperty( properties, Camera::s_enableSceneRenderStr, false );
    requireProperty( properties, Camera::s_enableUIStr, false );
    requireProperty( properties, Camera::s_clearEveryFrameStr, false );
    requireProperty( properties, Camera::s_overlaysEnabledStr, false );
    requireProperty( properties, Camera::s_autoUpdatedStr, false );
    requireProperty( properties, Camera::s_fovStr, 61.0f );
    requireProperty( properties, Camera::s_nearClipStr, 0.2f );
    requireProperty( properties, Camera::s_farClipStr, 4096.0f );
    requireProperty( properties, Camera::s_orthoWidthStr, 24.0f );
    requireProperty( properties, Camera::s_orthoHeightStr, 12.0f );
    BOOST_CHECK( properties->hasProperty( Camera::s_resetStr ) );
    BOOST_CHECK( properties->hasProperty( Camera::s_updateActiveStateStr ) );

    camera->unload( nullptr );
    camera = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_properties_round_trip_and_partial_updates_preserve_other_values )
{
    TestGuard guard;

    auto source = workphone::make_ptr<Camera>();
    auto restored = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( source );
    BOOST_REQUIRE( restored );

    const ColourF background( 0.7f, 0.1f, 0.3f, 1.0f );
    source->setZOrder( 42u );
    source->setEnableShadows( true );
    source->setFOV( 72.0f );
    source->setNearClipDistance( 0.5f );
    source->setFarClipDistance( 8000.0f );
    source->setOrthoWindowWidth( 30.0f );
    source->setOrthoWindowHeight( 15.0f );
    source->setViewportBackgroundColour( background );
    source->setEnableSceneRender( false );
    source->setEnableUI( false );
    source->setClearEveryFrame( false );
    source->setOverlaysEnabled( false );
    source->setAutoUpdated( false );

    restored->setProperties( source->getProperties() );

    BOOST_CHECK_EQUAL( restored->getZOrder(), 42u );
    BOOST_CHECK( restored->getEnableShadows() );
    BOOST_CHECK_EQUAL( restored->getFOV(), 72.0f );
    BOOST_CHECK_EQUAL( restored->getNearClipDistance(), 0.5f );
    BOOST_CHECK_EQUAL( restored->getFarClipDistance(), 8000.0f );
    BOOST_CHECK_EQUAL( restored->getOrthoWindowWidth(), 30.0f );
    BOOST_CHECK_EQUAL( restored->getOrthoWindowHeight(), 15.0f );
    checkColour( restored->getViewportBackgroundColour(), background );
    BOOST_CHECK( !restored->getEnableSceneRender() );
    BOOST_CHECK( !restored->getEnableUI() );
    BOOST_CHECK( !restored->getClearEveryFrame() );
    BOOST_CHECK( !restored->getOverlaysEnabled() );
    BOOST_CHECK( !restored->getAutoUpdated() );

    restored->setProperties( nullptr );
    auto partial = workphone::make_ptr<Properties>();
    partial->setProperty( Camera::s_fovStr, 90.0f );
    restored->setProperties( partial );

    BOOST_CHECK_EQUAL( restored->getFOV(), 90.0f );
    BOOST_CHECK_EQUAL( restored->getZOrder(), 42u );
    BOOST_CHECK_EQUAL( restored->getNearClipDistance(), 0.5f );
    BOOST_CHECK_EQUAL( restored->getFarClipDistance(), 8000.0f );
    checkColour( restored->getViewportBackgroundColour(), background );

    source->unload( nullptr );
    restored->unload( nullptr );

    source = nullptr;
    restored = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_setters_propagate_to_an_existing_render_camera )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingCamera>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( renderCamera );

    component->setCamera( renderCamera );
    component->setFOV( 68.0f );
    component->setNearClipDistance( 0.15f );
    component->setFarClipDistance( 6000.0f );

    BOOST_CHECK( component->getCamera().get() == renderCamera.get() );
    BOOST_CHECK_EQUAL( renderCamera->fov, 68.0f );
    BOOST_CHECK_EQUAL( renderCamera->nearClip, 0.15f );
    BOOST_CHECK_EQUAL( renderCamera->farClip, 6000.0f );
    BOOST_CHECK_EQUAL( renderCamera->fovSetCount, 1u );
    BOOST_CHECK_EQUAL( renderCamera->nearClipSetCount, 1u );
    BOOST_CHECK_EQUAL( renderCamera->farClipSetCount, 1u );

    component->setFOV( 68.0f );
    component->setNearClipDistance( 0.15f );
    component->setFarClipDistance( 6000.0f );
    BOOST_CHECK_EQUAL( renderCamera->fovSetCount, 1u );
    BOOST_CHECK_EQUAL( renderCamera->nearClipSetCount, 1u );
    BOOST_CHECK_EQUAL( renderCamera->farClipSetCount, 1u );

    component->setCamera( nullptr );
    BOOST_CHECK( !component->getCamera() );

    component->unload( nullptr );
    renderCamera->unload( nullptr );

    component = nullptr;
    renderCamera = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_setters_and_properties_propagate_to_an_existing_viewport )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto viewport = workphone::make_ptr<RecordingViewport>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( viewport );

    component->setViewport( viewport );
    const ColourF background( 0.11f, 0.22f, 0.33f, 0.44f );
    component->setViewportBackgroundColour( background );
    component->setZOrder( 55u );
    component->setEnableShadows( true );
    component->setEnableSceneRender( false );
    component->setEnableUI( false );
    component->setClearEveryFrame( false );
    component->setOverlaysEnabled( false );
    component->setAutoUpdated( false );

    checkColour( viewport->backgroundColour, background );
    BOOST_CHECK_EQUAL( viewport->zOrder, 55 );
    BOOST_CHECK( viewport->shadowsEnabled );
    BOOST_CHECK( !viewport->enableSceneRender );
    BOOST_CHECK( !viewport->enableUI );
    BOOST_CHECK( !viewport->clearEveryFrame );
    BOOST_CHECK( !viewport->overlaysEnabled );
    BOOST_CHECK( !viewport->autoUpdated );

    auto properties = workphone::make_ptr<Properties>();
    properties->setProperty( Camera::s_visibilityMaskStr, 0x13579BDFu );
    properties->setProperty( Camera::s_enableSceneRenderStr, true );
    properties->setProperty( Camera::s_enableUIStr, true );
    properties->setProperty( Camera::s_clearEveryFrameStr, true );
    properties->setProperty( Camera::s_overlaysEnabledStr, true );
    properties->setProperty( Camera::s_autoUpdatedStr, true );
    properties->setProperty( Camera::s_enableShadowsStr, false );
    component->setProperties( properties );

    BOOST_CHECK_EQUAL( viewport->visibilityMask, 0x13579BDFu );
    BOOST_CHECK( viewport->enableSceneRender );
    BOOST_CHECK( viewport->enableUI );
    BOOST_CHECK( viewport->clearEveryFrame );
    BOOST_CHECK( viewport->overlaysEnabled );
    BOOST_CHECK( viewport->autoUpdated );
    BOOST_CHECK( !viewport->shadowsEnabled );

    component->unload( nullptr );
    viewport->unload( nullptr );

    component = nullptr;
    viewport = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_activation_is_gated_by_actor_and_component_enabled_state )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.sceneManager );

    auto actor = guard.sceneManager->createActor();
    auto camera = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( actor );
    BOOST_REQUIRE( camera );
    actor->addComponentInstance( camera );

    camera->setEnabled( true );
    camera->setActive( true );
    BOOST_CHECK( camera->isActive() );

    camera->setEnabled( false );
    BOOST_CHECK( !camera->isActive() );
    camera->setEnabled( true );
    BOOST_CHECK( camera->isActive() );

    actor->setEnabled( false );
    BOOST_CHECK( !camera->isActive() );
    actor->setEnabled( true );
    BOOST_CHECK( camera->isActive() );

    camera->setActive( false );
    BOOST_CHECK( !camera->isActive() );
}

BOOST_AUTO_TEST_CASE( camera_queries_have_safe_fallbacks_and_delegate_when_configured )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    BOOST_REQUIRE( component );

    const AABB3<real_Num> bounds;
    const auto fallbackRay = component->getCameraToViewportRay( Vector2<real_Num>( 10.0, 20.0 ) );
    BOOST_CHECK( component->isInFrustum( bounds ) );
    BOOST_CHECK( fallbackRay.getOrigin() == Vector3<real_Num>::zero() );
    BOOST_CHECK( fallbackRay.getDirection() == Vector3<real_Num>::zero() );

    auto renderCamera = workphone::make_ptr<RecordingCamera>();
    BOOST_REQUIRE( renderCamera );
    renderCamera->objectVisible = false;
    component->setCamera( renderCamera );

    const auto ray = component->getCameraToViewportRay( Vector2<real_Num>( 320.0, 180.0 ) );
    BOOST_CHECK( ray.getOrigin() == renderCamera->ray.getOrigin() );
    BOOST_CHECK( ray.getDirection() == renderCamera->ray.getDirection() );
    BOOST_CHECK_EQUAL( renderCamera->lastRayCoordinates.x, 320.0 );
    BOOST_CHECK_EQUAL( renderCamera->lastRayCoordinates.y, 180.0 );
    BOOST_CHECK_EQUAL( renderCamera->rayQueryCount, 1u );
    BOOST_CHECK( !component->isInFrustum( bounds ) );
    BOOST_CHECK_EQUAL( renderCamera->visibilityQueryCount, 1u );

    renderCamera->objectVisible = true;
    BOOST_CHECK( component->isInFrustum( bounds ) );
    BOOST_CHECK_EQUAL( renderCamera->visibilityQueryCount, 2u );

    component->unload( nullptr );
    component = nullptr;
}

BOOST_AUTO_TEST_CASE( camera_child_objects_include_only_attached_runtime_dependencies )
{
    TestGuard guard;

    auto component = workphone::make_ptr<Camera>();
    auto renderCamera = workphone::make_ptr<RecordingCamera>();
    auto viewport = workphone::make_ptr<RecordingViewport>();
    BOOST_REQUIRE( component );
    BOOST_REQUIRE( renderCamera );
    BOOST_REQUIRE( viewport );

    component->setCamera( renderCamera );
    component->setViewport( viewport );

    auto children = component->getChildObjects();
    auto hasCamera = false;
    auto hasViewport = false;
    for( const auto &child : children )
    {
        hasCamera = hasCamera || child.get() == renderCamera.get();
        hasViewport = hasViewport || child.get() == viewport.get();
    }

    BOOST_CHECK( hasCamera );
    BOOST_CHECK( hasViewport );

    component->unload( nullptr );
    renderCamera->unload( nullptr );
    viewport->unload( nullptr );

    component = nullptr;
    renderCamera = nullptr;
    viewport = nullptr;
}

BOOST_AUTO_TEST_SUITE_END()
