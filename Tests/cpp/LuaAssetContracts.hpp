#ifndef WPLuaAssetContracts_h__
#define WPLuaAssetContracts_h__
#include <Workphone/Workphone.hpp>
#include <Workphone/Script/ScriptAsset.hpp>
#include <Workphone/Script/ScriptGenerator.hpp>
#include <Workphone/System/ResourceSystem.hpp>
#include <Workphone/System/ResourceCompilerRegistry.hpp>
#include <WPLua/LuaScriptCompiler.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <WPSQLite/ResourceCompilationDatabase.hpp>
#include <filesystem>
#include <fstream>

inline void runLuaAssetContracts( workphone::LuaManager &manager )
{
    using namespace workphone;
    using namespace workphone::resource;
    namespace fs = std::filesystem;
    auto check = [&]( bool condition, const char *message ) {
        if( !condition ) throw std::runtime_error( std::string( message ) + ": " + manager.getLastDiagnostic().c_str() );
    };
    const auto folder = fs::temp_directory_path() / ( std::string( "workphone_lua_" ) + StringUtil::getUUID().c_str() );
    const auto source = folder / "source";
    fs::create_directories( source / "logic" );
    auto write = [&]( const char *name, const char *text ) {
        std::ofstream output( source / name, std::ios::binary | std::ios::trunc );
        output << text;
        check( output.good(), "asset fixture write" );
    };
    struct ApplicationFixture
    {
        SmartPtr<core::ApplicationManager> app = make_ptr<core::ApplicationManager>();
        SmartPtr<FactoryManager> factories = make_ptr<FactoryManager>();
        SmartPtr<SQLitePlugin> sqlite = make_ptr<SQLitePlugin>();
        ApplicationFixture()
        {
            core::IApplicationManager::setInstance( app );
            factories->load( nullptr );
            app->setFactoryManager( factories );
            app->setFileSystem( make_ptr<FileSystem>() );
            sqlite->load( nullptr );
        }
        ~ApplicationFixture()
        {
            sqlite->unload( nullptr );
            app->setFileSystem( nullptr );
            app->setFactoryManager( nullptr );
            factories->unload( nullptr );
            core::IApplicationManager::setInstance( nullptr );
        }
    } application;
    struct ManagerFixture
    {
        LuaManager &manager;
        ~ManagerFixture()
        {
            // Release catalog/plugin references before the application fixture,
            // including assertion and exception paths.
            manager.unload( nullptr );
            manager.configureScriptResources( nullptr, nullptr );
        }
    } managerFixture{ manager };
    ScriptGenerator generator;
    for( const char *name : { "end.lua", "123-player.lua", "actor-name.lua" } )
    {
        const auto path = ( source / name ).string();
        generator.createScript( LanguageType::LUA, path.c_str() );
        check( manager.executeSource( application.app->getFileSystem()->readAllText( path.c_str() ),
                                      ( "@" + path ).c_str() ), "generated Lua class syntax" );
    }
    auto catalog = make_ptr<AssetDatabaseManager>();
    check( catalog->setProjectRoot( source.string().c_str() ), "asset source root" );
    catalog->loadFromFile( ( folder / "catalog.db" ).string().c_str() );
    check( catalog->getDatabase() && catalog->getDatabase()->isLoaded(), "real asset catalog backend" );
    auto registry = std::make_shared<ResourceCompilerRegistry>();
    String error;
    check( registry->registerCompiler( std::make_shared<LuaScriptCompiler>(), &error ), "Lua compiler registration" );
    auto resources = std::make_shared<ResourceSystem>( registry, std::make_shared<ResourceCompilationDatabase>() );
    ResourceSystemConfig config;
    config.sourceRoot = source.string().c_str();
    config.compiledRoot = ( folder / "compiled" ).string().c_str();
    check( resources->initialize( config, error ), error.c_str() );
    write( "logic/Helper.lua", "return { value = 41 }" );
    write( "logic/Actor.lua.deps", "# Installed module dependency\ndata://logic/Helper.lua\n" );
    write( "logic/Actor.lua", "assetLoads=(assetLoads or 0)+1; assert(require('logic.Helper').value==41); assetVersion=1" );
    auto asset = make_ptr<ScriptAsset>();
    const auto uuid = StringUtil::getUUID();
    auto component = make_ptr<scene::Script>();
    auto authored = make_ptr<Properties>();
    authored->setProperty( "className", "AssetActor" );
    authored->setProperty( "scriptAssetUuid", uuid );
    authored->setProperty( "updateInEditMode", true );
    component->setProperties( authored );
    check( component->getClassName() == "AssetActor" && component->getScriptAssetUuid() == uuid &&
           component->getUpdateInEditMode(), "unattached script component lost authored fields" );
    component->setProperties( make_ptr<Properties>() );
    check( component->getClassName() == "AssetActor" && component->getScriptAssetUuid() == uuid,
           "partial script properties cleared asset identity" );
    auto copy = make_ptr<scene::Script>();
    copy->setProperties( component->getProperties() );
    check( copy->getClassName() == "AssetActor" && copy->getScriptAssetUuid() == uuid,
           "script component property round trip" );
    authored->setProperty( "className", "" );
    authored->setProperty( "scriptAssetUuid", "" );
    copy->setProperties( authored );
    check( copy->getClassName().empty() && copy->getScriptAssetUuid().empty(), "explicit script identity clear" );
    asset->getHandle()->setUUID( uuid );
    asset->loadFromFile( ( source / "logic/Actor.lua" ).string().c_str() );
    catalog->addResourceEntry( asset );
    AssetDatabaseManager::EntrySnapshot snapshot;
    check( catalog->tryGetEntry( uuid, snapshot ) && snapshot.type == "script", "source asset identity/type" );
    auto database = make_ptr<ResourceDatabase>();
    database->setDatabaseManager( catalog );
    auto retrieved = dynamic_pointer_cast<ScriptAsset>( database->loadResource( StringUtil::parseUUID( uuid ) ) );
    check( retrieved && retrieved->getHandle()->getUUIDAsString() == uuid, "headless script UUID lookup" );
    retrieved = dynamic_pointer_cast<ScriptAsset>( database->loadResource( String( "logic/Actor.lua" ) ) );
    check( retrieved && retrieved->getHandle()->getUUIDAsString() == uuid, "headless script path lookup" );
    retrieved->unload( nullptr );
    check( !retrieved->isLoaded(), "script asset unload state" );
    check( !database->loadResource( String( "logic/missing.lua" ) ), "missing script lookup creates identity" );
    manager.configureScriptResources( catalog, resources );
    check( manager.loadScriptAsset( uuid ), "load script asset" );
    check( manager.loadScriptAsset( uuid ), "repeated asset load" );
    check( manager.executeSource( "assert(assetLoads==1 and assetVersion==1)", "=asset-check" ), "asset executed twice" );
    auto compiled = resources->load( ResourceID( "data://logic/Actor.lua" ), error );
    check( compiled && compiled->dependencies.size() == 1, "installed module dependency" );
    check( resources->compile( ResourceID( "data://logic/Actor.lua" ) ).status == CompilationStatus::UpToDate,
           "incremental Lua compile" );
    write( "logic/Actor.lua", "local =" );
    check( !resources->compile( ResourceID( "data://logic/Actor.lua" ) ).succeeded(), "invalid Lua source compiled" );
    auto oldState = manager.getLuaState();
    Thread::setCurrentTask( TaskId::Application );
    manager.reloadScripts();
    manager.update();
    check( manager.getError() && !manager.reloadPending() && manager.getLuaState() == oldState,
           "failed reload destroyed the live VM" );
    check( manager.executeSource( "assert(assetVersion==1)", "=rollback-check" ), "reload rollback lost globals" );
    write( "logic/Actor.lua", "assert(require('logic.Helper').value==41); assetVersion=2" );
    manager.reloadScripts();
    manager.update();
    check( !manager.getError() && !manager.reloadPending() && manager.getLuaState() != oldState,
           "valid reload did not promote" );
    check( manager.executeSource( "assert(assetVersion==2)", "=promotion-check" ), "promoted asset source" );
    manager.unload( nullptr );
    manager.configureScriptResources( catalog, resources, true );
    manager.load( nullptr );
    // The shipped resource executes with both source files absent.
    fs::rename( source / "logic/Actor.lua", source / "logic/Actor.hidden" );
    fs::rename( source / "logic/Helper.lua", source / "logic/Helper.hidden" );
    check( manager.loadScriptAsset( uuid ), "load script asset" );
    check( manager.executeSource( "assert(assetVersion==2); assert(not pcall(require,'missing'))", "=packaged-check" ),
           "compiled-only module load" );
    auto corrupt = std::make_shared<RuntimeResource>( *compiled );
    corrupt->payload.push_back( 0 );
    check( !manager.loadScriptResource( corrupt ), "corrupt Lua resource accepted" );
    auto subresource = std::make_shared<RuntimeResource>( *compiled );
    subresource->header.resourceId = ResourceID( "data://container.mat:entry.lua" );
    check( !manager.loadScriptResource( subresource ), "Lua subresource accepted as source module" );
    auto conflict = std::make_shared<RuntimeResource>( *compiled );
    auto alias = std::make_shared<RuntimeResource>( *compiled->dependencies.front() );
    alias->header.resourceId = ResourceID( "data://logic.Helper.lua" );
    conflict->dependencies.push_back( alias );
    check( !manager.loadScriptResource( conflict ), "ambiguous require module name accepted" );
    manager.unload( nullptr );
    manager.configureScriptResources( nullptr, nullptr );
    resources->shutdown();
    catalog->unload( nullptr );
    // This test owns exactly this generated temporary directory.
    if( folder.parent_path() == fs::temp_directory_path() && folder.filename().string().rfind( "workphone_lua_", 0 ) == 0 )
        fs::remove_all( folder );
}
#endif
