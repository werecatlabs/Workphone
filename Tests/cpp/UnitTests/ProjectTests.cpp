#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <boost/test/unit_test.hpp>

using namespace workphone;

BOOST_AUTO_TEST_CASE( project_cache_import )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( factoryManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto db = factoryManager->make_object<IDatabase>();
        if( !db )
        {
            BOOST_TEST_MESSAGE(
                "Skipping project cache import test because no database plugin is available." );
            return;
        }

        auto files = fileSystem->getSystemFiles();
        BOOST_CHECK( files.empty() == true );
        fileSystem->addFolder( "./", true );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto assetDatabaseManager = workphone::make_ptr<AssetDatabaseManager>();
        resourceDatabase->setDatabaseManager( assetDatabaseManager );

        auto sceneManager = applicationManager->getGameManager();

        auto scene = sceneManager->getCurrentScene();

        auto cachePath = applicationManager->getCachePath();
        //BOOST_CHECK( !StringUtil::isNullOrEmpty( cachePath ) );

        auto databaseFileName = String( "asset.db" );
        auto databaseFilePath = cachePath + "/" + databaseFileName;
        databaseFilePath = StringUtil::cleanupPath( databaseFilePath );

        assetDatabaseManager->setDatabase( db );
        BOOST_CHECK( assetDatabaseManager->getDatabase() );

        auto databasePath = Path::getFilePath( databaseFilePath );
        //BOOST_CHECK( !StringUtil::isNullOrEmpty( databasePath ) );

        if( !fileSystem->isExistingFolder( databasePath ) )
        {
            fileSystem->createDirectories( databasePath );
        }

        if( db )
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( databaseFilePath ) );

            BOOST_CHECK( db->isValid() );
            db->loadFromFile( databaseFilePath, "" );
            BOOST_CHECK( db->isValid() );
        }

        BOOST_CHECK( fileSystem->isValid() );
        fileSystem->refreshPath( databasePath, false );
        BOOST_CHECK( fileSystem->isValid() );

        // auto projectPath = String("");
        // applicationManager->setCachePath(projectPath + "/Cache/");

        auto filePath = String( "cube.fbx" );
        if( fileSystem->isExistingFile( filePath ) )
        {
            resourceDatabase->importFile( filePath );
        }
        else
        {
            WP_LOG_ERROR( "File not found: " + filePath );
        }
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( project_cache )
{
    SmartPtr<IResourceDatabase> previousResourceDatabase;
    String previousCachePath;
    String workingDirectory;

    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_CHECK( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_CHECK( factoryManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_CHECK( fileSystem );

        auto db = factoryManager->make_object<IDatabase>();
        if( !db )
        {
            BOOST_TEST_MESSAGE( "Skipping project cache test because no database plugin is available." );
            return;
        }

        previousResourceDatabase = applicationManager->getResourceDatabase();
        previousCachePath = applicationManager->getCachePath();
        workingDirectory = Path::getWorkingDirectory();

        auto files = fileSystem->getSystemFiles();
        BOOST_CHECK( files.empty() == true );
        fileSystem->addFolder( "./", true );

        auto resourceDatabase = workphone::make_ptr<ResourceDatabase>();
        applicationManager->setResourceDatabase( resourceDatabase );

        auto assetDatabaseManager = workphone::make_ptr<AssetDatabaseManager>();
        resourceDatabase->setDatabaseManager( assetDatabaseManager );

        // resourceDatabase->setDatabase(db);

        auto projectPath = workingDirectory + "/TestProject";
        if( !fileSystem->isExistingFolder( projectPath ) )
        {
            fileSystem->createDirectories( projectPath );
        }
        Path::setWorkingDirectory( projectPath );

        fileSystem->addFolder( projectPath, true );

        applicationManager->setCachePath( projectPath + "/Cache/" );

        resourceDatabase->importAssets();
        Path::setWorkingDirectory( workingDirectory );
        applicationManager->setCachePath( previousCachePath );
        applicationManager->setResourceDatabase( previousResourceDatabase );
    }
    catch( std::exception &e )
    {
        auto applicationManager = core::IApplicationManager::instance();
        if( applicationManager )
        {
            applicationManager->setCachePath( previousCachePath );
            applicationManager->setResourceDatabase( previousResourceDatabase );
        }
        Path::setWorkingDirectory( workingDirectory );
        WP_LOG_EXCEPTION( e );
    }
}

BOOST_AUTO_TEST_CASE( project_settings )
{
    try
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();

        Array<String> ignoreList;
        ignoreList.emplace_back( "UnityEngine.Transform" );
        sceneManager->setComponentFactoryIgnoreList( ignoreList );

        Map<String, String> componentMap;
        componentMap["saracen.ApplicationManager"] = "SimulatorApplication";
        componentMap["FB.FBFiniteStateMachine"] = "FiniteStateMachine";
        componentMap["UnityEngine.Canvas"] = "Layout";
        componentMap["UnityEngine.RectTransform"] = "LayoutTransform";
        componentMap["UnityEngine.UI.Text"] = "Text";
        componentMap["UnityEngine.UI.Image"] = "Image";
        componentMap["UnityEngine.UI.Button"] = "Button";
        componentMap["Unitycoding.UIWidgets.TooltipTrigger"] = "Tooltip";
        componentMap["UI.Tables.TableLayout"] = "TableLayout";
        componentMap["UnityEngine.UI.VerticalLayoutGroup"] = "TableLayout";
        componentMap["saracen.StartMenu"] = "StartMenu";

        componentMap["CameraComponent"] = "Camera";
        componentMap["MeshComponent"] = "Mesh";
        componentMap["LightComponent"] = "Light";
        componentMap["MaterialComponent"] = "Material";
        componentMap["RigidbodyComponent"] = "Rigidbody";
        componentMap["SkyboxComponent"] = "Skybox";
        componentMap["Canvas"] = "Layout";
        componentMap["CanvasTransform"] = "LayoutTransform";
        componentMap["TextComponent"] = "Text";
        componentMap["ImageComponent"] = "Image";
        componentMap["ButtonComponent"] = "Button";

        // unity map
        componentMap["5f7201a12d95ffc409449d95f23cf332"] = "Text";
        componentMap["dc42784cf147c0c48a680349fa168899"] = "Layout";
        componentMap["fe87c0e1cc204ed48ad3b37840f39efc"] = "Image";
        componentMap["4e29b1a8efbd4b44bb3f3716e73f07ff"] = "Button";

        sceneManager->setComponentFactoryMap( componentMap );

        auto properties = workphone::make_ptr<Properties>();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
    }
}
