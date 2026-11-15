#include "UnitTests.hpp"
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <Workphone/Scene/Components/Camera.hpp>
#include <Workphone/Scene/Components/RenderTexture.hpp>
#include <Workphone/Scene/CameraManager.hpp>
#include <limits>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;
using namespace workphone::scene;

namespace
{
    struct ActiveCameraLODFixture : TestGuard
    {
        SmartPtr<ICameraManager> previousCameras = applicationManager->getCameraManager();
        SmartPtr<Properties> previousSettings = applicationManager->getPlayerSettings();
        const bool wasEditor = applicationManager->isEditor();
        const bool wasPlaying = applicationManager->isPlaying();
        const bool wasEditorCamera = applicationManager->isEditorCamera();
        SmartPtr<CameraManager> cameras = make_ptr<CameraManager>();
        SmartPtr<LODGroup> group;
        SmartPtr<Renderer> detailed, distant;
        SmartPtr<LODSystem> system;
        bool ownsSystem = false;

        ActiveCameraLODFixture()
        {
            scene->clear( true );
            cameras->load( nullptr );
            applicationManager->setCameraManager( cameras );
            applicationManager->setEditor( false );
            applicationManager->setEditorCamera( false );
            auto settings = make_ptr<Properties>();
            settings->setProperty( "quality.mesh", 2 );
            settings->setProperty( "quality.lodBias", 1.f );
            applicationManager->setPlayerSettings( settings );

            auto actor = createBasicActor();
            auto nearActor = createBasicActor();
            auto farActor = createBasicActor();
            scene->addActor( actor );
            scene->addActor( nearActor );
            scene->addActor( farActor );
            detailed = nearActor->addComponent<Renderer>();
            distant = farActor->addComponent<Renderer>();
            group = actor->addComponent<LODGroup>();
            group->addLevel( .05f, { detailed } );
            group->addLevel( 0.f, { distant } );
            group->setLocalReferencePoint( { 0, 0, 10 } );
            group->setSize( 2.f );
            group->setCullBelowLastLOD( false );
            group->setHysteresis( 0.f );
            group->load( nullptr );
            system = dynamic_pointer_cast<LODSystem>( group->getComponentSystem() );
            ownsSystem = !system;
            if( ownsSystem )
            {
                system = make_ptr<LODSystem>();
                system->load( nullptr );
                system->addComponent( group );
            }
            system->clearViewOverride();
        }

        ~ActiveCameraLODFixture()
        {
            system->clearViewOverride();
            if( ownsSystem )
            {
                system->removeComponent( static_pointer_cast<IComponent>( group ) );
                system->unload( nullptr );
            }
            scene->clear( true );
            cameras->unload( nullptr );
            applicationManager->setCameraManager( previousCameras );
            applicationManager->setPlayerSettings( previousSettings );
            applicationManager->setEditor( wasEditor );
            applicationManager->setPlaying( wasPlaying );
            applicationManager->setEditorCamera( wasEditorCamera );
        }

        SmartPtr<Camera> addCamera( f32 z, u32 order = 1 )
        {
            auto actor = createBasicActor();
            scene->addActor( actor );
            auto camera = actor->addComponent<Camera>();
            camera->setFOV( 90.f );
            camera->setZOrder( order );
            actor->setPosition( { 0, 0, z } );
            actor->updateTransform();
            cameras->addCamera( actor );
            camera->setActive( true );
            return camera;
        }

        void checkDetail( bool expected )
        {
            system->update();
            BOOST_CHECK_EQUAL( detailed->isLODVisible(), expected );
            BOOST_CHECK_EQUAL( distant->isLODVisible(), !expected );
        }
    };
}

BOOST_AUTO_TEST_SUITE( LODSystemTests )

