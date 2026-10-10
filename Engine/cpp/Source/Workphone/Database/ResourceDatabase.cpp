#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Script/ScriptAsset.hpp>
#include <Workphone/Database/ResourceReference.hpp>
#include <Workphone/System/Director.hpp>
#include <Workphone/Scene/Directors/MaterialResourceDirector.hpp>
#include <Workphone/Scene/Directors/MeshResourceDirector.hpp>
#include <Workphone/Scene/Directors/TextureResourceDirector.hpp>
#include <Workphone/Scene/Directors/LightingDirector.hpp>
#include <Workphone/Scene/Directors/SoundResourceDirector.hpp>
#include <Workphone/Scene/GameManager.hpp>
#include <Workphone/Interface/Database/IDatabase.hpp>
#include <Workphone/Interface/Database/IDatabaseManager.hpp>
#include <Workphone/Interface/Database/IDatabaseQuery.hpp>
#include <Workphone/Interface/Database/IParameterizedDatabase.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSystem.hpp>
#include <Workphone/Interface/Graphics/IMaterial.hpp>
#include <Workphone/Interface/Graphics/IFont.hpp>
#include <Workphone/Interface/Graphics/IFontManager.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Interface/Graphics/IMeshConverter.hpp>
#include <Workphone/Interface/Graphics/ITexture.hpp>
#include <Workphone/Interface/Graphics/ITextureManager.hpp>
#include <Workphone/Interface/IO/IFolderExplorer.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/System/IFactoryManager.hpp>
#include <Workphone/Interface/System/IFSMManager.hpp>
#include <Workphone/Interface/System/IResource.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Interface/Scene/IGamePrefab.hpp>
#include <Workphone/Interface/Scene/ITransform.hpp>
#include <Workphone/Interface/Sound/ISound.hpp>
#include <Workphone/Interface/Sound/ISoundManager.hpp>
#include <Workphone/Interface/System/IJobQueue.hpp>
#include <Workphone/Interface/System/ITask.hpp>
#include <Workphone/Interface/System/ITaskManager.hpp>
#include <Workphone/Interface/System/IThreadPool.hpp>
#include <Workphone/IO/FileSystem.hpp>
#include <Workphone/System/ApplicationManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>

#include <initializer_list>

namespace workphone
{
    namespace
    {
        SmartPtr<IDatabaseQuery> queryResourceValues( SmartPtr<IDatabaseManager> manager,
                                                     const String &sql,
                                                     const Array<String> &values )
        {
            if( !manager )
                return nullptr;
            ScopedLock lock( manager.get() );
            auto database = manager->getDatabase();
            auto bound = database ? dynamic_cast<IParameterizedDatabase *>( database.get() ) : nullptr;
            return bound ? bound->queryBound( sql, values ) : nullptr;
        }
        const String ReferenceObjectType = "ResourceReference";
        const String ReferenceRootUUID = "reference-root";
        const String ReferenceRootPath = "reference";

        s32 getMaxPendingImportJobs( core::IApplicationManager *applicationManager )
        {
            constexpr s32 minPendingImportJobs = 4;
            constexpr s32 maxPendingImportJobs = 64;
            constexpr s32 maxPrimaryThreadPendingImportJobs = 8192;
            constexpr s32 pendingImportJobsPerWorker = 2;

            auto threadPool = applicationManager->getThreadPoolPtr();
            if( !threadPool || threadPool->getNumThreads() == 0 )
            {
                return maxPrimaryThreadPendingImportJobs;
            }

            auto numWorkers = static_cast<s32>( threadPool->getNumThreads() );
            return std::min( maxPendingImportJobs,
                             std::max( minPendingImportJobs, numWorkers * pendingImportJobsPerWorker ) );
        }

        String getFirstFieldValue( SmartPtr<IDatabaseQuery> query,
                                   std::initializer_list<const char *> fieldNames )
        {
            if( !query )
            {
                return String();
            }

            for( auto fieldName : fieldNames )
            {
                auto value = query->getFieldValue( String( fieldName ) );
                if( !StringUtil::isNullOrEmpty( value ) )
                {
                    return StringUtil::trim( value );
                }
            }

            return String();
        }

        Array<String> parseReferenceTokens( String value )
        {
            value = StringUtil::replaceAll( value, ",", ";" );
            value = StringUtil::replaceAll( value, "|", ";" );
            value = StringUtil::trim( value );

            Array<String> tokens;
            if( StringUtil::isNullOrEmpty( value ) )
            {
                return tokens;
            }

            if( value[value.length() - 1] != ';' )
            {
                value += ";";
            }

            StringUtil::parseArray( value, tokens );
            return tokens;
        }

        bool tableExists( SmartPtr<IDatabaseManager> databaseManager, const String &tableName )
        {
            if( !databaseManager || StringUtil::isNullOrEmpty( tableName ) )
            {
                return false;
            }

            auto sql = String( "select name from sqlite_master where type='table' and name='" ) +
                       tableName + String( "'" );
            auto query = databaseManager->executeQuery( sql );
            return query && !query->eof();
        }

        String buildReferencePath( const String &path, const String &name )
        {
            auto referencePath = StringUtil::trim( path );
            if( !StringUtil::isNullOrEmpty( referencePath ) )
            {
                return StringUtil::cleanupPath( referencePath );
            }

            auto referenceName = StringUtil::trim( name );
            if( StringUtil::isNullOrEmpty( referenceName ) )
            {
                return String();
            }

            return StringUtil::cleanupPath( ReferenceRootPath + "/" + referenceName );
        }

        void applyReferenceMetadata( SmartPtr<Properties> properties,
                                     SmartPtr<ResourceReference> reference )
        {
            if( !properties || !reference )
            {
                return;
            }

            properties->setProperty( "type", ReferenceObjectType );
            properties->setProperty( "objectType", ReferenceObjectType );
            properties->setProperty( "referenceKey", reference->getKey() );
            properties->setProperty( "diagnosticName", reference->getDiagnosticName() );
            properties->setProperty( "ownerUUID", reference->getOwnerUUID() );
            properties->setProperty( "resourceUUID", reference->getResourceUUID() );
            properties->setProperty( "ownerPath", reference->getOwnerPath() );
            properties->setProperty( "resourcePath", reference->getResourcePath() );
            properties->setProperty( "resourceType", reference->getResourceType() );
            properties->setProperty( "propertyName", reference->getPropertyName() );
            properties->setProperty( "dependencyMode", reference->getDependencyModeAsString() );
            properties->setProperty( "required", reference->isRequired() );
            properties->setProperty( "priority", reference->getPriority() );
            properties->setProperty( "platforms", reference->getPlatforms() );
            properties->setProperty( "tags", reference->getTags() );
        }

