#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Interface/Procedural/ICityGenerator.hpp>
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
#include "WPProcedural/LSystem/LRule.hpp"
#include "WPProcedural/LSystem/LSystem.hpp"

using namespace workphone;

BOOST_AUTO_TEST_CASE( procedural_lsystem )
{
    auto applicationManager = core::IApplicationManager::instance();

    auto system = workphone::make_ptr<procedural::LSystem>();

    system->addVariable( "F" );
    system->printVariables();

    system->addConstant( "+" );
    system->addConstant( "-" );
    system->printConstants();

    system->setStart( "F" );
    system->printStart();

    //system->addRule( procedural::LRule( "F", "F" ) );
    //system->addRule( procedural::LRule( "F", "F+F-F-F+F" ) );
    //system->printRules();

    auto count = 0;
    auto result = system->getNextLevel();
    while( !StringUtil::isNullOrEmpty( result ) && count < 5 )
    {
        std::cout << result << std::endl;
        //result = system->getNextLevel();
        count++;
    }

    BOOST_CHECK( count != 0 );
}
