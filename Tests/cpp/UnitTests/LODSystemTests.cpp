#include "UnitTests.hpp"
#include <Workphone/Scene/Systems/LODSystem.hpp>
#include <Workphone/Scene/Components/LODGroup.hpp>
#include <Workphone/Scene/Components/Renderer.hpp>
#include <limits>
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>

using namespace workphone;
using namespace workphone::scene;

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

BOOST_AUTO_TEST_SUITE_END()
