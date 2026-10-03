#include "UnitTests.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Memory/FactoryUtil.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/System/Resource.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <filesystem>

namespace workphone
{
    class ResourceDatabaseTestResource : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };

    WP_CLASS_REGISTER_DERIVED( workphone, ResourceDatabaseTestResource, Resource<IResource> );
}  // namespace workphone

using namespace workphone;

namespace
{
    bool isExistingNativePath( const String &path )
    {
        return std::filesystem::exists( path.c_str() );
    }

    bool runResourceDatabaseIntegrationTests()
    {
        return std::getenv( "WP_RUN_INTEGRATION_TESTS" ) != nullptr;
    }

    String getMediaFolderPath()
    {
#if defined WP_PLATFORM_WIN32
        return "../../../../Media/";
#elif defined WP_PLATFORM_APPLE
        return "../../Media/";
#else
        return "../../Media/";
#endif
    }

    void addMediaArchives( SmartPtr<IFileSystem> fileSystem, const String &mediaFolderPath )
    {
        auto packs = fileSystem->getFiles( mediaFolderPath + "/packs" );
        for( auto &pack : packs )
        {
            fileSystem->addFileArchive( pack, true, true, IFileSystem::ArchiveType::Zip );
        }
    }

    SmartPtr<ResourceDatabase> createLoadedMediaResourceDatabase( bool includeWorkingDirectory )
    {
        auto applicationManager = core::IApplicationManager::instance();
        BOOST_REQUIRE( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        BOOST_REQUIRE( factoryManager );

        auto fileSystem = applicationManager->getFileSystem();
        BOOST_REQUIRE( fileSystem );

        if( includeWorkingDirectory )
        {
            fileSystem->addFolder( Path::getWorkingDirectory() );
        }

        auto mediaFolderPath = getMediaFolderPath();
        if( !isExistingNativePath( mediaFolderPath ) )
        {
            BOOST_TEST_MESSAGE(
                "Skipping resource database media test because Media folder is unavailable." );
            return nullptr;
        }

        addMediaArchives( fileSystem, mediaFolderPath );
        fileSystem->addFolder( mediaFolderPath, true );
        applicationManager->setMediaPath( mediaFolderPath );

        auto resourceDatabase = workphone::make_ptr<ResourceDatabase>();
        resourceDatabase->load( nullptr );
        applicationManager->setResourceDatabase( resourceDatabase );

        return resourceDatabase;
    }

    void ensureTestResourceFactory( SmartPtr<IFactoryManager> factoryManager )
    {
        BOOST_REQUIRE( factoryManager );

        auto typeInfo = ResourceDatabaseTestResource::typeInfo();
        if( !factoryManager->hasFactoryById( typeInfo ) )
        {
            FactoryUtil::addFactory<ResourceDatabaseTestResource>( factoryManager );
        }
    }

    String makeUniqueDatabaseDirectory()
    {
        auto tempDirectory = std::filesystem::temp_directory_path();
        auto folderName = String( "lioncat_resource_database_tests_" ) + StringUtil::getUUID();
        auto path = String( tempDirectory.string().c_str() ) + "/" + folderName;
        return StringUtil::cleanupPath( path );
    }

    SmartPtr<ResourceDatabaseTestResource> makeTestResource( const String &path )
    {
        auto resource = workphone::make_ptr<ResourceDatabaseTestResource>();
        resource->setName( Path::getFileName( path ) );
        resource->setFilePath( path );

        if( auto handle = resource->getHandle() )
        {
            handle->setUUID( StringUtil::getUUID() );
        }

        return resource;
    }

    size_t countResourceRowsByPath( SmartPtr<AssetDatabaseManager> assetDatabaseManager,
                                    const String &path )
    {
        auto query = assetDatabaseManager->executeQuery(
            String( "SELECT * FROM resources WHERE path = '" ) + path + "'" );

        size_t count = 0;
        while( query && !query->eof() )
        {
            ++count;
            query->nextRow();
        }

        return count;
    }

    String getStoredResourceType( SmartPtr<AssetDatabaseManager> assetDatabaseManager,
                                  const String &path )
    {
        auto query = assetDatabaseManager->executeQuery(
            String( "SELECT type FROM resources WHERE path = '" ) + path + "'" );
        if( query && !query->eof() )
        {
            return query->getFieldValue( "type" );
        }

        return {};
    }

    struct ResourceDatabaseTestContext
    {
        ResourceDatabaseTestContext()
        {
            applicationManager = core::IApplicationManager::instance();
            BOOST_REQUIRE( applicationManager );

            factoryManager = applicationManager->getFactoryManager();
            BOOST_REQUIRE( factoryManager );

            fileSystem = applicationManager->getFileSystem();
            BOOST_REQUIRE( fileSystem );

            auto database = factoryManager->make_object<IDatabase>();
            if( !database )
            {
                BOOST_TEST_MESSAGE(
                    "Skipping resource database unit test because no database plugin is available." );
                return;
            }

            ensureTestResourceFactory( factoryManager );

            previousResourceDatabase = applicationManager->getResourceDatabase();
            previousCachePath = applicationManager->getCachePath();

            databaseDirectory = makeUniqueDatabaseDirectory();
            std::filesystem::create_directories( databaseDirectory.c_str() );

            databaseFileName = String( "asset_" ) + StringUtil::getUUID() + ".db";
            databasePath = StringUtil::cleanupPath( databaseDirectory + "/" + databaseFileName );

            database->loadFromFile( databasePath, "" );

            assetDatabaseManager = workphone::make_ptr<AssetDatabaseManager>();
            assetDatabaseManager->setDatabase( database );
            assetDatabaseManager->setDatabasePath( databasePath );
            assetDatabaseManager->create();

            resourceDatabase = workphone::make_ptr<ResourceDatabase>();
            resourceDatabase->load( nullptr );
            resourceDatabase->setFilePath( databaseFileName );
            resourceDatabase->setDatabaseManager( assetDatabaseManager );

            applicationManager->setCachePath( databaseDirectory + "/" );
            applicationManager->setResourceDatabase( resourceDatabase );

            isAvailable = true;
        }

        ~ResourceDatabaseTestContext()
        {
            if( applicationManager )
            {
                applicationManager->setResourceDatabase( previousResourceDatabase );
                applicationManager->setCachePath( previousCachePath );
            }

            if( resourceDatabase )
            {
                resourceDatabase->setDatabaseManager( nullptr );
            }

            if( assetDatabaseManager )
            {
                assetDatabaseManager->unload( nullptr );
                assetDatabaseManager = nullptr;
            }

            resourceDatabase = nullptr;

            if( !databaseDirectory.empty() )
            {
                std::error_code errorCode;
                std::filesystem::remove_all( databaseDirectory.c_str(), errorCode );
            }
        }

        TestGuard guard;
        SmartPtr<core::IApplicationManager> applicationManager;
        SmartPtr<IFactoryManager> factoryManager;
        SmartPtr<IFileSystem> fileSystem;
        SmartPtr<IResourceDatabase> previousResourceDatabase;
        SmartPtr<ResourceDatabase> resourceDatabase;
        SmartPtr<AssetDatabaseManager> assetDatabaseManager;
        String previousCachePath;
        String databaseDirectory;
        String databaseFileName;
        String databasePath;
        bool isAvailable = false;
    };
}  // namespace

BOOST_AUTO_TEST_CASE( material_texture_binding_survives_serialization )
{
    ResourceDatabaseTestContext context;
    BOOST_REQUIRE( context.isAvailable );
    auto graphics = context.applicationManager->getGraphicsSystem();
    BOOST_REQUIRE( graphics );
    auto manager = graphics->getMaterialManager();
    BOOST_REQUIRE( manager );
    auto material = dynamic_pointer_cast<render::IMaterial>(
        manager->create( "TextureBinding_" + StringUtil::getUUID() ) );
    BOOST_REQUIRE( material );
    material->load( nullptr );
    auto technique = material->createTechnique();
    BOOST_REQUIRE( technique );
    BOOST_REQUIRE( technique->createPass() );
    auto texture = context.resourceDatabase->loadResourceByType<render::ITexture>( "panel.png" );
    BOOST_REQUIRE( texture );
    context.resourceDatabase->addResource( texture );
    material->setTexture( texture, 0 );
    BOOST_REQUIRE( material->getTexture( 0 ) );
    BOOST_TEST( material->getTexture( 0 ).get() == texture.get() );
    BOOST_REQUIRE( !material->getTextures().empty() );
    BOOST_TEST( material->getTextures()[0].get() == texture.get() );

    auto restored = dynamic_pointer_cast<render::IMaterial>(
        manager->create( "RestoredTextureBinding_" + StringUtil::getUUID() ) );
    BOOST_REQUIRE( restored );
    restored->load( nullptr );
    restored->fromData( material->toData() );
    BOOST_REQUIRE( restored->getTexture( 0 ) );
    BOOST_TEST( restored->getTexture( 0 )->getFilePath() == texture->getFilePath() );

    // Legacy engine materials store only a UUID in the texture unit.
    auto legacy = workphone::make_ptr<Properties>();
    legacy->setProperty( "texture", texture->getHandle()->getUUIDAsString() );
    auto unit = context.factoryManager->make_object<render::IMaterialTexture>();
    BOOST_REQUIRE( unit );
    unit->fromData( legacy );
    BOOST_REQUIRE( unit->getTexture() );
    BOOST_TEST( unit->getTexture()->getFilePath() == texture->getFilePath() );
}

BOOST_AUTO_TEST_CASE( resource_database_load_initializes_defaults )
{
    try
    {
        auto resourceDatabase = workphone::make_ptr<ResourceDatabase>();
        resourceDatabase->load( nullptr );

        BOOST_CHECK( resourceDatabase->isLoaded() );
        BOOST_CHECK_EQUAL( resourceDatabase->getFilePath(), ResourceDatabase::defaultDatabaseFileName );
        BOOST_CHECK( resourceDatabase->getDatabaseManager() );

        resourceDatabase->setFilePath( "custom_asset.db" );
        BOOST_CHECK_EQUAL( resourceDatabase->getFilePath(), "custom_asset.db" );

        resourceDatabase->unload( nullptr );
        BOOST_CHECK( !resourceDatabase->isLoaded() );
        BOOST_CHECK( !resourceDatabase->getDatabaseManager() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_addAndRetrieveAsset )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        const auto path = String( "UnitTests/ResourceDatabase/AddAndRetrieve.resource" );
        auto resource = makeTestResource( path );
        auto uuid = resource->getHandle()->getUUIDAsString();

        BOOST_CHECK( !context.resourceDatabase->hasResource( resource ) );

        context.resourceDatabase->addResource( resource );

        BOOST_CHECK( context.resourceDatabase->hasResource( resource ) );
        BOOST_CHECK_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, path ), 1u );