BOOST_AUTO_TEST_CASE( lod_projection_helpers_use_viewport_relative_height )
{
    const auto perspective =
        LODSystem::calculatePerspectiveScreenRelativeHeight( 0.5f, 10.0f, Math<f32>::DegToRad( 90.0f ) );
    BOOST_CHECK_CLOSE( perspective, 0.05f, 0.01f );

    const auto orthographic = LODSystem::calculateOrthographicScreenRelativeHeight( 0.5f, 10.0f );
    BOOST_CHECK_CLOSE( orthographic, 0.1f, 0.01f );
}

BOOST_AUTO_TEST_CASE( lod_selection_handles_initial_forced_and_culled_levels )
{
    const Array<f32> thresholds = { 0.6f, 0.2f, 0.05f };

    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.7f, thresholds, -1, -1, 0.1f, true ), 0 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.3f, thresholds, -1, -1, 0.1f, true ), 1 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.1f, thresholds, -1, -1, 0.1f, true ), 2 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.01f, thresholds, -1, -1, 0.1f, true ), -1 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.01f, thresholds, -1, -1, 0.1f, false ), 2 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.01f, thresholds, -1, 99, 0.1f, true ), 2 );
}

BOOST_AUTO_TEST_CASE( lod_selection_applies_hysteresis_in_both_directions )
{
    const Array<f32> thresholds = { 0.6f, 0.2f, 0.05f };

    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.56f, thresholds, 0, -1, 0.1f, true ), 0 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.53f, thresholds, 0, -1, 0.1f, true ), 1 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.64f, thresholds, 1, -1, 0.1f, true ), 1 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.67f, thresholds, 1, -1, 0.1f, true ), 0 );
    BOOST_CHECK_EQUAL( LODSystem::selectLOD( 0.04f, thresholds, 2, -1, 0.1f, true ), -1 );
}

BOOST_AUTO_TEST_CASE( lod_merged_batch_uses_nearest_member_and_actual_scale )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.sceneManager );
    auto actor = guard.createBasicActor();
    guard.scene->addActor( actor );
    auto group = actor->addComponent<LODGroup>();
    auto nearActor = guard.createBasicActor();
    auto farActor = guard.createBasicActor();
    guard.scene->addActor( nearActor );
    guard.scene->addActor( farActor );
    auto nearRenderer = nearActor->addComponent<Renderer>();
    auto farRenderer = farActor->addComponent<Renderer>();
    group->addLevel( .1f, { nearRenderer } );
    group->addLevel( 0.f, { farRenderer } );
    group->setCullBelowLastLOD( false );
    group->setHysteresis( 0.f );
    group->setLocalReferencePoint( { 0, 0, 100 } );
    group->setSize( 2.f );
    group->setDetailBounds( { { { 0, 0, 10 }, 4.f }, { { 0, 0, 100 }, 2.f } } );
    group->load( nullptr );
    auto system = dynamic_pointer_cast<LODSystem>( group->getComponentSystem() );
    // The minimal UnitTests fixture does not create every production component system.
    const bool ownsSystem = !system;
    if( ownsSystem )
    {
        system = make_ptr<LODSystem>();
        system->load( nullptr );
        system->addComponent( group );
    }
    LODSystem::View view;
    view.verticalFovRadians = Math<f32>::DegToRad( 90.f );
    system->setViewOverride( view );
    system->update();
    BOOST_CHECK( nearRenderer->isLODVisible() );
    BOOST_CHECK( !farRenderer->isLODVisible() );

    group->setDetailBounds( {} );
    system->update();
    BOOST_CHECK( !nearRenderer->isLODVisible() );
    BOOST_CHECK( farRenderer->isLODVisible() );

    group->setDetailBounds( { { { 0, 0, 100 }, 2.f } } );
    actor->setScale( { 20, 1, 1 } );
    actor->updateTransform();
    system->update();
    BOOST_CHECK( nearRenderer->isLODVisible() );

    system->clearViewOverride();
    if( ownsSystem )
    {
        system->removeComponent( static_pointer_cast<IComponent>( group ) );
        system->unload( nullptr );
    }
}

