#include "UnitTests.hpp"
#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_SUITE( ProgressiveMeshOptionsTests )

BOOST_AUTO_TEST_CASE( automatic_presets_resolve_to_valid_progressive_levels )
{
    ProgressiveMeshOptions options;
    options.generationMode = MeshLodGenerationMode::Automatic;
    options.preset = MeshLodPreset::Balanced;

    String validationError;
    const auto levels = options.getResolvedLevels();

    BOOST_REQUIRE_EQUAL( levels.size(), 4u );
    BOOST_CHECK( options.validate( &validationError ) );
    BOOST_CHECK( validationError.empty() );
    BOOST_CHECK( levels.front().remainingGeometry > levels.back().remainingGeometry );
    BOOST_CHECK( levels.front().distance < levels.back().distance );
}

BOOST_AUTO_TEST_CASE( custom_levels_validate_transition_and_reduction_order )
{
    ProgressiveMeshOptions options;
    options.generationMode = MeshLodGenerationMode::Custom;
    options.transitionMode = MeshLodTransitionMode::Distance;

    ProgressiveMeshLodLevel nearLevel;
    nearLevel.distance = 20.0f;
    nearLevel.remainingGeometry = 0.75f;

    ProgressiveMeshLodLevel farLevel;
    farLevel.distance = 80.0f;
    farLevel.remainingGeometry = 0.25f;

    options.levels = { nearLevel, farLevel };
    BOOST_CHECK( options.validate() );

    options.levels[1].distance = 10.0f;
    BOOST_CHECK( !options.validate() );
}

BOOST_AUTO_TEST_CASE( progressive_mesh_options_round_trip_through_properties )
{
    ProgressiveMeshOptions original;
    original.generationMode = MeshLodGenerationMode::Custom;
    original.preset = MeshLodPreset::Custom;
    original.transitionMode = MeshLodTransitionMode::ScreenArea;
    original.simplificationMetric = MeshSimplificationMetric::Curvature;
    original.minimumSourceTriangleCount = 1200;
    original.minimumLodTriangleCount = 96;
    original.optimiseHiddenInterior = true;
    original.outsideImportance = 2.5f;
    original.qTangents = false;

    ProgressiveMeshLodLevel level;
    level.screenArea = 0.08f;
    level.remainingGeometry = 0.4f;
    original.levels.push_back( level );

    ProgressiveMeshOptions restored;
    restored.fromProperties( original.toProperties( "progressiveMeshOptions" ) );

    BOOST_CHECK( restored == original );
    BOOST_CHECK_EQUAL( restored.getHash(), original.getHash() );

    restored.levels.front().remainingGeometry = 0.3f;
    BOOST_CHECK_NE( restored.getHash(), original.getHash() );
}

BOOST_AUTO_TEST_CASE( mesh_import_director_persists_progressive_mesh_defaults )
{
    scene::MeshResourceDirector director;
    auto options = director.getProgressiveMeshOptions();
    options.generationMode = MeshLodGenerationMode::Automatic;
    options.preset = MeshLodPreset::Quality;
    options.transitionMode = MeshLodTransitionMode::ScreenHeight;
    director.setProgressiveMeshOptions( options );

    scene::MeshResourceDirector restored;
    restored.setProperties( director.getProperties() );

    BOOST_CHECK( restored.getProgressiveMeshOptions() == options );
}

BOOST_AUTO_TEST_SUITE_END()
