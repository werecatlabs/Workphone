#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/Scene/IGameActor.hpp>
#include <Workphone/Interface/Scene/IComponent.hpp>
#include <Workphone/System/Director.hpp>
#include <Workphone/Scene/Directors/ResourceDirector.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/LogManager.hpp>

namespace workphone
{

    WP_CLASS_REGISTER_DERIVED( workphone, AssetDatabaseManager, DatabaseManager );

    AssetDatabaseManager::AssetDatabaseManager() = default;

    AssetDatabaseManager::~AssetDatabaseManager() = default;

    void AssetDatabaseManager::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            DatabaseManager::load( data );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            ScopedLock lock( this );

            if( auto db = getDatabase() )
            {
                db->close();
                setDatabase( nullptr );
            }

            clearResourceEntryCache();
            DatabaseManager::unload( data );
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseManager::loadFromFile( const String &filePath )
    {
        auto database = getDatabase();
        auto shouldClearCache = getDatabasePath() != filePath || !database || !database->isLoaded();
        DatabaseManager::loadFromFile( filePath );
        if( shouldClearCache )
        {
            clearResourceEntryCache();
        }
        create();
    }

    void AssetDatabaseManager::loadFromFile( const StringW &filePath )
    {
        auto filePathStr = StringUtil::toStringC( filePath );
        auto database = getDatabase();
        auto shouldClearCache = getDatabasePath() != filePathStr || !database || !database->isLoaded();
        DatabaseManager::loadFromFile( filePath );
        if( shouldClearCache )
        {
            clearResourceEntryCache();
        }
        create();
    }

    void AssetDatabaseManager::create()
    {
        static const String createTableSql = String(
            "CREATE TABLE IF NOT EXISTS resources( id INTEGER PRIMARY KEY UNIQUE, "
            "uuid VARCHAR, path VARCHAR, type VARCHAR );" );
        executeDML( createTableSql );

        executeDML( "CREATE INDEX IF NOT EXISTS idx_resources_uuid ON resources(uuid);" );
        executeDML( "CREATE INDEX IF NOT EXISTS idx_resources_path ON resources(path);" );
    }

    void AssetDatabaseManager::destroy()
    {
    }