        auto entry = context.assetDatabaseManager->getResourceEntryFromPath( path );
        BOOST_REQUIRE( entry );

        auto resourceEntry = workphone::dynamic_pointer_cast<scene::ResourceDirector>( entry );
        BOOST_REQUIRE( resourceEntry );
        BOOST_CHECK_EQUAL( resourceEntry->getResourcePath(), path );
        BOOST_CHECK_EQUAL( resourceEntry->getResourceUUID(), uuid );
        BOOST_CHECK_EQUAL( getStoredResourceType( context.assetDatabaseManager, path ),
                           "workphone::ResourceDatabaseTestResource" );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_addResource_is_idempotent )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        const auto path = String( "UnitTests/ResourceDatabase/Duplicate.resource" );
        auto resource = makeTestResource( path );

        context.resourceDatabase->addResource( resource );
        context.resourceDatabase->addResource( resource );

        BOOST_CHECK( context.resourceDatabase->hasResource( resource ) );
        BOOST_CHECK_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, path ), 1u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_addAndRemoveAsset )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        const auto path = String( "UnitTests/ResourceDatabase/AddAndRemove.resource" );
        auto resource = makeTestResource( path );

        context.resourceDatabase->addResource( resource );
        BOOST_REQUIRE( context.resourceDatabase->hasResource( resource ) );
        BOOST_REQUIRE_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, path ), 1u );

        context.resourceDatabase->removeResource( resource );

        BOOST_CHECK( !context.resourceDatabase->hasResource( resource ) );
        BOOST_CHECK_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, path ), 0u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_removeResourceFromPath_onlyRemovesMatchingPath )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        const auto firstPath = String( "UnitTests/ResourceDatabase/First.resource" );
        const auto secondPath = String( "UnitTests/ResourceDatabase/Second.resource" );
        auto first = makeTestResource( firstPath );
        auto second = makeTestResource( secondPath );

        context.resourceDatabase->addResource( first );
        context.resourceDatabase->addResource( second );

        context.resourceDatabase->removeResourceFromPath( firstPath );
        context.resourceDatabase->removeResourceFromPath(
            "UnitTests/ResourceDatabase/Missing.resource" );

        BOOST_CHECK( !context.resourceDatabase->hasResource( first ) );
        BOOST_CHECK( context.resourceDatabase->hasResource( second ) );
        BOOST_CHECK_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, firstPath ), 0u );
        BOOST_CHECK_EQUAL( countResourceRowsByPath( context.assetDatabaseManager, secondPath ), 1u );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_retrieveNonExistentAsset )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        BOOST_CHECK( !context.resourceDatabase->loadResource( "" ) );
        BOOST_CHECK( !context.resourceDatabase->loadResource(
            "UnitTests/ResourceDatabase/non_existent_asset.unknown" ) );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_loadResource_createsRegisteredResourceFromAssetDatabase )
{
    if( UnitTests::isHeadlessGraphicsMode() )
    {
        BOOST_TEST_MESSAGE( "Headless graphics mode - skipping resource database test" );
        return;
    }

    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        if( !context.applicationManager->getGraphicsSystem() )
        {
            BOOST_TEST_MESSAGE( "Skipping loadResource test because graphics system is unavailable." );
            return;
        }

        const auto path = String( "UnitTests/ResourceDatabase/LoadRegistered.resource" );
        auto resource = makeTestResource( path );
        auto uuid = resource->getHandle()->getUUIDAsString();

        context.resourceDatabase->addResource( resource );

        auto loadedResource = context.resourceDatabase->loadResource( path );
        BOOST_REQUIRE( loadedResource );
        BOOST_CHECK( loadedResource->isDerived<ResourceDatabaseTestResource>() );
        BOOST_CHECK_EQUAL( loadedResource->getFilePath(), path );
        BOOST_CHECK_EQUAL( loadedResource->getHandle()->getUUIDAsString(), uuid );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_removeUnusedResources_removesUnreferencedInstances )
{
    try
    {
        ResourceDatabaseTestContext context;
        if( !context.isAvailable )
        {
            return;
        }

        auto instances = workphone::make_shared<Array<SmartPtr<IResource>>>();
        auto retained = makeTestResource( "UnitTests/ResourceDatabase/Retained.resource" );
        auto unused = makeTestResource( "UnitTests/ResourceDatabase/Unused.resource" );

        instances->push_back( retained );
        instances->push_back( unused );
        context.resourceDatabase->setInstancesPtr( instances );

        unused = nullptr;
        context.resourceDatabase->removeUnusedResources();

        BOOST_REQUIRE( context.resourceDatabase->getInstancesPtr() );
        BOOST_CHECK_EQUAL( context.resourceDatabase->getInstancesPtr()->size(), 1u );
        BOOST_CHECK( context.resourceDatabase->getInstancesPtr()->front() == retained );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_importFileJob_storesPathAndOverwriteFlag )
{
    auto job = workphone::make_ptr<ResourceDatabase::ImportFileJob>();

    BOOST_CHECK_EQUAL( job->getFilePath(), "" );
    BOOST_CHECK( !job->getOverwrite() );

    job->setFilePath( "UnitTests/ResourceDatabase/Import.resource" );
    job->setOverwrite( true );

    BOOST_CHECK_EQUAL( job->getFilePath(), "UnitTests/ResourceDatabase/Import.resource" );
    BOOST_CHECK( job->getOverwrite() );
}

BOOST_AUTO_TEST_CASE( resource_database )
{
    try
    {
        auto resourceDatabase = createLoadedMediaResourceDatabase( false );
        if( !resourceDatabase )
        {
            return;
        }

        BOOST_CHECK( resourceDatabase->isLoaded() );
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_build )
{
    if( !runResourceDatabaseIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping resource database build integration test." );
        return;
    }

    try
    {
        auto resourceDatabase = createLoadedMediaResourceDatabase( true );
        if( !resourceDatabase )
        {
            return;
        }

        BOOST_REQUIRE( resourceDatabase->isLoaded() );
        resourceDatabase->build();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_import )
{
    if( !runResourceDatabaseIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping resource database import integration test." );
        return;
    }

    try
    {
        auto resourceDatabase = createLoadedMediaResourceDatabase( true );
        if( !resourceDatabase )
        {
            return;
        }

        BOOST_REQUIRE( resourceDatabase->isLoaded() );
        resourceDatabase->importAssets();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}

BOOST_AUTO_TEST_CASE( resource_database_reimport )
{
    if( !runResourceDatabaseIntegrationTests() )
    {
        BOOST_TEST_MESSAGE( "Skipping resource database reimport integration test." );
        return;
    }

    try
    {
        auto resourceDatabase = createLoadedMediaResourceDatabase( true );
        if( !resourceDatabase )
        {
            return;
        }

        BOOST_REQUIRE( resourceDatabase->isLoaded() );
        resourceDatabase->reimportAssets();
    }
    catch( std::exception &e )
    {
        WP_LOG_EXCEPTION( e );
        BOOST_FAIL( e.what() );
    }
}
