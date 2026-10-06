#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Core/FileInfo.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AssetDatabaseManager, DatabaseManager );
    namespace
    {
        bool validValue( const String &value, size_t limit )
        {
            return !value.empty() && value.size() <= limit && value.find('\0') == String::npos;
        }
        SmartPtr<IDatabaseQuery> queryCatalog( AssetDatabaseManager &manager,
            const String &sql, const Array<String> &values = {} )
        {
            auto database = manager.getDatabase();
            auto bound = database ? dynamic_cast<IParameterizedDatabase *>(database.get()) : nullptr;
            if( !bound || !database->isLoaded() )
            {
                WP_LOG_ERROR("Asset catalog requires an open parameterized database backend.");
                return nullptr;
            }
            return bound->queryBound(sql, values);
        }
        // The manager's recursive mutex covers the transaction and publication.
        class CatalogTransaction
        {
        public:
            explicit CatalogTransaction( AssetDatabaseManager &manager ) : m_manager(manager)
            { m_active = queryCatalog(manager, "BEGIN IMMEDIATE") != nullptr; }
            ~CatalogTransaction() { if( m_active ) queryCatalog(m_manager, "ROLLBACK"); }
            bool active() const { return m_active; }
            bool commit()
            {
                if( !m_active || !queryCatalog(m_manager, "COMMIT") ) return false;
                m_active = false;
                return true;
            }
        private:
            AssetDatabaseManager &m_manager;
            bool m_active = false;
        };
        String resourcePath( SmartPtr<ISharedObject> object )
        {
            if( !object || !object->isDerived<IResource>() ) return "scene";
            auto resource = static_pointer_cast<IResource>(object);
            String path;
            auto app = core::IApplicationManager::instancePtr();
            auto filesystem = app ? app->getFileSystem() : nullptr;
            if( filesystem && !resource->getFileSystemId().is_nil() )
            {
                FileInfo info;
                if( filesystem->findFileInfo(resource->getFileSystemId(), info) ) path = info.filePath.str();
            }
            return path.empty() ? resource->getFilePath() : path;
        }
        SmartPtr<IBuildDirector> readEntry( SmartPtr<IDatabaseQuery> query )
        {
            if( !query || query->eof() ) return nullptr;
            const auto uuid = query->getFieldValue("uuid");
            const auto path = query->getFieldValue("path");
            query->nextRow();
            if( !query->eof() || !validValue(uuid,256) || !validValue(path,1024) )
            {
                WP_LOG_ERROR("Ambiguous or invalid asset catalog entry.");
                return nullptr;
            }
            auto director = make_ptr<scene::ResourceDirector>();
            director->setResourcePath(path);
            director->setResourceUUID(uuid);
            return director;
        }
    }
    AssetDatabaseManager::AssetDatabaseManager() { m_resourcesTableName = "resources"; }
    AssetDatabaseManager::~AssetDatabaseManager() = default;
    void AssetDatabaseManager::load( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock(this);
        DatabaseManager::load(data);
    }
    void AssetDatabaseManager::unload( SmartPtr<ISharedObject> data )
    {
        ScopedLock lock(this);
        if( auto db = getDatabase() ) db->close();
        setDatabase(nullptr);
        clearResourceEntryCache();
        DatabaseManager::unload(data);
    }
    void AssetDatabaseManager::loadFromFile( const String &path )
    {
        ScopedLock lock(this);
        clearResourceEntryCache();
        DatabaseManager::loadFromFile(path);
        create();
    }
    void AssetDatabaseManager::loadFromFile( const StringW &path )
    {
        // Both overloads use the same switching and project path policy.
        loadFromFile(StringUtilW::toUTF16to8(path));
    }
    void AssetDatabaseManager::create()
    {
        ScopedLock lock(this);
        CatalogTransaction transaction(*this);
        if( !transaction.active() ) return;
        if( !queryCatalog(*this, "CREATE TABLE IF NOT EXISTS resources(id INTEGER PRIMARY KEY, uuid VARCHAR, path VARCHAR, type VARCHAR)") ||
            !queryCatalog(*this, "CREATE UNIQUE INDEX IF NOT EXISTS idx_resources_uuid_unique ON resources(uuid)") ||
            !queryCatalog(*this, "CREATE TRIGGER IF NOT EXISTS resources_file_path_insert BEFORE INSERT ON resources "
                "WHEN NEW.path<>'scene' AND NEW.path<>'' AND EXISTS(SELECT 1 FROM resources WHERE path=NEW.path) "
                "BEGIN SELECT RAISE(ABORT,'Conflicting asset path'); END") ||
            !queryCatalog(*this, "CREATE TRIGGER IF NOT EXISTS resources_file_path_update BEFORE UPDATE OF path ON resources "
                "WHEN NEW.path<>'scene' AND NEW.path<>'' AND EXISTS(SELECT 1 FROM resources WHERE path=NEW.path AND id<>NEW.id) "
                "BEGIN SELECT RAISE(ABORT,'Conflicting asset path'); END") ||
            !queryCatalog(*this, "CREATE INDEX IF NOT EXISTS idx_resources_path ON resources(path)") || !transaction.commit() )
        {
            WP_LOG_ERROR("Catalog schema initialization failed; legacy duplicates require explicit repair.");
        }
    }
    void AssetDatabaseManager::destroy() { unload(nullptr); }
    void AssetDatabaseManager::clearDatabase()
    {
        ScopedLock lock(this);
        CatalogTransaction transaction(*this);
        if( transaction.active() && queryCatalog(*this,"DELETE FROM resources") && transaction.commit() ) clearResourceEntryCache();
    }
    void AssetDatabaseManager::clearResourceEntryCache()
    {
        ScopedLock lock(this);
        m_resourceEntriesByUUID.clear();
        m_resourceEntriesByPath.clear();
    }
    bool AssetDatabaseManager::hasResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock(this);
        if( !object || !object->getHandle() ) return false;
        if( hasResourceById(object->getHandle()->getUUIDAsString()) ) return true;
        if( !object->isDerived<IResource>() ) return false;
        return getResourceEntryFromPath(resourcePath(object)) != nullptr;
    }
    void AssetDatabaseManager::addResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock(this);
        if( !object || !object->getHandle() ) return;
        auto uuid = object->getHandle()->getUUIDAsString();
        if( uuid.empty() ) uuid = StringUtil::getUUID();
        const auto path = resourcePath(object);
        auto types = TypeManager::instance();
        if( !types || !validValue(uuid,256) || !validValue(path,1024) ) return;
        auto type = types->getName(object->getTypeInfo());
        if( object->isDerived<scene::IGameActor>() ) type = "Actor";
        else if( object->isDerived<render::IMaterial>() ) type = "Material";
        else if( object->isDerived<render::ITexture>() ) type = "Texture";
        if( !validValue(type,256) ) return;
        CatalogTransaction transaction(*this);
        if( !transaction.active() ) return;
        auto existing = queryCatalog(*this,"SELECT uuid,path,type FROM resources WHERE uuid=? OR (path=? AND path<>'scene')",{uuid,path});
        if( !existing ) return;
        if( !existing->eof() )
        {
            const bool identical = existing->getFieldValue("uuid") == uuid &&
                existing->getFieldValue("path") == path && existing->getFieldValue("type") == type;
            existing->nextRow();
            if( identical && existing->eof() ) transaction.commit();
            else { WP_LOG_ERROR("Conflicting catalog identity; insertion rejected."); }
            return;
        }
        if( queryCatalog(*this,"INSERT INTO resources(uuid,path,type) VALUES(?,?,?)",{uuid,path,type}) && transaction.commit() )
        {
            object->getHandle()->setUUID(uuid);
            clearResourceEntryCache();
        }
    }
    void AssetDatabaseManager::updateResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock(this);
        if( !object || !object->getHandle() ) return;
        const auto uuid = object->getHandle()->getUUIDAsString();
        const auto path = resourcePath(object);
        if( !validValue(uuid,256) || !validValue(path,1024) ) return;
        CatalogTransaction transaction(*this);
        if( transaction.active() && queryCatalog(*this,"UPDATE resources SET path=? WHERE uuid=?",{path,uuid}) && transaction.commit() ) clearResourceEntryCache();
    }
    void AssetDatabaseManager::removeResourceEntry( SmartPtr<ISharedObject> object )
    {
        ScopedLock lock(this);
        if( !object || !object->getHandle() ) return;
        const auto uuid = object->getHandle()->getUUIDAsString();
        // UUID is authoritative: never broaden deletion to a shared or stale path.
        if( !validValue(uuid,256) ) return;
        CatalogTransaction transaction(*this);
        if( transaction.active() && queryCatalog(*this,"DELETE FROM resources WHERE uuid=?",{uuid}) && transaction.commit() ) clearResourceEntryCache();
    }
    void AssetDatabaseManager::removeResourceEntryFromPath( const String &path )
    {
        ScopedLock lock(this);
        if( path == "scene" || !validValue(path,1024) ) return;
        CatalogTransaction transaction(*this);
        if( transaction.active() && queryCatalog(*this,"DELETE FROM resources WHERE path=?",{path}) && transaction.commit() ) clearResourceEntryCache();
    }
    bool AssetDatabaseManager::hasResourceById( const String &uuid )
    {
        ScopedLock lock(this);
        if( !validValue(uuid,256) ) return false;
        auto query = queryCatalog(*this,"SELECT 1 FROM resources WHERE uuid=? LIMIT 1",{uuid});
        return query && !query->eof();
    }
    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntry( const String &uuid )
    {
        ScopedLock lock(this);
        if( !validValue(uuid,256) ) return nullptr;
        return readEntry(queryCatalog(*this,"SELECT uuid,path FROM resources WHERE uuid=? LIMIT 2",{uuid}));
    }
    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntryFromPath( const String &path )
    {
        ScopedLock lock(this);
        if( !validValue(path,1024) ) return nullptr;
        // A miss never creates an identity. Detached results cannot mutate the catalog.
        return readEntry(queryCatalog(*this,"SELECT uuid,path FROM resources WHERE path=? LIMIT 2",{path}));
    }
    String AssetDatabaseManager::getResourcesTableName() const { return m_resourcesTableName.load(); }
    void AssetDatabaseManager::sestResourcesTableName( const String &name )
    {
        if( name != "resources" ) { WP_LOG_ERROR("Only the resources catalog table is supported."); return; }
        m_resourcesTableName = name;
    }
    String AssetDatabaseManager::getStaticComponentsTableName() const { return m_staticComponentsTableName.load(); }
    void AssetDatabaseManager::setStaticComponentsTableName( const String &name ) { m_staticComponentsTableName = name; }
    String AssetDatabaseManager::getComponentsTableName() const { return m_componentsTableName.load(); }
    void AssetDatabaseManager::setComponentsTableName( const String &name ) { m_componentsTableName = name; }
}