        SmartPtr<ResourceReference> createReferenceFromRow( SmartPtr<IDatabaseQuery> query,
                                                            bool configuredActorFallback )
        {
            if( !query )
            {
                return nullptr;
            }

            auto reference = workphone::make_ptr<ResourceReference>();

            auto ownerUUID = getFirstFieldValue(
                query, { "ownerUUID", "owner_uuid", "owner", "parentUUID", "parent_uuid" } );
            auto ownerPath =
                getFirstFieldValue( query, { "ownerPath", "owner_path", "parentPath", "parent_path" } );

            auto resourceUUID =
                getFirstFieldValue( query, { "resourceUUID", "resource_uuid", "resource", "uuid" } );
            auto resourcePath = getFirstFieldValue( query, { "resourcePath", "resource_path", "path" } );
            auto resourceType =
                getFirstFieldValue( query, { "resourceType", "resource_type", "type", "asset_type" } );
            auto propertyName = getFirstFieldValue(
                query, { "propertyName", "property_name", "property", "field", "name" } );
            auto dependencyMode =
                getFirstFieldValue( query, { "dependencyMode", "dependency_mode", "mode" } );

            if( configuredActorFallback )
            {
                auto id = getFirstFieldValue( query, { "id" } );
                auto parentId = getFirstFieldValue( query, { "parent_id", "parentId" } );
                auto name = getFirstFieldValue( query, { "name", "resource_name" } );

                if( StringUtil::isNullOrEmpty( ownerUUID ) )
                {
                    ownerUUID = StringUtil::isNullOrEmpty( parentId ) ? ReferenceRootUUID : parentId;
                }

                if( StringUtil::isNullOrEmpty( ownerPath ) )
                {
                    ownerPath = ReferenceRootPath;
                }

                if( StringUtil::isNullOrEmpty( resourceUUID ) )
                {
                    resourceUUID = id;
                }

                resourcePath = buildReferencePath( resourcePath, name );

                if( StringUtil::isNullOrEmpty( resourceType ) )
                {
                    resourceType = "Actor";
                }

                if( StringUtil::isNullOrEmpty( propertyName ) )
                {
                    propertyName = name;
                }
            }

            if( StringUtil::isNullOrEmpty( ownerUUID ) )
            {
                ownerUUID = ReferenceRootUUID;
            }

            if( StringUtil::isNullOrEmpty( ownerPath ) )
            {
                ownerPath = ReferenceRootPath;
            }

            if( StringUtil::isNullOrEmpty( resourceUUID ) )
            {
                resourceUUID = getFirstFieldValue( query, { "id" } );
            }

            reference->setOwnerUUID( ownerUUID );
            reference->setOwnerPath( ownerPath );
            reference->setResourceUUID( resourceUUID );
            reference->setResourcePath( resourcePath );
            reference->setResourceType( resourceType );
            reference->setPropertyName( propertyName );

            if( !StringUtil::isNullOrEmpty( dependencyMode ) )
            {
                reference->setDependencyModeFromString( dependencyMode );
            }
            else
            {
                auto required = getFirstFieldValue( query, { "required", "is_required" } );
                if( !StringUtil::isNullOrEmpty( required ) )
                {
                    reference->setRequired( StringUtil::parseBool( required, true ) );
                }
            }

            auto priority = getFirstFieldValue( query, { "priority", "order", "sort_order" } );
            if( !StringUtil::isNullOrEmpty( priority ) )
            {
                reference->setPriority( StringUtil::parseInt( priority, 0 ) );
            }

            auto platforms = getFirstFieldValue( query, { "platforms", "platform" } );
            if( !StringUtil::isNullOrEmpty( platforms ) )
            {
                reference->setPlatforms( parseReferenceTokens( platforms ) );
            }

            auto tags = getFirstFieldValue( query, { "tags", "tag" } );
            if( !StringUtil::isNullOrEmpty( tags ) )
            {
                reference->setTags( parseReferenceTokens( tags ) );
            }

            return reference;
        }

        void appendReferencesFromQuery( SmartPtr<IDatabaseQuery> query,
                                        Array<SmartPtr<ISharedObject>> &objects,
                                        bool configuredActorFallback )
        {
            if( !query )
            {
                return;
            }

            while( !query->eof() )
            {
                auto reference = createReferenceFromRow( query, configuredActorFallback );
                if( reference && reference->isValid() && !reference->isExcluded() )
                {
                    objects.emplace_back( reference );
                }

                query->nextRow();
            }
        }
    }  // namespace

    WP_CLASS_REGISTER_DERIVED( workphone, ResourceDatabase, IResourceDatabase );
    WP_CLASS_REGISTER_DERIVED( workphone, ResourceDatabase::ImportFileJob, Job );

    atomic_s32 ResourceDatabase::m_numJobs = 0;

    const String ResourceDatabase::defaultDatabaseFileName = "asset.db";

    ResourceDatabase::ResourceDatabase()
    {
    }

    ResourceDatabase::~ResourceDatabase()
    {
    }

    void ResourceDatabase::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            setFilePath( defaultDatabaseFileName );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto databaseManager = factoryManager->make_ptr<AssetDatabaseManager>();
            setDatabaseManager( databaseManager );

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            for( auto object : m_objects )
            {
                object->unload( nullptr );
            }

            m_objects.clear();
            m_resourceMap.clear();
            setDatabaseManager( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::build()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            // WP_ASSERT( graphicsSystem );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto databaseManager = getDatabaseManager();
            WP_ASSERT( databaseManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
            WP_ASSERT( assetDatabaseManager );

            auto cachePath = applicationManager->getCachePath();

            auto dbFilePath = getFilePath();
            auto databasePath = cachePath + "/" + dbFilePath;
            databasePath = StringUtil::cleanupPath( databasePath );

            assetDatabaseManager->loadFromFile( databasePath );

            auto sceneManager = applicationManager->getGameManager();
            if( sceneManager )
            {
                auto scene = sceneManager->getCurrentScene();
                WP_ASSERT( scene );

                Array<SmartPtr<ISharedObject>> objects;

                auto actors = scene->getActors();
                for( auto actor : actors )
                {
                    if( !assetDatabaseManager->hasResourceEntry( actor ) )
                    {
                        assetDatabaseManager->addResourceEntry( actor );
                        objects.emplace_back( actor );

                        auto components = actor->getComponents();
                        for( auto component : components )
                        {
                            assetDatabaseManager->addResourceEntry( component );
                            objects.emplace_back( component );
                        }

                        auto children = actor->getAllChildren();
                        for( auto child : children )
                        {
                            assetDatabaseManager->addResourceEntry( child );
                            objects.emplace_back( child );

                            auto childComponents = child->getComponents();
                            for( auto childComponent : childComponents )
                            {
                                assetDatabaseManager->addResourceEntry( childComponent );
                                objects.emplace_back( childComponent );
                            }
                        }
                    }
                }

                m_objects = objects;
            }

            static const String materialExt = ".mat";
            auto materialFiles = fileSystem->getFilesWithExtension( materialExt );
            if( !materialFiles.empty() )
            {
                if( graphicsSystem )
                {
                    auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

                    for( auto &file : materialFiles )
                    {
                        auto path = file.filePath;

                        auto pMaterial = materialManager->createOrRetrieve( path.c_str() );
                        auto material =
                            workphone::static_pointer_cast<render::IMaterial>( pMaterial.first );
                        if( material )
                        {
                            if( !assetDatabaseManager->hasResourceEntry( material ) )
                            {
                                assetDatabaseManager->addResourceEntry( material );
                            }
                        }
                    }
                }
            }

            auto textureTypes = ApplicationUtil::getSupportedTextureFormats();
            for( auto textureType : textureTypes )
            {
                auto textureFiles = fileSystem->getFilesWithExtension( textureType );
                if( !textureFiles.empty() )
                {
                    if( graphicsSystem )
                    {
                        auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

                        for( auto &file : textureFiles )
                        {
                            auto path = file.filePath;

                            auto textureResult = textureManager->createOrRetrieve( path.c_str() );
                            auto texture =
                                workphone::static_pointer_cast<render::ITexture>( textureResult.first );
                            if( texture )
                            {
                                if( !assetDatabaseManager->hasResourceEntry( texture ) )
                                {
                                    assetDatabaseManager->addResourceEntry( texture );
                                }
                            }
                        }
                    }
                }
            }

            auto audioTypes = ApplicationUtil::getSupportedAudioFormats();
            for( auto audioType : audioTypes )
            {
                auto audioFiles = fileSystem->getFilesWithExtension( audioType );
                if( !audioFiles.empty() )
                {
                    auto audioManager = applicationManager->getSoundManager();
                    if( audioManager )
                    {
                        for( auto &file : audioFiles )
                        {
                            auto path = file.filePath;

                            auto audioResult = audioManager->createOrRetrieve( path.c_str() );
                            auto audio = workphone::static_pointer_cast<ISound>( audioResult.first );
                            if( audio )
                            {
                                if( !assetDatabaseManager->hasResourceEntry( audio ) )
                                {
                                    assetDatabaseManager->addResourceEntry( audio );
                                }
                            }
                        }
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::refresh()
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        auto cachePath = applicationManager->getCachePath();

        auto dbFilePath = getFilePath();
        auto databasePath = cachePath + "/" + dbFilePath;
        databasePath = StringUtil::cleanupPath( databasePath );

        assetDatabaseManager->loadFromFile( databasePath );

        auto sceneManager = applicationManager->getGameManager();
        if( sceneManager )
        {
            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );

            auto objects = getSceneObjects();
            for( auto object : objects )
            {
                if( !assetDatabaseManager->hasResourceEntry( object ) )
                {
                    assetDatabaseManager->addResourceEntry( object );
                }
            }

            m_objects = objects;
        }
    }

    void ResourceDatabase::optimise()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        assetDatabaseManager->optimise();
    }

    void ResourceDatabase::clean()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        auto cleanMaterialsQueryStr =
            String( "DELETE FROM resources WHERE type = 'Material' AND path = '';" );
        assetDatabaseManager->executeQuery( cleanMaterialsQueryStr );

        auto cleanTexturesQueryStr =
            String( "DELETE FROM resources WHERE type = 'Texture' AND path = '';" );
        assetDatabaseManager->executeQuery( cleanTexturesQueryStr );

        auto cleanResourceScriptPath = "clean_resources_table.sql";
        assetDatabaseManager->runScript( cleanResourceScriptPath );

        auto resourcesQueryStr = String( "SELECT path FROM resources" );
        auto query = assetDatabaseManager->executeQuery( resourcesQueryStr );
        if( query )
        {
            while( !query->eof() )
            {
                auto path = query->getFieldValue( "path" );
                auto fileExists = fileSystem->isExistingFile( path );
                if( !fileExists )
                {
                    assetDatabaseManager->removeResourceEntryFromPath( path );
                }

                query->nextRow();
            }
        }
    }

    void ResourceDatabase::deleteCache()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto fileSystem = applicationManager->getFileSystem();

        auto cachePath = applicationManager->getCachePath();
        auto folderListing = fileSystem->getFolderListing( cachePath );

        auto &files = folderListing->getFiles();
        for( auto &file : files )
        {
            auto ext = Path::getFileExtension( file );
            if( ext == ".fbmeshbin" || ext == ".pxtrianglemesh" )
            {
                fileSystem->deleteFile( file );
            }
        }
    }