BOOST_AUTO_TEST_CASE( lod_detail_bounds_round_trip_and_reject_invalid_values )
{
    auto group = make_ptr<LODGroup>();
    group->setDetailBounds( { { { 1, 2, 3 }, 11.f }, { { 0, 0, 0 }, -1.f },
        { { std::numeric_limits<real_Num>::quiet_NaN(), 0, 0 }, 2.f } } );
    BOOST_REQUIRE_EQUAL( group->getDetailBounds().size(), 1u );
    auto restored = make_ptr<LODGroup>();
    restored->setProperties( group->getProperties() );
    BOOST_REQUIRE_EQUAL( restored->getDetailBounds().size(), 1u );
    BOOST_CHECK_EQUAL( restored->getDetailBounds()[0].diameter, 11.f );
    BOOST_CHECK_EQUAL( restored->getDetailBounds()[0].centre.z, 3.f );
    group->setDetailBounds( {} );
    restored->setProperties( group->getProperties() );
    BOOST_CHECK( restored->getDetailBounds().empty() );
}

BOOST_AUTO_TEST_CASE( runtime_mesh_quality_changes_authored_lod_visibility )
{
    TestGuard guard;
    BOOST_REQUIRE( guard.applicationManager );
    BOOST_REQUIRE( guard.sceneManager );
    const auto previous = guard.applicationManager->getPlayerSettings();
    guard.addCleanup( [app = guard.applicationManager, previous]() mutable {
        app->setPlayerSettings( previous );
    } );
    auto actor = guard.createBasicActor();
    auto detailed = guard.createBasicActor();
    auto reduced = guard.createBasicActor();
    guard.scene->addActor( actor );
    guard.scene->addActor( detailed );
    guard.scene->addActor( reduced );
    auto nearRenderer = detailed->addComponent<Renderer>();
    auto farRenderer = reduced->addComponent<Renderer>();
    auto group = actor->addComponent<LODGroup>();
    group->addLevel( .15f, { nearRenderer } );
    group->addLevel( 0.f, { farRenderer } );
    group->setCullBelowLastLOD( false );
    group->setHysteresis( 0.f );
    group->setLocalReferencePoint( { 0, 0, 10 } );
    group->setSize( 4.f );
    group->load( nullptr );
    auto system = dynamic_pointer_cast<LODSystem>( group->getComponentSystem() );
    const bool ownsSystem = !system;
    if( ownsSystem )
    {
        system = make_ptr<LODSystem>();
        system->load( nullptr );
        system->addComponent( group );
    }
    LODSystem::View view;
    view.verticalFovRadians = Math<f32>::DegToRad( 90.f );
    system->setViewOverride( view );
    const auto checkQuality = [&]( s32 quality, bool detailedVisible ) {
        auto settings = make_ptr<Properties>();
        settings->setProperty( "quality.mesh", quality );
        settings->setProperty( "quality.lodBias", 1.f );
        guard.applicationManager->setPlayerSettings( settings );
        system->update();
        BOOST_CHECK_EQUAL( nearRenderer->isLODVisible(), detailedVisible );
        BOOST_CHECK_EQUAL( farRenderer->isLODVisible(), !detailedVisible );
    };
    checkQuality( 2, true );
    checkQuality( 0, false );
    checkQuality( 3, true );
    checkQuality( 0, false );
    system->clearViewOverride();
    if( ownsSystem )
    {
        system->removeComponent( static_pointer_cast<IComponent>( group ) );
        system->unload( nullptr );
    }
}

BOOST_FIXTURE_TEST_CASE( active_camera_movement_switching_and_priority_update_visibility,
                         ActiveCameraLODFixture )
{
    auto first = addCamera( 0 );
    checkDetail( true );
    first->getActor()->setPosition( { 0, 0, 100 } );
    first->getActor()->updateTransform();
    checkDetail( false );

    auto second = addCamera( 0, 2 );
    checkDetail( true );
    second->setActive( false );
    checkDetail( false );
    second->setActive( true );
    checkDetail( true );
    // Equal priorities keep registration order; explicit priority changes win.
    second->setZOrder( first->getZOrder() );
    checkDetail( false );
    second->setZOrder( 3 );
    checkDetail( true );
    second->getActor()->setEnabled( false );
    checkDetail( false );
}

