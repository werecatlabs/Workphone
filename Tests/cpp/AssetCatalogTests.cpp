#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include <filesystem>
#include <iostream>
#include <thread>
#include <atomic>
#include <stdexcept>

namespace workphone
{
    class CatalogTestResource : public Resource<IResource>
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED(workphone, CatalogTestResource, Resource<IResource>);
    class CatalogTestSceneEntry : public ISharedObject
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED(workphone, CatalogTestSceneEntry, ISharedObject);
}
using namespace workphone;
namespace
{
    void require(bool value, const char *message)
    {
        if(!value) throw std::runtime_error(message);
    }
    SmartPtr<CatalogTestResource> asset(const String &path)
    {
        auto result = make_ptr<CatalogTestResource>();
        result->getHandle()->setUUID(StringUtil::getUUID());
        result->setFilePath(path);
        return result;
    }
    String entryId(SmartPtr<IBuildDirector> entry)
    {
        auto director = dynamic_pointer_cast<scene::ResourceDirector>( entry );
        require( director != nullptr, "Expected catalog ResourceDirector" );
        return director->getResourceUUID();
    }
    SmartPtr<IDatabase> openDatabase( const std::filesystem::path &path )
    {
        auto app = core::IApplicationManager::instance();
        auto database = app->getFactoryManager()->make_object<IDatabase>();
        require( database != nullptr, "SQLite backend is mandatory; cannot skip" );
        database->loadFromFile( String( path.string().c_str() ) );
        require( database->isLoaded(), "SQLite fixture must open" );
        return database;
    }
    SmartPtr<IDatabaseQuery> sql( SmartPtr<IDatabase> database, const String &statement,
                                  const Array<String> &values = {} )
    {
        auto bound = dynamic_cast<IParameterizedDatabase *>( database.get() );
        require( bound != nullptr, "Parameterized SQLite backend is mandatory; cannot skip" );
        return bound->queryBound( statement, values );
    }
    int scalar( SmartPtr<IDatabase> database, const String &statement, const Array<String> &values = {} )
    {
        auto result = sql( database, statement, values );
        require( result && !result->eof(), "Expected a scalar SQLite result" );
        return result->getFieldValueAsInt( "n" );
    }
    void seedLegacyCatalog( SmartPtr<IDatabase> database )
    {
        require( sql( database,
                      "CREATE TABLE Resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type "
                      "VARCHAR)" ) != nullptr,
                 "Legacy fixture schema must be created" );
        require( sql( database, "CREATE INDEX idx_resources_uuid ON resources(uuid)" ) != nullptr,
                 "Legacy nonunique UUID index must be created" );
    }
    void migrationContracts( const std::filesystem::path &folder )
    {
        const auto path = folder / "legacy.db";
        const String quoted = u8"textures/caf\u00e9/\u7eb9\u7406's.tex";
        auto database = openDatabase( path );
        seedLegacyCatalog( database );
        require( sql( database, "CREATE INDEX idx_resources_uuid_unique ON resources(uuid)" ) != nullptr,
                 "Legacy fixture must include a misleadingly named nonunique index" );
        require( sql( database, "INSERT INTO resources(id,uuid,path,type) VALUES(7,?,?,?)",
                      { "legacy-texture", quoted, "Texture" } ) != nullptr,
                 "Legacy texture must insert" );
        require( sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                      { "scene-a", "scene", "Actor" } ) != nullptr,
                 "First legacy scene entry must insert" );
        require( sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                      { "scene-b", "scene", "Actor" } ) != nullptr,
                 "Second legacy scene entry must insert" );
        require( sql( database, "PRAGMA user_version=42" ) != nullptr,
                 "Unrelated database version must set" );
        database->close();
        database = nullptr;

        auto catalog = make_ptr<AssetDatabaseManager>();
        catalog->loadFromFile( String( path.string().c_str() ) );
        database = catalog->getDatabase();
        require( database && database->isLoaded(), "Valid legacy catalog must migrate" );
        require( scalar( database, "SELECT version AS n FROM wp_asset_catalog_schema WHERE id=1" ) == 1,
                 "Migration must record its own catalog schema version" );
        require( scalar( database, "SELECT count(*) AS n FROM resources" ) == 3,
                 "Migration must preserve all legacy rows, including shared scene paths" );
        require(
            scalar( database, "SELECT id AS n FROM resources WHERE uuid=?", { "legacy-texture" } ) == 7,
            "Migration must preserve legacy row IDs" );
        require( entryId( catalog->getResourceEntryFromPath( quoted ) ) == "legacy-texture",
                 "Unicode/apostrophe path and UUID must survive migration" );
        require( scalar( database, "SELECT count(*) AS n FROM wp_asset_catalog_backup_v0" ) == 3,
                 "Migration must retain a pre-migration row backup" );
        require( !sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                       { "legacy-texture", "textures/duplicate.tex", "Texture" } ),
                 "Migration must replace a nonunique UUID guard with a unique index" );
        auto userVersion = sql( database, "PRAGMA user_version" );
        require( userVersion && userVersion->getFieldValueAsInt( "user_version" ) == 42,
                 "Catalog versioning must not overwrite another subsystem's user_version" );
        require( !sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                       { "invalid-empty", "", "Texture" } ),
                 "Schema must reject empty paths through direct SQL" );
        require( !sql( database, "UPDATE resources SET uuid='' WHERE uuid=?", { "legacy-texture" } ),
                 "Schema must reject invalid identity updates" );
        require( !sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                       { "invalid-long", String( 1025, 'x' ), "Texture" } ),
                 "Schema must reject oversized keys" );
        require( !sql( database,
                       "INSERT INTO resources(uuid,path,type) VALUES('invalid-nul',CAST(X'610062' AS "
                       "TEXT),'Texture')" ),
                 "Schema must reject embedded NULs even through direct SQL" );
        require( scalar( database, "SELECT count(*) AS n FROM resources" ) == 3,
                 "Rejected mutations must preserve the catalog" );
        require( sql( database, "UPDATE resources SET path=? WHERE uuid=?",
                      { "textures/renamed.tex", "legacy-texture" } ) != nullptr,
                 "Valid rename must succeed" );
        catalog->destroy();
        database = nullptr;
        catalog->loadFromFile( StringW( path.wstring().c_str() ) );
        database = catalog->getDatabase();
        require( database && database->isLoaded(), "Migrated schema must reopen idempotently" );
        require(
            entryId( catalog->getResourceEntryFromPath( "textures/renamed.tex" ) ) == "legacy-texture",
            "Renamed identity must survive migration/reopen" );
        auto backup = sql( database, "SELECT path FROM wp_asset_catalog_backup_v0 WHERE uuid=?",
                           { "legacy-texture" } );
        require( backup && backup->getFieldValue( "path" ) == quoted,
                 "Reopen must not overwrite the original migration backup" );
        catalog->destroy();
        database = nullptr;

        for( const auto fixture : { "duplicate-uuid", "duplicate-path", "invalid-key", "embedded-nul",
                                    "backup-conflict", "metadata-conflict", "future-version" } )
        {
            const auto rejectedPath = folder / ( std::string( fixture ) + ".db" );
            database = openDatabase( rejectedPath );
            seedLegacyCatalog( database );
            const String secondUUID = String( fixture ) == "duplicate-uuid" ? "first-id" : "second-id";
            const String secondPath = String( fixture ) == "duplicate-path" ? "first.mesh"
                                      : String( fixture ) == "invalid-key"  ? ""
                                                                            : "second.mesh";
            require( sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                          { "first-id", "first.mesh", "Mesh" } ) != nullptr,
                     "First invalid fixture row must seed" );
            require( sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                          { secondUUID, secondPath, "Mesh" } ) != nullptr,
                     "Second invalid fixture row must seed" );
            if( String( fixture ) == "embedded-nul" )
                require( sql( database, "UPDATE resources SET path=CAST(X'610062' AS TEXT) WHERE uuid=?",
                              { secondUUID } ) != nullptr,
                         "Embedded-NUL legacy fixture must seed" );
            if( String( fixture ) == "backup-conflict" )
                require( sql( database, "CREATE TABLE wp_asset_catalog_backup_v0(marker INTEGER)" ) !=
                             nullptr,
                         "Backup collision fixture must seed" );
            if( String( fixture ) == "metadata-conflict" )
                require( sql( database,
                              "CREATE VIEW wp_asset_catalog_schema AS SELECT 99 AS id, 1 AS version" ) !=
                             nullptr,
                         "Late migration failure fixture must seed" );
            if( String( fixture ) == "future-version" )
            {
                require( sql( database,
                              "CREATE TABLE wp_asset_catalog_schema(id INTEGER PRIMARY KEY, version "
                              "INTEGER NOT NULL)" ) != nullptr,
                         "Future metadata fixture must seed" );
                require( sql( database, "INSERT INTO wp_asset_catalog_schema VALUES(1,2)" ) != nullptr,
                         "Future schema version must seed" );
            }
            database->close();
            database = nullptr;
            catalog->loadFromFile( String( rejectedPath.string().c_str() ) );
            require( catalog->getDatabase() && !catalog->getDatabase()->isLoaded(),
                     "Invalid or newer catalog must fail closed" );
            catalog->destroy();
            database = openDatabase( rejectedPath );
            require( scalar( database, "SELECT count(*) AS n FROM resources" ) == 2,
                     "Failed migration must preserve every original row" );
            require(
                scalar(
                    database,
                    "SELECT count(*) AS n FROM sqlite_master WHERE name='idx_resources_uuid_unique'" ) ==
                    0,
                "Failed migration must roll back schema changes" );
            require(
                scalar(
                    database,
                    "SELECT count(*) AS n FROM sqlite_master WHERE name='wp_asset_catalog_schema'" ) ==
                    ( String( fixture ) == "future-version" || String( fixture ) == "metadata-conflict"
                          ? 1
                          : 0 ),
                "Failed migration must preserve original metadata state" );
            require( scalar( database,
                             "SELECT count(*) AS n FROM sqlite_master WHERE "
                             "name='wp_asset_catalog_backup_v0'" ) ==
                         ( String( fixture ) == "backup-conflict" ? 1 : 0 ),
                     "Failed migration must roll back new backups and preserve existing backups" );
            if( String( fixture ) == "future-version" )
                require( scalar( database,
                                 "SELECT version AS n FROM wp_asset_catalog_schema WHERE id=1" ) == 2,
                         "Newer schema must never be downgraded" );
            database->close();
            database = nullptr;
        }
    }
    void contracts(AssetDatabaseManager &catalog, const std::filesystem::path &folder)
    {
        auto db = catalog.getDatabase();
        auto bound = dynamic_cast<IParameterizedDatabase *>(db.get());
        require(bound != nullptr,"SQLite parameterized backend is mandatory; cannot skip");
        catalog.create();
        require(!catalog.getResourceEntryFromPath("missing.mesh"),"Missing lookup must return null");
        require(!catalog.getResourceEntry("missing-id"),"Missing UUID must return null");
        auto count = bound->queryBound("SELECT count(*) AS n FROM resources",{});
        require(count && count->getFieldValueAsInt("n") == 0,"Lookup must not persist an asset");

        const String quoted = "characters/hero's '; DROP TABLE resources;--.mesh";
        auto first = asset(quoted);
        const auto firstId = first->getHandle()->getUUIDAsString();
        catalog.addResourceEntry(first); catalog.addResourceEntry(first);
        require(entryId(catalog.getResourceEntryFromPath(quoted)) == firstId,"Quoted path must round-trip as bound data");
        count = bound->queryBound("SELECT count(*) AS n FROM resources",{});
        require(count && count->getFieldValueAsInt("n") == 1,"Repeated insert must be idempotent");
        auto detached = dynamic_pointer_cast<scene::ResourceDirector>(catalog.getResourceEntry(firstId));
        detached->setResourcePath("tampered");
        require(catalog.getResourceEntryFromPath(quoted) != nullptr,"Detached director cannot mutate authoritative catalog");

        auto conflict = asset(quoted);
        catalog.addResourceEntry(conflict);
        require(!catalog.hasResourceById(conflict->getHandle()->getUUIDAsString()),"Path collision cannot replace identity");
        auto second = asset("characters/second.mesh"); catalog.addResourceEntry(second);
        first->setFilePath(second->getFilePath()); catalog.updateResourceEntry(first);
        require(entryId(catalog.getResourceEntryFromPath(quoted)) == firstId,"Failed rename must retain original row");
        first->setFilePath("characters/renamed.mesh");
        first->setFileSystemId(workphone::UUID::generate());
        catalog.updateResourceEntry(first);
        require(!catalog.getResourceEntryFromPath(quoted),"Rename must remove old path");
        require(entryId(catalog.getResourceEntryFromPath(first->getFilePath())) == firstId,"Rename with unresolved filesystem ID preserves UUID");

        auto a = make_ptr<CatalogTestSceneEntry>(); a->getHandle()->setUUID(StringUtil::getUUID());
        auto b = make_ptr<CatalogTestSceneEntry>(); b->getHandle()->setUUID(StringUtil::getUUID());
        catalog.addResourceEntry(a); catalog.addResourceEntry(b);
        require(catalog.hasResourceById(a->getHandle()->getUUIDAsString()) && catalog.hasResourceById(b->getHandle()->getUUIDAsString()),"Scene entries must insert independently");
        require(!catalog.getResourceEntryFromPath("scene"),"Shared scene path lookup must reject ambiguity");
        catalog.removeResourceEntry(a);
        require(!catalog.hasResourceById(a->getHandle()->getUUIDAsString()) && catalog.hasResourceById(b->getHandle()->getUUIDAsString()),"Scene deletion must target just its UUID");
        catalog.removeResourceEntryFromPath("scene");
        require(catalog.hasResourceById(b->getHandle()->getUUIDAsString()),"Path deletion must never remove shared scene rows");

        first->setFilePath(second->getFilePath()); catalog.removeResourceEntry(first);
        require(catalog.getResourceEntryFromPath(second->getFilePath()) != nullptr,"Stale object path cannot broaden UUID deletion");
        catalog.removeResourceEntryFromPath(second->getFilePath());
        require(!catalog.hasResourceById(second->getHandle()->getUUIDAsString()),"Explicit file path deletion must remove matching asset");

        auto persisted = asset("textures/persist.tex"); catalog.addResourceEntry(persisted);
        const auto persistedId = persisted->getHandle()->getUUIDAsString();
        std::atomic<bool> readsOkay{true};
        auto reader = [&] {
            try
            {
                for(int i=0;i<100;++i)
                    if(entryId(catalog.getResourceEntry(persistedId)) != persistedId) readsOkay=false;
            }
            catch(...) { readsOkay=false; }
        };
        std::thread one(reader), two(reader); one.join(); two.join();
        require(readsOkay,"Concurrent catalog reads must retain identity");
        catalog.loadFromFile(StringW((folder / "other.db").wstring().c_str()));
        require(!catalog.hasResourceById(persistedId),"Wide-string switch must select database B");
        catalog.loadFromFile(String((folder / "catalog.db").string().c_str()));
        require(entryId(catalog.getResourceEntryFromPath(persisted->getFilePath())) == persistedId,"Reopen/switch must preserve persistent identity");
        db = catalog.getDatabase(); bound = dynamic_cast<IParameterizedDatabase *>(db.get());
        require(bound && !bound->queryBound("INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",{persistedId,"different.path","Test"}),"Unique UUID index must reject duplicates");
        auto later = asset("later.mesh"); catalog.addResourceEntry(later);
        require(catalog.hasResourceById(later->getHandle()->getUUIDAsString()),"Failed mutation must not strand transaction");
        require(!catalog.getResourceEntryFromPath(String(1025,'x')),"Oversized catalog key must reject cleanly");
        catalog.destroy();
        require(!catalog.getDatabase(),"Destroy must release connection");
        catalog.loadFromFile(String((folder / "catalog.db").string().c_str()));
        require(catalog.hasResourceById(persistedId),"Destroy must preserve persisted catalog data");
        catalog.clearDatabase();
        require(!catalog.hasResourceById(persistedId),"Explicit clear must remove rows");
    }
}
int main()
{
    TypeManager types; types.load(); TypeManager::setInstance(&types);
    auto app = make_ptr<core::ApplicationManager>(); core::IApplicationManager::setInstance(app);
    app->setLogManager(make_ptr<LogManagerDefault>());
    auto factories = make_ptr<FactoryManager>(); factories->load(nullptr); app->setFactoryManager(factories);
    app->setFileSystem(make_ptr<FileSystem>());
    auto plugin = make_ptr<SQLitePlugin>(); plugin->load(nullptr);
    const auto temp = std::filesystem::temp_directory_path();
    const auto folder = temp / (std::string("workphone_catalog_") + StringUtil::getUUID().c_str());
    std::filesystem::create_directories(folder);
    int result = 0;
    {
        auto catalog = make_ptr<AssetDatabaseManager>();
        try
        {
            catalog->loadFromFile(String((folder / "catalog.db").string().c_str()));
            require(catalog->getDatabase() && catalog->getDatabase()->isLoaded(),"Real SQLite backend must open");
            contracts(*catalog,folder);
            std::cout << "PASS: catalog identity, parameter binding, scoped deletion, rollback, detached lookups, concurrency and persistence\n";
            migrationContracts(folder);
            std::cout << "PASS: versioned catalog migration, Unicode identity, retained backup, invalid-row rollback and future-version rejection\n";
        }
        catch(const std::exception &error) { std::cerr << "FAIL: " << error.what() << '\n'; result=1; }
        catalog->unload(nullptr); catalog=nullptr;
    }
    plugin->unload(nullptr); plugin=nullptr;
    app->setFileSystem(nullptr); app->setFactoryManager(nullptr);
    factories->unload(nullptr); factories=nullptr;
    core::IApplicationManager::setInstance(nullptr); app=nullptr;
    TypeManager::setInstance(nullptr); types.unload();
    if(folder.parent_path() == temp && folder.filename().string().find("workphone_catalog_") == 0)
        std::filesystem::remove_all(folder);
    return result;
}