    auto ResourceDatabase::hasResource( SmartPtr<IResource> resource ) -> bool
    {
        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        return assetDatabaseManager->hasResourceEntry( resource );
    }

    void ResourceDatabase::addResource( SmartPtr<IResource> resource )
    {
        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        assetDatabaseManager->addResourceEntry( resource );
    }

    void ResourceDatabase::removeResource( SmartPtr<IResource> resource )
    {
        if( resource )
        {
            auto databaseManager = getDatabaseManager();
            WP_ASSERT( databaseManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
            WP_ASSERT( assetDatabaseManager );

            assetDatabaseManager->removeResourceEntry( resource );
        }
    }

    void ResourceDatabase::removeResourceFromPath( const String &path )
    {
        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        assetDatabaseManager->removeResourceEntryFromPath( path );
    }

    auto ResourceDatabase::findResource( u32 type, const String &path ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        const auto MATERIAL_TYPE = render::IMaterial::typeInfo();
        const auto TEXTURE_TYPE = render::ITexture::typeInfo();

        if( type == MATERIAL_TYPE )
        {
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

            return materialManager->getByName( path );
        }
        else if( type == TEXTURE_TYPE )
        {
            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

            return textureManager->getByName( path );
        }

        return nullptr;
    }

    auto ResourceDatabase::cloneResource( u32 type, SmartPtr<IResource> resource, const String &path )
        -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto taskManager = applicationManager->getTaskManager();
        WP_ASSERT( taskManager );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        WP_ASSERT( graphicsSystem );

        auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
        WP_ASSERT( materialManager );

        auto soundManager = applicationManager->getSoundManager();

        const auto MATERIAL_TYPE = render::IMaterial::typeInfo();
        const auto TEXTURE_TYPE = render::ITexture::typeInfo();
        const auto MESH_TYPE = IMeshResource::typeInfo();
        const auto SOUND_TYPE = ISound::typeInfo();

        if( type == MATERIAL_TYPE )
        {
            if( materialManager )
            {
                auto clonedMaterial = materialManager->cloneMaterial( resource, path );
                WP_ASSERT( clonedMaterial );

                return clonedMaterial;
            }
        }
        else if( type == TEXTURE_TYPE )
        {
            auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
            WP_ASSERT( textureManager );

            if( textureManager )
            {
                auto clonedTexture = textureManager->cloneTexture( resource, path );
                WP_ASSERT( clonedTexture );

                return clonedTexture;
            }
        }
        else if( type == MESH_TYPE )
        {
            auto meshManager = applicationManager->getMeshManager();
            WP_ASSERT( meshManager );

            if( meshManager )
            {
                auto clonedMesh = meshManager->cloneResource( resource, path );
                WP_ASSERT( clonedMesh );

                return clonedMesh;
            }
        }
        else if( type == SOUND_TYPE )
        {
            if( soundManager )
            {
                auto clonedSound = soundManager->cloneResource( resource, path );
                WP_ASSERT( clonedSound );

                return clonedSound;
            }
        }

        return nullptr;
    }