BOOST_FIXTURE_TEST_CASE( active_camera_editor_play_override_and_offscreen_views,
                         ActiveCameraLODFixture )
{
    applicationManager->setEditor( true );
    applicationManager->setPlaying( true );
    auto game = addCamera( 100 );
    auto offscreen = addCamera( 0, 50 );
    offscreen->setOutputRenderTexture( make_ptr<RenderTexture>() );
    auto editor = addCamera( 0, 100 );
    editor->getActor()->setFlag( IGameActor::ActorFlagIsEditor, true );
    cameras->setEditorCamera( editor->getActor() );

    cameras->reset();
    BOOST_CHECK( !editor->isActive() );
    checkDetail( false );
    applicationManager->setEditorCamera( true );
    cameras->reset();
    BOOST_CHECK( editor->isActive() );
    checkDetail( true );
    applicationManager->setEditorCamera( false );
    cameras->reset();
    checkDetail( false );

    // A UI-only scene activates the editor render fallback even in Play.
    game->setEnabled( false );
    cameras->reset();
    BOOST_CHECK( editor->isActive() );
    BOOST_CHECK( !applicationManager->isEditorCamera() );
    checkDetail( true );
    editor->setActive( false );
    LODSystem::View view;
    BOOST_CHECK( !system->getView( view ) ); // Offscreen camera is still active.
    checkDetail( true ); // Missing view retains the previous selection.
}

BOOST_FIXTURE_TEST_CASE( active_camera_projection_and_explicit_override_are_shared,
                         ActiveCameraLODFixture )
{
    auto camera = addCamera( 100 );
    checkDetail( false );
    camera->setFOV( 10.f );
    checkDetail( true );
    camera->setFOV( 90.f );
    checkDetail( false );
    camera->setOrthoWindowHeight( 10.f );
    camera->setNearClipDistance( .75f );
    checkDetail( true );
    LODSystem::View view;
    BOOST_REQUIRE( system->getView( view ) );
    BOOST_CHECK( view.orthographic );
    BOOST_CHECK_EQUAL( view.orthographicHeight, 10.f );
    BOOST_CHECK_EQUAL( view.nearClipDistance, .75f );
    BOOST_CHECK_CLOSE( view.verticalFovRadians, Math<f32>::DegToRad( 90.f ), .01f );

    LODSystem::View override;
    override.position = { 0, 0, 100 };
    override.verticalFovRadians = Math<f32>::DegToRad( 90.f );
    system->setViewOverride( override );
    checkDetail( false );
    BOOST_REQUIRE( system->getView( view ) );
    BOOST_CHECK( !view.orthographic );
    BOOST_CHECK_EQUAL( view.position.z, 100.f );
    override.position.x = std::numeric_limits<real_Num>::quiet_NaN();
    system->setViewOverride( override );
    view.position = { 7, 8, 9 };
    BOOST_CHECK( !system->getView( view ) );
    BOOST_CHECK_EQUAL( view.position.z, 9.f );
    checkDetail( false ); // An invalid observer cannot replace the valid selection.
    override.position.x = 0;
    system->setViewOverride( override );
    camera->setActive( false );
    checkDetail( false ); // Explicit render-only views do not need Camera components.
    system->clearViewOverride();
    view.position = { 7, 8, 9 };
    BOOST_CHECK( !system->getView( view ) );
    BOOST_CHECK_EQUAL( view.position.z, 9.f );
    checkDetail( false );
    camera->setActive( true );
    checkDetail( true );
}

BOOST_AUTO_TEST_SUITE_END()
