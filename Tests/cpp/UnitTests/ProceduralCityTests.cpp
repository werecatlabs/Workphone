#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <WPProcedural/WPProcedural.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;
using namespace workphone::procedural;

BOOST_AUTO_TEST_CASE( procedural_import )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( factoryManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        //auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
        //cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "London" );
        BOOST_CHECK( proceduralScene );

        proceduralWorld->addScene( proceduralScene );

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        // SmartPtr<IRoadGenerator> pRoadGenerator(new CRoadGeneratorCity);

        // cityGeneratorDefault->setRoadGenerator(pRoadGenerator);

        //auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        //cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        // cityGeneratorDefault->load("map.osm");

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();

            auto fileName = proceduralScene->getName();
            if( StringUtil::isNullOrEmpty( fileName ) )
            {
                fileName = "Scene";
            }

            fileSystem->writeAllText( fileName + ".fbscene", jsonData );
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        cityGeneratorDefault = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( procedural_bullsmoor )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto workingDirectory = Path::getWorkingDirectory();
        BOOST_CHECK( !StringUtil::isNullOrEmpty( workingDirectory ) );
        fileSystem->addFolder( workingDirectory, false );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
        cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "bullsmoor_small" );
        BOOST_CHECK( proceduralScene );

        proceduralWorld->addScene( proceduralScene );

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        // SmartPtr<IRoadGenerator> pRoadGenerator(new CRoadGeneratorCity);

        // cityGeneratorDefault->setRoadGenerator(pRoadGenerator);

        auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        auto osmFileName = String( "bullsmoor_small.osm" );
        // cityGeneratorDefault->load(osmFileName);

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();

            auto fileName = proceduralScene->getName();
            if( StringUtil::isNullOrEmpty( fileName ) )
            {
                fileName = "Scene";
            }

            // auto dir = String("E:/dev/fireblade_new/Demos/Procedural/Assets/");
            fileSystem->writeAllText( fileName + ".fbscene", jsonData );
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( procedural_turkey_street )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
        cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "TurkeyStreet" );
        BOOST_CHECK( proceduralScene );

        proceduralWorld->addScene( proceduralScene );

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        // SmartPtr<IRoadGenerator> pRoadGenerator(new CRoadGeneratorCity);

        // cityGeneratorDefault->setRoadGenerator(pRoadGenerator);

        auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        auto osmFileName = String( "TurkeyStreet_POC_data.osm" );
        // cityGeneratorDefault->load(osmFileName);

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();

            auto fileName = proceduralScene->getName();
            if( StringUtil::isNullOrEmpty( fileName ) )
            {
                fileName = "Scene";
            }

            auto dir = String( "E:/dev/fireblade_new/Demos/Procedural/Assets/" );
            // fileSystem->writeAllText(dir + fileName + ".fbscene", jsonData);
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        cityGeneratorDefault = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( procedural_tower_bridge )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
        cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "TowerBridge" );
        BOOST_CHECK( proceduralScene );

        proceduralWorld->addScene( proceduralScene );

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        // SmartPtr<IRoadGenerator> pRoadGenerator(new CRoadGeneratorCity);

        // cityGeneratorDefault->setRoadGenerator(pRoadGenerator);

        auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        auto osmFileName = String( "towerBridge.osm" );
        // cityGeneratorDefault->load(osmFileName);

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();

            auto fileName = proceduralScene->getName();
            if( StringUtil::isNullOrEmpty( fileName ) )
            {
                fileName = "Scene";
            }

            auto dir = String( "E:/dev/fireblade_new/Demos/Procedural/Assets/" );
            fileSystem->writeAllText( dir + fileName + ".fbscene", jsonData );
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        cityGeneratorDefault->unload( nullptr );
        cityGeneratorDefault = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( procedural_waltham_cross )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        auto terrainGenerator = workphone::make_ptr<CTerrainGenerator>();
        cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "WalthamCross" );
        BOOST_CHECK( proceduralScene );

        // cityGeneratorDefault->addScene(proceduralScene);

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        // SmartPtr<IRoadGenerator> pRoadGenerator(new CRoadGeneratorCity);

        // cityGeneratorDefault->setRoadGenerator(pRoadGenerator);

        auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->generate();

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();
            // fileSystem->writeAllText(proceduralScene->getName() + ".fbscene", jsonData);
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        cityGeneratorDefault = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( procedural_generate )
{
    auto applicationManager = core::IApplicationManager::instance();
    BOOST_CHECK( applicationManager );

    try
    {
        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( applicationManager->getFactoryManager() );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto cityGeneratorDefault = workphone::make_ptr<CityGeneratorDefault>();
        BOOST_CHECK( cityGeneratorDefault );

        auto terrainGenerator = workphone::make_ptr<CTerrainGeneratorDefault>();
        cityGeneratorDefault->setTerrainGenerator( terrainGenerator );

        auto proceduralWorld = workphone::make_ptr<CProceduralWorld>();
        cityGeneratorDefault->setProceduralWorld( proceduralWorld );

        auto proceduralScene = workphone::make_ptr<CProceduralScene>();
        proceduralScene->setName( "London" );
        BOOST_CHECK( proceduralScene );

        proceduralWorld->addScene( proceduralScene );

        auto blockGenerator = workphone::make_ptr<CBlockGenerator>();
        cityGeneratorDefault->setBlockGenerator( blockGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        auto proceduralCity = workphone::make_ptr<CProceduralCity>();
        BOOST_CHECK( proceduralCity );
        proceduralScene->addCity( proceduralCity );

        blockGenerator->setCity( proceduralCity );

        auto roadNetwork = workphone::make_ptr<CRoadNetwork>();
        proceduralCity->setRoadNetwork( roadNetwork );

        cityGeneratorDefault->addCity( proceduralCity );

        auto pRoadGenerator = workphone::make_ptr<CRoadGeneratorGrid>();

        cityGeneratorDefault->setRoadGenerator( pRoadGenerator );

        auto meshGenerator = workphone::make_ptr<MeshGeneratorDefault>();
        cityGeneratorDefault->setMeshGenerator( meshGenerator );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        cityGeneratorDefault->generate();

        auto data = cityGeneratorDefault->toData();
        if( data )
        {
            auto jsonData = data->toString();
            fileSystem->writeAllText( proceduralScene->getName() + ".fbscene", jsonData );
        }

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() != nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() != 0 );

        auto cityGeneratorData = cityGeneratorDefault->toData();
        if( cityGeneratorData )
        {
            auto result = cityGeneratorData->toString();

            BOOST_CHECK( StringUtil::isNullOrEmpty( result ) == false );
            BOOST_CHECK( result.size() > 3 );
        }

        cityGeneratorDefault->unload( nullptr );

        BOOST_CHECK( cityGeneratorDefault->getMeshGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getRoadGenerator() == nullptr );
        BOOST_CHECK( cityGeneratorDefault->getCities().size() == 0 );

        cityGeneratorDefault = nullptr;
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