    void AssetDatabaseManager::clearDatabase()
    {
        try
        {
            auto dropTableSql = "DROP TABLE IF EXISTS resources";
            executeDML( dropTableSql );

            auto createTableSql =
                "CREATE TABLE IF NOT EXISTS resources( id INTEGER PRIMARY KEY UNIQUE, uuid VARCHAR, "
                "path VARCHAR, type VARCHAR );";
            executeDML( createTableSql );
            executeDML( "CREATE INDEX IF NOT EXISTS idx_resources_uuid ON resources(uuid);" );
            executeDML( "CREATE INDEX IF NOT EXISTS idx_resources_path ON resources(path);" );
            clearResourceEntryCache();
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseManager::clearResourceEntryCache()
    {
        m_resourceEntriesByUUID.clear();
        m_resourceEntriesByPath.clear();
    }

    auto AssetDatabaseManager::hasResourceEntry( SmartPtr<ISharedObject> object ) -> bool
    {
        WP_ASSERT( object );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto handle = object->getHandle();
        WP_ASSERT( handle );

        auto filePath = String( "" );
        if( object->isDerived<IResource>() )
        {
            auto resource = workphone::static_pointer_cast<IResource>( object );
            auto fileSystemId = resource->getFileSystemId();

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( fileSystemId, fileInfo ) )
            {
                filePath = fileInfo.filePath.c_str();
            }

            if( StringUtil::isNullOrEmpty( filePath ) )
            {
                filePath = resource->getFilePath();
            }
        }
        else
        {
            filePath = String( "scene" );
        }

        if( object->isDerived<scene::IGameActor>() || object->isDerived<scene::IComponent>() )
        {
            auto uuidSql = String( "select 1 from resources where uuid = '" ) +
                           handle->getUUIDAsString() + "' limit 1";
            auto uuidQuery = executeQuery( uuidSql );
            if( uuidQuery )
            {
                if( !uuidQuery->eof() )
                {
                    return true;
                }
            }
        }
        else
        {
            auto uuidSql = String( "select 1 from resources where uuid = '" ) +
                           handle->getUUIDAsString() + "' limit 1";
            auto uuidQuery = executeQuery( uuidSql );
            if( uuidQuery )
            {
                if( !uuidQuery->eof() )
                {
                    return true;
                }
            }

            auto sql = String( "select 1 from resources where path = '" ) + filePath + "' limit 1";
            auto query = executeQuery( sql );
            if( query )
            {
                if( !query->eof() )
                {
                    return true;
                }
            }
        }

        return false;
    }

    void AssetDatabaseManager::addResourceEntry( SmartPtr<ISharedObject> object )
    {
        try
        {
            if( !object )
            {
                return;
            }

            if( hasResourceEntry( object ) )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto handle = object->getHandle();
            WP_ASSERT( handle );

            auto uuid = handle->getUUIDAsString();
            if( StringUtil::isNullOrEmpty( uuid ) )
            {
                uuid = StringUtil::getUUID();
                handle->setUUID( uuid );
            }

            //WP_ASSERT( hasResourceById( uuid ) == false );

            String filePath;

            if( object->isDerived<IResource>() )
            {
                auto resource = workphone::static_pointer_cast<IResource>( object );
                auto fileSystemId = resource->getFileSystemId();
                if( !fileSystemId.is_nil() )
                {
                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( fileSystemId, fileInfo ) )
                    {
                        filePath = fileInfo.filePath.str();
                    }

                    if( StringUtil::isNullOrEmpty( filePath ) )
                    {
                        filePath = resource->getFilePath();
                    }
                }
                else
                {
                    filePath = resource->getFilePath();
                }
            }
            else
            {
                filePath = String( "scene" );
            }

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            auto typeInfo = object->getTypeInfo();
            auto dataStr = typeManager->getName( typeInfo );

            if( object->isDerived<scene::IGameActor>() )
            {
                dataStr = "Actor";
            }
            else if( object->isDerived<render::IMaterial>() )
            {
                dataStr = "Material";
            }
            else if( object->isDerived<render::ITexture>() )
            {
                dataStr = "Texture";
            }

            auto sql = "INSERT INTO `resources`(`id`,`uuid`,`path`,`type`) VALUES (NULL, '" + uuid +
                       "', '" + filePath + "', '" + dataStr + "')";
            executeQuery( sql );
            clearResourceEntryCache();
        }
        catch( Exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssetDatabaseManager::updateResourceEntry( SmartPtr<ISharedObject> object )
    {
        WP_ASSERT( object );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto handle = object->getHandle();
        WP_ASSERT( handle );

        auto uuid = handle->getUUIDAsString();
        WP_ASSERT( !StringUtil::isNullOrEmpty( uuid ) );

        auto filePath = String( "" );
        if( object->isDerived<IResource>() )
        {
            auto resource = workphone::static_pointer_cast<IResource>( object );
            auto fileSystemId = resource->getFileSystemId();
            if( !fileSystemId.is_nil() )
            {
                FileInfo fileInfo;
                if( fileSystem->findFileInfo( fileSystemId, fileInfo ) )
                {
                    filePath = fileInfo.filePath.c_str();
                }
            }
            else
            {
                filePath = resource->getFilePath();
            }
        }
        else
        {
            filePath = String( "scene" );
        }

        auto sql =
            String( "select * from resources where uuid = '" ) + uuid + "' or path = '" + filePath + "'";
        auto query = executeQuery( sql );
        if( query )
        {
            if( !query->eof() )
            {
                auto updateSql = String( "UPDATE `resources` SET `path`='" ) + filePath +
                                 String( "' WHERE `uuid`='" ) + uuid + String( "'" );
                executeQuery( updateSql );
                clearResourceEntryCache();
            }
        }
    }

    void AssetDatabaseManager::removeResourceEntry( SmartPtr<ISharedObject> object )
    {
        if( !object )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto handle = object->getHandle();
        WP_ASSERT( handle );

        auto uuid = handle->getUUIDAsString();
        auto filePath = String( "" );
        if( object->isDerived<IResource>() )
        {
            auto resource = workphone::static_pointer_cast<IResource>( object );
            auto fileSystemId = resource->getFileSystemId();

            if( !fileSystemId.is_nil() )
            {
                FileInfo fileInfo;
                if( fileSystem->findFileInfo( fileSystemId, fileInfo ) )
                {
                    filePath = fileInfo.filePath.c_str();
                }
            }

            if( StringUtil::isNullOrEmpty( filePath ) )
            {
                filePath = resource->getFilePath();
            }
        }
        else
        {
            filePath = String( "scene" );
        }

        auto hasUuid = !StringUtil::isNullOrEmpty( uuid );
        auto hasPath = !StringUtil::isNullOrEmpty( filePath );
        if( !hasUuid && !hasPath )
        {
            return;
        }

        auto deleteSql = String( "DELETE FROM resources WHERE " );
        if( hasUuid )
        {
            deleteSql += String( "uuid = '" ) + uuid + "'";
        }

        if( hasUuid && hasPath )
        {
            deleteSql += String( " OR " );
        }

        if( hasPath )
        {
            deleteSql += String( "path = '" ) + filePath + "'";
        }

        executeQuery( deleteSql );
        clearResourceEntryCache();
    }

    void AssetDatabaseManager::removeResourceEntryFromPath( const String &path )
    {
        auto sql = String( "select 1 from resources where path = '" ) + path + "' limit 1";
        auto query = executeQuery( sql );
        if( query )
        {
            if( !query->eof() )
            {
                auto deleteSql = String( "DELETE FROM resources WHERE path = '" ) + path + "'";
                executeQuery( deleteSql );
                clearResourceEntryCache();
            }
        }
    }

    auto AssetDatabaseManager::hasResourceById( const String &uuid ) -> bool
    {
        auto sql = static_cast<String>( "select 1 from 'resources' where uuid='" + uuid + "' limit 1" );
        auto query = executeQuery( sql );
        if( query )
        {
            if( !query->eof() )
            {
                return true;
            }
        }

        return false;
    }

    String AssetDatabaseManager::getResourcesTableName() const
    {
        return m_resourcesTableName.load();
    }

    void AssetDatabaseManager::sestResourcesTableName( const String &resourcesTableName )
    {
        m_resourcesTableName = resourcesTableName;
    }

    String AssetDatabaseManager::getStaticComponentsTableName() const
    {
        return m_staticComponentsTableName.load();
    }

    void AssetDatabaseManager::setStaticComponentsTableName( const String &staticComponentsTableName )
    {
        m_staticComponentsTableName = staticComponentsTableName;
    }

    String AssetDatabaseManager::getComponentsTableName() const
    {
        return m_componentsTableName.load();
    }

    void AssetDatabaseManager::setComponentsTableName( const String &componentsTableName )
    {
        m_componentsTableName = componentsTableName;
    }

    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntry( const String &uuid )
    {
        auto cacheIt = m_resourceEntriesByUUID.find( uuid );
        if( cacheIt != m_resourceEntriesByUUID.end() )
        {
            return cacheIt->second;
        }

        auto sql = String( "select id, uuid, path, type from resources where uuid = '" ) + uuid +
                   String( "' limit 1" );
        auto query = executeQuery( sql );
        if( !query || query->eof() )
        {
            return nullptr;
        }

        auto idStr = query->getFieldValue( "id" );
        auto uuidStr = query->getFieldValue( "uuid" );
        auto pathStr = query->getFieldValue( "path" );
        auto typeStr = query->getFieldValue( "type" );

        auto director = workphone::make_ptr<scene::ResourceDirector>();
        director->setResourcePath( pathStr );
        director->setResourceUUID( uuidStr );
        m_resourceEntriesByUUID[uuid] = director;
        m_resourceEntriesByPath[pathStr] = director;
        return director;
    }

    SmartPtr<IBuildDirector> AssetDatabaseManager::getResourceEntryFromPath( const String &path )
    {
        auto cacheIt = m_resourceEntriesByPath.find( path );
        if( cacheIt != m_resourceEntriesByPath.end() )
        {
            return cacheIt->second;
        }

        auto sql = String( "select id, uuid, path, type from resources where path = '" ) + path +
                   String( "' limit 1" );
        auto query = executeQuery( sql );
        if( query )
        {
            if( !query->eof() )
            {
                auto idStr = query->getFieldValue( "id" );
                auto uuidStr = query->getFieldValue( "uuid" );
                auto pathStr = query->getFieldValue( "path" );
                auto typeStr = query->getFieldValue( "type" );

                auto director = workphone::make_ptr<scene::ResourceDirector>();
                director->setResourcePath( pathStr );
                director->setResourceUUID( uuidStr );
                m_resourceEntriesByPath[path] = director;
                m_resourceEntriesByUUID[uuidStr] = director;
                return director;
            }
        }

        auto director = workphone::make_ptr<scene::ResourceDirector>();
        director->setResourcePath( path );
        director->setResourceUUID( StringUtil::getUUID() );
        m_resourceEntriesByPath[path] = director;
        return director;
    }
}  // namespace workphone
