#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
#include <Workphone/Database/AssetReferenceGraph.hpp>
#include <Workphone/Database/AssetDocumentSave.hpp>
#include <Workphone/Database/AssetDocumentInventory.hpp>
#include "AssetPackagePolicyInternal.hpp"
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Core/PropertiesBinarySerializer.hpp>
#include <cJSON.h>
#include <tinyxml.h>
#include <memory>
#include <Workphone/Database/AssetReferenceChange.hpp>
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
#else
#    include <fcntl.h>
#    include <unistd.h>
#endif

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AssetDatabaseManager, DatabaseManager );
    namespace
    {
        bool referenceDeleteAllowed( AssetDatabaseManager &, const Array<String> &, const String &,
                                     String & );
        bool referencesPresent( AssetDatabaseManager & );
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
                m_serialized =
                    m_database ? dynamic_cast<ISerializedDatabase *>( m_database.get() ) : nullptr;
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
                !queryCatalog(
                    manager,
                    "INSERT OR IGNORE INTO wp_asset_catalog_changes(id,revision) VALUES(1,0)" ) )
                return false;
            auto revision = queryCatalog( manager,
                                          "SELECT revision FROM wp_asset_catalog_changes WHERE id=1 AND "
                                          "typeof(revision)='integer' AND revision>=0" );
            if( !revision || revision->eof() )
                return false;
            for( const auto &event : { String( "INSERT" ), String( "UPDATE" ), String( "DELETE" ) } )
            {
                const auto name = "wp_catalog_revision_" + event;
                auto owner = queryCatalog(
                    manager, "SELECT tbl_name FROM sqlite_master WHERE type='trigger' AND name=?",
                    { name } );
                if( !owner || ( !owner->eof() && owner->getFieldValue( "tbl_name" ) != "resources" ) ||
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
        auto row = queryCatalog( *this, "SELECT revision FROM wp_asset_catalog_changes WHERE id=1" );
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
        if( m_readOnly )
        {
            String error;
            if( !openReadOnly( getDatabasePath(), error ) )
                WP_LOG_ERROR( error );
            return;
        }
        loadFromFile( getDatabasePath() );
        if( m_catalogReady )
            for( auto database : m_attached )
                attach( database );
    }
    void AssetDatabaseManager::close()
    {
        ScopedLock lock( this );
        m_catalogReady = false;
        // A reopened connection is a new authoring session, even at the same
        // path. Detached edits may survive ordinary row revisions, not close.
        m_catalogInstance = nextCatalogInstance.fetch_add( 1 );
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
        if( m_readOnly )
            close();
        m_readOnly = false;
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
    bool AssetDatabaseManager::openReadOnly( const String &path, String &error )
    {
        ScopedLock lock( this );
        close();
        m_readOnly = true;
        setDatabase( nullptr );
        setDatabasePath( path );
        error.clear();
        auto app = core::IApplicationManager::instancePtr();
        auto factory = app ? app->getFactoryManager() : nullptr;
        auto database = factory ? factory->make_object<IDatabase>() : nullptr;
        auto reader = database ? dynamic_cast<IReadOnlyDatabase *>( database.get() ) : nullptr;
        if( !reader || !captureProjectRoot() || !reader->openReadOnly( path, error ) )
        {
            if( error.empty() )
                error = "Read-only catalog backend or source root is unavailable.";
            return false;
        }
        setDatabase( database );
        setDatabasePath( path );
        auto version = queryCatalog( *this, "SELECT id,version FROM wp_asset_catalog_schema" );
        bool valid = version && !version->eof() && version->getFieldValue( "id" ) == "1" &&
                     version->getFieldValue( "version" ) == "2";
        if( valid )
        {
            version->nextRow();
            valid = version->eof();
        }
        if( !valid || !validateIdentity( *this, m_projectRoot ) || !refreshDatabaseRevision() )
        {
            error = "Read-only catalog schema, identity or revision validation failed.";
            close();
            return false;
        }
        m_catalogReady = true;
        invalidateCatalog();
        return true;
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
        auto backup = database ? dynamic_cast<IBackupDatabase *>( database.get() ) : nullptr;
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
        Array<String> targets;
        auto identities =
            transaction.active() ? queryCatalog( *this, "SELECT uuid FROM resources" ) : nullptr;
        while( identities && !identities->eof() )
        {
            targets.push_back( identities->getFieldValue( "uuid" ) );
            identities->nextRow();
        }
        String error;
        if( identities && referenceDeleteAllowed( *this, targets, {}, error ) && rows && !rows->eof() &&
            queryCatalog( *this, "DELETE FROM resources" ) && transaction.commit() )
            invalidateCatalog();
        if( !error.empty() )
            WP_LOG_ERROR( "Catalog clear rejected: " + error );
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
        addResourceEntries( { object } );
    }

    void AssetDatabaseManager::addResourceEntries( const Array<SmartPtr<ISharedObject>> &objects )
    {
        ScopedLock lock( this );
        if( !m_catalogReady || objects.empty() )
            return;
        CatalogTransaction transaction( *this );
        if( !transaction.active() )
            return;
        Array<Pair<SmartPtr<ISharedObject>, String>> inserted;
        for( const auto &object : objects )
        {
            if( !object || !object->getHandle() )
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
                if( !identical || !existing->eof() )
                {
                    WP_LOG_ERROR( "Conflicting catalog identity; insertion rejected." );
                    return;
                }
                continue;
            }
            if( !queryCatalog( *this, "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,?,?)",
                               { uuid, path.path, type, kind, path.key } ) )
                return;
            inserted.push_back( { object, uuid } );
        }
        if( transaction.commit() && !inserted.empty() )
        {
            for( auto &entry : inserted )
            {
                entry.first->getHandle()->setUUID( entry.second );
            }
            // Observe our committed revision now, so the next lookup does not
            // publish the same import a second time as an external change.
            refreshDatabaseRevision();
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
        String error;
        if( transaction.active() && referenceDeleteAllowed( *this, { uuid }, {}, error ) &&
            hasResourceById( uuid ) &&
            queryCatalog( *this, "DELETE FROM resources WHERE uuid=?", { uuid } ) &&
            transaction.commit() )
            invalidateCatalog();
        if( !error.empty() )
            WP_LOG_ERROR( "Catalog removal rejected: " + error );
    }
    void AssetDatabaseManager::removeResourceEntryFromPath( const String &path )
    {
        ScopedLock lock( this );
        AssetCatalogPath canonical;
        String error;
        if( !m_catalogReady || !canonicalAssetCatalogPath( m_projectRoot, path, canonical, error ) )
            return;
        CatalogTransaction transaction( *this );
        String referenceError;
        auto identity =
            transaction.active()
                ? queryCatalog( *this, "SELECT uuid FROM resources WHERE kind='file' AND path_key=?",
                                { canonical.key } )
                : nullptr;
        if( identity && !identity->eof() &&
            referenceDeleteAllowed( *this, { identity->getFieldValue( "uuid" ) }, {}, referenceError ) &&
            getResourceEntryFromPath( path ) &&
            queryCatalog( *this, "DELETE FROM resources WHERE kind='file' AND path_key=?",
                          { canonical.key } ) &&
            transaction.commit() )
            invalidateCatalog();
        if( !referenceError.empty() )
            WP_LOG_ERROR( "Catalog removal rejected: " + referenceError );
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
    AssetDatabaseManager::EntryPage AssetDatabaseManager::queryEntries( const EntryQuery &query,
                                                                        const EntryCursor *cursor )
    {
        ScopedLock lock( this );
        EntryPage page;
        auto fail = [&page]( const String &error ) {
            page = EntryPage();
            page.error = error;
            return page;
        };
        if( query.pageSize == 0 || query.pageSize > 256 || query.search.size() > 1024 ||
            query.type.size() > 256 || query.search.find( '\0' ) != String::npos ||
            query.type.find( '\0' ) != String::npos )
            return fail(
                "Invalid catalog query: page size must be 1-256 and filters must be bounded text." );
        if( !m_catalogReady )
            return fail( "No open asset catalog." );
        auto database = getDatabase();
        DatabaseConnectionLock connection(
            database ? dynamic_cast<ISerializedDatabase *>( database.get() ) : nullptr );
        if( !refreshDatabaseRevision() )
            return fail( "Cannot read the catalog revision." );
        const auto generation = m_catalogGeneration;
        if( cursor &&
            ( cursor->catalogInstance != m_catalogInstance || cursor->generation != generation ||
              cursor->search != query.search || cursor->type != query.type ||
              cursor->includeScenes != query.includeScenes || !validValue( cursor->afterUuid, 256 ) ) )
            return fail( "Catalog or filters changed; restart browsing from the first page." );
        // The bundled SQLite predates instr(). GLOB is case-sensitive; escape
        // its metacharacters so user text remains a literal substring.
        String pattern = "*";
        for( const auto ch : query.search )
        {
            if( ch == '*' )
                pattern += "[*]";
            else if( ch == '?' )
                pattern += "[?]";
            else if( ch == '[' )
                pattern += "[[]";
            else
                pattern += ch;
        }
        pattern += '*';
        auto rows = queryCatalog(
            *this,
            "SELECT uuid,path,type,kind,path_key FROM resources "
            "WHERE (?='' OR path GLOB ? OR uuid GLOB ?) "
            "AND (?='' OR type=?) AND (?='1' OR kind='file') "
            "AND uuid COLLATE BINARY > ? ORDER BY uuid COLLATE BINARY LIMIT ?",
            { query.search, pattern, pattern, query.type, query.type, query.includeScenes ? "1" : "0",
              cursor ? cursor->afterUuid : String(), StringUtil::toString( query.pageSize + 1 ) } );
        if( !rows )
            return fail( "Asset catalog query failed." );
        while( !rows->eof() )
        {
            if( page.entries.size() == query.pageSize )
            {
                page.hasMore = true;
                break;
            }
            EntrySnapshot entry;
            entry.uuid = rows->getFieldValue( "uuid" );
            entry.path = rows->getFieldValue( "path" );
            entry.type = rows->getFieldValue( "type" );
            const auto kind = rows->getFieldValue( "kind" );
            const auto key = rows->getFieldValue( "path_key" );
            if( !validValue( entry.uuid, 256 ) || !validValue( entry.type, 256 ) )
                return fail( "Invalid catalog identity; repair the catalog before browsing." );
            if( kind == "file" )
            {
                AssetCatalogPath canonical;
                String error;
                if( !canonicalAssetCatalogPath( m_projectRoot, entry.path, canonical, error ) ||
                    canonical.path != entry.path || canonical.key != key )
                    return fail( "Invalid catalog path; repair the catalog before browsing." );
            }
            else if( kind == "scene" && entry.path == "scene" && key.empty() )
                entry.kind = EntryKind::Scene;
            else
                return fail( "Invalid catalog entry kind." );
            entry.generation = generation;
            entry.catalogInstance = m_catalogInstance;
            page.entries.push_back( entry );
            rows->nextRow();
        }
        // Also catches writes from another connection during this materialized query.
        if( !refreshDatabaseRevision() || generation != m_catalogGeneration )
            return fail( "Catalog changed during browsing; refresh the first page." );
        page.next.search = query.search;
        page.next.type = query.type;
        page.next.includeScenes = query.includeScenes;
        page.next.generation = generation;
        page.next.catalogInstance = m_catalogInstance;
        if( !page.entries.empty() )
            page.next.afterUuid = page.entries.back().uuid;
        page.succeeded = true;
        return page;
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
               current.path == snapshot.path && current.type == snapshot.type &&
               current.kind == snapshot.kind;
    }
    namespace
    {
        bool referenceSchema( AssetDatabaseManager &manager )
        {
            if( !referencesPresent( manager ) )
            {
                if( !queryCatalog( manager,
                                   "CREATE TABLE wp_asset_reference_schema(id INTEGER PRIMARY KEY "
                                   "CHECK(id=1),version INTEGER NOT NULL)" ) ||
                    !queryCatalog( manager, "INSERT INTO wp_asset_reference_schema VALUES(1,1)" ) )
                    return false;
            }
            for( const auto *sql :
                 { "CREATE TABLE IF NOT EXISTS wp_asset_reference_documents(path_key TEXT PRIMARY KEY "
                   "CHECK(length(CAST(path_key AS BLOB)) BETWEEN 1 AND 1024),path TEXT NOT NULL "
                   "CHECK(length(CAST(path AS BLOB)) BETWEEN 1 AND 1024),revision TEXT NOT NULL "
                   "CHECK(length(revision) BETWEEN 1 AND 128),digest TEXT NOT NULL CHECK(length(digest) "
                   "BETWEEN 1 AND 128),state TEXT NOT NULL CHECK(state IN "
                   "('pending','ready','quarantined')))",
                   "CREATE TABLE IF NOT EXISTS wp_asset_references(id INTEGER PRIMARY KEY "
                   "AUTOINCREMENT,source_key TEXT NOT NULL CHECK(length(CAST(source_key AS BLOB)) "
                   "BETWEEN 1 AND 1024),slot TEXT NOT NULL CHECK(length(CAST(slot AS BLOB)) BETWEEN 1 "
                   "AND 1024),target_uuid TEXT NOT NULL CHECK(length(CAST(target_uuid AS BLOB)) BETWEEN "
                   "1 AND 256),target_catalog TEXT NOT NULL CHECK(target_catalog IN "
                   "('project','media')),expected_type TEXT NOT NULL CHECK(length(CAST(expected_type AS "
                   "BLOB)) BETWEEN 1 AND 256),required INTEGER NOT NULL CHECK(required IN "
                   "(0,1)),UNIQUE(source_key,slot))",
                   "CREATE INDEX IF NOT EXISTS wp_asset_reference_reverse ON "
                   "wp_asset_references(target_catalog,target_uuid,id)",
                   "CREATE INDEX IF NOT EXISTS wp_asset_reference_forward ON "
                   "wp_asset_references(source_key,id)" } )
                if( !queryCatalog( manager, sql ) )
                    return false;
            for( const String table :
                 { String( "wp_asset_reference_documents" ), String( "wp_asset_references" ) } )
                for( const String event :
                     { String( "INSERT" ), String( "UPDATE" ), String( "DELETE" ) } )
                    if( !queryCatalog( manager, "CREATE TRIGGER IF NOT EXISTS " + table + "_" + event +
                                                    " AFTER " + event + " ON " + table +
                                                    " BEGIN UPDATE wp_asset_catalog_changes SET "
                                                    "revision=revision+1 WHERE id=1; END" ) )
                        return false;
            return true;
        }
        bool referencesPresent( AssetDatabaseManager &manager )
        {
            auto rows = queryCatalog(
                manager,
                "SELECT name FROM sqlite_master WHERE type='table' AND name IN "
                "('wp_asset_reference_schema','wp_asset_reference_documents','wp_asset_references')" );
            if( !rows )
                throw std::runtime_error( "Cannot inspect reference graph schema" );
            size_t count = 0;
            while( !rows->eof() )
            {
                ++count;
                rows->nextRow();
            }
            if( !count )
                return false;
            if( count != 3 )
                throw std::runtime_error(
                    "Reference graph schema is incomplete; explicit repair required" );
            auto version = queryCatalog( manager,
                                         "SELECT version FROM wp_asset_reference_schema WHERE id=1 AND "
                                         "typeof(version)='integer' AND version=1" );
            if( !version || version->eof() )
                throw std::runtime_error( "Unsupported reference graph schema version" );
            return true;
        }
        String referenceDigest( const u8 *data, size_t size )
        {
            u64 hash = 14695981039346656037ull;
            for( size_t i = 0; i < size; ++i )
            {
                hash ^= data[i];
                hash *= 1099511628211ull;
            }
            return ( std::to_string( size ) + ":" + std::to_string( hash ) ).c_str();
        }
        String referenceFileDigest( AssetDatabaseManager &manager, const String &path, size_t &budget )
        {
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( manager.getProjectRoot(), path, canonical, error ) ||
                canonical.path != path )
                throw std::runtime_error( "Reference source path no longer resolves: " +
                                          std::string( path.c_str() ) );
            const auto file = std::filesystem::u8path( manager.getProjectRoot().c_str() ) /
                              std::filesystem::u8path( path.c_str() );
            const auto size = std::filesystem::file_size( file );
            if( size > 64 * 1024 * 1024 || size > budget )
                throw std::runtime_error( "Reference source validation exceeds byte budget" );
            budget -= static_cast<size_t>( size );
            std::ifstream input( file, std::ios::binary );
            if( !input )
                throw std::runtime_error( "Cannot read reference source document" );
            std::vector<u8> bytes( static_cast<size_t>( size ) );
            if( size )
                input.read( reinterpret_cast<char *>( bytes.data() ),
                            static_cast<std::streamsize>( size ) );
            if( !input || input.peek() != std::char_traits<char>::eof() )
                throw std::runtime_error( "Reference source changed while reading" );
            return referenceDigest( bytes.data(), bytes.size() );
        }
        bool referencePathWithin( const String &prefix, const String &key )
        {
            return !prefix.empty() &&
                   ( prefix == key ||
                     ( key.size() > prefix.size() && key.compare( 0, prefix.size(), prefix ) == 0 &&
                       key[prefix.size()] == '/' ) );
        }
        bool referenceDeleteAllowed( AssetDatabaseManager &manager, const Array<String> &uuids,
                                     const String &removedSourceKey, String &error )
        {
            try
            {
                if( !referencesPresent( manager ) )
                    return true;
                // Validate all surviving indexed documents, including pending saves:
                // a stale document may have added a reference absent from the old graph.
                auto docs = queryCatalog(
                    manager,
                    "SELECT substr(path_key,1,1025) AS path_key,substr(path,1,1025) AS "
                    "path,substr(state,1,32) AS state,substr(digest,1,129) AS digest FROM "
                    "wp_asset_reference_documents WHERE state<>'quarantined' LIMIT 10001" );
                if( !docs )
                    throw std::runtime_error( "Cannot validate reference graph" );
                size_t count = 0, budget = 256 * 1024 * 1024;
                while( !docs->eof() )
                {
                    const auto key = docs->getFieldValue( "path_key" );
                    if( ++count > 10000 || !validValue( key, 1024 ) ||
                        !validValue( docs->getFieldValue( "path" ), 1024 ) ||
                        !validValue( docs->getFieldValue( "digest" ), 128 ) )
                        throw std::runtime_error( "Malformed or over-budget reference graph documents" );
                    if( !referencePathWithin( removedSourceKey, key ) )
                    {
                        if( docs->getFieldValue( "state" ) != "ready" ||
                            referenceFileDigest( manager, docs->getFieldValue( "path" ), budget ) !=
                                docs->getFieldValue( "digest" ) )
                            throw std::runtime_error(
                                "Reference graph is pending or stale. Save/reindex the document before "
                                "deletion: " +
                                std::string( docs->getFieldValue( "path" ).c_str() ) );
                    }
                    docs->nextRow();
                }
                for( const auto &uuid : uuids )
                {
                    auto rows = queryCatalog(
                        manager,
                        "SELECT substr(d.path,1,1025) AS path,substr(r.slot,1,1025) AS slot FROM "
                        "wp_asset_references r JOIN wp_asset_reference_documents d "
                        "ON d.path_key=r.source_key WHERE d.state='ready' AND "
                        "r.target_catalog='project' AND r.target_uuid=? AND r.required=1 "
                        "AND (?='' OR (d.path_key<>? AND substr(d.path_key,1,length(?4))<>?4)) LIMIT 1",
                        { uuid, removedSourceKey, removedSourceKey, removedSourceKey + "/" } );
                    if( !rows )
                        throw std::runtime_error( "Cannot query required asset users" );
                    if( !rows->eof() )
                        throw std::runtime_error( "Asset is required by " +
                                                  std::string( rows->getFieldValue( "path" ).c_str() ) +
                                                  " at " +
                                                  std::string( rows->getFieldValue( "slot" ).c_str() ) +
                                                  " (" + std::string( uuid.c_str() ) + ")" );
                }
                return true;
            }
            catch( const std::exception &e )
            {
                error = e.what();
                return false;
            }
        }
        bool applyReferenceDocuments( AssetDatabaseManager &manager, const String &kind,
                                      const String &source, const String &destination, bool undo )
        {
            if( !referencesPresent( manager ) )
                return true;
            AssetCatalogPath from, to;
            String error;
            if( !canonicalAssetCatalogPath( manager.getProjectRoot(), source, from, error ) ||
                !canonicalAssetCatalogPath( manager.getProjectRoot(), destination, to, error ) )
                return false;
            if( kind == "copy" )
            {
                // Copies of indexed authored documents require an identity remapper.
                auto row = queryCatalog( manager,
                                         "SELECT path_key FROM wp_asset_reference_documents WHERE "
                                         "path_key=?1 OR substr(path_key,1,length(?2))=?2 LIMIT 1",
                                         { from.key, from.key + "/" } );
                return row && row->eof();
            }
            auto pending = queryCatalog(
                manager,
                "SELECT path_key FROM wp_asset_reference_documents WHERE (path_key=?1 OR "
                "substr(path_key,1,length(?2))=?2) AND state NOT IN ('ready','quarantined') LIMIT 1",
                { from.key, from.key + "/" } );
            if( !pending || !pending->eof() )
                return false;
            if( kind == "delete" )
                return queryCatalog( manager,
                                     "UPDATE wp_asset_reference_documents SET state=?1 WHERE "
                                     "(path_key=?2 OR substr(path_key,1,length(?3))=?3) AND state=?4",
                                     { undo ? "ready" : "quarantined", from.key, from.key + "/",
                                       undo ? "quarantined" : "ready" } ) != nullptr;
            if( undo )
                std::swap( from, to );
            auto rows = queryCatalog( manager,
                                      "SELECT substr(path_key,1,1025) AS path_key,substr(path,1,1025) "
                                      "AS path FROM wp_asset_reference_documents WHERE path_key=?1 OR "
                                      "substr(path_key,1,length(?2))=?2 LIMIT 10001",
                                      { from.key, from.key + "/" } );
            if( !rows )
                return false;
            Array<std::pair<String, String>> paths;
            while( !rows->eof() )
            {
                if( paths.size() >= 10000 || !validValue( rows->getFieldValue( "path_key" ), 1024 ) ||
                    !validValue( rows->getFieldValue( "path" ), 1024 ) )
                    return false;
                paths.push_back( { rows->getFieldValue( "path_key" ), rows->getFieldValue( "path" ) } );
                rows->nextRow();
            }
            for( const auto &p : paths )
            {
                const auto newKey = to.key + p.first.substr( from.key.size() );
                const auto newPath = to.path + p.second.substr( from.path.size() );
                if( !queryCatalog(
                        manager,
                        "UPDATE wp_asset_reference_documents SET path_key=?,path=? WHERE path_key=?",
                        { newKey, newPath, p.first } ) ||
                    !queryCatalog( manager,
                                   "UPDATE wp_asset_references SET source_key=? WHERE source_key=?",
                                   { newKey, p.first } ) )
                    return false;
            }
            return true;
        }
    }  // namespace
    namespace
    {
        bool beginReferenceSave( SmartPtr<AssetDatabaseManager> catalog, const String &sourcePath,
                                 SmartPtr<Properties> serialized, const Array<u8> &bytes,
                                 AssetReferenceSave &save, String &error,
                                 SmartPtr<AssetDatabaseManager> media, bool ownTransaction,
                                 bool captureOnly = false )
        {
            save = {};
            error.clear();
            try
            {
                if( !catalog || ( catalog->isReadOnly() && !captureOnly ) || !serialized ||
                    bytes.empty() || bytes.size() > 64 * 1024 * 1024 )
                    throw std::runtime_error(
                        "Reference save requires a writable catalog, serialized data and bounded "
                        "bytes" );
                ScopedLock lock( catalog.get() );
                AssetCatalogPath source;
                if( !canonicalAssetCatalogPath( catalog->getProjectRoot(), sourcePath, source, error ) ||
                    referencePathWithin( ".workphone", source.key ) )
                    throw std::runtime_error( "Invalid reference document source path" );
                AssetCatalogSession mediaSession;
                if( media && ( !media->isReadOnly() || !mediaSession.capture( media ) ) )
                    throw std::runtime_error(
                        "Bundled media reference catalog is unavailable or writable" );
                AssetReferenceSave candidate;
                if( !candidate.session.capture( catalog ) )
                    throw std::runtime_error( "Catalog session is unavailable" );
                candidate.sourcePath = source.path;
                candidate.sourceKey = source.key;
                candidate.revision = StringUtil::getUUID();
                candidate.digest = referenceDigest( bytes.data(), bytes.size() );
                struct Node
                {
                    SmartPtr<Properties> value;
                    String slot;
                    size_t depth;
                };
                Array<Node> nodes{ { serialized, "root", 0 } };
                std::set<const Properties *> visited;
                size_t properties = 0;
                for( size_t i = 0; i < nodes.size(); ++i )
                {
                    auto &node = nodes[i];
                    if( !node.value || node.depth > 64 || !visited.insert( node.value.get() ).second )
                        throw std::runtime_error( "Cyclic/shared or over-depth reference document" );
                    auto fields = node.value->getPropertiesAsArray();
                    if( ( properties += fields.size() ) > 100000 )
                        throw std::runtime_error( "Reference document exceeds property limit" );
                    auto children = node.value->getChildren();
                    if( nodes.size() + children.size() > 10000 )
                        throw std::runtime_error( "Reference document exceeds node limit" );
                    // Copy these before push_back can reallocate the vector.
                    const auto slot = node.slot;
                    const auto depth = node.depth;
                    for( size_t j = 0; j < children.size(); ++j )
                        nodes.push_back(
                            { children[j], slot + "/" + StringUtil::toString( j ), depth + 1 } );
                }
                for( const auto &node : nodes )
                    for( const auto &property : node.value->getPropertiesAsArray() )
                    {
                        auto type = property.getAttribute( "resourceType" );
                        // Script's durable UUID predates resource metadata.
                        if( type.empty() && property.getName() == "scriptAssetUuid" )
                            type = "script";
                        if( property.getValue().empty() )
                            continue;
                        if( ( type.empty() || type == "None" ) &&
                            property.getTypeName() == Properties::resourceStr )
                            type = "workphone::IResource";
                        if( type.empty() || type == "None" )
                            continue;
                        const auto family = AssetReferenceChange::referenceType( type );
                        auto types = TypeManager::instance();
                        if( types && family &&
                            ( types->isDerived( family, scene::IGameActor::typeInfo() ) ||
                              types->isDerived( family, scene::IComponent::typeInfo() ) ) )
                            continue;
                        const String slot = node.slot + ":" + property.getName();
                        if( !validValue( property.getValue(), 256 ) || !validValue( type, 256 ) ||
                            !validValue( slot, 1024 ) )
                            throw std::runtime_error( "Invalid typed reference fields" );
                        String domain = "project";
                        AssetDatabaseManager::EntrySnapshot projectEntry, mediaEntry;
                        const bool inProject = catalog->tryGetEntry( property.getValue(), projectEntry );
                        const bool inMedia =
                            media && media->tryGetEntry( property.getValue(), mediaEntry );
                        if( inProject && inMedia )
                            throw std::runtime_error( "Reference UUID collides with bundled media" );
                        if( inMedia )
                            domain = "media";
                        const auto requirement = property.getAttribute( "referenceRequired" );
                        if( !requirement.empty() && requirement != "true" && requirement != "false" )
                            throw std::runtime_error( "referenceRequired must be true or false" );
                        candidate.edges.push_back( { source.path, slot, property.getValue(), domain,
                                                     type, requirement != "false" } );
                        if( candidate.edges.size() > 10000 )
                            throw std::runtime_error( "Reference document exceeds edge limit" );
                    }
                if( captureOnly )
                {
                    if( media && !mediaSession.isCurrent( media ) )
                        throw std::runtime_error( "Bundled media reference session changed" );
                    if( referencesPresent( *catalog ) )
                    {
                        auto row = queryCatalog(
                            *catalog, "SELECT state FROM wp_asset_reference_documents WHERE path_key=?",
                            { source.key } );
                        if( !row || ( !row->eof() && row->getFieldValue( "state" ) == "quarantined" ) )
                            throw std::runtime_error(
                                "Restore quarantined reference identity before reindex" );
                    }
                    save = std::move( candidate );
                    return true;
                }
                std::unique_ptr<CatalogTransaction> transaction;
                if( ownTransaction )
                    transaction = std::make_unique<CatalogTransaction>( *catalog );
                if( media && !mediaSession.isCurrent( media ) )
                    throw std::runtime_error( "Bundled media reference session changed" );
                if( ( transaction && !transaction->active() ) || !referenceSchema( *catalog ) )
                    throw std::runtime_error( "Cannot initialize reference graph transaction" );
                auto existing = queryCatalog(
                    *catalog, "SELECT state FROM wp_asset_reference_documents WHERE path_key=?",
                    { source.key } );
                if( !existing ||
                    ( !existing->eof() && existing->getFieldValue( "state" ) == "quarantined" ) )
                    throw std::runtime_error(
                        "Restore the quarantined document before reusing its reference identity/path" );
                if( !queryCatalog( *catalog, "DELETE FROM wp_asset_references WHERE source_key=?",
                                   { source.key } ) ||
                    !queryCatalog( *catalog, "DELETE FROM wp_asset_reference_documents WHERE path_key=?",
                                   { source.key } ) ||
                    !queryCatalog( *catalog,
                                   "INSERT INTO wp_asset_reference_documents VALUES(?,?,?,?,'pending')",
                                   { source.key, source.path, candidate.revision, candidate.digest } ) )
                    throw std::runtime_error( "Cannot persist pending reference save" );
                for( const auto &edge : candidate.edges )
                    if( !queryCatalog( *catalog,
                                       "INSERT INTO "
                                       "wp_asset_references(source_key,slot,target_uuid,target_catalog,"
                                       "expected_type,required) VALUES(?,?,?,?,?,?)",
                                       { source.key, edge.slot, edge.targetUuid, edge.targetCatalog,
                                         edge.expectedType, edge.required ? "1" : "0" } ) )
                        throw std::runtime_error( "Cannot persist typed reference edge" );
                if( transaction && !transaction->commit() )
                    throw std::runtime_error( "Cannot commit pending reference save" );
                save = std::move( candidate );
                return true;
            }
            catch( const std::exception &e )
            {
                error = e.what();
                return false;
            }
        }
        bool finishReferenceSave( SmartPtr<AssetDatabaseManager> catalog, const AssetReferenceSave &save,
                                  String &error )
        {
            if( !save.session.isCurrent( catalog ) )
                throw std::runtime_error( "Reference save project session expired" );
            if( !referencesPresent( *catalog ) )
                throw std::runtime_error( "Reference graph is unavailable" );
            auto row = queryCatalog( *catalog,
                                     "SELECT path,digest FROM wp_asset_reference_documents WHERE "
                                     "path_key=? AND revision=? AND state='pending'",
                                     { save.sourceKey, save.revision } );
            if( !row || row->eof() || row->getFieldValue( "path" ) != save.sourcePath )
                throw std::runtime_error( "Reference save was superseded or is no longer pending" );
            size_t budget = 64 * 1024 * 1024;
            if( referenceFileDigest( *catalog, save.sourcePath, budget ) !=
                row->getFieldValue( "digest" ) )
                throw std::runtime_error(
                    "Saved bytes differ from the captured reference document; graph remains pending" );
            if( !queryCatalog( *catalog,
                               "UPDATE wp_asset_reference_documents SET state='ready' WHERE path_key=? "
                               "AND revision=?",
                               { save.sourceKey, save.revision } ) )
                throw std::runtime_error( "Cannot complete durable reference save" );
            return true;
        }
    }  // namespace
    bool beginAssetReferenceSave( SmartPtr<AssetDatabaseManager> catalog, const String &path,
                                  SmartPtr<Properties> serialized, const Array<u8> &bytes,
                                  AssetReferenceSave &save, String &error,
                                  SmartPtr<AssetDatabaseManager> media )
    {
        return beginReferenceSave( catalog, path, serialized, bytes, save, error, media, true );
    }
    bool completeAssetReferenceSave( SmartPtr<AssetDatabaseManager> catalog,
                                     const AssetReferenceSave &save, String &error )
    {
        error.clear();
        try
        {
            if( !catalog || catalog->isReadOnly() )
                throw std::runtime_error( "Reference save catalog is unavailable/read-only" );
            ScopedLock lock( catalog.get() );
            CatalogTransaction transaction( *catalog );
            if( !transaction.active() || !finishReferenceSave( catalog, save, error ) ||
                !transaction.commit() )
                throw std::runtime_error( "Cannot complete durable reference save" );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }

    namespace
    {
        class StagedAssetDocument
        {
        public:
            explicit StagedAssetDocument( const std::filesystem::path &destination ) :
                target( destination )
            {
                stage =
                    target.parent_path() /
                    std::filesystem::u8path( ( target.filename().u8string() + ".workphone-save-" +
                                               std::string( StringUtil::getUUID().c_str() ) + ".tmp" ) );
            }
            ~StagedAssetDocument()
            {
                if( owned )
                {
                    std::error_code ignored;
                    std::filesystem::remove( stage, ignored );
                }
            }
            void write( const Array<u8> &bytes )
            {
#if defined WP_PLATFORM_WIN32
                auto handle = CreateFileW( stage.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                           FILE_ATTRIBUTE_NORMAL, nullptr );
                if( handle == INVALID_HANDLE_VALUE )
                    throw std::runtime_error( "Cannot exclusively stage asset document" );
                owned = true;
                DWORD written = 0;
                const bool ok = WriteFile( handle, bytes.data(), static_cast<DWORD>( bytes.size() ),
                                           &written, nullptr ) &&
                                written == bytes.size() && FlushFileBuffers( handle );
                const bool closed = CloseHandle( handle ) != 0;
                if( !ok || !closed )
                    throw std::runtime_error( "Cannot flush staged asset document" );
#else
                const auto fd = ::open( stage.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600 );
                if( fd < 0 )
                    throw std::runtime_error( "Cannot exclusively stage asset document" );
                owned = true;
                size_t offset = 0;
                while( offset < bytes.size() )
                {
                    const auto count = ::write( fd, bytes.data() + offset, bytes.size() - offset );
                    if( count <= 0 )
                    {
                        ::close( fd );
                        throw std::runtime_error( "Cannot write staged asset document" );
                    }
                    offset += static_cast<size_t>( count );
                }
                const bool flushed = ::fsync( fd ) == 0;
                const bool closed = ::close( fd ) == 0;
                if( !flushed || !closed )
                    throw std::runtime_error( "Cannot flush staged asset document" );
#endif
            }
            void publish()
            {
#if defined WP_PLATFORM_WIN32
                if( !MoveFileExW( stage.c_str(), target.c_str(),
                                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) )
                    throw std::runtime_error( "Cannot replace asset document (Windows error " +
                                              std::to_string( GetLastError() ) + ")" );
#else
                std::filesystem::rename( stage, target );
#endif
                owned = false;
            }

        private:
            std::filesystem::path target, stage;
            bool owned = false;
        };
    }  // namespace
    bool saveAssetDocument( const String &path, SmartPtr<Properties> serialized, const Array<u8> &bytes,
                            String &installedPath, String &error )
    {
        installedPath.clear();
        error.clear();
        try
        {
            namespace fs = std::filesystem;
            if( !validValue( path, 4096 ) || !serialized || bytes.empty() ||
                bytes.size() > 64 * 1024 * 1024 )
                throw std::runtime_error(
                    "Asset save requires a path, Properties and 1..64 MiB of serialized bytes" );
            auto app = core::IApplicationManager::instancePtr();
            if( !app )
                throw std::runtime_error( "Asset save application is unavailable" );
            const auto projectPath = app->getProjectPath();
            auto resources = app->getResourceDatabase();
            auto catalog =
                resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                          : nullptr;
            auto concrete = dynamic_pointer_cast<ResourceDatabase>( resources );
            auto media = concrete ? concrete->getMediaCatalog() : nullptr;
            String root;
            if( !canonicalAssetCatalogRoot(
                    app->getProjectPath().empty()
                        ? ( catalog ? catalog->getProjectRoot()
                                    : String( fs::current_path().u8string().c_str() ) )
                        : app->getProjectPath(),
                    root, error ) )
                throw std::runtime_error( error.c_str() );
            auto destination = fs::u8path( path.c_str() );
            const bool relative = destination.is_relative();
            if( relative )
                destination = fs::u8path( root.c_str() ) / destination;
            destination = fs::weakly_canonical( destination );
            const String absolute = destination.u8string().c_str();
            AssetCatalogPath source, cache, mediaPath;
            const bool underRoot = canonicalAssetCatalogPath( root, absolute, source, error );
            if( relative && !underRoot )
                throw std::runtime_error(
                    "Relative asset saves must remain inside the captured project root" );
            if( underRoot && referencePathWithin( ".workphone", source.key ) )
                throw std::runtime_error( "Asset saves cannot overwrite private catalog storage" );
            if( media &&
                canonicalAssetCatalogPath( media->getProjectRoot(), absolute, mediaPath, error ) )
                throw std::runtime_error( "Bundled media is read-only; save a project copy instead" );
            bool indexed = false;
            if( catalog )
            {
                if( root != catalog->getProjectRoot() )
                    throw std::runtime_error( "Asset save project catalog has not been rebound" );
                if( underRoot )
                {
                    if( catalog->isReadOnly() )
                        throw std::runtime_error( "Project asset catalog is read-only" );
                    const auto cachePath = app->getCachePath();
                    const bool inCache = !cachePath.empty() &&
                                         canonicalAssetCatalogPath( root, cachePath, cache, error ) &&
                                         referencePathWithin( cache.key, source.key );
                    indexed = !inCache;
                }
            }
            error.clear();
            if( fs::exists( destination ) && !fs::is_regular_file( destination ) )
                throw std::runtime_error( "Asset save destination is not a regular file" );
            fs::create_directories( destination.parent_path() );
            // Stage and flush before changing the durable reference revision.
            StagedAssetDocument staged( destination );
            staged.write( bytes );
            auto bindingCurrent = [&]() {
                return app->getProjectPath() == projectPath && app->getResourceDatabase() == resources &&
                       ( !catalog || ( resources->getDatabaseManager() == catalog &&
                                       catalog->getProjectRoot() == root ) ) &&
                       ( !concrete || concrete->getMediaCatalog() == media );
            };
            if( !bindingCurrent() )
                throw std::runtime_error( "Asset save project/media binding changed during staging" );
            if( indexed )
            {
                ScopedLock lock( catalog.get() );
                AssetReferenceSave save;
                if( !beginReferenceSave( catalog, absolute, serialized, bytes, save, error, media,
                                         true ) )
                    throw std::runtime_error( error.c_str() );
                // Fence other catalog writers across publication. A failure after
                // replacement leaves the durable pending revision for repair.
                CatalogTransaction transaction( *catalog );
                auto pending = transaction.active()
                                   ? queryCatalog( *catalog,
                                                   "SELECT revision FROM wp_asset_reference_documents "
                                                   "WHERE path_key=? AND revision=? AND state='pending'",
                                                   { save.sourceKey, save.revision } )
                                   : nullptr;
                if( !pending || pending->eof() || !save.session.isCurrent( catalog ) ||
                    !bindingCurrent() )
                    throw std::runtime_error( "Asset save was superseded before publication" );
                staged.publish();
                if( !finishReferenceSave( catalog, save, error ) || !transaction.commit() )
                    throw std::runtime_error(
                        "Document installed; reference graph remains pending. Reindex or save again" );
            }
            else
                staged.publish();
            installedPath = absolute;
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    bool saveAssetDocumentText( const String &path, SmartPtr<Properties> serialized, const String &text,
                                String &error )
    {
        Array<u8> bytes;
        bytes.resize( text.size() );
        std::copy( text.begin(), text.end(), bytes.begin() );
        String installed;
        return saveAssetDocument( path, serialized, bytes, installed, error );
    }

    namespace
    {
        // Bound recursion and allocation before invoking the existing JSON parser.
        // Reject encoded NULs rather than letting cJSON truncate identity strings.
        void preflightReferenceJson( const Array<u8> &bytes )
        {
            size_t depth = 0, tokens = 0;
            bool quoted = false, escaped = false;
            for( size_t i = 0; i < bytes.size(); ++i )
            {
                const auto c = bytes[i];
                if( !c )
                    throw std::runtime_error( "Reference JSON contains NUL bytes" );
                if( quoted )
                {
                    if( escaped )
                    {
                        if( c == 'u' && i + 4 < bytes.size() && bytes[i + 1] == '0' &&
                            bytes[i + 2] == '0' && bytes[i + 3] == '0' && bytes[i + 4] == '0' )
                            throw std::runtime_error( "Reference JSON contains an encoded NUL" );
                        escaped = false;
                    }
                    else if( c == '\\' )
                        escaped = true;
                    else if( c == '"' )
                        quoted = false;
                }
                else if( c == '"' )
                {
                    quoted = true;
                    if( ++tokens > 300000 )
                        throw std::runtime_error( "Reference JSON exceeds token budget" );
                }
                else if( c == ',' )
                {
                    if( ++tokens > 300000 )
                        throw std::runtime_error( "Reference JSON exceeds token budget" );
                }
                else if( c == '{' || c == '[' )
                {
                    if( ++depth > 128 || ++tokens > 300000 )
                        throw std::runtime_error( "Reference JSON exceeds structural budget" );
                }
                else if( c == '}' || c == ']' )
                {
                    if( !depth )
                        throw std::runtime_error( "Unbalanced reference JSON" );
                    --depth;
                }
            }
            if( quoted || depth )
                throw std::runtime_error( "Incomplete reference JSON" );
        }
        const cJSON *jsonMember( const cJSON *node, const char *name )
        {
            return cJSON_GetObjectItemCaseSensitive( node, name );
        }
        String jsonString( const cJSON *node )
        {
            if( !cJSON_IsString( node ) || !node->valuestring )
                throw std::runtime_error( "Reference JSON requires string property values" );
            return node->valuestring;
        }
        SmartPtr<Properties> referenceProperties( const cJSON *node, size_t depth, size_t &nodes,
                                                  size_t &fields )
        {
            if( !cJSON_IsObject( node ) || depth > 64 || ++nodes > 10000 )
                throw std::runtime_error( "Reference JSON exceeds Properties tree budget" );
            auto metadata = jsonMember( node, "__wp_properties_v1" );
            if( !cJSON_IsArray( metadata ) )
                throw std::runtime_error(
                    "Legacy JSON lacks typed property metadata. Open and save it before reindexing" );
            auto result = make_ptr<Properties>();
            result->setName( jsonString( jsonMember( node, "handle_name" ) ) );
            std::set<String> names;
            for( auto entry = metadata->child; entry; entry = entry->next )
            {
                if( !cJSON_IsObject( entry ) || ++fields > 100000 )
                    throw std::runtime_error( "Invalid or excessive reference property metadata" );
                const auto name = jsonString( jsonMember( entry, "name" ) );
                if( name.empty() || name == "__wp_properties_v1" || name == "handle_name" ||
                    name == "children" || !names.insert( name ).second )
                    throw std::runtime_error( "Duplicate/reserved reference property metadata" );
                const auto value = jsonString( jsonMember( node, name.c_str() ) );
                const auto type = jsonString( jsonMember( entry, "type" ) );
                const auto readOnly = jsonMember( entry, "readOnly" );
                const auto attributes = jsonMember( entry, "attributes" );
                if( !cJSON_IsBool( readOnly ) || !cJSON_IsObject( attributes ) )
                    throw std::runtime_error( "Malformed property policy metadata" );
                result->setProperty( name, value, type );
                auto &property = result->getPropertyObject( name );
                property.setReadOnly( cJSON_IsTrue( readOnly ) );
                for( auto attribute = attributes->child; attribute; attribute = attribute->next )
                    property.setAttribute( attribute->string, jsonString( attribute ) );
            }
            // Every value must have metadata; guessing an unknown property's
            // resource semantics could authorize deleting a required asset.
            for( auto entry = node->child; entry; entry = entry->next )
            {
                const String name = entry->string;
                if( name != "handle_name" && name != "children" && name != "__wp_properties_v1" &&
                    !names.count( name ) )
                    throw std::runtime_error( "Reference JSON has values without property metadata" );
            }
            const auto children = jsonMember( node, "children" );
            if( !cJSON_IsArray( children ) )
                throw std::runtime_error( "Reference JSON requires a children array" );
            for( auto child = children->child; child; child = child->next )
                result->addChild( referenceProperties( child, depth + 1, nodes, fields ) );
            return result;
        }

        void preflightReferenceXml( const std::string &text )
        {
            if( text.find( '\0' ) != std::string::npos )
                throw std::runtime_error( "Reference XML contains NUL bytes" );
            for( size_t at = text.find( "&#" ); at != std::string::npos; at = text.find( "&#", at + 2 ) )
            {
                auto end = text.find( ';', at + 2 );
                if( end == std::string::npos || end - at > 12 )
                    throw std::runtime_error( "Malformed XML character reference" );
                const auto number = text.substr( at + 2, end - at - 2 );
                size_t consumed = 0;
                const bool hex = !number.empty() && number[0] == 'x';
                const auto value =
                    std::stoul( hex ? number.substr( 1 ) : number, &consumed, hex ? 16 : 10 );
                if( !value || value > 0x10ffff || ( value >= 0xd800 && value <= 0xdfff ) ||
                    consumed != number.size() - ( hex ? 1 : 0 ) )
                    throw std::runtime_error( "Invalid XML character reference" );
            }
            size_t depth = 0, tags = 0;
            for( size_t i = 0; i < text.size(); ++i )
            {
                if( text[i] == '\0' )
                    throw std::runtime_error( "Reference XML contains NUL bytes" );
                if( text[i] != '<' )
                    continue;
                if( text.compare( i, 4, "<!--" ) == 0 || text.compare( i, 9, "<![CDATA[" ) == 0 )
                {
                    const bool comment = text.compare( i, 4, "<!--" ) == 0;
                    const auto end = text.find( comment ? "-->" : "]]>", i + ( comment ? 4 : 9 ) );
                    if( end == std::string::npos )
                        throw std::runtime_error( "Incomplete reference XML" );
                    i = end + 2;
                    continue;
                }
                if( text.compare( i, 2, "<!" ) == 0 )
                    throw std::runtime_error( "Reference XML declarations/entities are unsupported" );
                const bool closing = text.compare( i, 2, "</" ) == 0;
                const bool declaration = text.compare( i, 2, "<?" ) == 0;
                char quote = 0;
                size_t end = i + 1;
                for( ; end < text.size(); ++end )
                {
                    const auto c = text[end];
                    if( quote )
                    {
                        if( c == quote )
                            quote = 0;
                    }
                    else if( c == '\'' || c == '"' )
                    {
                        if( ++tags > 300000 )
                            throw std::runtime_error( "Reference XML exceeds attribute budget" );
                        quote = c;
                    }
                    else if( c == '>' )
                        break;
                }
                if( end == text.size() )
                    throw std::runtime_error( "Incomplete reference XML tag" );
                if( ++tags > 300000 )
                    throw std::runtime_error( "Reference XML exceeds element budget" );
                if( !declaration )
                {
                    if( closing )
                    {
                        if( !depth )
                            throw std::runtime_error( "Unbalanced reference XML" );
                        --depth;
                    }
                    else if( text[end - 1] != '/' )
                    {
                        if( ++depth > 128 )
                            throw std::runtime_error( "Reference XML exceeds depth budget" );
                    }
                }
                i = end;
            }
            if( depth )
                throw std::runtime_error( "Unbalanced reference XML" );
        }
        SmartPtr<Properties> referenceXmlProperties( const TiXmlElement *node, size_t depth,
                                                     size_t &nodes, size_t &fields )
        {
            if( !node || String( node->Value() ) != "Properties" || depth > 64 || ++nodes > 10000 )
                throw std::runtime_error( "Invalid or over-budget XML Properties tree" );
            const auto version = node->Attribute( "PropertyMetadataVersion" );
            const auto name = node->Attribute( "Name" );
            if( !version || String( version ) != "1" || !name )
                throw std::runtime_error(
                    "Legacy XML lacks typed property metadata. Open and save it before reindexing" );
            auto result = make_ptr<Properties>();
            result->setName( name );
            std::set<String> names;
            bool childrenSeen = false;
            for( auto child = node->FirstChildElement(); child; child = child->NextSiblingElement() )
            {
                const String kind = child->Value();
                if( kind == "Children" )
                {
                    if( childrenSeen )
                        throw std::runtime_error( "Duplicate XML Children group" );
                    childrenSeen = true;
                    for( auto entry = child->FirstChildElement(); entry;
                         entry = entry->NextSiblingElement() )
                        result->addChild( referenceXmlProperties( entry, depth + 1, nodes, fields ) );
                }
                else if( kind == "Property" )
                {
                    const auto key = child->Attribute( "Name" ), type = child->Attribute( "Type" ),
                               readOnly = child->Attribute( "readOnly" );
                    if( ++fields > 100000 || !key || !*key || !type || !readOnly ||
                        ( String( readOnly ) != "true" && String( readOnly ) != "false" ) ||
                        !names.insert( key ).second )
                        throw std::runtime_error( "Malformed/duplicate XML property metadata" );
                    result->setProperty( key, child->GetText() ? child->GetText() : "", type );
                    auto &property = result->getPropertyObject( key );
                    property.setReadOnly( String( readOnly ) == "true" );
                    std::set<String> attributes;
                    for( auto entry = child->FirstChildElement(); entry;
                         entry = entry->NextSiblingElement() )
                    {
                        const auto attribute = entry->Attribute( "Name" ),
                                   value = entry->Attribute( "Value" );
                        if( String( entry->Value() ) != "Attribute" || !attribute || !value ||
                            !attributes.insert( attribute ).second )
                            throw std::runtime_error( "Malformed/duplicate XML property attribute" );
                        property.setAttribute( attribute, value );
                    }
                }
                else
                    throw std::runtime_error( "Unsupported XML Properties element" );
            }
            return result;
        }
        SmartPtr<Properties> parseReferenceDocument( const Array<u8> &bytes )
        {
            if( PropertiesBinarySerializer::hasBinaryHeader( bytes.data(), bytes.size() ) )
            {
                auto result = make_ptr<Properties>();
                String error;
                if( !PropertiesBinarySerializer::deserialize( bytes, *result, &error ) )
                    throw std::runtime_error( error.c_str() );
                return result;
            }
            const std::string textBytes( bytes.begin(), bytes.end() );
            const auto first = textBytes.find_first_not_of( " \t\r\n" );
            if( first != std::string::npos && textBytes[first] == '<' )
            {
                preflightReferenceXml( textBytes );
                TiXmlDocument document;
                document.Parse( textBytes.c_str() );
                const auto root = document.RootElement();
                if( document.Error() || !root || String( root->Value() ) != "Root" ||
                    root->NextSiblingElement() )
                    throw std::runtime_error( "Malformed XML reference document" );
                const auto properties = root->FirstChildElement();
                if( !properties || properties->NextSiblingElement() )
                    throw std::runtime_error( "Malformed XML reference root" );
                size_t nodes = 0, fields = 0;
                return referenceXmlProperties( properties, 0, nodes, fields );
            }
            preflightReferenceJson( bytes );
            std::vector<char> text( bytes.begin(), bytes.end() );
            text.push_back( 0 );
            std::unique_ptr<cJSON, decltype( &cJSON_Delete )> json(
                cJSON_ParseWithLengthOpts( text.data(), text.size(), nullptr, 1 ), &cJSON_Delete );
            if( !json )
                throw std::runtime_error(
                    "Malformed or unsupported reference document; reindex supports typed JSON and "
                    "portable binary" );
            // cJSON permits duplicate keys. Reject them before any identity lookup.
            std::vector<const cJSON *> pending{ json.get() };
            size_t visited = 0;
            for( size_t i = 0; i < pending.size(); ++i )
            {
                auto node = pending[i];
                std::set<std::string> keys;
                for( auto child = node->child; child; child = child->next )
                {
                    if( ++visited > 300000 )
                        throw std::runtime_error( "Reference JSON exceeds value budget" );
                    if( cJSON_IsObject( node ) &&
                        ( !child->string || !keys.insert( child->string ).second ) )
                        throw std::runtime_error( "Reference JSON contains duplicate object keys" );
                    if( child->child )
                        pending.push_back( child );
                }
            }
            size_t nodes = 0, fields = 0;
            return referenceProperties( json.get(), 0, nodes, fields );
        }
    }  // namespace
    bool reindexAssetDocument( SmartPtr<AssetDatabaseManager> catalog, const String &path, String &error,
                               SmartPtr<AssetDatabaseManager> media )
    {
        error.clear();
        try
        {
            if( !catalog || catalog->isReadOnly() )
                throw std::runtime_error( "Reindex requires an active writable project catalog" );
            ScopedLock lock( catalog.get() );
            auto app = core::IApplicationManager::instancePtr();
            auto resources =
                app ? dynamic_pointer_cast<ResourceDatabase>( app->getResourceDatabase() ) : nullptr;
            if( resources && resources->getDatabaseManager() == catalog )
            {
                String activeRoot;
                if( !canonicalAssetCatalogRoot( app->getProjectPath(), activeRoot, error ) ||
                    activeRoot != catalog->getProjectRoot() )
                    throw std::runtime_error( "Reindex project catalog has not been rebound" );
                if( !media )
                    media = resources->getMediaCatalog();
                AssetCatalogPath cache;
                const auto cachePath = app->getCachePath();
                AssetCatalogPath candidate;
                if( canonicalAssetCatalogPath( catalog->getProjectRoot(), path, candidate, error ) &&
                    !cachePath.empty() &&
                    canonicalAssetCatalogPath( catalog->getProjectRoot(), cachePath, cache, error ) &&
                    referencePathWithin( cache.key, candidate.key ) )
                    throw std::runtime_error( "Cache scratch files cannot own project references" );
            }
            AssetCatalogSession session;
            if( !session.capture( catalog ) )
                throw std::runtime_error( "Reindex project session is unavailable" );
            AssetCatalogPath source;
            if( !canonicalAssetCatalogPath( catalog->getProjectRoot(), path, source, error ) ||
                referencePathWithin( ".workphone", source.key ) )
                throw std::runtime_error(
                    "Reindex source must be a project document outside private storage" );
            const auto file = std::filesystem::u8path( catalog->getProjectRoot().c_str() ) /
                              std::filesystem::u8path( source.path.c_str() );
            if( !std::filesystem::is_regular_file( file ) )
                throw std::runtime_error( "Reindex source is not a regular file" );
            const auto size = std::filesystem::file_size( file );
            if( !size || size > 64 * 1024 * 1024 )
                throw std::runtime_error( "Reindex document exceeds byte budget" );
            Array<u8> bytes;
            bytes.resize( static_cast<size_t>( size ) );
            std::ifstream input( file, std::ios::binary );
            input.read( reinterpret_cast<char *>( bytes.data() ),
                        static_cast<std::streamsize>( bytes.size() ) );
            if( !input || input.peek() != std::char_traits<char>::eof() )
                throw std::runtime_error( "Reindex source changed while reading" );
            auto serialized = parseReferenceDocument( bytes );
            CatalogTransaction transaction( *catalog );
            AssetReferenceSave save;
            if( !transaction.active() || !session.isCurrent( catalog ) ||
                !beginReferenceSave( catalog, source.path, serialized, bytes, save, error, media,
                                     false ) )
                throw std::runtime_error( error.empty() ? "Cannot begin document reindex"
                                                        : error.c_str() );
            if( !finishReferenceSave( catalog, save, error ) || !transaction.commit() )
                throw std::runtime_error( "Cannot commit document reindex" );
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    struct AssetDocumentInventorySnapshot
    {
        AssetCatalogSession session, mediaSession;
        ObjectObserver<AssetDatabaseManager> media;
        ObjectObserver<ResourceDatabase> activeResources;
        AssetDocumentScanOptions options;
        Array<AssetDocumentInventoryEntry> documents;
        String root, cacheKey, mediaKey;
        u64 generation = 0, mediaGeneration = 0;
        bool active = false, hasMedia = false;
    };
    namespace
    {
        bool documentLink( const std::filesystem::path &path )
        {
#if defined WP_PLATFORM_WIN32
            const auto attributes = GetFileAttributesW( path.c_str() );
            if( attributes == INVALID_FILE_ATTRIBUTES )
                throw std::runtime_error( "Cannot inspect document scan entry" );
            if( attributes & FILE_ATTRIBUTE_REPARSE_POINT )
                return true;
#endif
            return std::filesystem::is_symlink( std::filesystem::symlink_status( path ) );
        }
        bool documentExtension( const std::filesystem::path &path )
        {
            const auto extension = StringUtil::make_lower( path.extension().u8string().c_str() );
            return extension == ".fbscene" || extension == ".fbscenebin" || extension == ".fbscenexml" ||
                   extension == ".prefab" || extension == ".fbprefab" || extension == ".resource" ||
                   extension == ".mat" || extension == ".material";
        }
        class DocumentScanBudgetExceeded : public std::runtime_error
        {
        public:
            DocumentScanBudgetExceeded() :
                std::runtime_error( "Project document scan exceeds the byte budget" )
            {
            }
        };
        Array<u8> readInventoryDocument( const std::filesystem::path &file, u64 &budget )
        {
            const auto size = std::filesystem::file_size( file );
            if( !size || size > 64ull * 1024 * 1024 )
                throw std::runtime_error( "Document is empty or exceeds the 64 MiB document limit" );
            if( size > budget )
                throw DocumentScanBudgetExceeded();
            budget -= size;
            Array<u8> bytes;
            bytes.resize( static_cast<size_t>( size ) );
            std::ifstream input( file, std::ios::binary );
            input.read( reinterpret_cast<char *>( bytes.data() ),
                        static_cast<std::streamsize>( bytes.size() ) );
            if( !input || input.peek() != std::char_traits<char>::eof() )
                throw std::runtime_error( "Document changed or could not be read during scan" );
            return bytes;
        }
        bool sameInventory( const AssetDocumentInventorySnapshot &a,
                            const AssetDocumentInventorySnapshot &b )
        {
            if( a.root != b.root || a.cacheKey != b.cacheKey || a.mediaKey != b.mediaKey ||
                a.active != b.active || a.activeResources.lock() != b.activeResources.lock() ||
                a.hasMedia != b.hasMedia || a.media.lock() != b.media.lock() ||
                a.mediaGeneration != b.mediaGeneration || a.documents.size() != b.documents.size() )
                return false;
            for( size_t i = 0; i < a.documents.size(); ++i )
            {
                const auto &left = a.documents[i];
                const auto &right = b.documents[i];
                if( left.path != right.path || left.digest != right.digest ||
                    left.canReindex != right.canReindex || left.diagnostic != right.diagnostic )
                    return false;
            }
            return true;
        }
    }  // namespace
    AssetDocumentInventory scanAssetDocuments( SmartPtr<AssetDatabaseManager> catalog,
                                               const AssetDocumentScanOptions &options,
                                               SmartPtr<AssetDatabaseManager> media )
    {
        AssetDocumentInventory report;
        try
        {
            namespace fs = std::filesystem;
            if( !catalog || !options.maxDocuments || options.maxDocuments > 10000 ||
                !options.maxEntries || options.maxEntries > 100000 || !options.maxDepth ||
                options.maxDepth > 64 || !options.maxBytes || options.maxBytes > 256ull * 1024 * 1024 )
                throw std::runtime_error(
                    "Scan requires an available project catalog and bounded limits" );
            ScopedLock lock( catalog.get() );
            auto snapshot = std::make_shared<AssetDocumentInventorySnapshot>();
            snapshot->options = options;
            snapshot->root = catalog->getProjectRoot();
            if( !snapshot->session.capture( catalog ) )
                throw std::runtime_error( "Document scan catalog session is unavailable" );
            snapshot->generation = catalog->getCatalogGeneration();
            auto app = core::IApplicationManager::instancePtr();
            auto resources =
                app ? dynamic_pointer_cast<ResourceDatabase>( app->getResourceDatabase() ) : nullptr;
            String error;
            AssetCatalogPath domain;
            if( resources && resources->getDatabaseManager() == catalog )
            {
                String activeRoot;
                if( !canonicalAssetCatalogRoot( app->getProjectPath(), activeRoot, error ) ||
                    activeRoot != snapshot->root )
                    throw std::runtime_error( "Document scan project catalog has not been rebound" );
                snapshot->active = true;
                snapshot->activeResources = resources;
                if( media && media != resources->getMediaCatalog() )
                    throw std::runtime_error(
                        "Document scan media mount does not match the active project" );
                media = resources->getMediaCatalog();
                if( !app->getCachePath().empty() &&
                    canonicalAssetCatalogPath( snapshot->root, app->getCachePath(), domain, error ) )
                    snapshot->cacheKey = domain.key;
                // A cache rooted at the project itself cannot silently exclude every document.
                else if( !app->getCachePath().empty() )
                {
                    String cacheRoot;
                    const auto configured = fs::u8path( app->getCachePath().c_str() );
                    const auto absolute = configured.is_absolute()
                                              ? configured
                                              : fs::u8path( snapshot->root.c_str() ) / configured;
                    if( canonicalAssetCatalogRoot( absolute.u8string().c_str(), cacheRoot, error ) &&
                        cacheRoot == snapshot->root )
                        throw std::runtime_error( "Project root cannot also be the cache root" );
                }
            }
            if( media )
            {
                if( !media->isReadOnly() || !snapshot->mediaSession.capture( media ) )
                    throw std::runtime_error(
                        "Document scan requires an available read-only media mount" );
                snapshot->hasMedia = true;
                snapshot->media = media;
                snapshot->mediaGeneration = media->getCatalogGeneration();
                if( media->getProjectRoot() == snapshot->root )
                    throw std::runtime_error( "Project and bundled media roots must be separate" );
                if( canonicalAssetCatalogPath( snapshot->root, media->getProjectRoot(), domain, error ) )
                    snapshot->mediaKey = domain.key;
            }
            auto excluded = [&]( const String &key ) {
                return referencePathWithin( ".workphone", key ) || referencePathWithin( ".git", key ) ||
                       referencePathWithin( ".svn", key ) ||
                       referencePathWithin( snapshot->cacheKey, key ) ||
                       referencePathWithin( snapshot->mediaKey, key );
            };
            struct Candidate
            {
                String path, diagnostic;
            };
            std::map<std::string, Candidate> candidates;
            auto add = [&]( const String &key, const String &path, const String &diagnostic ) {
                auto found = candidates.find( key.c_str() );
                if( found == candidates.end() )
                {
                    if( candidates.size() >= options.maxDocuments )
                        throw std::runtime_error( "Project document scan exceeds the document budget" );
                    candidates.emplace( key.c_str(), Candidate{ path, diagnostic } );
                }
                else if( !diagnostic.empty() )
                    found->second.diagnostic = diagnostic;
            };
            if( referencesPresent( *catalog ) )
            {
                auto rows = queryCatalog(
                    *catalog,
                    "SELECT substr(path_key,1,1025) AS path_key,substr(path,1,1025) AS path FROM "
                    "wp_asset_reference_documents WHERE state<>'quarantined' "
                    "ORDER BY path_key LIMIT 10001" );
                if( !rows )
                    throw std::runtime_error( "Cannot enumerate indexed reference documents" );
                while( !rows->eof() )
                {
                    const auto path = rows->getFieldValue( "path" );
                    const auto key = rows->getFieldValue( "path_key" );
                    AssetCatalogPath canonical;
                    if( !validValue( path, 1024 ) || !validValue( key, 1024 ) ||
                        !canonicalAssetCatalogPath( snapshot->root, path, canonical, error ) ||
                        canonical.path != path || canonical.key != key )
                        throw std::runtime_error( "Indexed reference document has an invalid path" );
                    add( key, path, excluded( key ) ? "Indexed document is in an excluded domain" : "" );
                    rows->nextRow();
                }
            }
            const auto root = fs::u8path( snapshot->root.c_str() );
            fs::recursive_directory_iterator it( root ), end;
            for( ; it != end; ++it )
            {
                if( ++report.visited > options.maxEntries )
                    throw std::runtime_error( "Project document scan exceeds the entry budget" );
                const auto file = it->path();
                const String relative = file.lexically_relative( root ).generic_u8string().c_str();
                // Inspect exclusion lexically before resolving links in excluded storage.
                auto lexicalKey = relative;
#ifdef _WIN32
                for( auto &character : lexicalKey )
                    if( character >= 'A' && character <= 'Z' )
                        character += 'a' - 'A';
#endif
                if( excluded( lexicalKey ) )
                {
                    ++report.excluded;
                    it.disable_recursion_pending();
                    continue;
                }
                if( static_cast<u32>( it.depth() ) >= options.maxDepth )
                    throw std::runtime_error(
                        "Project document scan exceeds the directory depth budget" );
                if( documentLink( file ) )
                {
                    it.disable_recursion_pending();
                    add( lexicalKey, relative, "Links and reparse points require explicit import" );
                    continue;
                }
                if( fs::is_directory( file ) )
                    continue;
                if( !fs::is_regular_file( file ) )
                {
                    add( lexicalKey, relative, "Special files cannot own saved references" );
                    continue;
                }
                if( !documentExtension( file ) &&
                    candidates.find( lexicalKey.c_str() ) == candidates.end() )
                    continue;
                AssetCatalogPath canonical;
                if( !canonicalAssetCatalogPath( snapshot->root, relative, canonical, error ) )
                    throw std::runtime_error( "Cannot canonicalize discovered document path" );
                add( canonical.key, canonical.path, "" );
            }
            u64 budget = options.maxBytes;
            for( const auto &item : candidates )
            {
                AssetDocumentInventoryEntry entry;
                entry.path = item.second.path;
                entry.diagnostic = item.second.diagnostic;
                if( entry.diagnostic.empty() )
                {
                    const auto file = root / fs::u8path( entry.path.c_str() );
                    try
                    {
                        if( documentLink( file ) || !fs::is_regular_file( file ) )
                            throw std::runtime_error(
                                "Indexed document is missing or is not a regular file" );
                        auto bytes = readInventoryDocument( file, budget );
                        entry.digest = referenceDigest( bytes.data(), bytes.size() );
                        auto serialized = parseReferenceDocument( bytes );
                        AssetReferenceSave save;
                        if( !beginReferenceSave( catalog, entry.path, serialized, bytes, save, error,
                                                 media, false, true ) )
                            throw std::runtime_error( error.c_str() );
                        entry.canReindex = true;
                    }
                    catch( const DocumentScanBudgetExceeded & )
                    {
                        throw;
                    }
                    catch( const std::exception &e )
                    {
                        entry.diagnostic = e.what();
                    }
                }
                if( entry.canReindex )
                    ++report.ready;
                else
                    ++report.blocked;
                report.documents.push_back( std::move( entry ) );
            }
            report.bytesRead = options.maxBytes - budget;
            if( !snapshot->session.isCurrent( catalog ) ||
                snapshot->generation != catalog->getCatalogGeneration() ||
                ( media && ( !snapshot->mediaSession.isCurrent( media ) ||
                             snapshot->mediaGeneration != media->getCatalogGeneration() ) ) )
                throw std::runtime_error( "Catalog changed during document scan; scan again" );
            snapshot->documents = report.documents;
            report.m_snapshot = snapshot;
            report.succeeded = true;
        }
        catch( const std::exception &e )
        {
            // Never publish a truncated inventory as a usable preview.
            report = {};
            report.error = e.what();
        }
        return report;
    }
    bool reindexAssetDocuments( SmartPtr<AssetDatabaseManager> catalog,
                                const AssetDocumentInventory &inventory, u32 &indexed, String &error )
    {
        indexed = 0;
        error.clear();
        try
        {
            const auto snapshot = inventory.m_snapshot;
            if( !catalog || catalog->isReadOnly() || !snapshot ||
                !snapshot->session.isCurrent( catalog ) )
                throw std::runtime_error( "Document inventory session expired; scan the project again" );
            ScopedLock lock( catalog.get() );
            auto media = snapshot->media.lock();
            if( snapshot->hasMedia && ( !media || !snapshot->mediaSession.isCurrent( media ) ) )
                throw std::runtime_error( "Document inventory media session expired; scan again" );
            CatalogTransaction transaction( *catalog );
            if( !transaction.active() || snapshot->generation != catalog->getCatalogGeneration() )
                throw std::runtime_error( "Catalog changed since document preview; scan again" );
            auto current = scanAssetDocuments( catalog, snapshot->options, media );
            if( !current.succeeded || !sameInventory( *snapshot, *current.m_snapshot ) )
                throw std::runtime_error(
                    "Project documents or mounts changed since preview; scan again" );
            u64 budget = snapshot->options.maxBytes;
            u32 count = 0;
            for( const auto &entry : snapshot->documents )
            {
                if( !entry.canReindex )
                    continue;
                auto bytes = readInventoryDocument( std::filesystem::u8path( snapshot->root.c_str() ) /
                                                        std::filesystem::u8path( entry.path.c_str() ),
                                                    budget );
                if( referenceDigest( bytes.data(), bytes.size() ) != entry.digest )
                    throw std::runtime_error( "Document changed during batch reindex" );
                auto serialized = parseReferenceDocument( bytes );
                AssetReferenceSave save;
                if( !beginReferenceSave( catalog, entry.path, serialized, bytes, save, error, media,
                                         false ) ||
                    !finishReferenceSave( catalog, save, error ) )
                    throw std::runtime_error( error.empty() ? "Cannot reindex document batch"
                                                            : error.c_str() );
                ++count;
            }
            // Check membership and every candidate again before publishing the batch.
            // External filesystem writers cannot be locked by SQLite; this bounds the
            // detectable race without claiming a cross-store atomic filesystem snapshot.
            auto finalScan = scanAssetDocuments( catalog, snapshot->options, media );
            if( !finalScan.succeeded || !sameInventory( *snapshot, *finalScan.m_snapshot ) ||
                !snapshot->session.isCurrent( catalog ) || !transaction.commit() )
                throw std::runtime_error(
                    "Project changed or batch commit failed; no graphs published" );
            indexed = count;
            return true;
        }
        catch( const std::exception &e )
        {
            error = e.what();
            return false;
        }
    }
    AssetReferencePage queryAssetReferences( SmartPtr<AssetDatabaseManager> catalog, const String &value,
                                             bool reverse, const String &targetCatalog, u32 limit,
                                             const AssetReferenceCursor *cursor )
    {
        AssetReferencePage page;
        try
        {
            if( !catalog || !limit || limit > 256 || !validValue( value, reverse ? 256 : 1024 ) ||
                ( targetCatalog != "project" && targetCatalog != "media" ) )
                throw std::runtime_error( "Invalid reference query" );
            ScopedLock lock( catalog.get() );
            AssetDatabaseManager::EntryQuery query;
            query.pageSize = 1;
            const auto identity = catalog->queryEntries( query );
            if( !identity.succeeded )
                throw std::runtime_error( "Reference catalog is unavailable" );
            page.next = {
                0, identity.next.generation, identity.next.catalogInstance, value, targetCatalog, reverse
            };
            if( cursor &&
                ( cursor->generation != page.next.generation ||
                  cursor->catalogInstance != page.next.catalogInstance || cursor->value != value ||
                  cursor->targetCatalog != targetCatalog || cursor->reverse != reverse ) )
                throw std::runtime_error( "Reference query continuation expired; refresh" );
            DatabaseConnectionLock connection(
                dynamic_cast<ISerializedDatabase *>( catalog->getDatabase().get() ) );
            if( referencesPresent( *catalog ) )
            {
                String lookup = value;
                if( !reverse )
                {
                    AssetCatalogPath canonical;
                    String error;
                    if( !canonicalAssetCatalogPath( catalog->getProjectRoot(), value, canonical,
                                                    error ) )
                        throw std::runtime_error( error.c_str() );
                    lookup = canonical.key;
                }
                auto rows = queryCatalog(
                    *catalog,
                    "SELECT r.id,substr(d.path,1,1025) AS path,substr(r.slot,1,1025) AS slot,"
                    "substr(r.target_uuid,1,257) AS target_uuid,substr(r.target_catalog,1,16) AS "
                    "target_catalog,"
                    "substr(r.expected_type,1,257) AS expected_type,r.required,d.state FROM "
                    "wp_asset_references r "
                    "JOIN wp_asset_reference_documents d ON d.path_key=r.source_key WHERE "
                    "d.state<>'quarantined' AND " +
                        String( reverse ? "r.target_uuid=? AND r.target_catalog=?"
                                        : "r.source_key=? AND ? IN ('project','media')" ) +
                        " AND r.id>? ORDER BY r.id LIMIT ?",
                    { lookup, targetCatalog, StringUtil::toString( cursor ? cursor->afterId : 0 ),
                      StringUtil::toString( limit + 1 ) } );
                if( !rows )
                    throw std::runtime_error( "Cannot query reference graph" );
                while( !rows->eof() )
                {
                    if( page.edges.size() == limit )
                    {
                        page.hasMore = true;
                        break;
                    }
                    AssetReferenceEdge edge{
                        rows->getFieldValue( "path" ),          rows->getFieldValue( "slot" ),
                        rows->getFieldValue( "target_uuid" ),   rows->getFieldValue( "target_catalog" ),
                        rows->getFieldValue( "expected_type" ), rows->getFieldValue( "required" ) == "1"
                    };
                    if( !validValue( edge.sourcePath, 1024 ) || !validValue( edge.slot, 1024 ) ||
                        !validValue( edge.targetUuid, 256 ) || !validValue( edge.expectedType, 256 ) ||
                        ( edge.targetCatalog != "project" && edge.targetCatalog != "media" ) ||
                        ( rows->getFieldValue( "required" ) != "0" &&
                          rows->getFieldValue( "required" ) != "1" ) ||
                        ( rows->getFieldValue( "state" ) != "ready" &&
                          rows->getFieldValue( "state" ) != "pending" ) )
                        throw std::runtime_error( "Malformed reference graph record" );
                    edge.sourceState = rows->getFieldValue( "state" );
                    if( edge.targetCatalog == "project" )
                    {
                        AssetDatabaseManager::EntrySnapshot target;
                        if( !catalog->tryGetEntry( edge.targetUuid, target ) )
                            edge.diagnostic = "Missing project asset";
                        else if( !AssetReferenceChange::typeMatches(
                                     target.type,
                                     AssetReferenceChange::referenceType( edge.expectedType ) ) )
                            edge.diagnostic = "Reference type mismatch";
                    }
                    page.next.afterId = std::stoull( rows->getFieldValue( "id" ).c_str() );
                    page.edges.push_back( std::move( edge ) );
                    rows->nextRow();
                }
            }
            if( catalog->getCatalogGeneration() != page.next.generation )
                throw std::runtime_error( "Reference graph changed during query" );
            page.succeeded = true;
        }
        catch( const std::exception &e )
        {
            page.edges.clear();
            page.hasMore = false;
            page.error = e.what();
        }
        return page;
    }

    namespace
    {
        namespace assetfs = std::filesystem;
        bool operationSchema( AssetDatabaseManager &manager )
        {
            return queryCatalog( manager,
                                 "CREATE TABLE IF NOT EXISTS wp_asset_operations("
                                 "id TEXT PRIMARY KEY,kind TEXT NOT NULL,source TEXT NOT NULL,"
                                 "destination TEXT NOT NULL,root TEXT NOT NULL,state TEXT NOT NULL,"
                                 "digest TEXT NOT NULL)" ) &&
                   queryCatalog(
                       manager,
                       "CREATE TABLE IF NOT EXISTS wp_asset_operation_entries("
                       "operation_id TEXT NOT NULL,uuid TEXT NOT NULL,path TEXT NOT NULL,"
                       "type TEXT NOT NULL,path_key TEXT NOT NULL,new_uuid TEXT NOT NULL,"
                       "new_path TEXT NOT NULL,new_key TEXT NOT NULL,PRIMARY KEY(operation_id,uuid))" );
        }
        bool pathWithin( const String &parent, const String &path )
        {
            return path == parent ||
                   ( path.size() > parent.size() && path.compare( 0, parent.size(), parent ) == 0 &&
                     path[parent.size()] == '/' );
        }
        bool reparsePath( const assetfs::path &path )
        {
#if defined WP_PLATFORM_WIN32
            const auto attributes = GetFileAttributesW( path.c_str() );
            if( attributes != INVALID_FILE_ATTRIBUTES && ( attributes & FILE_ATTRIBUTE_REPARSE_POINT ) )
                return true;
#endif
            return assetfs::is_symlink( assetfs::symlink_status( path ) );
        }
        String treeDigest( const assetfs::path &path )
        {
            std::map<std::string, assetfs::path> entries;
            auto add = [&]( const assetfs::path &entry, const std::string &relative ) {
                if( reparsePath( entry ) ||
                    ( !assetfs::is_directory( entry ) && !assetfs::is_regular_file( entry ) ) )
                    throw std::runtime_error( "Asset operations reject links and special files" );
                if( entries.size() >= 100000 )
                    throw std::runtime_error( "Asset operation exceeds the tree entry limit" );
                entries.emplace( relative, entry );
            };
            add( path, "." );
            if( assetfs::is_directory( path ) )
                for( const auto &entry : assetfs::recursive_directory_iterator( path ) )
                    add( entry.path(), entry.path().lexically_relative( path ).generic_u8string() );
            u64 hash = 14695981039346656037ull;
            auto feed = [&]( const char *bytes, size_t size ) {
                for( size_t i = 0; i < size; ++i )
                {
                    hash ^= static_cast<unsigned char>( bytes[i] );
                    hash *= 1099511628211ull;
                }
            };
            for( const auto &entry : entries )
            {
                feed( entry.first.data(), entry.first.size() );
                const char kind = assetfs::is_directory( entry.second ) ? 'D' : 'F';
                feed( &kind, 1 );
                if( kind == 'F' )
                {
                    std::ifstream input( entry.second, std::ios::binary );
                    if( !input )
                        throw std::runtime_error( "Cannot snapshot asset bytes" );
                    char bytes[65536];
                    while( input )
                    {
                        input.read( bytes, sizeof( bytes ) );
                        feed( bytes, static_cast<size_t>( input.gcount() ) );
                    }
                    if( !input.eof() )
                        throw std::runtime_error( "Cannot finish asset snapshot" );
                }
                const char separator = 0;
                feed( &separator, 1 );
            }
            return std::to_string( hash ).c_str();
        }
        bool renameExclusive( const assetfs::path &source, const assetfs::path &destination,
                              String &error )
        {
#if defined WP_PLATFORM_WIN32
            if( !MoveFileExW( source.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH ) )
            {
                error =
                    ( "Asset rename failed (Windows error " + std::to_string( GetLastError() ) + ")" )
                        .c_str();
                return false;
            }
#else
            std::error_code ec;
            if( assetfs::exists( destination, ec ) || ec )
            {
                error = "Asset destination exists or is inaccessible";
                return false;
            }
            assetfs::rename( source, destination, ec );
            if( ec )
            {
                error = ec.message().c_str();
                return false;
            }
#endif
            return true;
        }
        assetfs::path operationPath( const String &root, const String &relative )
        {
            AssetCatalogPath canonical;
            String error;
            if( !canonicalAssetCatalogPath( root, relative, canonical, error ) )
                throw std::runtime_error( error.c_str() );
            return assetfs::u8path( root.c_str() ) / assetfs::u8path( canonical.path.c_str() );
        }
        struct OperationEntry
        {
            String uuid, path, type, key, newUuid, newPath, newKey;
        };
        Array<OperationEntry> operationEntries( AssetDatabaseManager &manager, const String &id )
        {
            auto rows = queryCatalog(
                manager, "SELECT * FROM wp_asset_operation_entries WHERE operation_id=?", { id } );
            if( !rows )
                throw std::runtime_error( "Cannot read asset operation identities" );
            Array<OperationEntry> entries;
            while( !rows->eof() )
            {
                entries.push_back( { rows->getFieldValue( "uuid" ), rows->getFieldValue( "path" ),
                                     rows->getFieldValue( "type" ), rows->getFieldValue( "path_key" ),
                                     rows->getFieldValue( "new_uuid" ),
                                     rows->getFieldValue( "new_path" ),
                                     rows->getFieldValue( "new_key" ) } );
                rows->nextRow();
            }
            return entries;
        }
        bool applyOperationEntries( AssetDatabaseManager &manager, const Array<OperationEntry> &entries,
                                    const String &kind, bool undo )
        {
            for( const auto &entry : entries )
            {
                const auto &lookup = undo && kind == "copy" ? entry.newUuid : entry.uuid;
                auto current = queryCatalog(
                    manager, "SELECT path,path_key,type FROM resources WHERE uuid=?", { lookup } );
                if( !current )
                    return false;
                if( undo && kind == "delete" )
                {
                    if( !current->eof() )
                        return false;
                }
                else
                {
                    const auto expected = undo ? entry.newPath : entry.path;
                    const auto expectedKey = undo ? entry.newKey : entry.key;
                    if( current->eof() || current->getFieldValue( "path" ) != expected ||
                        current->getFieldValue( "path_key" ) != expectedKey ||
                        current->getFieldValue( "type" ) != entry.type )
                        return false;
                }
                if( ( kind == "delete" && !undo ) || ( kind == "copy" && undo ) )
                {
                    if( !queryCatalog( manager, "DELETE FROM resources WHERE uuid=?", { lookup } ) )
                        return false;
                }
                else if( ( kind == "copy" && !undo ) || ( kind == "delete" && undo ) )
                {
                    if( !queryCatalog(
                            manager,
                            "INSERT INTO resources(uuid,path,type,kind,path_key) VALUES(?,?,?,'file',?)",
                            { undo ? entry.uuid : entry.newUuid, undo ? entry.path : entry.newPath,
                              entry.type, undo ? entry.key : entry.newKey } ) )
                        return false;
                    if( kind == "copy" && !undo )
                    {
                        String policyError;
                        if( !asset_package_detail::copyPolicy(
                                SmartPtr<AssetDatabaseManager>( &manager ), entry.uuid, entry.newUuid,
                                policyError ) )
                            return false;
                    }
                }
                else if( !queryCatalog( manager, "UPDATE resources SET path=?,path_key=? WHERE uuid=?",
                                        { undo ? entry.path : entry.newPath,
                                          undo ? entry.key : entry.newKey, lookup } ) )
                    return false;
            }
            return true;
        }
    }  // namespace

    AssetDatabaseManager::FileOperationResult AssetDatabaseManager::performFileOperation(
        FileOperation operation, const String &sourceInput, const String &destinationInput )
    {
        ScopedLock lock( this );
        FileOperationResult result;
        try
        {
            if( m_readOnly )
                throw std::runtime_error( "Built-in media catalog is read-only" );
            if( !m_catalogReady || !refreshDatabaseRevision() )
                throw std::runtime_error( "Catalog is not open" );
            for( const auto &recovery : recoverFileOperations() )
                if( !recovery.succeeded )
                    throw std::runtime_error(
                        "Resolve pending asset operation recovery before changing files" );
            AssetCatalogPath source, destination;
            if( !canonicalAssetCatalogPath( m_projectRoot, sourceInput, source, result.error ) )
                return result;
            if( pathWithin( ".workphone", source.key ) )
                throw std::runtime_error( "Asset operation storage is reserved" );
            result.operationId = StringUtil::getUUID();
            const String storage = ".workphone/asset-operations/" + result.operationId + "/payload";
            const String kind = operation == FileOperation::Delete ? "delete"
                                : operation == FileOperation::Copy ? "copy"
                                                                   : "move";
            if( !canonicalAssetCatalogPath(
                    m_projectRoot, operation == FileOperation::Delete ? storage : destinationInput,
                    destination, result.error ) )
                return result;
            if( operation != FileOperation::Delete && pathWithin( ".workphone", destination.key ) )
                throw std::runtime_error( "Asset operation storage is reserved" );
            if( pathWithin( source.key, destination.key ) )
                throw std::runtime_error( "Cannot place an asset inside itself" );
            const auto from = operationPath( m_projectRoot, source.path );
            const auto to = operationPath( m_projectRoot, destination.path );
            const auto stage = operationPath( m_projectRoot, storage );
            if( !assetfs::exists( from ) )
                throw std::runtime_error( "Asset source does not exist" );
            if( assetfs::exists( to ) )
                throw std::runtime_error( "Asset destination already exists" );
            if( operation != FileOperation::Delete && !assetfs::is_directory( to.parent_path() ) )
                throw std::runtime_error( "Destination folder does not exist" );
            const auto digest = treeDigest( from );
            const auto expectedRevision = m_databaseRevision;
            Array<OperationEntry> entries;
            auto rows =
                queryCatalog( *this, "SELECT uuid,path,type,path_key FROM resources WHERE kind='file'" );
            if( !rows )
                throw std::runtime_error( "Cannot plan catalog mutation" );
            while( !rows->eof() )
            {
                const auto key = rows->getFieldValue( "path_key" );
                if( pathWithin( source.key, key ) )
                {
                    const auto path = rows->getFieldValue( "path" );
                    AssetCatalogPath target;
                    const auto newPath = destination.path + path.substr( source.path.size() );
                    if( !canonicalAssetCatalogPath( m_projectRoot, newPath, target, result.error ) )
                        return result;
                    entries.push_back(
                        { rows->getFieldValue( "uuid" ), path, rows->getFieldValue( "type" ), key,
                          operation == FileOperation::Copy ? StringUtil::getUUID()
                                                           : rows->getFieldValue( "uuid" ),
                          target.path, target.key } );
                }
                rows->nextRow();
            }
            if( operation == FileOperation::Copy )
            {
                for( const auto &entry : entries )
                {
                    AssetPackagePolicy policy;
                    if( !getAssetPackagePolicy( SmartPtr<AssetDatabaseManager>( this ), entry.uuid,
                                                policy, result.error ) )
                        return result;
                }
                auto checkFormat = []( const assetfs::path &file ) {
                    const auto extension = StringUtil::make_lower( file.extension().u8string().c_str() );
                    if( extension == ".fbscene" || extension == ".fbscenebin" ||
                        extension == ".fbscenexml" || extension == ".prefab" ||
                        extension == ".fbprefab" || extension == ".resource" )
                        throw std::runtime_error(
                            "Copying scenes/prefabs/resources requires a registered identity remapper" );
                };
                checkFormat( from );
                if( assetfs::is_directory( from ) )
                    for( const auto &entry : assetfs::recursive_directory_iterator( from ) )
                        if( assetfs::is_regular_file( entry.path() ) )
                            checkFormat( entry.path() );
            }
            Array<String> removedIds;
            for( const auto &entry : entries )
                removedIds.push_back( entry.uuid );
            {
                CatalogTransaction transaction( *this );
                if( !transaction.active() ||
                    ( operation == FileOperation::Delete &&
                      !referenceDeleteAllowed( *this, removedIds, source.key, result.error ) ) ||
                    !applyReferenceDocuments( *this, kind, source.path, destination.path, false ) )
                    throw std::runtime_error(
                        result.error.empty() ? "Operation conflicts with indexed reference documents"
                                             : result.error.c_str() );
                // Preflight graph mutations are rolled back, just like catalog undo preflight.
            }
            assetfs::create_directories( stage.parent_path() );
            // Re-audit after creating the reserved storage prefix.
            operationPath( m_projectRoot, storage );
            {
                CatalogTransaction transaction( *this );
                if( !transaction.active() || !operationSchema( *this ) ||
                    !queryCatalog( *this,
                                   "INSERT INTO wp_asset_operations VALUES(?,?,?,?,?,'prepared',?)",
                                   { result.operationId, kind, source.path, destination.path,
                                     m_projectRoot, digest } ) )
                    throw std::runtime_error( "Cannot persist asset operation journal" );
                for( const auto &entry : entries )
                    if( !queryCatalog( *this,
                                       "INSERT INTO wp_asset_operation_entries VALUES(?,?,?,?,?,?,?,?)",
                                       { result.operationId, entry.uuid, entry.path, entry.type,
                                         entry.key, entry.newUuid, entry.newPath, entry.newKey } ) )
                        throw std::runtime_error( "Cannot persist operation identity snapshot" );
                if( !transaction.commit() )
                    throw std::runtime_error( "Cannot commit operation preflight" );
            }
            if( operation == FileOperation::Copy )
                assetfs::copy( from, stage, assetfs::copy_options::recursive );
            else if( !renameExclusive( from, stage, result.error ) )
                return result;
            if( treeDigest( stage ) != digest )
                throw std::runtime_error( "Asset changed while staging; recovery required" );
            if( operation != FileOperation::Delete && !renameExclusive( stage, to, result.error ) )
                return result;
            {
                CatalogTransaction transaction( *this );
                if( !transaction.active() || !refreshDatabaseRevision() ||
                    m_databaseRevision != expectedRevision ||
                    ( operation == FileOperation::Delete &&
                      !referenceDeleteAllowed( *this, removedIds, source.key, result.error ) ) ||
                    !applyOperationEntries( *this, entries, kind, false ) ||
                    !applyReferenceDocuments( *this, kind, source.path, destination.path, false ) ||
                    !queryCatalog( *this, "UPDATE wp_asset_operations SET state='committed' WHERE id=?",
                                   { result.operationId } ) ||
                    !transaction.commit() )
                    throw std::runtime_error(
                        result.error.empty() ? "Asset catalog commit failed; journal recovery required"
                                             : result.error.c_str() );
            }
            invalidateCatalog();
            result.succeeded = true;
        }
        catch( const std::exception &error )
        {
            result.error = error.what();
        }
        return result;
    }

    AssetDatabaseManager::FileOperationResult AssetDatabaseManager::undoFileOperation( const String &id )
    {
        ScopedLock lock( this );
        FileOperationResult result;
        result.operationId = id;
        try
        {
            if( m_readOnly )
                throw std::runtime_error( "Built-in media catalog is read-only" );
            if( !m_catalogReady )
                throw std::runtime_error( "Original project catalog is closed" );
            auto row = queryCatalog( *this, "SELECT * FROM wp_asset_operations WHERE id=?", { id } );
            if( !row || row->eof() || row->getFieldValue( "state" ) != "committed" ||
                row->getFieldValue( "root" ) != m_projectRoot )
                throw std::runtime_error( "Asset operation is not committed in this project" );
            const auto kind = row->getFieldValue( "kind" );
            const auto from = operationPath( m_projectRoot, row->getFieldValue( "destination" ) );
            const auto to = operationPath(
                m_projectRoot, kind == "copy" ? ".workphone/asset-operations/" + id + "/payload"
                                              : row->getFieldValue( "source" ) );
            if( assetfs::exists( to ) || treeDigest( from ) != row->getFieldValue( "digest" ) )
                throw std::runtime_error( "Undo conflicts with external files or changed asset bytes" );
            const auto entries = operationEntries( *this, id );
            Array<String> removedIds;
            for( const auto &entry : entries )
                removedIds.push_back( entry.newUuid );
            const auto originalSource = row->getFieldValue( "source" );
            const auto originalDestination = row->getFieldValue( "destination" );
            {
                CatalogTransaction transaction( *this );
                // Preflight catalog conflicts without committing the reverse mutation.
                if( !transaction.active() ||
                    ( kind == "copy" &&
                      !referenceDeleteAllowed( *this, removedIds, {}, result.error ) ) ||
                    !applyOperationEntries( *this, entries, kind, true ) ||
                    !applyReferenceDocuments( *this, kind, originalSource, originalDestination, true ) )
                    throw std::runtime_error(
                        result.error.empty()
                            ? "Undo conflicts with current asset identities/reference documents"
                            : result.error.c_str() );
            }
            if( !queryCatalog( *this, "UPDATE wp_asset_operations SET state='undoing' WHERE id=?",
                               { id } ) )
                throw std::runtime_error( "Cannot journal undo" );
            if( !renameExclusive( from, to, result.error ) )
                return result;
            {
                CatalogTransaction transaction( *this );
                if( !transaction.active() ||
                    ( kind == "copy" &&
                      !referenceDeleteAllowed( *this, removedIds, {}, result.error ) ) ||
                    !applyOperationEntries( *this, entries, kind, true ) ||
                    !applyReferenceDocuments( *this, kind, originalSource, originalDestination, true ) ||
                    !queryCatalog( *this, "UPDATE wp_asset_operations SET state='undone' WHERE id=?",
                                   { id } ) ||
                    !transaction.commit() )
                    throw std::runtime_error( "Undo catalog commit failed; recovery required" );
            }
            invalidateCatalog();
            result.succeeded = true;
        }
        catch( const std::exception &error )
        {
            result.error = error.what();
        }
        return result;
    }

    Array<AssetDatabaseManager::FileOperationResult> AssetDatabaseManager::recoverFileOperations()
    {
        ScopedLock lock( this );
        Array<FileOperationResult> results;
        if( m_readOnly )
            return results;
        if( !m_catalogReady )
            return results;
        auto table = queryCatalog(
            *this, "SELECT name FROM sqlite_master WHERE type='table' AND name='wp_asset_operations'" );
        if( !table || table->eof() )
            return results;
        auto rows = queryCatalog(
            *this,
            "SELECT * FROM wp_asset_operations WHERE state IN ('prepared','undoing') ORDER BY id" );
        if( !rows )
            return results;
        while( !rows->eof() )
        {
            FileOperationResult result;
            result.operationId = rows->getFieldValue( "id" );
            try
            {
                if( rows->getFieldValue( "root" ) != m_projectRoot )
                    throw std::runtime_error( "Recovery root differs from journal root" );
                const auto source = operationPath( m_projectRoot, rows->getFieldValue( "source" ) );
                const auto destination =
                    operationPath( m_projectRoot, rows->getFieldValue( "destination" ) );
                const auto stage = operationPath(
                    m_projectRoot, ".workphone/asset-operations/" + result.operationId + "/payload" );
                const auto kind = rows->getFieldValue( "kind" );
                const auto state = rows->getFieldValue( "state" );
                const auto digest = rows->getFieldValue( "digest" );
                String next;
                if( state == "undoing" )
                {
                    const auto reversed = kind == "copy" ? stage : source;
                    if( assetfs::exists( destination ) )
                    {
                        if( treeDigest( destination ) != digest || assetfs::exists( reversed ) )
                            throw std::runtime_error( "Ambiguous interrupted undo; content retained" );
                    }
                    else if( !assetfs::exists( reversed ) || treeDigest( reversed ) != digest ||
                             !renameExclusive( reversed, destination, result.error ) )
                        throw std::runtime_error(
                            "Interrupted undo recovery conflicts; content retained" );
                    next = "committed";
                }
                else if( kind == "copy" )
                {
                    // The source survives copy. Retain any staged/published copy in
                    // quarantine; never recursively delete authored data during recovery.
                    if( assetfs::exists( destination ) )
                    {
                        if( assetfs::exists( stage ) || treeDigest( destination ) != digest ||
                            !renameExclusive( destination, stage, result.error ) )
                            throw std::runtime_error( "Copy recovery conflicts; content retained" );
                    }
                    next = "rolled-back";
                }
                else
                {
                    const bool atSource = assetfs::exists( source );
                    const bool atStage = assetfs::exists( stage );
                    const bool atDestination = kind != "delete" && assetfs::exists( destination );
                    if( static_cast<int>( atSource ) + static_cast<int>( atStage ) +
                            static_cast<int>( atDestination ) !=
                        1 )
                        throw std::runtime_error( "Ambiguous asset recovery; content retained" );
                    const auto location = atSource ? source : atStage ? stage : destination;
                    if( treeDigest( location ) != digest ||
                        ( !atSource && !renameExclusive( location, source, result.error ) ) )
                        throw std::runtime_error(
                            "Recovery conflicts with external bytes; content retained" );
                    next = "rolled-back";
                }
                result.succeeded =
                    queryCatalog( *this, "UPDATE wp_asset_operations SET state=? WHERE id=?",
                                  { next, result.operationId } ) != nullptr;
                if( !result.succeeded )
                    result.error = "Could not finish journal recovery";
            }
            catch( const std::exception &error )
            {
                result.error = error.what();
            }
            results.push_back( result );
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
