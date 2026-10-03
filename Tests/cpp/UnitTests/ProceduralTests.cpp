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
#include <boost/test/unit_test.hpp>

using namespace workphone;

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