    void ResourceDatabase::importFolder( SmartPtr<IFolderExplorer> folderListing, bool overwrite )
    {
        try
        {
            if( folderListing )
            {
                WP_LOG( "Importing: " + folderListing->getFolderName() );

                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                const auto &files = folderListing->getFiles();
                for( const auto &file : files )
                {
                    auto relativePath = Path::getRelativePath( projectPath, file );
                    importFile( relativePath, overwrite );
                }

                auto subFolders = folderListing->getSubFolders();
                for( const auto &subFolder : subFolders )
                {
                    try
                    {
                        importFolder( subFolder, overwrite );
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::importFolder( const String &path, bool overwrite )
    {
        try
        {
            if( !StringUtil::isNullOrEmpty( path ) )
            {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto folderListing = fileSystem->getFolderListing( path );
                if( folderListing )
                {
                    importFolder( folderListing, overwrite );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::importFile( const String &filePath, bool overwrite )
    {
        if( !isLoaded() )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        if( applicationManager->getQuit() )
        {
            return;
        }

        if( applicationManager->isRunning() == false )
        {
            return;
        }

        const auto maxPendingImportJobs = getMaxPendingImportJobs( applicationManager );
        while( ResourceDatabase::m_numJobs >= maxPendingImportJobs )
        {
            if( applicationManager->getQuit() || applicationManager->isRunning() == false )
            {
                return;
            }

            Thread::sleep( 0.1 );
        }

        auto jobQueue = applicationManager->getJobQueuePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        auto job = factoryManager->make_ptr<ImportFileJob>();
        job->setFilePath( filePath );
        job->setOverwrite( overwrite );

        if( applicationManager->hasTasks() )
        {
            if( jobQueue )
            {
                jobQueue->addJob( job );
            }
            else
            {
                job->execute();
            }
        }
        else
        {
            job->execute();
        }
    }

    void ResourceDatabase::importCache()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        clean();

        auto projectPath = applicationManager->getProjectPath();
        if( StringUtil::isNullOrEmpty( projectPath ) )
        {
            projectPath = Path::getWorkingDirectory();
        }

        auto cachePath = applicationManager->getCachePath();

        std::cout << "Importing: " << cachePath << std::endl;

        importFolder( cachePath );

        std::cout << "Finished Importing: " << cachePath << std::endl;
    }

    void ResourceDatabase::importAssets()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto databaseManager = getDatabaseManager();
            WP_ASSERT( databaseManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
            WP_ASSERT( assetDatabaseManager );

            clean();

            assetDatabaseManager->create();

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto cachePath = applicationManager->getCachePath();
            importFolder( cachePath );

            auto projectAssetsPath = projectPath + "/Assets";
            auto folderListing = fileSystem->getFolderListing( projectAssetsPath );
            if( folderListing )
            {
                importFolder( folderListing );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::reimportAssets()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            clean();

            auto cachePath = applicationManager->getCachePath();
            fileSystem->deleteFilesFromPath( cachePath );

            auto projectPath = applicationManager->getProjectPath();
            auto assetsPath = projectPath + "/Assets";
            auto folderListing = fileSystem->getFolderListing( assetsPath );
            if( folderListing )
            {
                importFolder( folderListing );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::calculateDependencies()
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto resources = getResources();
        for( auto resource : resources )
        {
            auto dependencies = resource->getDependencies();
        }
    }

    auto ResourceDatabase::getResourceData() const -> Array<SmartPtr<IBuildDirector>>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto typeManager = TypeManager::instance();
            WP_ASSERT( typeManager );

            Array<SmartPtr<IBuildDirector>> resources;
            resources.reserve( 1024 );

            auto referenceObjects = getReferenceObjects();
            for( auto referenceObject : referenceObjects )
            {
                auto reference = workphone::dynamic_pointer_cast<ResourceReference>( referenceObject );
                if( !reference || !reference->isValid() || reference->isExcluded() )
                {
                    continue;
                }

                auto resourceDirector = factoryManager->make_object<IBuildDirector>();
                if( !resourceDirector )
                {
                    continue;
                }

                resourceDirector->load( nullptr );
                resourceDirector->setName( reference->getDiagnosticName() );

                if( auto handle = resourceDirector->getHandle() )
                {
                    handle->setUUID( reference->getResourceUUID() );
                }

                auto directorProperties = resourceDirector->getProperties();
                applyReferenceMetadata( directorProperties, reference );
                resourceDirector->setProperties( directorProperties );

                resources.push_back( resourceDirector );
            }

            auto objects = getSceneObjects();
            for( auto object : objects )
            {
                hash_type id = 0;
                auto uuid = String();
                auto path = String();
                auto type = String();

                if( auto handle = object->getHandle() )
                {
                    id = handle->getId();
                    uuid = handle->getUUIDAsString();
                }

                if( object->isDerived<scene::IGameActor>() )
                {
                    type = "Actor";
                }
                else if( object->isDerived<scene::IComponent>() )
                {
                    type = "Component";

                    auto component = workphone::static_pointer_cast<scene::IComponent>( object );
                    if( auto actor = component->getActor() )
                    {
                        path = actor->getName() + "_" + typeManager->getName( component->getTypeInfo() );
                    }
                }

                auto resourceDirector = factoryManager->make_object<IBuildDirector>();
                resourceDirector->load( nullptr );
                resourceDirector->setName( path );

                if( auto handle = resourceDirector->getHandle() )
                {
                    if( StringUtil::isNullOrEmpty( uuid ) )
                    {
                        uuid = StringUtil::getUUID();
                    }

                    handle->setUUID( uuid );
                }

                auto directorProperties = resourceDirector->getProperties();
                directorProperties->setProperty( "type", type );

                resources.push_back( resourceDirector );
            }

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto databaseManager = getDatabaseManager();
            WP_ASSERT( databaseManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
            WP_ASSERT( assetDatabaseManager );

            auto cachePath = applicationManager->getCachePath();

            auto dbFilePath = getFilePath();
            auto databasePath = Path::lexically_normal( cachePath, dbFilePath );
            assetDatabaseManager->loadFromFile( databasePath );

            auto sql = String( "select * from 'resources'" );
            auto query = assetDatabaseManager->executeQuery( sql );
            if( query )
            {
                while( !query->eof() )
                {
                    auto id = query->getFieldValue( "id" );
                    auto uuid = query->getFieldValue( "uuid" );
                    auto path = query->getFieldValue( "path" );
                    auto type = query->getFieldValue( "type" );

                    if( StringUtil::isNullOrEmpty( path ) )
                    {
                        query->nextRow();
                        continue;
                    }

                    auto resourceDirector = factoryManager->make_object<IBuildDirector>();
                    resourceDirector->load( nullptr );

                    resourceDirector->setName( path );

                    if( auto handle = resourceDirector->getHandle() )
                    {
                        handle->setUUID( uuid );
                    }

                    auto directorProperties = resourceDirector->getProperties();

                    auto numFields = query->getNumFields();

                    for( size_t i = 0; i < numFields; ++i )
                    {
                        auto fieldName = query->getFieldName( static_cast<u32>( i ) );
                        auto fieldValue = query->getFieldValue( static_cast<u32>( i ) );

                        directorProperties->setProperty( fieldName, fieldValue );
                    }

                    resourceDirector->setProperties( directorProperties );
                    resources.push_back( resourceDirector );

                    query->nextRow();
                }
            }

            return resources;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto ResourceDatabase::getResources() const -> Array<SmartPtr<IResource>>
    {
        Array<SmartPtr<IResource>> resources;
        resources.reserve( 1024 );

        auto objects = getSceneObjects();
        for( auto object : objects )
        {
            resources.emplace_back( object );
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto projectPath = applicationManager->getProjectPath();
        if( StringUtil::isNullOrEmpty( projectPath ) )
        {
            projectPath = Path::getWorkingDirectory();
        }

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        auto cachePath = applicationManager->getCachePath();

        auto dbFilePath = getFilePath();
        auto databasePath = Path::lexically_normal( cachePath, dbFilePath );
        assetDatabaseManager->loadFromFile( databasePath );

        auto sql = String( "select * from 'resources'" );
        auto query = assetDatabaseManager->executeQuery( sql );
        if( query )
        {
            while( !query->eof() )
            {
                auto id = query->getFieldValue( "id" );
                auto uuid = query->getFieldValue( "uuid" );
                auto path = query->getFieldValue( "path" );
                auto type = query->getFieldValue( "type" );

                if( StringUtil::isNullOrEmpty( path ) )
                {
                    query->nextRow();
                    continue;
                }

                if( type == "Material" )
                {
                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

                    auto materialResult = materialManager->createOrRetrieve( uuid, path, type );
                    if( materialResult.first )
                    {
                        resources.push_back( materialResult.first );
                    }
                }
                else if( type == "Texture" )
                {
                    auto graphicsSystem = applicationManager->getGraphicsSystem();
                    auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

                    auto textureResult = textureManager->createOrRetrieve( uuid, path, type );
                    if( textureResult.first )
                    {
                        resources.push_back( textureResult.first );
                    }
                }

                query->nextRow();
            }
        }

        return resources;
    }

    void ResourceDatabase::createActor( SmartPtr<scene::IGameActor> parent,
                                        SmartPtr<Properties> properties )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto actor = sceneManager->createActor();
        WP_ASSERT( actor );

        parent->addChild( actor );

        auto name = properties->getProperty( "name" );
        actor->setName( name );

        auto children = properties->getChildren();
        for( auto child : children )
        {
            createActor( actor, child );
        }
    }

    auto ResourceDatabase::loadResource( const UUID &id ) -> SmartPtr<IResource>
    {
        // Source metadata must also be usable by headless tools and packaged applications.
        if( auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( getDatabaseManager() ) )
        {
            AssetDatabaseManager::EntrySnapshot entry;
            if( catalog->tryGetEntry( StringUtil::toString( id ), entry ) &&
                entry.kind == AssetDatabaseManager::EntryKind::File )
            {
                if( entry.type != "script" )
                    return loadResourceById( id );
                auto script = make_ptr<ScriptAsset>();
                script->getHandle()->setUUID( entry.uuid );
                script->loadFromFile( catalog->getProjectRoot() + "/" + entry.path );
                return catalog->isEntryCurrent( entry ) ? script : nullptr;
            }
        }
        auto applicationManager = core::IApplicationManager::instancePtr();
        if( !applicationManager )
        {
            return nullptr;
        }

        auto sceneManager = applicationManager->getGameManagerPtr();
        auto scene = sceneManager ? sceneManager->getCurrentScenePtr() : nullptr;

        auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
        if( !graphicsSystem )
        {
            return nullptr;
        }

        auto prefabManager = applicationManager->getPrefabManager();

        auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
        auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
        if( !materialManager || !textureManager )
        {
            return nullptr;
        }

        auto properties = loadAssetProperties( id );

        auto uuid = properties->getProperty( "uuid" );
        if( StringUtil::isNullOrEmpty( uuid ) )
        {
            uuid = StringUtil::getUUID();
        }

        auto type = properties->getProperty( "asset_type" );
        if( type == "material" )
        {
        }
        else if( type == "texture" )
        {
        }
        else if( type == "prefab" )
        {
            auto result = prefabManager->createOrRetrieve( uuid, "", type );
            return result.first;
        }
        else if( type == "scene" )
        {
        }
        else if( type == "shader" )
        {
        }
        else if( type == "mesh" )
        {
        }
        else if( type == "model" )
        {
            SmartPtr<scene::IGamePrefab> prefab = prefabManager->create( uuid );
            prefab->setData( properties );
            return prefab;
        }
        else if( type == "animation" )
        {
        }
        else if( type == "audio" )
        {
        }
        else if( type == "font" )
        {
        }
        else if( type == "script" )
        {
        }
        else if( type == "script_class" )
        {
        }
        else if( type == "script_function" )
        {
        }
        else if( type == "script_module" )
        {
        }
        else if( type == "script_property" )
        {
        }
        else if( type == "script_variable" )
        {
        }
        else if( type == "shader" )
        {
        }
        else if( type == "shader_program" )
        {
        }
        else if( type == "shader_variable" )
        {
        }
        else if( type == "shader_variable_block" )
        {
        }
        else if( type == "shader_variable_sampler" )
        {
        }
        else if( type == "shader_variable_texture" )
        {
        }
        else if( type == "shader_variable_uniform" )
        {
        }
        else if( type == "vehicle" )
        {
        }
        else
        {
        }

        return nullptr;
    }

    auto ResourceDatabase::loadResource( const String &path ) -> SmartPtr<IResource>
    {
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
        if( !catalog || path.empty() )
            return nullptr;
        auto director = dynamic_pointer_cast<scene::ResourceDirector>(
            catalog->getResourceEntryFromPath( path ) );
        if( !director )
            return nullptr; // Loading never imports or invents persistent identity.
        const auto uuid = director->getResourceUUID();
        AssetDatabaseManager::EntrySnapshot snapshot;
        if( !catalog->tryGetEntry( uuid, snapshot ) ||
            snapshot.kind != AssetDatabaseManager::EntryKind::File )
            return nullptr;
        if( snapshot.type == "script" )
            return loadResource( StringUtil::parseUUID( uuid ) );
        auto query = queryResourceValues( catalog, "SELECT * FROM resources WHERE uuid=?", {uuid} );
        if( !query || query->eof() )
            return nullptr;
        auto resource = createOrRetrieveResource( query, true );
        return catalog->isEntryCurrent( snapshot ) ? resource : nullptr;
    }
    auto ResourceDatabase::loadDirector( const String &path ) -> SmartPtr<IBuildDirector>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto dataStr = fileSystem->readAllText( path );
        auto data = workphone::make_ptr<Properties>();
        DataUtil::parse<Properties>( dataStr, data.get() );

        auto director = workphone::make_ptr<Director>();
        director->setFilePath( path );
        director->setProperties( data );

        return nullptr;
    }

    auto ResourceDatabase::loadDirectorFromResourcePath( const String &filePath )
        -> SmartPtr<IBuildDirector>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto projectPath = applicationManager->getProjectPath();
        auto settingsCachePath = applicationManager->getSettingsPath();

        auto filePathHash = StringUtil::getUUID( filePath );

        auto fileDataPath = settingsCachePath + StringUtil::toString( filePathHash ) + ".resourcedata";
        fileDataPath = Path::getRelativePath( projectPath, fileDataPath );

        auto dataStr = fileSystem->readAllText( fileDataPath );
        auto data = workphone::make_ptr<Properties>();
        DataUtil::parse<Properties>( dataStr, data.get() );

        auto director = workphone::make_ptr<Director>();
        director->setFilePath( fileDataPath );
        director->setProperties( data );

        return director;
    }

    auto ResourceDatabase::loadDirectorFromResourcePath( const String &filePath, u32 type )
        -> SmartPtr<IBuildDirector>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto projectPath = applicationManager->getProjectPath();
        auto settingsCachePath = applicationManager->getSettingsPath();
        auto filePathHash = StringUtil::getUUID( filePath );

        auto resourcedataExt = String( ".resourcedata" );
        auto fileDataPath = settingsCachePath + StringUtil::toString( filePathHash ) + resourcedataExt;
        auto relativeFileDataPath = Path::getRelativePath( projectPath, fileDataPath );

        for( auto &object : m_objects )
        {
            auto resource = workphone::dynamic_pointer_cast<IResource>( object );
            if( resource->getFilePath() == fileDataPath )
            {
                return resource;
            }
        }

        auto data = workphone::make_ptr<Properties>();

        if( fileSystem->isExistingFile( relativeFileDataPath ) )
        {
            auto dataStr = fileSystem->readAllText( relativeFileDataPath );
            if( !StringUtil::isNullOrEmpty( dataStr ) )
            {
                DataUtil::parse<Properties>( dataStr, data.get() );
            }
        }
        else
        {
            auto dataStr = fileSystem->readAllText( fileDataPath );
            if( !StringUtil::isNullOrEmpty( dataStr ) )
            {
                DataUtil::parse<Properties>( dataStr, data.get() );
            }
        }

        if( scene::MeshResourceDirector::typeInfo() == type )
        {
            auto director = workphone::make_ptr<scene::MeshResourceDirector>();
            director->setResourcePath( filePath );
            director->setFilePath( fileDataPath );
            director->setProperties( data );
            //m_objects.push_back( director );
            return director;
        }
        else if( scene::SoundResourceDirector::typeInfo() == type )
        {
            auto director = workphone::make_ptr<scene::SoundResourceDirector>();
            director->setResourcePath( filePath );
            director->setFilePath( fileDataPath );
            director->setProperties( data );
            //m_objects.push_back( director );
            return director;
        }
        else if( scene::TextureResourceDirector::typeInfo() == type )
        {
            auto director = workphone::make_ptr<scene::TextureResourceDirector>();
            director->setResourcePath( filePath );
            director->setFilePath( fileDataPath );
            director->setProperties( data );
            //m_objects.push_back( director );
            return director;
        }

        auto director = workphone::make_ptr<Director>();
        director->setFilePath( fileDataPath );
        director->setProperties( data );

        return director;
    }

    SmartPtr<IBuildDirector> ResourceDatabase::loadDirector( SmartPtr<IResource> resource )
    {
        if( resource )
        {
            auto filePath = resource->getFilePath();
            u32 directorType = 0;

            if( resource->isDerived<render::ITexture>() )
            {
                directorType = scene::TextureResourceDirector::typeInfo();
            }

            return loadDirectorFromResourcePath( filePath, directorType );
        }

        return nullptr;
    }

    auto ResourceDatabase::loadResourceById( const UUID &uuid ) -> SmartPtr<IResource>
    {
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
        AssetDatabaseManager::EntrySnapshot snapshot;
        if( !catalog || !catalog->tryGetEntry( StringUtil::toString( uuid ), snapshot ) ||
            snapshot.kind != AssetDatabaseManager::EntryKind::File )
            return nullptr;
        if( snapshot.type == "script" )
            return loadResource( uuid );
        auto query = queryResourceValues( catalog, "SELECT * FROM resources WHERE uuid=?",
                                          {snapshot.uuid} );
        auto resource = query && !query->eof() ? createOrRetrieveResource( query, true ) : nullptr;
        return catalog->isEntryCurrent( snapshot ) ? resource : nullptr;
    }
    void ResourceDatabase::unloadUnusedResources()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        for( auto object : m_objects )
        {
            if( object )
            {
                if( object->getReferences() == 1 )
                {
                    object->unload( nullptr );
                }
            }
        }

        if( auto p = getInstancesPtr() )
        {
            auto &instances = *p;

            for( auto instance : instances )
            {
                if( instance )
                {
                    if( instance->getReferences() == 1 )
                    {
                        instance->unload( nullptr );
                    }
                }
            }
        }
    }

    void ResourceDatabase::removeUnusedResources()
    {
        RecursiveMutex::ScopedLock lock( m_mutex );

        auto objectsToRemove = Array<SmartPtr<ISharedObject>>();
        objectsToRemove.reserve( m_objects.size() );

        for( auto &object : m_objects )
        {
            if( object->getReferences() == 1 )
            {
                object->unload( nullptr );
                objectsToRemove.push_back( object );
            }
        }

        for( auto &object : objectsToRemove )
        {
            m_objects.erase( std::remove( m_objects.begin(), m_objects.end(), object ),
                             m_objects.end() );
        }

        if( auto p = getInstancesPtr() )
        {
            auto &instances = *p;

            auto instancesToRemove = Array<SmartPtr<ISharedObject>>();
            instancesToRemove.reserve( instances.size() );

            for( auto &instance : instances )
            {
                if( instance->getReferences() == 1 )
                {
                    instance->unload( nullptr );
                    instancesToRemove.push_back( instance );
                }
            }

            for( auto &object : instancesToRemove )
            {
                instances.erase( std::remove( instances.begin(), instances.end(), object ),
                                 instances.end() );
            }
        }
    }

    auto ResourceDatabase::createOrRetrieveResource( SmartPtr<IDatabaseQuery> query, bool bLoadResource )
        -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        auto soundManager = applicationManager->getSoundManager();

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        auto assetDatabaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
        WP_ASSERT( assetDatabaseManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

        auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

        auto id = query->getFieldValue( "id" );
        auto uuid = query->getFieldValue( "uuid" );
        auto path = query->getFieldValue( "path" );
        auto type = query->getFieldValue( "type" );

        if( type == "Material" )
        {
            if( !materialManager )
                return nullptr;
            auto materialResult = materialManager->createOrRetrieve( uuid, path, type );

            auto resource = materialResult.first;
            if( bLoadResource )
            {
                graphicsSystem->loadObject( resource );
            }

            return materialResult.first;
        }
        if( StringUtil::contains( type, "MeshResource" ) )
        {
            auto meshManager = applicationManager->getMeshManager();
            if( !meshManager || ( bLoadResource && !graphicsSystem ) )
                return nullptr;
            auto meshResult = meshManager->createOrRetrieve( uuid, path, type );

            auto resource = meshResult.first;
            if( bLoadResource )
            {
                graphicsSystem->loadObject( resource );
            }

            return meshResult.first;
        }
        if( type == "Texture" )
        {
            if( !textureManager )
                return nullptr;
            auto textureResult = textureManager->createOrRetrieve( uuid, path, type );

            auto resource = textureResult.first;
            if( bLoadResource )
            {
                graphicsSystem->loadObject( resource );
            }

            return textureResult.first;
        }
        if( StringUtil::contains( type, "Sound" ) )
        {
            if( !soundManager )
                return nullptr;
            auto soundResult = soundManager->createOrRetrieve( uuid, path, type );

            auto resource = soundResult.first;
            if( bLoadResource )
            {
                soundManager->loadObject( resource );
            }

            return soundResult.first;
        }
        else
        {
            auto object = factoryManager->createObjectFromType<IResource>( type );
            if( object )
            {
                auto handle = object->getHandle();
                if( handle )
                {
                    handle->setUUID( uuid );
                }

                object->setFilePath( path );
                object->loadFromFile( path );

                return object;
            }
        }

        return nullptr;
    }

    auto ResourceDatabase::getDatabaseManager() const -> SmartPtr<IDatabaseManager>
    {
        return m_databaseManager;
    }

    void ResourceDatabase::setDatabaseManager( SmartPtr<IDatabaseManager> databaseManager )
    {
        m_databaseManager = databaseManager;
    }

    auto ResourceDatabase::createOrRetrieveFromDirector( hash_type type, const String &path,
                                                         SmartPtr<IBuildDirector> director )
        -> Pair<SmartPtr<IResource>, bool>
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            static const auto fontType = render::IFont::typeInfo();
            static const auto materialType = render::IMaterial::typeInfo();

            Pair<SmartPtr<IResource>, bool> result;

            if( auto graphicsSystem = applicationManager->getGraphicsSystem() )
            {
                auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
                auto meshManager = applicationManager->getMeshManager();
                auto fontManager = graphicsSystem->getFontManager();

                static const auto materialType = render::IMaterial::typeInfo();
                static const auto meshResourceType = IMeshResource::typeInfo();

                String uuid;

                auto resourceDirector =
                    workphone::dynamic_pointer_cast<scene::ResourceDirector>( director );
                if( resourceDirector )
                {
                    uuid = resourceDirector->getResourceUUID();
                }

                if( StringUtil::isNullOrEmpty( uuid ) )
                {
                    uuid = StringUtil::getUUID();
                }

                auto typeManager = TypeManager::instance();
                WP_ASSERT( typeManager );

                if( typeManager->isDerived( static_cast<u32>( type ), materialType ) )
                {
                    if( materialManager )
                    {
                        result = materialManager->createOrRetrieve( uuid, path, "" );
                    }
                }

                if( typeManager->isDerived( static_cast<u32>( type ), meshResourceType ) )
                {
                    if( meshManager )
                    {
                        result = meshManager->createOrRetrieve( uuid, path, "" );
                    }
                }

                if( typeManager->isDerived( static_cast<u32>( type ), fontType ) )
                {
                    result = fontManager->createOrRetrieve( uuid, path, "" );
                }

                result.second = false;  // false = resource already exists
                return result;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    auto ResourceDatabase::createOrRetrieve( hash_type type, const String &path )
        -> Pair<SmartPtr<IResource>, bool>
    {
        try
        {
            if( StringUtil::isNullOrEmpty( path ) )
            {
                return {};
            }

            WP_ASSERT( type != 0 );
            WP_ASSERT( !StringUtil::isNullOrEmpty( path ) );

            // Check if the resource already exists in the database.
            auto it = m_resourceMap.find( type );
            if( it != m_resourceMap.end() )
            {
                auto resIt = it->second.find( path );
                if( resIt != it->second.end() )
                {
                    if( auto resource = resIt->second.lock() )
                    {
                        // The resource already exists in memory.
                        return Pair<SmartPtr<IResource>, bool>( resource, false );
                    }

                    it->second.erase( resIt );
                }
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );

            if( !assetDatabaseManager )
            {
                WP_LOG_ERROR( "AssetDatabaseManager is null!" );
                return {};
            }

            auto director = assetDatabaseManager->getResourceEntryFromPath( path );

            auto result = createOrRetrieveFromDirector( type, path, director );

            if( result.first )
            {
                m_resourceMap[type][path] = result.first;
            }

            return result;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return Pair<SmartPtr<IResource>, bool>( nullptr, false );
    }

    auto ResourceDatabase::createOrRetrieve( const String &path ) -> Pair<SmartPtr<IResource>, bool>
    {
        try
        {
            if( StringUtil::isNullOrEmpty( path ) )
            {
                return {};
            }

            WP_ASSERT( !StringUtil::isNullOrEmpty( path ) );

            // Check if the resource already exists in the database.
            for( auto &[hash, map] : m_resourceMap )
            {
                auto resIt = map.find( path );
                if( resIt != map.end() )
                {
                    if( auto resource = resIt->second.lock() )
                    {
                        // The resource already exists in memory.
                        return Pair<SmartPtr<IResource>, bool>( resource, false );
                    }

                    map.erase( resIt );
                    break;
                }
            }

            static const auto materialType = render::IMaterial::typeInfo();
            static const auto meshResourceType = IMeshResource::typeInfo();
            static const auto textureResourceType = render::ITexture::typeInfo();

            u32 type = 0;

            if( ApplicationUtil::isSupportedMesh( path ) )
            {
                type = meshResourceType;
            }
            else if( Path::getFileExtension( path ) == ".mat" )
            {
                type = materialType;
            }
            else if( ApplicationUtil::isSupportedTexture( path ) )
            {
                type = textureResourceType;
            }

            if( type == 0 )
            {
                return {};
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
            auto director = assetDatabaseManager->getResourceEntryFromPath( path );

            auto result = createOrRetrieveFromDirector( type, path, director );

            if( result.first )
            {
                m_resourceMap[type][path] = result.first;
            }

            return result;
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return Pair<SmartPtr<IResource>, bool>( nullptr, false );
    }

    auto ResourceDatabase::getObject( const UUID &id ) -> SmartPtr<ISharedObject>
    {
        if( !id.is_nil() )
        {
            auto objects = getSceneObjects();
            for( auto object : objects )
            {
                if( object )
                {
                    auto handle = object->getHandle();
                    auto objectUuid = handle->getUUID();

                    if( id == objectUuid )
                    {
                        return object;
                    }
                }
            }

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto factoryManager = applicationManager->getFactoryManager();

            auto databaseManager = getDatabaseManager();
            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );

            auto graphicsSystem = applicationManager->getGraphicsSystem();

            auto materialManager = SmartPtr<render::IMaterialManager>();
            auto textureManager = SmartPtr<render::ITextureManager>();

            if( graphicsSystem )
            {
                materialManager = graphicsSystem->getMaterialManager();
                textureManager = graphicsSystem->getTextureManager();
            }

            auto soundManager = applicationManager->getSoundManager();

            auto cachePath = applicationManager->getCachePath();

            auto dbFilePath = getFilePath();
            assetDatabaseManager->loadFromFile( Path::lexically_normal( cachePath, dbFilePath ) );

            auto query = queryResourceValues( assetDatabaseManager,
                "SELECT * FROM resources WHERE uuid=?", {StringUtil::toString( id )} );
            if( query )
            {
                while( !query->eof() )
                {
                    auto dbid = query->getFieldValue( "id" );
                    auto uuid = query->getFieldValue( "uuid" );
                    auto path = query->getFieldValue( "path" );
                    auto type = query->getFieldValue( "type" );

                    if( type == "Material" )
                    {
                        if( !materialManager )
                            return nullptr;
                        auto materialResult = materialManager->createOrRetrieve( uuid, path, type );
                        return materialResult.first;
                    }
                    else if( type == "Texture" )
                    {
                        if( !textureManager )
                            return nullptr;
                        auto textureResult = textureManager->createOrRetrieve( uuid, path, type );
                        return textureResult.first;
                    }
                    else if( StringUtil::contains( type, "Sound" ) )
                    {
                        if( soundManager )
                        {
                            auto soundResult = soundManager->createOrRetrieve( uuid, path, type );
                            return soundResult.first;
                        }
                    }
                    else
                    {
                        auto resource = factoryManager->createObjectFromType<IResource>( type );
                        if( resource )
                        {
                            auto handle = resource->getHandle();
                            if( handle )
                            {
                                handle->setUUID( uuid );
                            }

                            resource->loadFromFile( path );

                            return resource;
                        }
                    }

                    query->nextRow();
                }
            }
        }

        return nullptr;
    }

    auto ResourceDatabase::loadAssetNode( SmartPtr<IDatabaseManager> db, SmartPtr<Properties> parent,
                                          const UUID &id ) -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto properties = workphone::make_ptr<Properties>();
        parent->addChild( properties );

        auto sAssetId = StringUtil::toString( id );
        String sql = "select * from configured_actors where id = '" + sAssetId + "'";

        if( auto query = db->executeQuery( sql ) )
        {
            while( !query->eof() )
            {
                auto numFields = query->getNumFields();
                for( size_t i = 0; i < numFields; ++i )
                {
                    auto name = query->getFieldName( static_cast<u32>( i ) );
                    auto value = query->getFieldValue( static_cast<u32>( i ) );

                    properties->setProperty( name, value );
                }

                query->nextRow();
            }
        }

        String parentSql = "select * from configured_actors where parent_id = '" + sAssetId + "'";
        if( auto query = db->executeQuery( parentSql ) )
        {
            while( !query->eof() )
            {
                auto id = query->getFieldValue( "id" );
                auto iId = StringUtil::parseUUID( id );
                loadAssetNode( db, properties, iId );
                query->nextRow();
            }
        }

        return properties;
    }

    auto ResourceDatabase::loadAssetProperties( const UUID &id ) -> SmartPtr<Properties>
    {
        auto applicationManager = core::IApplicationManager::instance();

        auto databaseManager = workphone::make_ptr<AssetDatabaseManager>();

        auto mediaPath = applicationManager->getMediaPath();

        auto databaseFilePath = String( "/AssetDatabase.db" );
        databaseManager->loadFromFile( mediaPath + databaseFilePath );

        auto properties = workphone::make_ptr<Properties>();
        properties->setName( "asset" );

        auto propertyName = String( "asset_type" );
        auto propertyValue = String( "model" );
        properties->setProperty( propertyName, propertyValue );

        loadAssetNode( databaseManager, properties, id );

        auto sAssetId = StringUtil::toString( id );
        String sql = "select * from attribs where actor_id = '" + sAssetId + "'";

        if( auto query = databaseManager->executeQuery( sql ) )
        {
            while( !query->eof() )
            {
                auto name = query->getFieldValue( "title" );
                auto value = query->getFieldValue( "value" );

                properties->setProperty( name, value );

                query->nextRow();
            }
        }

        return properties;
    }

    void ResourceDatabase::getSceneObjects( SmartPtr<scene::IGameActor> actor,
                                            Array<SmartPtr<ISharedObject>> &objects ) const
    {
        objects.emplace_back( actor );

        if( auto transform = actor->getTransform() )
        {
            objects.emplace_back( transform );
        }

        auto components = actor->getComponents();
        objects.insert( objects.end(), components.begin(), components.end() );

        auto children = actor->getChildren();
        for( auto &child : children )
        {
            if( child )
            {
                getSceneObjects( child, objects );
            }
        }
    }

    auto ResourceDatabase::getReferenceObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 4096 );

        auto databaseManager = getDatabaseManager();
        WP_ASSERT( databaseManager );

        // Domain reference reads never rebind the project's live catalog.
        auto assetDatabaseManager = workphone::make_ptr<DatabaseManager>();
        auto databasePath = applicationManager->getMediaPath() + "/AssetDatabase.db";
        auto fileSystem = applicationManager->getFileSystem();
        if( !fileSystem || !fileSystem->isExistingFile( databasePath ) )
            return objects;
        assetDatabaseManager->loadFromFile( databasePath );

        Array<String> referenceTables;
        referenceTables.push_back( "resource_references" );
        referenceTables.push_back( "resourceReferences" );
        referenceTables.push_back( "references" );

        for( const auto &tableName : referenceTables )
        {
            if( !tableExists( assetDatabaseManager, tableName ) )
            {
                continue;
            }

            auto sql = String( "select * from '" ) + tableName + String( "'" );
            auto query = assetDatabaseManager->executeQuery( sql );
            appendReferencesFromQuery( query, objects, false );
        }

        if( !objects.empty() )
        {
            return objects;
        }

        auto sql = String( "select * from 'configured_actors' where parent_id=1" );
        auto query = assetDatabaseManager->executeQuery( sql );
        if( query )
        {
            appendReferencesFromQuery( query, objects, true );
        }

        return objects;
    }

    auto ResourceDatabase::getSceneObjects() const -> Array<SmartPtr<ISharedObject>>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        Array<SmartPtr<ISharedObject>> objects;
        objects.reserve( 4096 * 8 );

        if( auto sceneManager = applicationManager->getGameManager() )
        {
            if( auto scene = sceneManager->getCurrentScene() )
            {
                auto actors = scene->getActors();
                for( auto actor : actors )
                {
                    if( actor )
                    {
                        getSceneObjects( actor, objects );
                    }
                }
            }
        }

        return objects;
    }

    auto ResourceDatabase::getObjectByFileId( const String &fileId ) const -> SmartPtr<ISharedObject>
    {
        auto fileUUID = StringUtil::parseUUID( fileId );

        for( auto &object : m_objects )
        {
            if( auto handle = object->getHandle() )
            {
                if( handle->getFileId() == fileUUID )
                {
                    return object;
                }
            }
        }

        return nullptr;
    }

    auto ResourceDatabase::getInstancesPtr() const -> SharedPtr<Array<SmartPtr<IResource>>>
    {
        return m_instances;
    }

    void ResourceDatabase::setInstancesPtr( SharedPtr<Array<SmartPtr<IResource>>> instances )
    {
        m_instances = instances;
    }

    String ResourceDatabase::getFilePath() const
    {
        return m_filePath;
    }

    void ResourceDatabase::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    void ResourceDatabase::createDatabase()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto cachePath = applicationManager->getCachePath();

        auto dbFilePath = getFilePath();
        auto databasePath = cachePath + "/" + dbFilePath;
        databasePath = StringUtil::cleanupPath( databasePath );

        auto databaseManager =
            workphone::static_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
        if( !databaseManager )
        {
            databaseManager = factoryManager->make_ptr<AssetDatabaseManager>();
        }

        databaseManager->setDatabasePath( databasePath );
        databaseManager->open();
        databaseManager->create();

        setDatabaseManager( databaseManager );
    }

    void ResourceDatabase::destroyDatabase()
    {
    }

    ResourceDatabase::ImportFileJob::ImportFileJob()
    {
        ResourceDatabase::m_numJobs++;
    }

    ResourceDatabase::ImportFileJob::~ImportFileJob()
    {
        ResourceDatabase::m_numJobs--;
    }

    void ResourceDatabase::ImportFileJob::execute()
    {
        try
        {
            auto filePath = getFilePath();

            // WP_LOG("Importing File: " + filePath);

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto resourceDatabase = applicationManager->getResourceDatabasePtr();
            WP_ASSERT( resourceDatabase );

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto soundManager = applicationManager->getSoundManagerPtr();

            auto sceneManager = applicationManager->getGameManagerPtr();

            auto prefabManager = applicationManager->getPrefabManager();
            WP_ASSERT( prefabManager );

            auto cachePath = applicationManager->getCachePath();

            auto dbFilePath = resourceDatabase->getFilePath();
            auto databasePath = cachePath + "/" + dbFilePath;
            databasePath = StringUtil::cleanupPath( databasePath );

            if( !resourceDatabase->getDatabaseManager() )
            {
                resourceDatabase->createDatabase();
            }

            if( !Path::isExistingFile( databasePath ) )
            {
                resourceDatabase->createDatabase();
            }

            auto databaseManager = resourceDatabase->getDatabaseManager();
            WP_ASSERT( databaseManager );

            auto assetDatabaseManager =
                workphone::static_pointer_cast<AssetDatabaseManager>( databaseManager );
            WP_ASSERT( assetDatabaseManager );

            assetDatabaseManager->loadFromFile( databasePath );

            auto projectPath = applicationManager->getProjectPath();
            auto settingsCachePath = applicationManager->getSettingsPath();

            auto basePath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( basePath ) )
            {
                basePath = Path::getWorkingDirectory();
            }

            auto data = String( "" );

            auto fileExt = Path::getFileExtension( filePath );
            fileExt = StringUtil::make_lower( fileExt );

            auto filePathHash = StringUtil::getUUID( filePath );

            static const String resourcedataExt = String( ".resourcedata" );
            static const String builtinMeshExt = String( ".fbmeshbin" );
            static const String materialExt = String( ".mat" );
            static const String databaseExt = String( ".db" );
            static const String lightingPresetExt = String( ".lightingpreset" );

            auto fileDataPath =
                settingsCachePath + StringUtil::toString( filePathHash ) + resourcedataExt;
            auto relativeFileDataPath = Path::getRelativePath( projectPath, fileDataPath );

            if( fileExt == ".lua" )
            {
                // Reimport retains the catalog UUID; it never runs source code.
                if( !assetDatabaseManager->getResourceEntryFromPath( filePath ) )
                {
                    auto script = make_ptr<ScriptAsset>();
                    script->getHandle()->setUUID( StringUtil::getUUID() );
                    script->loadFromFile( filePath );
                    assetDatabaseManager->addResourceEntry( script );
                }
            }
            else if( fileExt == builtinMeshExt )
            {
                auto meshManager = applicationManager->getMeshManager();
                auto mesh = meshManager->loadFromFile( filePath );
                if( mesh )
                {
                    if( assetDatabaseManager->hasResourceEntry( mesh ) )
                    {
                        assetDatabaseManager->updateResourceEntry( mesh );
                    }
                    else
                    {
                        assetDatabaseManager->addResourceEntry( mesh );
                    }
                }
            }
            else if( ApplicationUtil::isSupportedMesh( filePath ) )
            {
                auto meshLoader = applicationManager->getMeshLoader();
                if( meshLoader )
                {
                    auto overwrite = getOverwrite();
                    meshLoader->setOverwrite( overwrite );

                    auto actor = meshLoader->loadActor( filePath );
                    if( actor )
                    {
                        if( graphicsSystem )
                        {
                            auto converter = graphicsSystem->getMeshConverter();
                            if( converter )
                            {
                                converter->writeMesh( actor );
                            }
                            else
                            {
                                WP_LOG_ERROR( "Mesh converter is not available." );
                            }
                        }

                        auto prefabFilePath = ApplicationUtil::getPrefabPath( filePath );
                        if( overwrite || !fileSystem->isExistingFile( prefabFilePath ) )
                        {
                            prefabManager->savePrefab( prefabFilePath, actor );
                        }
                    }

                    if( sceneManager )
                    {
                        sceneManager->destroyActor( actor );
                    }
                }

                if( !fileSystem->isExistingFile( relativeFileDataPath ) )
                {
                    auto resourceDirector = factoryManager->make_ptr<scene::MeshResourceDirector>();
                    if( resourceDirector )
                    {
                        auto handle = resourceDirector->getHandle();
                        auto uuid = handle->getUUIDAsString();
                        if( StringUtil::isNullOrEmpty( uuid ) )
                        {
                            uuid = StringUtil::getUUID();
                            handle->setUUID( uuid );
                        }

                        resourceDirector->setResourcePath( filePath );
                        resourceDirector->setFilePath( relativeFileDataPath );

                        auto properties = resourceDirector->getProperties();
                        auto dataStr = DataUtil::toString( properties.get(), true );
                        fileSystem->writeAllText( relativeFileDataPath, dataStr );
                    }
                }
            }
            else if( fileExt == materialExt )
            {
                if( graphicsSystem )
                {
                    auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
                    auto material = materialManager->loadFromFile( filePath );
                    if( material )
                    {
                        if( assetDatabaseManager->hasResourceEntry( material ) )
                        {
                            assetDatabaseManager->updateResourceEntry( material );
                        }
                        else
                        {
                            assetDatabaseManager->addResourceEntry( material );
                        }
                    }
                }
            }
            else if( fileExt == databaseExt )
            {
                //if( assetDatabaseManager->hasResourceEntry( texture ) )
                //{
                //    assetDatabaseManager->updateResourceEntry( texture );
                //}
                //else
                //{
                //    assetDatabaseManager->addResourceEntry( texture );
                //}
            }
            else if( ApplicationUtil::isSupportedTexture( filePath ) )
            {
                if( graphicsSystem )
                {
                    auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;
                    auto texture = textureManager->loadFromFile( filePath );
                    if( texture )
                    {
                        if( assetDatabaseManager->hasResourceEntry( texture ) )
                        {
                            assetDatabaseManager->updateResourceEntry( texture );
                        }
                        else
                        {
                            assetDatabaseManager->addResourceEntry( texture );
                        }
                    }
                }

                if( !fileSystem->isExistingFile( relativeFileDataPath ) )
                {
                    auto resourceDirector = factoryManager->make_ptr<scene::TextureResourceDirector>();
                    if( resourceDirector )
                    {
                        auto handle = resourceDirector->getHandle();
                        auto uuid = handle->getUUIDAsString();
                        if( StringUtil::isNullOrEmpty( uuid ) )
                        {
                            uuid = StringUtil::getUUID();
                            handle->setUUID( uuid );
                        }

                        resourceDirector->setResourcePath( filePath );
                        resourceDirector->setFilePath( relativeFileDataPath );

                        auto properties = resourceDirector->getProperties();
                        auto dataStr = DataUtil::toString( properties.get(), true );
                        fileSystem->writeAllText( relativeFileDataPath, dataStr );
                    }
                }
            }
            else if( ApplicationUtil::isSupportedSound( filePath ) )
            {
                if( soundManager )
                {
                    auto sound = soundManager->loadFromFile( filePath );
                    if( sound )
                    {
                        if( assetDatabaseManager->hasResourceEntry( sound ) )
                        {
                            assetDatabaseManager->updateResourceEntry( sound );
                        }
                        else
                        {
                            assetDatabaseManager->addResourceEntry( sound );
                        }
                    }
                }
            }
            else if( fileExt == lightingPresetExt )
            {
                auto lightingpreset = workphone::make_ptr<scene::LightingDirector>();
                if( lightingpreset )
                {
                    auto handle = lightingpreset->getHandle();
                    auto uuid = handle->getUUIDAsString();
                    if( StringUtil::isNullOrEmpty( uuid ) )
                    {
                        uuid = StringUtil::getUUID();
                        handle->setUUID( uuid );
                    }

                    lightingpreset->setFilePath( filePath );

                    auto dataStr = fileSystem->readAllText( filePath );

                    auto properties = workphone::make_ptr<Properties>();
                    DataUtil::parse( dataStr, properties.get() );

                    lightingpreset->setProperties( properties );

                    if( assetDatabaseManager->hasResourceEntry( lightingpreset ) )
                    {
                        assetDatabaseManager->updateResourceEntry( lightingpreset );
                    }
                    else
                    {
                        assetDatabaseManager->addResourceEntry( lightingpreset );
                    }
                }
            }

            resourceDatabase->unloadUnusedResources();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void ResourceDatabase::ImportFileJob::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    String ResourceDatabase::ImportFileJob::getFilePath() const
    {
        return m_filePath;
    }

    bool ResourceDatabase::ImportFileJob::getOverwrite() const
    {
        return m_overwrite;
    }

    void ResourceDatabase::ImportFileJob::setOverwrite( bool overwrite )
    {
        m_overwrite = overwrite;
    }

}  // namespace workphone
