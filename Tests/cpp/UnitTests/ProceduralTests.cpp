#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
#include <WPProcedural/WPProceduralPipeline.hpp>
#include <WPProcedural/CityGeneratorDefault.hpp>
#include <WPProcedural/CProceduralCity.hpp>
#include <WPProcedural/CRoadGenerator.hpp>
#include <WPProcedural/CProceduralWorld.hpp>
#include <WPProcedural/CProceduralScene.hpp>
#include <WPProcedural/CRoadGeneratorGrid.hpp>
#include <WPProcedural/CRoadGeneratorCity.hpp>
#include <WPProcedural/CRoadNetwork.hpp>
#include <WPProcedural/CBlockGenerator.hpp>
#include <WPProcedural/CTerrainGeneratorDefault.hpp>
#include <WPProcedural/MeshGeneratorDefault.hpp>
#include <WPProcedural/CRoad.hpp>
#include <WPProcedural/CRoadConnection.hpp>
#include <WPProcedural/RoadSegmentResult.hpp>
#include <WPProcedural/IntersectionResult.hpp>
#ifdef WP_PLATFORM_WIN32
#include <crtdbg.h>
#endif
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( procedural_generated_mesh_results_release_on_replacement_and_unload )
{
    using namespace workphone::procedural;
    auto road = workphone::make_ptr<CRoad>();
    auto connection = workphone::make_ptr<CRoadConnection>();
    const auto roadId = StringUtil::getHash( "WPRoadSegmentResult" );
    const auto intersectionId = StringUtil::getHash( "WPIntersectionResult" );
    auto exercise = [&]() {
        for( int i = 0; i < 2; ++i )
        {
            auto segment = new RoadSegmentResult;
            segment->roadSurface.vertices.resize( 256 );
            segment->roadSurface.indices.resize( 768 );
            road->setUserData( roadId, segment );
            road->setUserData( roadId, segment );
            auto intersection = new IntersectionResult;
            intersection->surface.vertices.resize( 128 );
            connection->setUserData( intersectionId, intersection );
        }
        road->unload( nullptr );
        connection->unload( nullptr );
    };
    exercise(); // Warm the user-data maps and diagnostic tracking.
#ifdef WP_PLATFORM_WIN32
    _CrtMemState before{}, after{}, difference{};
    _CrtMemCheckpoint( &before );
#endif
    exercise();
#ifdef WP_PLATFORM_WIN32
    _CrtMemCheckpoint( &after );
    BOOST_CHECK_EQUAL( _CrtMemDifference( &difference, &before, &after ), 0 );
#endif
    BOOST_CHECK( !road->getUserData( roadId ) );
    BOOST_CHECK( !connection->getUserData( intersectionId ) );
}

BOOST_AUTO_TEST_CASE( procedural_pipeline )
{
    workphone::procedural::WPProceduralPipeline pipeline( 0xc0de );
    pipeline.initialize();

    workphone::procedural::BuildingSpec spec;
    spec.floors = 5;
    spec.width = 12.0f;
    spec.depth = 10.0f;
    auto result = pipeline.buildings().generate( spec, { 0, 0, 0 } );

    auto skyState = pipeline.sky().getResults();
    auto sunDir = skyState.sunDir;
}

BOOST_AUTO_TEST_CASE( procedural_level_25 )
{
    auto applicationManager = core::IApplicationManager::instance();

    try
    {
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
