#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/System/Resource.hpp>
#include <Workphone/Core/LogManagerDefault.hpp>
#include <WPSQLite/WPSQLite.hpp>
#include "AssetCatalogPathContracts.hpp"
#ifdef WP_CATALOG_RESOURCE_SYSTEM_TESTS
#    include "CatalogResourceAdapterContracts.hpp"
#endif
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
    WP_CLASS_REGISTER_DERIVED( workphone, CatalogTestResource, Resource<IResource> );
    class CatalogTestSceneEntry : public ISharedObject
    {
    public:
        WP_CLASS_REGISTER_DECL;
    };
    WP_CLASS_REGISTER_DERIVED( workphone, CatalogTestSceneEntry, ISharedObject );
}  // namespace workphone
using namespace workphone;
namespace
{
    void require( bool value, const char *message )
    {
        if( !value )
            throw std::runtime_error( message );
    }
    SmartPtr<CatalogTestResource> asset( const String &path )
    {
        auto result = make_ptr<CatalogTestResource>();
        result->getHandle()->setUUID( StringUtil::getUUID() );
        result->setFilePath( path );
        return result;
    }
    String entryId( SmartPtr<IBuildDirector> entry )
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
        require( scalar( database, "SELECT version AS n FROM wp_asset_catalog_schema WHERE id=1" ) == 2,
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
        require( scalar( database, "SELECT count(*) AS n FROM wp_asset_catalog_backup_v1" ) == 3,
                 "Canonical identity migration must retain pre-v2 rows" );
        require(
            !sql( database, "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                  { "legacy-texture", "textures/duplicate.tex", "Texture", "textures/duplicate.tex" } ),
            "Migration must replace a nonunique UUID guard with a unique index" );
        auto userVersion = sql( database, "PRAGMA user_version" );
        require( userVersion && userVersion->getFieldValueAsInt( "user_version" ) == 42,
                 "Catalog versioning must not overwrite another subsystem's user_version" );
        require(
            !sql( database, "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                  { "invalid-empty", "", "Texture", "" } ),
            "Schema must reject empty paths through direct SQL" );
        require( !sql( database, "UPDATE resources SET uuid='' WHERE uuid=?", { "legacy-texture" } ),
                 "Schema must reject invalid identity updates" );
        require(
            !sql( database, "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                  { "invalid-long", String( 1025, 'x' ), "Texture", String( 1025, 'x' ) } ),
            "Schema must reject oversized keys" );
        require( !sql( database,
                       "INSERT INTO resources(uuid,path,type,kind,path_key) "
                       "VALUES('invalid-nul',CAST(X'610062' AS "
                       "TEXT),'Texture','file',CAST(X'610062' AS TEXT))" ),
                 "Schema must reject embedded NULs even through direct SQL" );
        require( scalar( database, "SELECT count(*) AS n FROM resources" ) == 3,
                 "Rejected mutations must preserve the catalog" );
        require( sql( database, "UPDATE resources SET path=?,path_key=? WHERE uuid=?",
                      { "textures/renamed.tex", "textures/renamed.tex", "legacy-texture" } ) != nullptr,
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

        for( const auto fixture :
             { "duplicate-uuid", "duplicate-path", "invalid-key", "embedded-nul", "backup-conflict",
               "metadata-conflict", "future-version", "canonical-collision" } )
        {
            const auto rejectedPath = folder / ( std::string( fixture ) + ".db" );
            database = openDatabase( rejectedPath );
            seedLegacyCatalog( database );
            const String secondUUID = String( fixture ) == "duplicate-uuid" ? "first-id" : "second-id";
            const String secondPath = String( fixture ) == "duplicate-path"        ? "first.mesh"
                                      : String( fixture ) == "invalid-key"         ? ""
                                      : String( fixture ) == "canonical-collision" ? "./first.mesh"
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
                require( sql( database, "INSERT INTO wp_asset_catalog_schema VALUES(1,3)" ) != nullptr,
                         "Future schema version must seed" );
            }
            database->close();
            database = nullptr;
            catalog->loadFromFile( String( rejectedPath.string().c_str() ) );
            require( !catalog->getDatabase() || !catalog->getDatabase()->isLoaded(),
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
                                 "SELECT version AS n FROM wp_asset_catalog_schema WHERE id=1" ) == 3,
                         "Newer schema must never be downgraded" );
            database->close();
            database = nullptr;
        }
    }
    void identityLifecycleContracts( const std::filesystem::path &folder )
    {
        const auto project = folder / "identity-project";
        const auto relocated = folder / "identity-relocated";
        std::filesystem::create_directories( project / "textures" );
        std::filesystem::create_directories( relocated / "textures" );
        auto app = core::IApplicationManager::instance();
        const auto oldProject = app->getProjectPath();
        app->setProjectPath( project.generic_u8string().c_str() );
        const String databasePath = ( folder / "identity.db" ).generic_u8string().c_str();
        auto catalog = make_ptr<AssetDatabaseManager>();
        catalog->loadFromFile( databasePath );
        const auto firstGeneration = catalog->getCatalogGeneration();
        app->setProjectPath( relocated.generic_u8string().c_str() );
        catalog->loadFromFile( databasePath );
        require( catalog->getCatalogGeneration() != firstGeneration &&
                     std::filesystem::equivalent(
                         std::filesystem::u8path( catalog->getProjectRoot().c_str() ), relocated ),
                 "Loading the same database after a project switch must rebind and invalidate" );
        app->setProjectPath( project.generic_u8string().c_str() );
        catalog->loadFromFile( databasePath );
        require( std::filesystem::equivalent(
                     std::filesystem::u8path( catalog->getProjectRoot().c_str() ), project ),
                 "Opening must capture the application's project root" );
        auto database = catalog->getDatabase();
        require( database && database->isLoaded(), "Identity fixture must open real SQLite" );
        require( scalar( database,
                         "SELECT count(*) AS n FROM sqlite_master WHERE name IN "
                         "('wp_asset_catalog_backup_v0','wp_asset_catalog_backup_v1')" ) == 0,
                 "A fresh catalog must not manufacture migration backups" );

        auto first = asset( "textures/./hero.tex" );
        catalog->addResourceEntry( first );
        const auto uuid = first->getHandle()->getUUIDAsString();
        AssetDatabaseManager::EntrySnapshot snapshot;
        require( catalog->tryGetEntry( uuid, snapshot ) && snapshot.path == "textures/hero.tex" &&
                     snapshot.kind == AssetDatabaseManager::EntryKind::File &&
                     catalog->isEntryCurrent( snapshot ),
                 "Snapshots must expose canonical file identity and current generation" );
        require( entryId( catalog->getResourceEntryFromPath( "textures\\hero.tex" ) ) == uuid &&
                     entryId( catalog->getResourceEntryFromPath(
                         ( project / "textures" / "hero.tex" ).generic_u8string().c_str() ) ) == uuid,
                 "Separator aliases and absolute paths must resolve the same catalog UUID" );
#ifdef _WIN32
        require( entryId( catalog->getResourceEntryFromPath( "TEXTURES/HERO.TEX" ) ) == uuid,
                 "Windows ASCII case aliases must resolve the stored canonical spelling" );
#endif
        const auto generation = catalog->getCatalogGeneration();
        first->setFilePath( "textures\\hero.tex" );
        catalog->addResourceEntry( first );
        catalog->updateResourceEntry( first );
        require( catalog->getCatalogGeneration() == generation && catalog->isEntryCurrent( snapshot ),
                 "Idempotent insertion and equivalent-path updates must preserve snapshots" );
        auto conflict = asset( "textures/./hero.tex" );
        catalog->addResourceEntry( conflict );
        require( !catalog->hasResourceById( conflict->getHandle()->getUUIDAsString() ) &&
                     catalog->getCatalogGeneration() == generation,
                 "Canonical path collision must reject without invalidating existing identity" );
        auto invalid = asset( "../escaped.tex" );
        catalog->addResourceEntry( invalid );
        require( !catalog->hasResourceById( invalid->getHandle()->getUUIDAsString() ) &&
                     catalog->getCatalogGeneration() == generation,
                 "Outside-root resource must reject without publishing a mutation" );
        auto detached = snapshot;
        detached.path = "tampered.tex";
        require( !catalog->isEntryCurrent( detached ) && catalog->isEntryCurrent( snapshot ),
                 "Detached snapshot edits must not change authoritative identity" );
#ifdef _WIN32
        {
            std::ofstream sourceFile( project / "textures" / "HeRo.TeX" );
            sourceFile << "source appeared after catalog insertion";
            require( sourceFile.good(), "Mixed-case source fixture must be writable" );
        }
        catalog->close();
        catalog->open();
        require( catalog->tryGetEntry( uuid, snapshot ) && snapshot.path == "textures/hero.tex" &&
                     entryId( catalog->getResourceEntryFromPath( "textures/HeRo.TeX" ) ) == uuid,
                 "A missing file appearing with different ASCII case must preserve stored identity" );
#endif

        auto sceneA = make_ptr<CatalogTestSceneEntry>();
        auto sceneB = make_ptr<CatalogTestSceneEntry>();
        sceneA->getHandle()->setUUID( StringUtil::getUUID() );
        sceneB->getHandle()->setUUID( StringUtil::getUUID() );
        catalog->addResourceEntry( sceneA );
        catalog->addResourceEntry( sceneB );
        auto namedScene = asset( "scene" );
        catalog->addResourceEntry( namedScene );
        AssetDatabaseManager::EntrySnapshot sceneSnapshot;
        require( catalog->tryGetEntry( sceneA->getHandle()->getUUIDAsString(), sceneSnapshot ) &&
                     sceneSnapshot.kind == AssetDatabaseManager::EntryKind::Scene &&
                     entryId( catalog->getResourceEntryFromPath( "scene" ) ) ==
                         namedScene->getHandle()->getUUIDAsString(),
                 "A file named scene must remain distinct from UUID-addressed scene entries" );
        catalog->removeResourceEntryFromPath( "scene" );
        require( !catalog->hasResourceById( namedScene->getHandle()->getUUIDAsString() ) &&
                     catalog->hasResourceById( sceneA->getHandle()->getUUIDAsString() ) &&
                     catalog->hasResourceById( sceneB->getHandle()->getUUIDAsString() ),
                 "Removing a file named scene must retain all scene entries" );
        require( !catalog->isEntryCurrent( snapshot ),
                 "A committed catalog mutation must invalidate older generation snapshots" );
        require( catalog->tryGetEntry( uuid, snapshot ),
                 "Original UUID must survive unrelated changes" );

        app->setProjectPath( relocated.generic_u8string().c_str() );
        require( std::filesystem::equivalent(
                     std::filesystem::u8path( catalog->getProjectRoot().c_str() ), project ) &&
                     !catalog->getResourceEntryFromPath(
                         ( relocated / "textures" / "hero.tex" ).generic_u8string().c_str() ),
                 "Changing global project state must not silently rebind an open catalog" );
        require( !catalog->setProjectRoot( relocated.generic_u8string().c_str() ) &&
                     catalog->isEntryCurrent( snapshot ),
                 "Explicit root changes while open must reject without changing generation" );
        catalog->close();
        require( !catalog->isEntryCurrent( snapshot ), "Close must invalidate outstanding snapshots" );
        require( catalog->setProjectRoot( relocated.generic_u8string().c_str() ),
                 "Closed catalog must permit explicit relocation" );
        catalog->open();
        require( catalog->tryGetEntry( uuid, snapshot ) && snapshot.path == "textures/hero.tex" &&
                     entryId( catalog->getResourceEntryFromPath(
                         ( relocated / "textures" / "hero.tex" ).generic_u8string().c_str() ) ) == uuid,
                 "Open after relocation must preserve relative path and durable UUID" );
        const auto beforeSwitch = snapshot;
        catalog->loadFromFile( ( folder / "identity-other.db" ).generic_u8string().c_str() );
        require( !catalog->isEntryCurrent( beforeSwitch ) && !catalog->hasResourceById( uuid ),
                 "Database switch must invalidate snapshots and hide the prior catalog" );
        catalog->loadFromFile( databasePath );
        require( !catalog->isEntryCurrent( beforeSwitch ) && catalog->tryGetEntry( uuid, snapshot ),
                 "Switching back must preserve rows without reviving old generations" );
        auto otherManager = make_ptr<AssetDatabaseManager>();
        require( otherManager->setProjectRoot( relocated.generic_u8string().c_str() ),
                 "Second manager root must bind" );
        otherManager->loadFromFile( databasePath );
        require( !otherManager->isEntryCurrent( snapshot ),
                 "A snapshot from another manager must not match even for the same database" );
        otherManager->unload( nullptr );
        catalog->unload( nullptr );
        require( !catalog->isEntryCurrent( snapshot ), "Unload must invalidate outstanding snapshots" );
        catalog->loadFromFile( databasePath );
        require( !catalog->isEntryCurrent( snapshot ) && catalog->tryGetEntry( uuid, snapshot ),
                 "Reload after unload must preserve rows with a new generation" );
        catalog->removeResourceEntry( first );
        require( !catalog->isEntryCurrent( snapshot ) && !catalog->hasResourceById( uuid ),
                 "UUID deletion must invalidate its detached identity" );
        catalog->unload( nullptr );
        app->setProjectPath( oldProject );
    }

    void versionOneMigrationContracts( const std::filesystem::path &folder )
    {
        for( const auto fixture :
             { "valid", "canonical-collision", "backup-collision", "foreign-guard",
               "foreign-guard-uppercase", "late-update-failure", "trigger-table-case" } )
        {
            const bool foreignGuard =
                String( fixture ) == "foreign-guard" || String( fixture ) == "foreign-guard-uppercase";
            const auto path = folder / ( std::string( "version-one-" ) + fixture + ".db" );
            auto database = openDatabase( path );
            seedLegacyCatalog( database );
            require(
                sql( database,
                     "CREATE TABLE wp_asset_catalog_schema(id INTEGER PRIMARY KEY,version INTEGER NOT "
                     "NULL)" ) &&
                    sql( database, "INSERT INTO wp_asset_catalog_schema VALUES(1,1)" ) &&
                    sql( database, "CREATE UNIQUE INDEX idx_resources_uuid_unique ON resources(uuid)" ),
                "Version-one fixture metadata and UUID guard must seed" );
            require( sql( database, "INSERT INTO resources(id,uuid,path,type) VALUES(9,?,?,?)",
                          { "v1-file", "textures/./paint.tex", "Texture" } ) &&
                         sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                              { "v1-scene", "scene", "Actor" } ),
                     "Version-one fixture must include file and scene rows" );
            require( sql( database, "CREATE TABLE wp_asset_catalog_backup_v0(note TEXT)" ) &&
                         sql( database,
                              "INSERT INTO wp_asset_catalog_backup_v0 VALUES('retained-original')" ),
                     "Version-one fixture must retain an independent legacy row snapshot" );
            if( String( fixture ) == "canonical-collision" )
                require( sql( database, "INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",
                              { "v1-conflict", "textures/paint.tex", "Texture" } ) != nullptr,
                         "Version-one canonical collision must seed" );
            if( String( fixture ) == "backup-collision" )
                require( sql( database, "CREATE TABLE wp_asset_catalog_backup_v1(note TEXT)" ) &&
                             sql( database,
                                  "INSERT INTO wp_asset_catalog_backup_v1 VALUES('do-not-replace')" ),
                         "Existing v1 backup collision must seed" );
            if( foreignGuard )
                require( sql( database, "CREATE TABLE unrelated(value INTEGER)" ) &&
                             sql( database, "INSERT INTO unrelated VALUES(7)" ) &&
                             sql( database, String( "CREATE TRIGGER " ) +
                                                ( String( fixture ) == "foreign-guard-uppercase"
                                                      ? "RESOURCES_VALUES_INSERT"
                                                      : "resources_values_insert" ) +
                                                " BEFORE INSERT ON unrelated BEGIN SELECT "
                                                "RAISE(ABORT,'Reserved foreign guard fixture'); END" ),
                         "Reserved catalog guard name on an unrelated table must seed" );
            const bool updateFailure =
                String( fixture ) == "late-update-failure" || String( fixture ) == "trigger-table-case";
            if( updateFailure )
            {
                // SQLite 3.7.9 reloads triggers after ALTER using a case-sensitive
                // sqlite_master.tbl_name comparison. Exact spelling tests late
                // rollback; mismatched spelling must fail preflight to avoid
                // silently bypassing existing custom trigger logic.
                const String tableName =
                    String( fixture ) == "late-update-failure" ? "Resources" : "resources";
                require(
                    sql( database,
                         "CREATE TRIGGER fixture_fail_identity_upgrade BEFORE UPDATE ON " + tableName +
                             " BEGIN SELECT RAISE(ABORT,'Late migration failure'); END" ) != nullptr,
                    "Late failure fixture must abort identity normalization after schema alteration" );
                require( scalar( database,
                                 "SELECT count(*) AS n FROM sqlite_master WHERE "
                                 "name='fixture_fail_identity_upgrade'" ) == 1 &&
                             !sql( database, "UPDATE resources SET path=path WHERE uuid='v1-file'" ),
                         "Late failure injection must demonstrably reject UPDATE before migration" );
            }
            database->close();
            database = nullptr;
            auto catalog = make_ptr<AssetDatabaseManager>();
            catalog->loadFromFile( path.generic_u8string().c_str() );
            const bool valid = String( fixture ) == "valid";
            require( valid ? catalog->getDatabase() && catalog->getDatabase()->isLoaded()
                           : !catalog->getDatabase() || !catalog->getDatabase()->isLoaded(),
                     "Version-one migration must either commit completely or fail closed" );
            if( valid )
            {
                AssetDatabaseManager::EntrySnapshot snapshot;
                require( catalog->tryGetEntry( "v1-file", snapshot ) &&
                             snapshot.path == "textures/paint.tex" &&
                             snapshot.kind == AssetDatabaseManager::EntryKind::File,
                         "Version-one upgrade must canonicalize file identity" );
                require( catalog->tryGetEntry( "v1-scene", snapshot ) &&
                             snapshot.kind == AssetDatabaseManager::EntryKind::Scene,
                         "Version-one sentinel must migrate to explicit scene kind" );
            }
            catalog->unload( nullptr );
            database = openDatabase( path );
            require( scalar( database, "SELECT version AS n FROM wp_asset_catalog_schema WHERE id=1" ) ==
                         ( valid ? 2 : 1 ),
                     "Failed identity upgrade must retain original schema version" );
            require( scalar( database, "SELECT id AS n FROM resources WHERE uuid='v1-file'" ) == 9,
                     "Identity migration must preserve existing SQLite row identity" );
            auto row = sql( database, "SELECT path FROM resources WHERE uuid='v1-file'" );
            require( row && row->getFieldValue( "path" ) ==
                                ( valid ? "textures/paint.tex" : "textures/./paint.tex" ),
                     "Failed migration must preserve the original unnormalized source path" );
            row = sql( database, "SELECT note FROM wp_asset_catalog_backup_v0" );
            require( row && row->getFieldValue( "note" ) == "retained-original",
                     "Canonical migration must never overwrite an earlier migration backup" );
            if( valid )
            {
                row =
                    sql( database, "SELECT path FROM wp_asset_catalog_backup_v1 WHERE uuid='v1-file'" );
                require( row && row->getFieldValue( "path" ) == "textures/./paint.tex",
                         "Version-two migration must snapshot original version-one rows" );
            }
            else if( String( fixture ) == "backup-collision" )
            {
                row = sql( database, "SELECT note FROM wp_asset_catalog_backup_v1" );
                require( row && row->getFieldValue( "note" ) == "do-not-replace",
                         "Backup-name collision must preserve the existing backup" );
            }
            else
                require( scalar( database,
                                 "SELECT count(*) AS n FROM sqlite_master WHERE "
                                 "name='wp_asset_catalog_backup_v1'" ) == 0,
                         "Failed canonical migration must roll back its new backup" );
            auto columns = sql( database, "PRAGMA table_info(resources)" );
            bool hasKind = false;
            while( columns && !columns->eof() )
            {
                hasKind = hasKind || columns->getFieldValue( "name" ) == "kind";
                columns->nextRow();
            }
            require( hasKind == valid, "Failed migration must roll back added identity columns" );
            if( updateFailure )
                require( scalar( database,
                                 "SELECT count(*) AS n FROM sqlite_master WHERE "
                                 "name='fixture_fail_identity_upgrade'" ) == 1 &&
                             !sql( database, "UPDATE resources SET path=path WHERE uuid='v1-file'" ),
                         "Failed schema upgrade must retain the trigger that caused the late failure" );
            if( foreignGuard )
            {
                row = sql( database,
                           "SELECT tbl_name FROM sqlite_master WHERE name='resources_values_insert' "
                           "COLLATE NOCASE" );
                require( row && row->getFieldValue( "tbl_name" ) == "unrelated" &&
                             scalar( database, "SELECT value AS n FROM unrelated" ) == 7 &&
                             !sql( database, "INSERT INTO unrelated VALUES(9)" ),
                         "Migration must preserve unrelated data and the reserved-name trigger" );
            }
            database->close();
        }
    }

    void versionTwoCorruptionContracts( const std::filesystem::path &folder )
    {
        for( const auto field : { "kind", "path_key", "version", "path_policy" } )
        {
            const auto path = folder / ( std::string( "version-two-corrupt-" ) + field + ".db" );
            auto catalog = make_ptr<AssetDatabaseManager>();
            catalog->loadFromFile( String( path.generic_u8string().c_str() ) );
            auto source = asset( "good.tex" );
            catalog->addResourceEntry( source );
            const auto uuid = source->getHandle()->getUUIDAsString();
            require( catalog->hasResourceById( uuid ), "Version-two corruption fixture must seed" );
            catalog->unload( nullptr );
            auto database = openDatabase( path );
            const bool metadata = String( field ) == "version" || String( field ) == "path_policy";
            if( !metadata )
                require( sql( database, "DROP TRIGGER resources_values_update" ) != nullptr,
                         "Corruption fixture must bypass the direct SQL value guard" );
            String hex = String( field ) == "kind"      ? "66696C650078"
                         : String( field ) == "version" ? "320078"
                                                        : "676F6F642E746578007374616C65";
            if( String( field ) == "path_policy" )
            {
                auto policy =
                    sql( database, "SELECT hex(path_policy) AS bytes FROM wp_asset_catalog_schema" );
                require( policy && !policy->eof(), "Current path policy must exist" );
                hex = policy->getFieldValue( "bytes" ) + "0078";
            }
            const String table = metadata ? "wp_asset_catalog_schema" : "resources";
            require( sql( database, String( "UPDATE " ) + table + " SET " + field + "=CAST(X'" + hex +
                                        "' AS TEXT)" ) != nullptr,
                     "Embedded-NUL identity corruption must seed below the manager API" );
            database->close();
            database = nullptr;
            catalog->loadFromFile( String( path.generic_u8string().c_str() ) );
            require( !catalog->getDatabase() || !catalog->getDatabase()->isLoaded(),
                     "Version-two reopen must reject embedded-NUL identity and schema metadata" );
            catalog->unload( nullptr );
            database = openDatabase( path );
            auto row = sql( database, String( "SELECT hex(" ) + field + ") AS bytes FROM " + table );
            require( row && row->getFieldValue( "bytes" ) == hex &&
                         scalar( database, "SELECT count(*) AS n FROM resources" ) == 1,
                     "Rejected corrupted reopen must retain original data and schema for repair" );
            require(
                scalar(
                    database,
                    "SELECT count(*) AS n FROM sqlite_master WHERE name='resources_values_update'" ) ==
                    ( metadata ? 1 : 0 ),
                "Failed reopen must not partially recreate schema guards" );
            database->close();
        }
    }

    void contracts( AssetDatabaseManager &catalog, const std::filesystem::path &folder )
    {
        auto db = catalog.getDatabase();
        auto bound = dynamic_cast<IParameterizedDatabase *>( db.get() );
        require( bound != nullptr, "SQLite parameterized backend is mandatory; cannot skip" );
        catalog.create();
        const auto generation = catalog.getCatalogGeneration();
        for( int i = 0; i < 10; ++i )
            catalog.loadFromFile( String( ( folder / "catalog.db" ).string().c_str() ) );
        require( catalog.getDatabase() == db && catalog.getCatalogGeneration() == generation,
                 "Repeated resource lookups must reuse the open catalog and its generation" );
        require( !catalog.getResourceEntryFromPath( "missing.mesh" ),
                 "Missing lookup must return null" );
        require( !catalog.getResourceEntry( "missing-id" ), "Missing UUID must return null" );
        auto count = bound->queryBound( "SELECT count(*) AS n FROM resources", {} );
        require( count && count->getFieldValueAsInt( "n" ) == 0, "Lookup must not persist an asset" );

        const String quoted = "characters/hero's '; DROP TABLE resources;--.mesh";
        auto first = asset( quoted );
        const auto firstId = first->getHandle()->getUUIDAsString();
        catalog.addResourceEntry( first );
        catalog.addResourceEntry( first );
        require( entryId( catalog.getResourceEntryFromPath( quoted ) ) == firstId,
                 "Quoted path must round-trip as bound data" );
        count = bound->queryBound( "SELECT count(*) AS n FROM resources", {} );
        require( count && count->getFieldValueAsInt( "n" ) == 1, "Repeated insert must be idempotent" );
        auto detached =
            dynamic_pointer_cast<scene::ResourceDirector>( catalog.getResourceEntry( firstId ) );
        detached->setResourcePath( "tampered" );
        require( catalog.getResourceEntryFromPath( quoted ) != nullptr,
                 "Detached director cannot mutate authoritative catalog" );

        auto conflict = asset( quoted );
        catalog.addResourceEntry( conflict );
        require( !catalog.hasResourceById( conflict->getHandle()->getUUIDAsString() ),
                 "Path collision cannot replace identity" );
        auto second = asset( "characters/second.mesh" );
        catalog.addResourceEntry( second );
        first->setFilePath( second->getFilePath() );
        catalog.updateResourceEntry( first );
        require( entryId( catalog.getResourceEntryFromPath( quoted ) ) == firstId,
                 "Failed rename must retain original row" );
        first->setFilePath( "characters/renamed.mesh" );
        first->setFileSystemId( workphone::UUID::generate() );
        catalog.updateResourceEntry( first );
        require( !catalog.getResourceEntryFromPath( quoted ), "Rename must remove old path" );
        require( entryId( catalog.getResourceEntryFromPath( first->getFilePath() ) ) == firstId,
                 "Rename with unresolved filesystem ID preserves UUID" );

        auto a = make_ptr<CatalogTestSceneEntry>();
        a->getHandle()->setUUID( StringUtil::getUUID() );
        auto b = make_ptr<CatalogTestSceneEntry>();
        b->getHandle()->setUUID( StringUtil::getUUID() );
        catalog.addResourceEntry( a );
        catalog.addResourceEntry( b );
        require( catalog.hasResourceById( a->getHandle()->getUUIDAsString() ) &&
                     catalog.hasResourceById( b->getHandle()->getUUIDAsString() ),
                 "Scene entries must insert independently" );
        require( !catalog.getResourceEntryFromPath( "scene" ),
                 "Shared scene path lookup must reject ambiguity" );
        catalog.removeResourceEntry( a );
        require( !catalog.hasResourceById( a->getHandle()->getUUIDAsString() ) &&
                     catalog.hasResourceById( b->getHandle()->getUUIDAsString() ),
                 "Scene deletion must target just its UUID" );
        catalog.removeResourceEntryFromPath( "scene" );
        require( catalog.hasResourceById( b->getHandle()->getUUIDAsString() ),
                 "Path deletion must never remove shared scene rows" );

        first->setFilePath( second->getFilePath() );
        catalog.removeResourceEntry( first );
        require( catalog.getResourceEntryFromPath( second->getFilePath() ) != nullptr,
                 "Stale object path cannot broaden UUID deletion" );
        catalog.removeResourceEntryFromPath( second->getFilePath() );
        require( !catalog.hasResourceById( second->getHandle()->getUUIDAsString() ),
                 "Explicit file path deletion must remove matching asset" );

        auto persisted = asset( "textures/persist.tex" );
        catalog.addResourceEntry( persisted );
        const auto persistedId = persisted->getHandle()->getUUIDAsString();
        std::atomic<bool> readsOkay{ true };
        auto reader = [&] {
            try
            {
                for( int i = 0; i < 100; ++i )
                    if( entryId( catalog.getResourceEntry( persistedId ) ) != persistedId )
                        readsOkay = false;
            }
            catch( ... )
            {
                readsOkay = false;
            }
        };
        std::thread one( reader ), two( reader );
        one.join();
        two.join();
        require( readsOkay, "Concurrent catalog reads must retain identity" );
        catalog.loadFromFile( StringW( ( folder / "other.db" ).wstring().c_str() ) );
        require( !catalog.hasResourceById( persistedId ), "Wide-string switch must select database B" );
        catalog.loadFromFile( String( ( folder / "catalog.db" ).string().c_str() ) );
        require( entryId( catalog.getResourceEntryFromPath( persisted->getFilePath() ) ) == persistedId,
                 "Reopen/switch must preserve persistent identity" );
        db = catalog.getDatabase();
        bound = dynamic_cast<IParameterizedDatabase *>( db.get() );
        require(
            bound && !bound->queryBound(
                         "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                         { persistedId, "different.path", "Test", "different.path" } ),
            "Unique UUID index must reject duplicates" );
        auto later = asset( "later.mesh" );
        catalog.addResourceEntry( later );
        require( catalog.hasResourceById( later->getHandle()->getUUIDAsString() ),
                 "Failed mutation must not strand transaction" );
        require( !catalog.getResourceEntryFromPath( String( 1025, 'x' ) ),
                 "Oversized catalog key must reject cleanly" );
        catalog.destroy();
        require( !catalog.getDatabase(), "Destroy must release connection" );
        catalog.loadFromFile( String( ( folder / "catalog.db" ).string().c_str() ) );
        require( catalog.hasResourceById( persistedId ),
                 "Destroy must preserve persisted catalog data" );
        catalog.clearDatabase();
        require( !catalog.hasResourceById( persistedId ), "Explicit clear must remove rows" );
    }
}  // namespace
int main()
{
    TypeManager types;
    types.load();
    TypeManager::setInstance( &types );
    auto app = make_ptr<core::ApplicationManager>();
    core::IApplicationManager::setInstance( app );
    app->setLogManager( make_ptr<LogManagerDefault>() );
    auto factories = make_ptr<FactoryManager>();
    factories->load( nullptr );
    app->setFactoryManager( factories );
    app->setFileSystem( make_ptr<FileSystem>() );
    auto plugin = make_ptr<SQLitePlugin>();
    plugin->load( nullptr );
    const auto temp = std::filesystem::temp_directory_path();
    const auto folder = temp / ( std::string( "workphone_catalog_" ) + StringUtil::getUUID().c_str() );
    std::filesystem::create_directories( folder );
    int result = 0;
    {
        auto catalog = make_ptr<AssetDatabaseManager>();
        try
        {
            catalog->loadFromFile( String( ( folder / "catalog.db" ).string().c_str() ) );
            require( catalog->getDatabase() && catalog->getDatabase()->isLoaded(),
                     "Real SQLite backend must open" );
            contracts( *catalog, folder );
            std::cout << "PASS: catalog identity, parameter binding, scoped deletion, rollback, "
                         "detached lookups, concurrency and persistence\n";
            migrationContracts( folder );
            std::cout << "PASS: versioned catalog migration, Unicode identity, retained backup, "
                         "invalid-row rollback and future-version rejection\n";
            versionOneMigrationContracts( folder );
            versionTwoCorruptionContracts( folder );
            identityLifecycleContracts( folder );
            std::cout << "PASS: canonical catalog aliases, explicit kinds, project relocation, "
                         "generation lifecycle and version-one migration rollback\n";
            runAssetCatalogPathContracts( folder );
#ifdef WP_CATALOG_RESOURCE_SYSTEM_TESTS
            catalogResourceAdapterContracts( folder );
            std::cout << "PASS: catalog ResourceID bridge, compilation, dependencies, failed "
                         "replacement, stale request rejection and durable rename\n";
#endif
        }
        catch( const std::exception &error )
        {
            std::cerr << "FAIL: " << error.what() << '\n';
            result = 1;
        }
        catalog->unload( nullptr );
        catalog = nullptr;
    }
    plugin->unload( nullptr );
    plugin = nullptr;
    app->setFileSystem( nullptr );
    app->setFactoryManager( nullptr );
    factories->unload( nullptr );
    factories = nullptr;
    core::IApplicationManager::setInstance( nullptr );
    app = nullptr;
    TypeManager::setInstance( nullptr );
    types.unload();
    if( folder.parent_path() == temp && folder.filename().string().find( "workphone_catalog_" ) == 0 )
        std::filesystem::remove_all( folder );
    return result;
}
