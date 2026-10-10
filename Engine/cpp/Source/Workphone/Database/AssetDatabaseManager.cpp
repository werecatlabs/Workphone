#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Script/ScriptAsset.hpp>
#include <atomic>
#include <filesystem>
#include <set>
#include <map>
#include <fstream>
#include <stdexcept>
#if defined WP_PLATFORM_WIN32
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    include <Windows.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AssetDatabaseManager, DatabaseManager );
    namespace
    {
        std::atomic<u64> nextCatalogInstance{ 1 };
#ifdef _WIN32
        const String pathPolicy = "windows-ascii-fold";
#else
        const String pathPolicy = "exact";
#endif

        bool validValue( const String &value, size_t limit )
        {
            return !value.empty() && value.size() <= limit && value.find( '\0' ) == String::npos;
        }
        SmartPtr<IDatabaseQuery> queryCatalog( AssetDatabaseManager &manager, const String &sql,
                                               const Array<String> &values = {} )
        {
            auto database = manager.getDatabase();
            auto bound = database ? dynamic_cast<IParameterizedDatabase *>( database.get() ) : nullptr;
            if( !bound || !database->isLoaded() )
            {
                WP_LOG_ERROR( "Asset catalog requires an open parameterized database backend." );
                return nullptr;
            }
            return bound->queryBound( sql, values );
        }
        // The manager's recursive mutex covers the transaction and publication.
        class CatalogTransaction
        {
        public:
            explicit CatalogTransaction( AssetDatabaseManager &manager ) : m_manager( manager )
            {
                m_database = manager.getDatabase();
                m_serialized = m_database ? dynamic_cast<ISerializedDatabase *>( m_database.get() )
                                          : nullptr;
                if( !m_serialized )
                    return;
                m_serialized->lockConnection();
                m_active = queryCatalog( manager, "BEGIN IMMEDIATE" ) != nullptr;
            }
            ~CatalogTransaction()
            {
                if( m_active )
                    queryCatalog( m_manager, "ROLLBACK" );
                if( m_serialized )
                    m_serialized->unlockConnection();
            }
            bool active() const
            {
                return m_active;
            }
            bool commit()
            {
                if( !m_active || !queryCatalog( m_manager, "COMMIT" ) )
                    return false;
                m_active = false;
                return true;
            }

        private:
            AssetDatabaseManager &m_manager;
            SmartPtr<IDatabase> m_database;
            ISerializedDatabase *m_serialized = nullptr;
            bool m_active = false;
        };
        String resourcePath( SmartPtr<ISharedObject> object )
        {
            if( !object || object->isDerived<scene::IGameActor>() ||
                object->isDerived<scene::IComponent>() || !object->isDerived<IResource>() )
                return "scene";
            auto resource = static_pointer_cast<IResource>( object );
            String path;
            auto app = core::IApplicationManager::instancePtr();
            auto filesystem = app ? app->getFileSystem() : nullptr;
            if( filesystem && !resource->getFileSystemId().is_nil() )
            {
                FileInfo info;
                if( filesystem->findFileInfo( resource->getFileSystemId(), info ) )
                    path = info.filePath.str();
            }
            return path.empty() ? resource->getFilePath() : path;
        }
        AssetDatabaseManager::EntryKind resourceKind( SmartPtr<ISharedObject> object )
        {
            return object && object->isDerived<IResource>() && !object->isDerived<scene::IGameActor>() &&
                           !object->isDerived<scene::IComponent>()
                       ? AssetDatabaseManager::EntryKind::File
                       : AssetDatabaseManager::EntryKind::Scene;
        }
        bool resolveResourcePath( AssetDatabaseManager &manager, SmartPtr<ISharedObject> object,
                                  AssetCatalogPath &path )
        {
            if( resourceKind( object ) == AssetDatabaseManager::EntryKind::Scene )
            {
                path.path = "scene";
                path.key.clear();
                return true;
            }
            String error;
            if( canonicalAssetCatalogPath( manager.getProjectRoot(), resourcePath( object ), path,
                                           error ) )
                return true;
            WP_LOG_ERROR( "Invalid catalog source path: " + error );
            return false;
        }
        String kindName( AssetDatabaseManager::EntryKind kind )
        {
            return kind == AssetDatabaseManager::EntryKind::File ? "file" : "scene";
        }

        // Version 2 stores explicit file/scene kinds and a canonical file lookup key.
        // Keep this separate from ResourceSystem's compilation metadata and user_version.
        bool migrateIdentity( AssetDatabaseManager &manager, const String &root, bool backup )
        {
            auto rows = queryCatalog( manager, "SELECT uuid,path FROM resources ORDER BY id" );
            if( !rows )
                return false;
            struct Identity
            {
                String uuid;
                AssetCatalogPath path;
                String kind;
            };
            Array<Identity> identities;
            std::set<std::string> keys;
            while( !rows->eof() )
            {
                Identity identity;
                identity.uuid = rows->getFieldValue( "uuid" );
                const auto oldPath = rows->getFieldValue( "path" );
                identity.kind = oldPath == "scene" ? "scene" : "file";
                if( identity.kind == "scene" )
                    identity.path.path = "scene";
                else
                {
                    String error;
                    if( !canonicalAssetCatalogPath( root, oldPath, identity.path, error ) ||
                        !keys.insert( identity.path.key.c_str() ).second )
                    {
                        WP_LOG_ERROR( "Catalog path migration needs explicit repair: " + oldPath + " " +
                                      error );
                        return false;
                    }
                }
                identities.push_back( identity );
                rows->nextRow();
            }
            if( backup &&
                !queryCatalog( manager,
                               "CREATE TABLE wp_asset_catalog_backup_v1 AS SELECT * FROM resources" ) )
                return false;
            for( const auto statement :
                 { "DROP TRIGGER IF EXISTS resources_file_path_insert",
                   "DROP TRIGGER IF EXISTS resources_file_path_update",
                   "DROP TRIGGER IF EXISTS resources_values_insert",
                   "DROP TRIGGER IF EXISTS resources_values_update",
                   "ALTER TABLE resources ADD COLUMN kind TEXT NOT NULL DEFAULT 'file'",
                   "ALTER TABLE resources ADD COLUMN path_key TEXT NOT NULL DEFAULT ''",
                   "ALTER TABLE wp_asset_catalog_schema ADD COLUMN path_policy TEXT NOT NULL DEFAULT "
                   "'legacy'" } )
                if( !queryCatalog( manager, statement ) )
                    return false;
            for( const auto &identity : identities )
                if( !queryCatalog(
                        manager, "UPDATE resources SET path=?,kind=?,path_key=? WHERE uuid=?",
                        { identity.path.path, identity.kind, identity.path.key, identity.uuid } ) )
                    return false;
            return queryCatalog( manager,
                                 "UPDATE wp_asset_catalog_schema SET version=2,path_policy=? WHERE id=1",
                                 { pathPolicy } ) != nullptr;
        }

        String invalidIdentityFields();
        bool validateIdentity( AssetDatabaseManager &manager, const String &root )
        {
            auto policy = queryCatalog( manager,
                                        "SELECT path_policy FROM wp_asset_catalog_schema WHERE id=1 AND "
                                        "typeof(path_policy)='text' AND path_policy=?",
                                        { pathPolicy } );
            if( !policy || policy->eof() || policy->getFieldValue( "path_policy" ) != pathPolicy )
                return false;
            auto invalid = queryCatalog( manager, "SELECT id FROM resources AS NEW WHERE " +
                                                      invalidIdentityFields() + " LIMIT 1" );
            if( !invalid || !invalid->eof() )
                return false;
            auto rows = queryCatalog( manager, "SELECT uuid,path,type,kind,path_key FROM resources" );
            if( !rows )
                return false;
            std::set<std::string> ids, keys;
            while( !rows->eof() )
            {
                const auto uuid = rows->getFieldValue( "uuid" );
                const auto path = rows->getFieldValue( "path" );
                const auto kind = rows->getFieldValue( "kind" );
                const auto key = rows->getFieldValue( "path_key" );
                if( !validValue( uuid, 256 ) || !validValue( rows->getFieldValue( "type" ), 256 ) ||
                    !ids.insert( uuid.c_str() ).second )
                    return false;
                if( kind == "file" )
                {
                    AssetCatalogPath canonical;
                    String error;
                    if( !canonicalAssetCatalogPath( root, path, canonical, error ) ||
                        canonical.key != key || !keys.insert( key.c_str() ).second )
                        return false;
                }
                else if( kind != "scene" || path != "scene" || !key.empty() )
                    return false;
                rows->nextRow();
            }
            return true;
        }

        String invalidIdentityFields()
        {
            auto invalidValue = []( const String &field, const String &limit ) {
                return "typeof(" + field + ")<>'text' OR " + field + "='' OR length(CAST(" + field +
                       " AS BLOB))>" + limit + " OR hex(" + field + ")<>hex(substr(" + field +
                       ",1,length(" + field + ")))";
            };
            String invalid = invalidValue( "NEW.uuid", "256" ) + " OR " +
                             invalidValue( "NEW.path", "1024" ) + " OR " +
                             invalidValue( "NEW.type", "256" ) +
                             " OR typeof(NEW.kind)<>'text' OR NEW.kind NOT IN ('file','scene')"
                             " OR typeof(NEW.path_key)<>'text'"
                             " OR (NEW.kind='scene' AND (NEW.path<>'scene' OR NEW.path_key<>''))"
                             " OR (NEW.kind='file' AND (" +
                             invalidValue( "NEW.path_key", "1024" ) +
                             " OR NEW.path LIKE '/%' OR NEW.path LIKE '%//%'"
                             " OR NEW.path<>replace(NEW.path,'\\','/')"
                             " OR NEW.path IN ('.','..') OR NEW.path LIKE './%' OR NEW.path LIKE '../%'"
                             " OR NEW.path LIKE '%/./%' OR NEW.path LIKE '%/../%'"
                             " OR NEW.path LIKE '%/.' OR NEW.path LIKE '%/..' OR NEW.path LIKE '%/'";
#ifdef _WIN32
            invalid += " OR NEW.path LIKE '%:%' OR NEW.path_key<>lower(NEW.path)";
#else
            invalid += " OR NEW.path_key<>NEW.path";
#endif
            invalid += "))";
            return invalid;
        }
        bool installChangeTracking( AssetDatabaseManager &manager )
        {
            if( !queryCatalog( manager,
                    "CREATE TABLE IF NOT EXISTS wp_asset_catalog_changes("
                    "id INTEGER PRIMARY KEY CHECK(id=1),revision INTEGER NOT NULL "
                    "CHECK(typeof(revision)='integer' AND revision>=0))" ) ||
                !queryCatalog( manager,
                    "INSERT OR IGNORE INTO wp_asset_catalog_changes(id,revision) VALUES(1,0)" ) )
                return false;
            auto revision = queryCatalog( manager,
                "SELECT revision FROM wp_asset_catalog_changes WHERE id=1 AND "
                "typeof(revision)='integer' AND revision>=0" );
            if( !revision || revision->eof() )
                return false;
            for( const auto &event : { String("INSERT"), String("UPDATE"), String("DELETE") } )
            {
                const auto name = "wp_catalog_revision_" + event;
                auto owner = queryCatalog( manager,
                    "SELECT tbl_name FROM sqlite_master WHERE type='trigger' AND name=?", {name} );
                if( !owner || ( !owner->eof() && owner->getFieldValue("tbl_name") != "resources" ) ||
                    !queryCatalog( manager, "DROP TRIGGER IF EXISTS " + name ) ||
                    !queryCatalog( manager, "CREATE TRIGGER " + name + " AFTER " + event +
                        " ON resources BEGIN UPDATE wp_asset_catalog_changes "
                        "SET revision=revision+1 WHERE id=1; END" ) )
                    return false;
            }
            return true;
        }
        bool guardIdentity( AssetDatabaseManager &manager )
        {
            const auto invalid = invalidIdentityFields();
            // These names belong to this catalog. Rebuild them transactionally so
            // older or incorrectly non-unique guards cannot silently survive.
            for( const auto statement : { "DROP INDEX IF EXISTS idx_resources_uuid_unique",
                                          "DROP INDEX IF EXISTS idx_resources_kind_path",
                                          "DROP TRIGGER IF EXISTS resources_file_path_insert",
                                          "DROP TRIGGER IF EXISTS resources_file_path_update",
                                          "DROP TRIGGER IF EXISTS resources_values_insert",
                                          "DROP TRIGGER IF EXISTS resources_values_update" } )
                if( !queryCatalog( manager, statement ) )
                    return false;
            return queryCatalog( manager,
                                 "CREATE UNIQUE INDEX IF NOT EXISTS idx_resources_uuid_unique ON "
                                 "resources(uuid)" ) &&
                   queryCatalog( manager,
                                 "CREATE INDEX IF NOT EXISTS idx_resources_kind_path ON "
                                 "resources(kind,path_key)" ) &&
                   queryCatalog( manager,
                                 "CREATE TRIGGER IF NOT EXISTS resources_file_path_insert BEFORE INSERT "
                                 "ON resources "
                                 "WHEN NEW.kind='file' AND EXISTS(SELECT 1 FROM resources WHERE "
                                 "kind='file' AND path_key=NEW.path_key) "
                                 "BEGIN SELECT RAISE(ABORT,'Conflicting canonical asset path'); END" ) &&
                   queryCatalog( manager,
                                 "CREATE TRIGGER IF NOT EXISTS resources_file_path_update BEFORE UPDATE "
                                 "OF path_key,kind ON resources "
                                 "WHEN NEW.kind='file' AND EXISTS(SELECT 1 FROM resources WHERE "
                                 "kind='file' AND path_key=NEW.path_key AND id<>NEW.id) "
                                 "BEGIN SELECT RAISE(ABORT,'Conflicting canonical asset path'); END" ) &&
                   queryCatalog( manager,
                                 "CREATE TRIGGER IF NOT EXISTS resources_values_insert BEFORE INSERT ON "
                                 "resources WHEN " +
                                     invalid +
                                     " BEGIN SELECT RAISE(ABORT,'Invalid catalog identity'); END" ) &&
                   queryCatalog( manager,
                                 "CREATE TRIGGER IF NOT EXISTS resources_values_update BEFORE UPDATE OF "
                                 "uuid,path,type,kind,path_key ON resources WHEN " +
                                     invalid +
                                     " BEGIN SELECT RAISE(ABORT,'Invalid catalog identity'); END" ) &&
                   installChangeTracking( manager );
        }
    }  // namespace
    AssetDatabaseManager::AssetDatabaseManager()
    {
        m_resourcesTableName = "resources";
        m_catalogInstance = nextCatalogInstance.fetch_add( 1 );
    }
    AssetDatabaseManager::~AssetDatabaseManager() = default;
    void AssetDatabaseManager::invalidateCatalog()
    {
        ++m_catalogGeneration;
        clearResourceEntryCache();
    }
    bool AssetDatabaseManager::setProjectRoot( const String &root )
    {
        ScopedLock lock( this );
        if( auto database = getDatabase(); database && database->isLoaded() )
            return false;
        String canonical, error;
        if( !canonicalAssetCatalogRoot( root, canonical, error ) )
        {
            WP_LOG_ERROR( "Invalid catalog project root: " + error );
            return false;
        }
        m_projectRootOverride = canonical;
        m_projectRoot = canonical;
        invalidateCatalog();
        return true;
    }
    String AssetDatabaseManager::getProjectRoot()
    {
        ScopedLock lock( this );
        return m_projectRoot;
    }
    u64 AssetDatabaseManager::getCatalogGeneration()
    {
        ScopedLock lock( this );
        if( m_catalogReady )
            refreshDatabaseRevision();
        return m_catalogGeneration;
    }
    bool AssetDatabaseManager::refreshDatabaseRevision()
    {
        auto row = queryCatalog( *this,
            "SELECT revision FROM wp_asset_catalog_changes WHERE id=1" );
        if( !row || row->eof() )
        {
            invalidateCatalog();
            return false;
        }
        const auto revision = row->getFieldValue( "revision" );
        if( revision != m_databaseRevision )
        {
            m_databaseRevision = revision;
            invalidateCatalog();
        }
        return true;
    }
    bool AssetDatabaseManager::captureProjectRoot()
    {
        String root = m_projectRootOverride;
        if( root.empty() )
        {
            auto application = core::IApplicationManager::instancePtr();
            root = application ? application->getProjectPath() : String();
        }
        if( root.empty() )
        {
            std::error_code error;
            const auto databasePath = getDatabasePath();
            if( databasePath.empty() )
                return false;
            const auto absolute =
                std::filesystem::absolute( std::filesystem::u8path( databasePath.c_str() ), error );
            if( error )
                return false;
            root = absolute.parent_path().generic_u8string().c_str();
        }
        String error;
        if( !canonicalAssetCatalogRoot( root, m_projectRoot, error ) )
        {
            WP_LOG_ERROR( "Cannot capture catalog project root: " + error );
            return false;
        }
        return true;
    }
    void AssetDatabaseManager::open()
    {
        ScopedLock lock( this );
        loadFromFile( getDatabasePath() );
        if( m_catalogReady )
            for( auto database : m_attached )
                attach( database );
    }
    void AssetDatabaseManager::close()
    {
        ScopedLock lock( this );
        m_catalogReady = false;
        invalidateCatalog();
        DatabaseManager::close();
    }
    void AssetDatabaseManager::load( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        DatabaseManager::load( data );
    }
    void AssetDatabaseManager::unload( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock( this );
        m_catalogReady = false;
        invalidateCatalog();
        if( auto db = getDatabase() )
            db->close();
        setDatabase( nullptr );
        clearResourceEntryCache();
        DatabaseManager::unload( data );
    }
    void AssetDatabaseManager::loadFromFile( const String &path )
    {
        ScopedLock lock( this );
        // Resource lookups repeatedly request this connection. Reopening and
        // validating the catalog per mesh/material makes procedural startup slow.
        // The same relative name can still refer to a different project.
        if( m_catalogReady && path == getDatabasePath() )
            if( auto database = getDatabase(); database && database->isLoaded() )
            {
                const auto previousRoot = m_projectRoot;
                if( captureProjectRoot() && m_projectRoot == previousRoot )
                    return;
            }
        close();
        m_projectRoot.clear();
        DatabaseManager::loadFromFile( path );
        create();
    }
    void AssetDatabaseManager::loadFromFile( const StringW &path )
    {
        // Both overloads use the same switching and project path policy.
        loadFromFile( StringUtilW::toUTF16to8( path ) );
    }
    void AssetDatabaseManager::create()
    {
        ScopedLock lock( this );
        if( m_catalogReady && getDatabase() && getDatabase()->isLoaded() )
            return;
        // Configure authored-store durability before BEGIN; SQLite ignores
        // foreign_keys changes and rejects synchronous changes inside a transaction.
        if( !queryCatalog( *this, "PRAGMA foreign_keys=ON" ) ||
            !queryCatalog( *this, "PRAGMA synchronous=FULL" ) )
        {
            close();
            return;
        }
        auto durability = queryCatalog( *this, "PRAGMA synchronous" );
        if( !durability || durability->getFieldValueAsInt( "synchronous" ) < 2 )
        {
            close();
            return;
        }
        bool ready = false;
        {
            CatalogTransaction transaction( *this );
            auto initialize = [&]() {
                if( !transaction.active() || !captureProjectRoot() )
                    return false;
                auto foreignGuard =
                    queryCatalog( *this,
                                  "SELECT name FROM sqlite_master WHERE name COLLATE NOCASE IN "
                                  "('idx_resources_uuid_unique','idx_resources_kind_path',"
                                  "'resources_file_path_insert','resources_file_path_update',"
                                  "'resources_values_insert','resources_values_update') "
                                  "AND tbl_name<>'resources' COLLATE NOCASE LIMIT 1" );
                if( !foreignGuard || !foreignGuard->eof() )
                    return false;
                auto resourceTable = queryCatalog( *this,
                                                   "SELECT name FROM sqlite_master WHERE type='table' "
                                                   "AND name='resources' COLLATE NOCASE" );
                auto schemaTable = queryCatalog( *this,
                                                 "SELECT name FROM sqlite_master WHERE type='table' AND "
                                                 "name='wp_asset_catalog_schema' COLLATE NOCASE" );
                if( !resourceTable || !schemaTable )
                    return false;
                const bool hadResources = !resourceTable->eof();
                const bool hadSchema = !schemaTable->eof();
                String schemaVersion;
                if( hadSchema )
                {
                    auto invalidMetadata = queryCatalog(
                        *this,
                        "SELECT id FROM wp_asset_catalog_schema WHERE typeof(id)<>'integer' OR "
                        "id<>1 OR typeof(version)<>'integer' OR version NOT IN (1,2) LIMIT 1" );
                    if( !invalidMetadata || !invalidMetadata->eof() )
                        return false;
                    auto version =
                        queryCatalog( *this, "SELECT id,version FROM wp_asset_catalog_schema" );
                    if( !hadResources || !version || version->eof() ||
                        version->getFieldValue( "id" ) != "1" ||
                        ( version->getFieldValue( "version" ) != "1" &&
                          version->getFieldValue( "version" ) != "2" ) )
                    {
                        WP_LOG_ERROR(
                            "Unsupported or invalid asset catalog schema version; catalog left "
                            "unchanged." );
                        return false;
                    }
                    schemaVersion = version->getFieldValue( "version" );
                    version->nextRow();
                    if( !version->eof() )
                        return false;
                }
                if( !queryCatalog( *this,
                                   "CREATE TABLE IF NOT EXISTS resources(id INTEGER PRIMARY KEY, uuid "
                                   "VARCHAR, path VARCHAR, type VARCHAR)" ) )
                    return false;
                // SQLite 3.7.9 has no partial indexes or instr(). The same value
                // predicate protects legacy migration and subsequent SQL writes.
                // Comparing the full text's hex with its length-bounded substring
                // also detects embedded NULs without rejecting Unicode paths.
                auto invalidFields = []( const String &prefix ) {
                    auto invalidField = [&]( const String &name, const String &limit ) {
                        const auto field = prefix + name;
                        String condition = "typeof(" + field + ")<>'text' OR " + field + "=''";
                        condition += " OR length(CAST(" + field + " AS BLOB))>" + limit;
                        condition +=
                            " OR hex(" + field + ")<>hex(substr(" + field + ",1,length(" + field + ")))";
                        return condition;
                    };
                    return invalidField( "uuid", "256" ) + " OR " + invalidField( "path", "1024" ) +
                           " OR " + invalidField( "type", "256" );
                };
                auto invalidRows = queryCatalog(
                    *this, "SELECT id FROM resources WHERE " + invalidFields( "" ) + " LIMIT 1" );
                if( !invalidRows || !invalidRows->eof() )
                {
                    WP_LOG_ERROR(
                        "Invalid legacy catalog identity, path or type; explicit repair required." );
                    return false;
                }
                if( schemaVersion == "2" )
                    return validateIdentity( *this, m_projectRoot ) && guardIdentity( *this ) &&
                           transaction.commit();
                if( hadResources )
                {
                    // SQLite 3.7.9 reloads triggers after ALTER using a binary
                    // tbl_name comparison. A custom trigger whose ON spelling
                    // differs from the actual table name would be silently skipped.
                    // Our own guards are replaced below; other triggers need repair
                    // before migration rather than bypassing their constraints.
                    auto unsafeTriggers = queryCatalog(
                        *this,
                        "SELECT name FROM sqlite_master WHERE type='trigger' AND "
                        "tbl_name='resources' COLLATE NOCASE AND tbl_name<>? COLLATE BINARY "
                        "AND name COLLATE NOCASE NOT IN ('resources_file_path_insert',"
                        "'resources_file_path_update','resources_values_insert',"
                        "'resources_values_update') LIMIT 1",
                        { resourceTable->getFieldValue( "name" ) } );
                    if( !unsafeTriggers || !unsafeTriggers->eof() )
                    {
                        WP_LOG_ERROR(
                            "Catalog migration requires custom trigger table spelling "
                            "to match the resources table exactly." );
                        return false;
                    }
                }
                const char *checks[] = {
                    "SELECT uuid FROM resources GROUP BY uuid HAVING count(*)>1 LIMIT 1",
                    "SELECT path FROM resources WHERE path<>'scene' GROUP BY path HAVING count(*)>1 "
                    "LIMIT 1"
                };
                for( const auto check : checks )
                {
                    auto rows = queryCatalog( *this, check );
                    if( !rows || !rows->eof() )
                        return false;
                }
                // Retain original legacy rows once. A colliding backup name fails
                // the transaction rather than overwriting potentially useful data.
                if( !hadSchema && hadResources &&
                    !queryCatalog(
                        *this, "CREATE TABLE wp_asset_catalog_backup_v0 AS SELECT * FROM resources" ) )
                    return false;
                if( !hadSchema )
                {
                    // Do not use user_version: other database services may own it.
                    if( !queryCatalog(
                            *this,
                            "CREATE TABLE wp_asset_catalog_schema("
                            "id INTEGER PRIMARY KEY CHECK(id=1),version INTEGER NOT NULL)" ) ||
                        !queryCatalog( *this,
                                       "INSERT INTO wp_asset_catalog_schema(id,version) VALUES(1,1)" ) )
                        return false;
                }
                return migrateIdentity( *this, m_projectRoot, hadResources ) &&
                       validateIdentity( *this, m_projectRoot ) && guardIdentity( *this ) &&
                       transaction.commit();
            };
            ready = initialize();
        }  // Roll back schema work before closing an unusable catalog.
        m_catalogReady = ready;
        m_databaseRevision.clear();
        invalidateCatalog();
        if( ready )
        {
            refreshDatabaseRevision();
            for( const auto &recovery : recoverFileOperations() )
                if( !recovery.succeeded )
                    WP_LOG_ERROR( "Asset operation recovery requires attention: " + recovery.error );
        }
        if( !ready )
        {
            WP_LOG_ERROR( "Catalog schema initialization failed; catalog changes rolled back." );
            if( auto database = getDatabase() )
                database->close();
        }
    }
    void AssetDatabaseManager::destroy()
    {
        unload( nullptr );
    }
    bool AssetDatabaseManager::backupTo( const String &path, String &error )
    {
        ScopedLock lock( this );
        error.clear();
        auto database = getDatabase();
        auto backup = database ? dynamic_cast<IBackupDatabase *>(database.get()) : nullptr;
        if( !m_catalogReady || !backup )
        {
            error = "Catalog backup requires an open snapshot-capable backend";
            return false;
        }
        return backup->backupTo( path, error );
    }
    void AssetDatabaseManager::clearDatabase()
    {
        ScopedLock lock( this );
        if( !m_catalogReady )
            return;
        CatalogTransaction transaction( *this );
        auto rows =
            transaction.active() ? queryCatalog( *this, "SELECT 1 FROM resources LIMIT 1" ) : nullptr;
        if( rows && !rows->eof() && queryCatalog( *this, "DELETE FROM resources" ) &&
            transaction.commit() )
            invalidateCatalog();
    }
    void AssetDatabaseManager::clearResourceEntryCache()
    {
        ScopedLock lock( this );
        m_resourceEntriesByUUID.clear();
        m_resourceEntriesByPath.clear();
    }
    bool AssetDatabaseManager::hasResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !object || !object->getHandle() )
            return false;
        if( hasResourceById( object->getHandle()->getUUIDAsString() ) )
            return true;
        if( resourceKind( object ) != EntryKind::File )
            return false;
        return getResourceEntryFromPath( resourcePath( object ) ) != nullptr;
    }
    void AssetDatabaseManager::addResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !object || !object->getHandle() )
            return;
        auto uuid = object->getHandle()->getUUIDAsString();
        if( uuid.empty() )
            uuid = StringUtil::getUUID();
        AssetCatalogPath path;
        const auto kind = kindName( resourceKind( object ) );
        auto types = TypeManager::instance();
        if( !types || !validValue( uuid, 256 ) || !resolveResourcePath( *this, object, path ) )
            return;
        auto type = types->getName( object->getTypeInfo() );
        if( object->isDerived<scene::IGameActor>() )
            type = "Actor";
        else if( object->isDerived<render::IMaterial>() )
            type = "Material";
        else if( object->isDerived<render::ITexture>() )
            type = "Texture";
        else if( object->isDerived<ScriptAsset>() )
            type = "script";
        if( !validValue( type, 256 ) )
            return;
        CatalogTransaction transaction( *this );
        if( !transaction.active() )
            return;
        auto existing = queryCatalog( *this,
                                      "SELECT uuid,path_key,kind,type FROM resources WHERE uuid=? OR "
                                      "(kind='file' AND ?='file' AND path_key=?)",
                                      { uuid, kind, path.key } );
        if( !existing )
            return;
        if( !existing->eof() )
        {
            const bool identical = existing->getFieldValue( "uuid" ) == uuid &&
                                   existing->getFieldValue( "path_key" ) == path.key &&
                                   existing->getFieldValue( "kind" ) == kind &&
                                   existing->getFieldValue( "type" ) == type;
            existing->nextRow();
            if( identical && existing->eof() )
                transaction.commit();
            else
            {
                WP_LOG_ERROR( "Conflicting catalog identity; insertion rejected." );
            }
            return;
        }
        if( queryCatalog( *this, "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,?,?)",
                          { uuid, path.path, type, kind, path.key } ) &&
            transaction.commit() )
        {
            object->getHandle()->setUUID( uuid );
            invalidateCatalog();
        }
    }
    void AssetDatabaseManager::updateResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !object || !object->getHandle() )
            return;
        const auto uuid = object->getHandle()->getUUIDAsString();
        AssetCatalogPath path;
        const auto kind = kindName( resourceKind( object ) );
        if( !validValue( uuid, 256 ) || !resolveResourcePath( *this, object, path ) )
            return;
        CatalogTransaction transaction( *this );
        auto existing =
            transaction.active()
                ? queryCatalog( *this, "SELECT path_key,kind FROM resources WHERE uuid=?", { uuid } )
                : nullptr;
        if( !existing || existing->eof() || existing->getFieldValue( "kind" ) != kind )
            return;
        if( existing->getFieldValue( "path_key" ) == path.key )
            return;
        if( queryCatalog( *this, "UPDATE resources SET path=?,path_key=? WHERE uuid=?",
                          { path.path, path.key, uuid } ) &&
            transaction.commit() )
            invalidateCatalog();
    }
    void AssetDatabaseManager::removeResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !object || !object->getHandle() )
            return;
        const auto uuid = object->getHandle()->getUUIDAsString();
        // UUID is authoritative: never broaden deletion to a shared or stale path.
        if( !validValue( uuid, 256 ) )
            return;
        CatalogTransaction transaction( *this );
        if( transaction.active() && hasResourceById( uuid ) &&
            queryCatalog( *this, "DELETE FROM resources WHERE uuid=?", { uuid } ) &&
            transaction.commit() )
            invalidateCatalog();
    }
    void AssetDatabaseManager::removeResourceEntryFromPath( const String &path )
    {
        ScopedLock lock( this );
        AssetCatalogPath canonical;
        String error;
        if( !m_catalogReady || !canonicalAssetCatalogPath( m_projectRoot, path, canonical, error ) )
            return;
        CatalogTransaction transaction( *this );
        if( transaction.active() && getResourceEntryFromPath( path ) &&
            queryCatalog( *this, "DELETE FROM resources WHERE kind='file' AND path_key=?",
                          { canonical.key } ) &&
            transaction.commit() )
            invalidateCatalog();
    }
    bool AssetDatabaseManager::hasResourceById( const String &uuid )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !validValue( uuid, 256 ) || !refreshDatabaseRevision() )
            return false;
        // Membership and UUID-scoped deletion must remain available even when
        // the source has become inaccessible or no longer resolves safely.
        auto query = queryCatalog( *this, "SELECT 1 FROM resources WHERE uuid=? LIMIT 1", { uuid } );
        return query && !query->eof();
    }
    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntry( const String &uuid )
    {
        ScopedLock lock( this );
        EntrySnapshot entry;
        if( !tryGetEntry( uuid, entry ) )
            return nullptr;
        auto director = make_ptr<scene::ResourceDirector>();
        director->setResourcePath( entry.path );
        director->setResourceUUID( entry.uuid );
        return director;
    }
    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntryFromPath( const String &path )
    {
        ScopedLock lock( this );
        AssetCatalogPath canonical;
        String error;
        if( !m_catalogReady || !canonicalAssetCatalogPath( m_projectRoot, path, canonical, error ) )
            return nullptr;
        // A miss never creates an identity. Detached results cannot mutate the catalog.
        auto rows =
            queryCatalog( *this, "SELECT uuid FROM resources WHERE kind='file' AND path_key=? LIMIT 2",
                          { canonical.key } );
        if( !rows || rows->eof() )
            return nullptr;
        const auto uuid = rows->getFieldValue( "uuid" );
        rows->nextRow();
        return rows->eof() ? getResourceEntry( uuid ) : nullptr;
    }
    bool AssetDatabaseManager::tryGetEntry( const String &uuid, EntrySnapshot &output )
    {
        ScopedLock lock( this );
        output = EntrySnapshot();
        if( !m_catalogReady || !validValue( uuid, 256 ) || !refreshDatabaseRevision() )
            return false;
        auto row = queryCatalog(
            *this, "SELECT uuid,path,type,kind,path_key FROM resources WHERE uuid=? LIMIT 2", { uuid } );
        if( !row || row->eof() )
            return false;
        EntrySnapshot entry;
        entry.uuid = row->getFieldValue( "uuid" );
        entry.path = row->getFieldValue( "path" );
        entry.type = row->getFieldValue( "type" );
        const auto kind = row->getFieldValue( "kind" );
        const auto key = row->getFieldValue( "path_key" );
        if( !validValue( entry.type, 256 ) )
            return false;
        if( kind == "file" )
        {
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( m_projectRoot, entry.path, canonical, error ) ||
                canonical.key != key )
                return false;
        }
        else if( kind == "scene" && entry.path == "scene" && key.empty() )
            entry.kind = EntryKind::Scene;
        else
            return false;
        row->nextRow();
        if( !row->eof() )
            return false;
        entry.generation = m_catalogGeneration;
        entry.catalogInstance = m_catalogInstance;
        output = entry;
        return true;
    }
    bool AssetDatabaseManager::isEntryCurrent( const EntrySnapshot &snapshot )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || !refreshDatabaseRevision() )
            return false;
        if( snapshot.catalogInstance != m_catalogInstance || snapshot.generation != m_catalogGeneration )
            return false;
        EntrySnapshot current;
        return tryGetEntry( snapshot.uuid, current ) && current.generation == snapshot.generation &&
               current.path == snapshot.path &&
               current.type == snapshot.type && current.kind == snapshot.kind;
    }
    namespace
    {
        namespace assetfs = std::filesystem;
        bool operationSchema( AssetDatabaseManager &manager )
        {
            return queryCatalog(manager,
                "CREATE TABLE IF NOT EXISTS wp_asset_operations("
                "id TEXT PRIMARY KEY,kind TEXT NOT NULL,source TEXT NOT NULL,"
                "destination TEXT NOT NULL,root TEXT NOT NULL,state TEXT NOT NULL,"
                "digest TEXT NOT NULL)") &&
                queryCatalog(manager,
                "CREATE TABLE IF NOT EXISTS wp_asset_operation_entries("
                "operation_id TEXT NOT NULL,uuid TEXT NOT NULL,path TEXT NOT NULL,"
                "type TEXT NOT NULL,path_key TEXT NOT NULL,new_uuid TEXT NOT NULL,"
                "new_path TEXT NOT NULL,new_key TEXT NOT NULL,PRIMARY KEY(operation_id,uuid))");
        }
        bool pathWithin( const String &parent, const String &path )
        {
            return path == parent || (path.size()>parent.size() &&
                path.compare(0,parent.size(),parent)==0 && path[parent.size()]=='/');
        }
        bool reparsePath( const assetfs::path &path )
        {
#if defined WP_PLATFORM_WIN32
            const auto attributes = GetFileAttributesW(path.c_str());
            if(attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                return true;
#endif
            return assetfs::is_symlink(assetfs::symlink_status(path));
        }
        String treeDigest( const assetfs::path &path )
        {
            std::map<std::string,assetfs::path> entries;
            auto add = [&](const assetfs::path &entry, const std::string &relative) {
                if(reparsePath(entry) || (!assetfs::is_directory(entry) && !assetfs::is_regular_file(entry)))
                    throw std::runtime_error("Asset operations reject links and special files");
                if(entries.size()>=100000)
                    throw std::runtime_error("Asset operation exceeds the tree entry limit");
                entries.emplace(relative,entry);
            };
            add(path,".");
            if(assetfs::is_directory(path))
                for(const auto &entry:assetfs::recursive_directory_iterator(path))
                    add(entry.path(),entry.path().lexically_relative(path).generic_u8string());
            u64 hash = 14695981039346656037ull;
            auto feed = [&](const char *bytes, size_t size) {
                for(size_t i=0;i<size;++i) { hash ^= static_cast<unsigned char>(bytes[i]); hash *= 1099511628211ull; }
            };
            for(const auto &entry:entries)
            {
                feed(entry.first.data(),entry.first.size());
                const char kind=assetfs::is_directory(entry.second)?'D':'F';
                feed(&kind,1);
                if(kind=='F')
                {
                    std::ifstream input(entry.second,std::ios::binary);
                    if(!input) throw std::runtime_error("Cannot snapshot asset bytes");
                    char bytes[65536];
                    while(input) { input.read(bytes,sizeof(bytes)); feed(bytes,static_cast<size_t>(input.gcount())); }
                    if(!input.eof()) throw std::runtime_error("Cannot finish asset snapshot");
                }
                const char separator=0;
                feed(&separator,1);
            }
            return std::to_string(hash).c_str();
        }
        bool renameExclusive( const assetfs::path &source, const assetfs::path &destination, String &error )
        {
#if defined WP_PLATFORM_WIN32
            if(!MoveFileExW(source.c_str(),destination.c_str(),MOVEFILE_WRITE_THROUGH))
            {
                error=("Asset rename failed (Windows error " + std::to_string(GetLastError()) + ")").c_str();
                return false;
            }
#else
            std::error_code ec;
            if(assetfs::exists(destination,ec) || ec) { error="Asset destination exists or is inaccessible"; return false; }
            assetfs::rename(source,destination,ec);
            if(ec) { error=ec.message().c_str(); return false; }
#endif
            return true;
        }
        assetfs::path operationPath( const String &root, const String &relative )
        {
            AssetCatalogPath canonical;
            String error;
            if(!canonicalAssetCatalogPath(root,relative,canonical,error))
                throw std::runtime_error(error.c_str());
            return assetfs::u8path(root.c_str()) / assetfs::u8path(canonical.path.c_str());
        }
        struct OperationEntry
        {
            String uuid,path,type,key,newUuid,newPath,newKey;
        };
        Array<OperationEntry> operationEntries( AssetDatabaseManager &manager, const String &id )
        {
            auto rows=queryCatalog(manager,"SELECT * FROM wp_asset_operation_entries WHERE operation_id=?",{id});
            if(!rows) throw std::runtime_error("Cannot read asset operation identities");
            Array<OperationEntry> entries;
            while(!rows->eof())
            {
                entries.push_back({rows->getFieldValue("uuid"),rows->getFieldValue("path"),
                    rows->getFieldValue("type"),rows->getFieldValue("path_key"),
                    rows->getFieldValue("new_uuid"),rows->getFieldValue("new_path"),rows->getFieldValue("new_key")});
                rows->nextRow();
            }
            return entries;
        }
        bool applyOperationEntries( AssetDatabaseManager &manager, const Array<OperationEntry> &entries,
                                     const String &kind, bool undo )
        {
            for(const auto &entry:entries)
            {
                const auto &lookup = undo && kind=="copy" ? entry.newUuid : entry.uuid;
                auto current=queryCatalog(manager,"SELECT path,path_key,type FROM resources WHERE uuid=?",{lookup});
                if(!current) return false;
                if(undo && kind=="delete")
                {
                    if(!current->eof()) return false;
                }
                else
                {
                    const auto expected = undo ? entry.newPath : entry.path;
                    const auto expectedKey = undo ? entry.newKey : entry.key;
                    if(current->eof() || current->getFieldValue("path")!=expected ||
                        current->getFieldValue("path_key")!=expectedKey || current->getFieldValue("type")!=entry.type)
                        return false;
                }
                if((kind=="delete" && !undo) || (kind=="copy" && undo))
                {
                    if(!queryCatalog(manager,"DELETE FROM resources WHERE uuid=?",{lookup})) return false;
                }
                else if((kind=="copy" && !undo) || (kind=="delete" && undo))
                {
                    if(!queryCatalog(manager,"INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                        {undo?entry.uuid:entry.newUuid,undo?entry.path:entry.newPath,entry.type,undo?entry.key:entry.newKey}))
                        return false;
                }
                else if(!queryCatalog(manager,"UPDATE resources SET path=?,path_key=? WHERE uuid=?",
                    {undo?entry.path:entry.newPath,undo?entry.key:entry.newKey,lookup})) return false;
            }
            return true;
        }
    }

    AssetDatabaseManager::FileOperationResult AssetDatabaseManager::performFileOperation(
        FileOperation operation, const String &sourceInput, const String &destinationInput )
    {
        ScopedLock lock(this);
        FileOperationResult result;
        try
        {
            if(!m_catalogReady || !refreshDatabaseRevision()) throw std::runtime_error("Catalog is not open");
            for(const auto &recovery : recoverFileOperations())
                if(!recovery.succeeded)
                    throw std::runtime_error("Resolve pending asset operation recovery before changing files");
            AssetCatalogPath source,destination;
            if(!canonicalAssetCatalogPath(m_projectRoot,sourceInput,source,result.error)) return result;
            if(pathWithin(".workphone",source.key)) throw std::runtime_error("Asset operation storage is reserved");
            result.operationId=StringUtil::getUUID();
            const String storage=".workphone/asset-operations/"+result.operationId+"/payload";
            const String kind=operation==FileOperation::Delete?"delete":operation==FileOperation::Copy?"copy":"move";
            if(!canonicalAssetCatalogPath(m_projectRoot,operation==FileOperation::Delete?storage:destinationInput,
                                           destination,result.error)) return result;
            if(operation!=FileOperation::Delete && pathWithin(".workphone",destination.key))
                throw std::runtime_error("Asset operation storage is reserved");
            if(pathWithin(source.key,destination.key)) throw std::runtime_error("Cannot place an asset inside itself");
            const auto from=operationPath(m_projectRoot,source.path);
            const auto to=operationPath(m_projectRoot,destination.path);
            const auto stage=operationPath(m_projectRoot,storage);
            if(!assetfs::exists(from)) throw std::runtime_error("Asset source does not exist");
            if(assetfs::exists(to)) throw std::runtime_error("Asset destination already exists");
            if(operation!=FileOperation::Delete && !assetfs::is_directory(to.parent_path()))
                throw std::runtime_error("Destination folder does not exist");
            const auto digest=treeDigest(from);
            const auto expectedRevision=m_databaseRevision;
            Array<OperationEntry> entries;
            auto rows=queryCatalog(*this,"SELECT uuid,path,type,path_key FROM resources WHERE kind='file'");
            if(!rows) throw std::runtime_error("Cannot plan catalog mutation");
            while(!rows->eof())
            {
                const auto key=rows->getFieldValue("path_key");
                if(pathWithin(source.key,key))
                {
                    const auto path=rows->getFieldValue("path");
                    AssetCatalogPath target;
                    const auto newPath=destination.path+path.substr(source.path.size());
                    if(!canonicalAssetCatalogPath(m_projectRoot,newPath,target,result.error)) return result;
                    entries.push_back({rows->getFieldValue("uuid"),path,rows->getFieldValue("type"),key,
                        operation==FileOperation::Copy?StringUtil::getUUID():rows->getFieldValue("uuid"),target.path,target.key});
                }
                rows->nextRow();
            }
            if(operation==FileOperation::Copy)
            {
                auto checkFormat=[](const assetfs::path &file) {
                    const auto extension=StringUtil::make_lower(file.extension().u8string().c_str());
                    if(extension==".fbscene" || extension==".fbscenebin" || extension==".fbscenexml" ||
                       extension==".prefab" || extension==".fbprefab" || extension==".resource")
                        throw std::runtime_error("Copying scenes/prefabs/resources requires a registered identity remapper");
                };
                checkFormat(from);
                if(assetfs::is_directory(from)) for(const auto &entry:assetfs::recursive_directory_iterator(from))
                    if(assetfs::is_regular_file(entry.path())) checkFormat(entry.path());
            }
            assetfs::create_directories(stage.parent_path());
            // Re-audit after creating the reserved storage prefix.
            operationPath(m_projectRoot,storage);
            {
                CatalogTransaction transaction(*this);
                if(!transaction.active() || !operationSchema(*this) || !queryCatalog(*this,
                    "INSERT INTO wp_asset_operations VALUES(?,?,?,?,?,'prepared',?)",
                    {result.operationId,kind,source.path,destination.path,m_projectRoot,digest}))
                    throw std::runtime_error("Cannot persist asset operation journal");
                for(const auto &entry:entries)
                    if(!queryCatalog(*this,"INSERT INTO wp_asset_operation_entries VALUES(?,?,?,?,?,?,?,?)",
                        {result.operationId,entry.uuid,entry.path,entry.type,entry.key,entry.newUuid,entry.newPath,entry.newKey}))
                        throw std::runtime_error("Cannot persist operation identity snapshot");
                if(!transaction.commit()) throw std::runtime_error("Cannot commit operation preflight");
            }
            if(operation==FileOperation::Copy)
                assetfs::copy(from,stage,assetfs::copy_options::recursive);
            else if(!renameExclusive(from,stage,result.error)) return result;
            if(treeDigest(stage)!=digest) throw std::runtime_error("Asset changed while staging; recovery required");
            if(operation!=FileOperation::Delete && !renameExclusive(stage,to,result.error)) return result;
            {
                CatalogTransaction transaction(*this);
                if(!transaction.active() || !refreshDatabaseRevision() || m_databaseRevision!=expectedRevision ||
                    !applyOperationEntries(*this,entries,kind,false) || !queryCatalog(*this,
                    "UPDATE wp_asset_operations SET state='committed' WHERE id=?",{result.operationId}) ||
                    !transaction.commit())
                    throw std::runtime_error("Asset catalog commit failed; journal recovery required");
            }
            invalidateCatalog();
            result.succeeded=true;
        }
        catch(const std::exception &error) { result.error=error.what(); }
        return result;
    }

    AssetDatabaseManager::FileOperationResult AssetDatabaseManager::undoFileOperation(const String &id)
    {
        ScopedLock lock(this);
        FileOperationResult result;
        result.operationId=id;
        try
        {
            if(!m_catalogReady) throw std::runtime_error("Original project catalog is closed");
            auto row=queryCatalog(*this,"SELECT * FROM wp_asset_operations WHERE id=?",{id});
            if(!row || row->eof() || row->getFieldValue("state")!="committed" || row->getFieldValue("root")!=m_projectRoot)
                throw std::runtime_error("Asset operation is not committed in this project");
            const auto kind=row->getFieldValue("kind");
            const auto from=operationPath(m_projectRoot,row->getFieldValue("destination"));
            const auto to=operationPath(m_projectRoot,kind=="copy"?
                ".workphone/asset-operations/"+id+"/payload":row->getFieldValue("source"));
            if(assetfs::exists(to) || treeDigest(from)!=row->getFieldValue("digest"))
                throw std::runtime_error("Undo conflicts with external files or changed asset bytes");
            const auto entries=operationEntries(*this,id);
            {
                CatalogTransaction transaction(*this);
                // Preflight catalog conflicts without committing the reverse mutation.
                if(!transaction.active() || !applyOperationEntries(*this,entries,kind,true))
                    throw std::runtime_error("Undo conflicts with current asset identities");
            }
            if(!queryCatalog(*this,"UPDATE wp_asset_operations SET state='undoing' WHERE id=?",{id}))
                throw std::runtime_error("Cannot journal undo");
            if(!renameExclusive(from,to,result.error)) return result;
            {
                CatalogTransaction transaction(*this);
                if(!transaction.active() || !applyOperationEntries(*this,entries,kind,true) ||
                    !queryCatalog(*this,"UPDATE wp_asset_operations SET state='undone' WHERE id=?",{id}) ||
                    !transaction.commit()) throw std::runtime_error("Undo catalog commit failed; recovery required");
            }
            invalidateCatalog();
            result.succeeded=true;
        }
        catch(const std::exception &error) { result.error=error.what(); }
        return result;
    }

    Array<AssetDatabaseManager::FileOperationResult> AssetDatabaseManager::recoverFileOperations()
    {
        ScopedLock lock(this);
        Array<FileOperationResult> results;
        if(!m_catalogReady) return results;
        auto table=queryCatalog(*this,"SELECT name FROM sqlite_master WHERE type='table' AND name='wp_asset_operations'");
        if(!table || table->eof()) return results;
        auto rows=queryCatalog(*this,"SELECT * FROM wp_asset_operations WHERE state IN ('prepared','undoing') ORDER BY id");
        if(!rows) return results;
        while(!rows->eof())
        {
            FileOperationResult result;
            result.operationId=rows->getFieldValue("id");
            try
            {
                if(rows->getFieldValue("root")!=m_projectRoot) throw std::runtime_error("Recovery root differs from journal root");
                const auto source=operationPath(m_projectRoot,rows->getFieldValue("source"));
                const auto destination=operationPath(m_projectRoot,rows->getFieldValue("destination"));
                const auto stage=operationPath(m_projectRoot,".workphone/asset-operations/"+result.operationId+"/payload");
                const auto kind=rows->getFieldValue("kind");
                const auto state=rows->getFieldValue("state");
                const auto digest=rows->getFieldValue("digest");
                String next;
                if(state=="undoing")
                {
                    const auto reversed=kind=="copy"?stage:source;
                    if(assetfs::exists(destination))
                    {
                        if(treeDigest(destination)!=digest || assetfs::exists(reversed))
                            throw std::runtime_error("Ambiguous interrupted undo; content retained");
                    }
                    else if(!assetfs::exists(reversed) || treeDigest(reversed)!=digest ||
                            !renameExclusive(reversed,destination,result.error))
                        throw std::runtime_error("Interrupted undo recovery conflicts; content retained");
                    next="committed";
                }
                else if(kind=="copy")
                {
                    // The source survives copy. Retain any staged/published copy in
                    // quarantine; never recursively delete authored data during recovery.
                    if(assetfs::exists(destination))
                    {
                        if(assetfs::exists(stage) || treeDigest(destination)!=digest ||
                           !renameExclusive(destination,stage,result.error))
                            throw std::runtime_error("Copy recovery conflicts; content retained");
                    }
                    next="rolled-back";
                }
                else
                {
                    const bool atSource=assetfs::exists(source);
                    const bool atStage=assetfs::exists(stage);
                    const bool atDestination=kind!="delete" && assetfs::exists(destination);
                    if(static_cast<int>(atSource)+static_cast<int>(atStage)+static_cast<int>(atDestination)!=1)
                        throw std::runtime_error("Ambiguous asset recovery; content retained");
                    const auto location=atSource?source:atStage?stage:destination;
                    if(treeDigest(location)!=digest || (!atSource && !renameExclusive(location,source,result.error)))
                        throw std::runtime_error("Recovery conflicts with external bytes; content retained");
                    next="rolled-back";
                }
                result.succeeded=queryCatalog(*this,"UPDATE wp_asset_operations SET state=? WHERE id=?",{next,result.operationId})!=nullptr;
                if(!result.succeeded) result.error="Could not finish journal recovery";
            }
            catch(const std::exception &error) { result.error=error.what(); }
            results.push_back(result);
            rows->nextRow();
        }
        invalidateCatalog();
        return results;
    }

    String AssetDatabaseManager::getResourcesTableName() const
    {
        return m_resourcesTableName.load();
    }
    void AssetDatabaseManager::sestResourcesTableName( const String &name )
    {
        if( name != "resources" )
        {
            WP_LOG_ERROR( "Only the resources catalog table is supported." );
            return;
        }
        m_resourcesTableName = name;
    }
    String AssetDatabaseManager::getStaticComponentsTableName() const
    {
        return m_staticComponentsTableName.load();
    }
    void AssetDatabaseManager::setStaticComponentsTableName( const String &name )
    {
        m_staticComponentsTableName = name;
    }
    String AssetDatabaseManager::getComponentsTableName() const
    {
        return m_componentsTableName.load();
    }
    void AssetDatabaseManager::setComponentsTableName( const String &name )
    {
        m_componentsTableName = name;
    }
}  // namespace workphone
