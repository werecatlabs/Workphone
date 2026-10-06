#include "UnitTests.hpp"
#include <Workphone/Mesh/ProgressiveMeshOptions.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <Workphone/Core/Properties.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Interface/IApplicationManager.hpp>
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

BOOST_AUTO_TEST_SUITE( MeshImportWindingTests )

BOOST_AUTO_TEST_CASE( winding_option_defaults_off_and_round_trips )
{
    scene::MeshResourceDirector director;
    BOOST_CHECK( !director.getFlipWindingOrder() );
    director.setFlipWindingOrder( true );

    scene::MeshResourceDirector restored;
    restored.setProperties( director.getProperties() );
    BOOST_CHECK( restored.getFlipWindingOrder() );

    restored.setFlipWindingOrder( false );
    director.setProperties( restored.getProperties() );
    BOOST_CHECK( !director.getFlipWindingOrder() );
}

BOOST_AUTO_TEST_CASE( legacy_properties_preserve_winding_defaults )
{
    auto properties = make_ptr<Properties>();
    properties->setProperty( scene::MeshResourceDirector::triangulateStr, true );
    scene::MeshResourceDirector director;
    director.setProperties( properties );
    BOOST_CHECK( !director.getFlipWindingOrder() );
    BOOST_CHECK( director.getTriangulate() );

    director.setFlipWindingOrder( true );
    director.setProperties( properties );
    BOOST_CHECK( director.getFlipWindingOrder() );
}

BOOST_AUTO_TEST_CASE( save_and_import_buttons_observe_updated_winding )
{
    class TestDirector : public scene::MeshResourceDirector
    {
    public:
        bool savedWinding = false;
        bool importedWinding = false;
        unsigned savedCalls = 0;
        unsigned importedCalls = 0;
        String importedPath;
        void save() override { ++savedCalls; savedWinding = getFlipWindingOrder(); }
        void import() override
        {
            ++importedCalls;
            importedWinding = getFlipWindingOrder();
            importedPath = getResourcePath();
        }
    };

    TestDirector director;
    auto properties = director.getProperties();
    properties->setProperty( scene::MeshResourceDirector::flipWindingOrderStr, true );
    properties->setProperty( scene::ResourceDirector::resourcePathStr, String("winding_fixture.glb") );
    properties->setButtonPressed( scene::ResourceDirector::saveStr, true );
    properties->setButtonPressed( scene::ResourceDirector::importStr, true );
    director.setProperties( properties );
    BOOST_CHECK( director.savedWinding );
    BOOST_CHECK( director.importedWinding );
    BOOST_CHECK_EQUAL( director.savedCalls, 1u );
    BOOST_CHECK_EQUAL( director.importedCalls, 1u );
    BOOST_CHECK_EQUAL( director.importedPath, "winding_fixture.glb" );
}

BOOST_AUTO_TEST_CASE( explicit_import_rebuilds_existing_mesh_cache )
{
    class TestDatabase : public ResourceDatabase
    {
    public:
        bool overwriteCache = false;
        String importedPath;
        void importFile( const String &path, bool overwrite ) override
        {
            importedPath = path;
            overwriteCache = overwrite;
        }
    };
    auto application = core::IApplicationManager::instance();
    BOOST_REQUIRE( application );
    struct RestoreDatabase
    {
        SmartPtr<core::IApplicationManager> application;
        SmartPtr<IResourceDatabase> database;
        ~RestoreDatabase() { application->setResourceDatabase(database); }
    } restore{ application, application->getResourceDatabase() };
    auto database = make_ptr<TestDatabase>();
    application->setResourceDatabase(database);
    scene::MeshResourceDirector director;
    director.setResourcePath("winding_fixture.glb");
    director.import();
    BOOST_CHECK( database->overwriteCache );
    BOOST_CHECK_EQUAL( database->importedPath, "winding_fixture.glb" );
}

BOOST_AUTO_TEST_SUITE_END()
