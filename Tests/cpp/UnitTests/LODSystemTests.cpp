#include "UnitTests.hpp"
#include <Workphone/Scene/Systems/LODSystem.hpp>
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

BOOST_AUTO_TEST_SUITE_END()
