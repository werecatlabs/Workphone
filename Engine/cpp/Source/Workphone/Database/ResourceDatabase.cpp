#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Database/ResourceDatabase.hpp>
#include <Workphone/Database/AssetDatabaseManager.hpp>
#include <Workphone/Database/AssetImportQueue.hpp>
#include <Workphone/Database/AssetCatalogBrowser.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>
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
#include <set>
#include <functional>
#include <filesystem>
#include <unordered_map>

namespace workphone
{
    namespace
    {
        SmartPtr<IDatabaseQuery> queryResourceValues( SmartPtr<IDatabaseManager> manager,
                                                      const String &sql, const Array<String> &values )
        {
            if( !manager )
            {
                return nullptr;
            }
            ScopedLock lock( manager.get() );
            auto database = manager->getDatabase();
            auto bound = database ? dynamic_cast<IParameterizedDatabase *>( database.get() ) : nullptr;
            return bound ? bound->queryBound( sql, values ) : nullptr;
        }

        SmartPtr<AssetDatabaseManager> catalogForUuid( const ResourceDatabase &resources,
                                                       const String &uuid,
                                                       AssetDatabaseManager::EntrySnapshot &entry )
        {
            auto project = dynamic_pointer_cast<AssetDatabaseManager>( resources.getDatabaseManager() );
            auto media = resources.getMediaCatalog();
            AssetDatabaseManager::EntrySnapshot projectEntry, mediaEntry;
            const bool inProject = project && project->tryGetEntry( uuid, projectEntry );
            const bool inMedia = media && media->tryGetEntry( uuid, mediaEntry );
            if( inProject && inMedia )
            {
                WP_LOG_ERROR( "Ambiguous UUID exists in both project and bundled media catalogs: " +
                              uuid );
                entry = projectEntry;
                return nullptr;
            }
            if( inProject )
            {
                entry = projectEntry;
                return project;
            }
            if( inMedia )
            {
                entry = mediaEntry;
                return media;
            }
            return nullptr;
        }

        const String ReferenceObjectType = "ResourceReference";
        const String ReferenceRootUUID = "reference-root";
        const String ReferenceRootPath = "reference";

        thread_local ResourceDatabase *activeImport = nullptr;
        thread_local String importProjectPath, importCachePath, importSettingsPath;
        thread_local std::function<bool()> currentImportSession;
        auto importQueue = std::make_shared<AssetImportQueue>();

        class ImportPump : public Job
        {
        public:
            explicit ImportPump( std::shared_ptr<AssetImportQueue> queue ) : queue( std::move( queue ) )
            {
            }

            ~ImportPump() override
            {
                if( !executed )
                {
                    queue->cancel();
                }
            }

            void execute() override
            {
                executed = true;
                queue->drain();
            }

        private:
            std::shared_ptr<AssetImportQueue> queue;
            bool executed = false;
        };

        void dispatchImport( ResourceDatabase *database, std::function<void()> operation,
                             bool nestedFile = false )
        {
            if( nestedFile && activeImport == database )
            {
                if( currentImportSession && currentImportSession() )
                {
                    operation();
                }
                return;
            }
            auto app = core::IApplicationManager::instance();
            if( !app || app->getQuit() || !app->isRunning() || !database->isLoaded() )
            {
                return;
            }
            auto owner = SmartPtr<ResourceDatabase>( database );
            auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database->getDatabaseManager() );
            const auto project = app->getProjectPath(), cache = app->getCachePath(),
                       settings = app->getSettingsPath();
            auto session = std::make_shared<AssetCatalogSession>();
            const bool bound = catalog && catalog->getDatabase() && catalog->getDatabase()->isLoaded();
            if( bound && !session->capture( catalog ) )
            {
                return;
            }
            if( catalog && !bound &&
                !catalog->setProjectRoot( project.empty() ? Path::getWorkingDirectory() : project ) )
            {
                return;
            }
            auto valid = [owner, catalog, project, cache, settings, session, bound]() {
                auto current = core::IApplicationManager::instance();
                return current && !current->getQuit() && current->isRunning() && owner->isLoaded() &&
                       current->getResourceDatabase() == owner &&
                       owner->getDatabaseManager() == catalog && current->getProjectPath() == project &&
                       current->getCachePath() == cache && current->getSettingsPath() == settings &&
                       ( !bound || session->isCurrent( catalog ) );
            };
            if( !importQueue->enqueue( [owner, valid, project, cache, settings,
                                        operation = std::move( operation )]() {
                    if( !valid() )
                    {
                        WP_LOG_ERROR( "Skipped asset import from an expired project/catalog session." );
                        return;
                    }
                    activeImport = owner.get();
                    importProjectPath = project;
                    importCachePath = cache;
                    importSettingsPath = settings;
                    currentImportSession = valid;
                    try
                    {
                        operation();
                    }
                    catch( const std::exception &error )
                    {
                        WP_LOG_ERROR( error.what() );
                    }
                    catch( ... )
                    {
                        WP_LOG_ERROR( "Asset import failed with an unknown exception." );
                    }
                    currentImportSession = {};
                    activeImport = nullptr;
                } ) )
            {
                return;
            }
            auto queue = app->getJobQueue();
            auto pool = app->getThreadPool();
            if( app->hasTasks() && queue && queue->isRunning() && pool && pool->getNumThreads() > 1 )
            {
                queue->addJob( make_ptr<ImportPump>( importQueue ) );
            }
            else
            {
                importQueue->drain();
            }
        }

        struct ResourceCleanupCandidate
        {
            SmartPtr<ISharedObject> object;
            s32 ownedReferences;
            bool unused = false;
        };

        // Count all database aliases before inspecting ownership. One temporary
        // strong reference per object keeps destruction outside the container locks.
        Array<ResourceCleanupCandidate> collectUnusedResources(
            ConcurrentMap<UUID, SmartPtr<ISharedObject>> &objects,
            ConcurrentArray<SmartPtr<IResource>> &instances, bool remove )
        {
            Array<ResourceCleanupCandidate> candidates;
            std::unordered_map<ISharedObject *, size_t> indices;
            {
                auto registry = objects.writeLocked();
                ScopedLock instanceLock( &instances, true );
                const auto add = [&]( const auto &object ) {
                    if( !object )
                    {
                        return;
                    }
                    const auto inserted = indices.emplace( object.get(), candidates.size() );
                    if( inserted.second )
                    {
                        candidates.push_back( { object, 1, false } );
                    }
                    else
                    {
                        ++candidates[inserted.first->second].ownedReferences;
                    }
                };
                for( const auto &[id, object] : registry )
                {
                    add( object );
                }
                for( const auto &object : instances )
                {
                    add( object );
                }
                for( auto &candidate : candidates )
                {
                    candidate.unused =
                        candidate.object->getReferences() == candidate.ownedReferences + 1;
                }
                if( remove )
                {
                    const auto unused = [&]( const auto &object ) {
                        return !object || candidates[indices.at( object.get() )].unused;
                    };
                    for( auto it = registry.begin(); it != registry.end(); )
                    {
                        if( unused( it->second ) )
                        {
                            it = registry.erase( it );
                        }
                        else
                        {
                            ++it;
                        }
                    }
                    size_t kept = 0;
                    for( size_t i = 0; i < instances.size(); ++i )
                    {
                        if( !unused( instances[i] ) )
                        {
                            if( kept != i )
                            {
                                instances[kept] = std::move( instances[i] );
                            }
                            ++kept;
                        }
                    }
                    instances.resize( kept );
                }
            }
            return candidates;
        }

        class ResourceCleanupJob : public Job
        {
        public:
            ResourceCleanupJob( SmartPtr<ResourceDatabase> owner, bool remove ) :
                owner( owner ),
                remove( remove )
            {
                setPrimary( true );
            }

            void execute() override
            {
                if( owner->isLoaded() )
                {
                    if( remove )
                    {
                        owner->removeUnusedResources();
                    }
                    else
                    {
                        owner->unloadUnusedResources();
                    }
                }
            }

        private:
            SmartPtr<ResourceDatabase> owner;
            bool remove;
        };

        bool deferResourceCleanup( ResourceDatabase *owner, bool remove )
        {
            if( Thread::getCurrentThreadId() == Thread::ThreadId::Primary )
            {
                return false;
            }
            auto app = core::IApplicationManager::instance();
            auto queue = app ? app->getJobQueue() : nullptr;
            if( queue && queue->isRunning() && app->isRunning() && !app->getQuit() )
            {
                queue->addJob(
                    make_ptr<ResourceCleanupJob>( SmartPtr<ResourceDatabase>( owner ), remove ) );
            }
            else
            {
                WP_LOG_ERROR( "Resource cleanup requires an available primary-thread job queue." );
            }
            return true;
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

            for( auto [id, object] : m_objects )
            {
                object->unload( nullptr );
            }

            m_objects.clear();
            m_resourceMap.clear();
            setDatabaseManager( nullptr );
            setMediaCatalog( nullptr );

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

                for( auto object : objects )
                {
                    auto handle = object->getHandle();
                    auto uuid = handle->getUUID();
                    m_objects[uuid] = object;
                }
            }

            static const String materialExt = ".mat";
            auto materialFiles = fileSystem->getFilesWithExtension( materialExt );
            if( !materialFiles.empty() )
            {
                if( graphicsSystem )
                {
                    auto materialManager =
                        graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

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
                        auto textureManager =
                            graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

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
        auto databasePath =
            Path::isPathAbsolute( dbFilePath ) ? dbFilePath : cachePath + "/" + dbFilePath;
        databasePath = StringUtil::cleanupPath( databasePath );

        assetDatabaseManager->loadFromFile( databasePath );

        m_objects.clear();

        auto sceneManager = applicationManager->getGameManager();
        if( sceneManager )
        {
            auto scene = sceneManager->getCurrentScene();
            WP_ASSERT( scene );

            auto objects = getSceneObjects();
            for( auto object : objects )
            {
                auto handle = object->getHandle();
                auto uuid = handle->getUUID();
                m_objects[uuid] = object;
            }
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

        // Release the read statement before starting deletion transactions.
        auto query =
            assetDatabaseManager->executeQuery( "SELECT path FROM resources WHERE kind='file'" );
        Array<String> missing;
        const auto root = assetDatabaseManager->getProjectRoot();
        if( query )
        {
            while( !query->eof() )
            {
                const auto path = query->getFieldValue( "path" );
                std::error_code error;
                if( !std::filesystem::exists( std::filesystem::u8path( ( root + "/" + path ).c_str() ),
                                              error ) &&
                    !error )
                {
                    missing.push_back( path );
                }
                query->nextRow();
            }
        }
        query = nullptr;
        for( const auto &path : missing )
        {
            assetDatabaseManager->removeResourceEntryFromPath( path );
        }
    }

    void ResourceDatabase::deleteCache()
    {
        dispatchImport(
            this,
            [this]() {
                auto applicationManager = core::IApplicationManager::instance();
                auto fileSystem = applicationManager->getFileSystem();

                auto cachePath = importCachePath;
                auto folderListing = fileSystem->getFolderListing( cachePath );

                if( !folderListing )
                {
                    return;
                }
                String root, error;
                if( !canonicalAssetCatalogRoot( cachePath, root, error ) )
                {
                    return;
                }
                auto &files = folderListing->getFiles();
                for( auto &file : files )
                {
                    if( !currentImportSession || !currentImportSession() )
                    {
                        return;
                    }
                    auto ext = Path::getFileExtension( file );
                    AssetCatalogPath candidate;
                    if( !canonicalAssetCatalogPath( root, file, candidate, error ) )
                    {
                        continue;
                    }
                    auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
                    AssetCatalogPath live;
                    if( catalog &&
                        canonicalAssetCatalogPath( root, catalog->getDatabasePath(), live, error ) &&
                        ( candidate.key == live.key || candidate.key == live.key + "-wal" ||
                          candidate.key == live.key + "-shm" ||
                          candidate.key == live.key + "-journal" ) )
                    {
                        continue;
                    }
                    // Only known regenerable payloads are cache-owned. Catalogs, WAL/SHM,
                    // manifests, source files and unknown files must survive reimport.
                    if( ext == ".fbmeshbin" || ext == ".pxtrianglemesh" )
                    {
                        fileSystem->deleteFile( file );
                    }
                }
            },
            true );
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
        dispatchImport(
            this,
            [this, folderListing, overwrite]() {
                try
                {
                    if( folderListing )
                    {
                        WP_LOG( "Importing: " + folderListing->getFolderName() );

                        auto applicationManager = core::IApplicationManager::instance();
                        WP_ASSERT( applicationManager );

                        auto projectPath = importProjectPath;
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
            },
            true );
    }

    void ResourceDatabase::importFolder( const String &path, bool overwrite )
    {
        dispatchImport(
            this,
            [this, path, overwrite]() {
                try
                {
                    if( !StringUtil::isNullOrEmpty( path ) )
                    {
                        auto applicationManager = core::IApplicationManager::instance();
                        WP_ASSERT( applicationManager );

                        auto fileSystem = applicationManager->getFileSystem();
                        WP_ASSERT( fileSystem );

                        auto projectPath = importProjectPath;
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
            },
            true );
    }

    void ResourceDatabase::importFile( const String &filePath, bool overwrite )
    {
        dispatchImport(
            this,
            [this, filePath, overwrite]() {
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

                auto factoryManager = applicationManager->getFactoryManagerPtr();
                auto job = factoryManager->make_ptr<ImportFileJob>();
                job->setFilePath( filePath );
                job->setOverwrite( overwrite );
                job->execute();
            },
            true );
    }

    void ResourceDatabase::importCache()
    {
        dispatchImport(
            this,
            [this]() {
                auto applicationManager = core::IApplicationManager::instance();
                WP_ASSERT( applicationManager );

                auto fileSystem = applicationManager->getFileSystem();
                WP_ASSERT( fileSystem );

                clean();

                auto projectPath = importProjectPath;
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto cachePath = importCachePath;

                std::cout << "Importing: " << cachePath << std::endl;

                importFolder( cachePath );

                std::cout << "Finished Importing: " << cachePath << std::endl;
            },
            false );
    }

    void ResourceDatabase::importAssets()
    {
        dispatchImport(
            this,
            [this]() {
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

                    auto projectPath = importProjectPath;
                    if( StringUtil::isNullOrEmpty( projectPath ) )
                    {
                        projectPath = Path::getWorkingDirectory();
                    }

                    auto cachePath = importCachePath;
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
            },
            false );
    }

    void ResourceDatabase::reimportAssets()
    {
        dispatchImport(
            this,
            [this]() {
                try
                {
                    auto applicationManager = core::IApplicationManager::instance();
                    WP_ASSERT( applicationManager );

                    auto fileSystem = applicationManager->getFileSystem();
                    WP_ASSERT( fileSystem );

                    clean();

                    auto cachePath = importCachePath;
                    deleteCache();

                    auto projectPath = importProjectPath;
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
            },
            false );
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
                    auto materialManager =
                        graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

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
        if( id.is_nil() )
        {
            return nullptr;
        }
        AssetDatabaseManager::EntrySnapshot entry;
        if( auto catalog = catalogForUuid( *this, StringUtil::toString( id ), entry ) )
        {
            if( entry.kind != AssetDatabaseManager::EntryKind::File )
            {
                return nullptr;
            }
            if( entry.type == "script" )
            {
                auto script = make_ptr<ScriptAsset>();
                script->getHandle()->setUUID( entry.uuid );
                script->loadFromFile( catalog->getProjectRoot() + "/" + entry.path );
                return catalog->isEntryCurrent( entry ) ? script : nullptr;
            }
            auto query =
                queryResourceValues( catalog, "SELECT * FROM resources WHERE uuid=?", { entry.uuid } );
            auto resource = query && !query->eof()
                                ? createOrRetrieveResource( query, true, catalog->getProjectRoot() )
                                : nullptr;
            return catalog->isEntryCurrent( entry ) ? resource : nullptr;
        }
        if( !entry.uuid.empty() )
        {
            return nullptr;  // An ambiguous authored identity cannot use the legacy fallback.
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
        if( !properties )
        {
            return nullptr;
        }

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

    SmartPtr<IResource> ResourceDatabase::loadMediaResource( const String &path )
    {
        auto catalog = getMediaCatalog();
        if( !catalog || path.empty() )
        {
            return nullptr;
        }
        auto director =
            dynamic_pointer_cast<scene::ResourceDirector>( catalog->getResourceEntryFromPath( path ) );
        if( !director )
        {
            return nullptr;
        }
        AssetDatabaseManager::EntrySnapshot snapshot;
        const auto uuid = director->getResourceUUID();
        if( !catalog->tryGetEntry( uuid, snapshot ) ||
            snapshot.kind != AssetDatabaseManager::EntryKind::File )
        {
            return nullptr;
        }
        auto resource = loadResourceById( StringUtil::parseUUID( uuid ) );
        return getMediaCatalog() == catalog && catalog->isEntryCurrent( snapshot ) ? resource : nullptr;
    }

    auto ResourceDatabase::loadResource( const String &path ) -> SmartPtr<IResource>
    {
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( getDatabaseManager() );
        if( !catalog || path.empty() )
        {
            return nullptr;
        }
        auto director =
            dynamic_pointer_cast<scene::ResourceDirector>( catalog->getResourceEntryFromPath( path ) );
        if( !director )
        {
            return nullptr;  // Loading never imports or invents persistent identity.
        }
        const auto uuid = director->getResourceUUID();
        AssetDatabaseManager::EntrySnapshot snapshot;
        if( !catalog->tryGetEntry( uuid, snapshot ) ||
            snapshot.kind != AssetDatabaseManager::EntryKind::File )
        {
            return nullptr;
        }
        if( snapshot.type == "script" )
        {
            return loadResource( StringUtil::parseUUID( uuid ) );
        }
        auto query = queryResourceValues( catalog, "SELECT * FROM resources WHERE uuid=?", { uuid } );
        if( !query || query->eof() )
        {
            return nullptr;
        }
        auto resource = createOrRetrieveResource( query, true, catalog->getProjectRoot() );
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

        for( auto &[id, object] : m_objects )
        {
            auto resource = workphone::dynamic_pointer_cast<IResource>( object );
            if( resource )
            {
                if( resource->getFilePath() == fileDataPath )
                {
                    return resource;
                }
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
        return loadResource( uuid );
    }

    void ResourceDatabase::unloadUnusedResources()
    {
        if( deferResourceCleanup( this, false ) )
        {
            return;
        }
        Array<ResourceCleanupCandidate> candidates;
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            candidates = collectUnusedResources( m_objects, m_instances, false );
        }
        for( auto &candidate : candidates )
        {
            if( candidate.unused && candidate.object->getReferences() == candidate.ownedReferences + 1 )
            {
                candidate.object->unload( nullptr );
            }
        }
    }

    void ResourceDatabase::removeUnusedResources()
    {
        if( deferResourceCleanup( this, true ) )
        {
            return;
        }
        Array<ResourceCleanupCandidate> candidates;
        {
            RecursiveMutex::ScopedLock lock( m_mutex );
            candidates = collectUnusedResources( m_objects, m_instances, true );
        }
        // External owners acquired during detachment keep a live object. Backend
        // unload callbacks can safely re-enter the database after locks release.
        for( auto &candidate : candidates )
        {
            if( candidate.unused && candidate.object->getReferences() == 1 )
            {
                candidate.object->unload( nullptr );
            }
        }
    }

    auto ResourceDatabase::createOrRetrieveResource( SmartPtr<IDatabaseQuery> query, bool bLoadResource,
                                                     const String &sourceRoot ) -> SmartPtr<IResource>
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto factoryManager = applicationManager->getFactoryManager();
        auto soundManager = applicationManager->getSoundManager();

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto materialManager = graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;

        auto textureManager = graphicsSystem ? graphicsSystem->getTextureManager() : nullptr;

        auto id = query->getFieldValue( "id" );
        auto uuid = query->getFieldValue( "uuid" );
        auto path = sourceRoot + "/" + query->getFieldValue( "path" );
        auto type = query->getFieldValue( "type" );

        if( type == "Material" )
        {
            if( !materialManager )
            {
                return nullptr;
            }
            auto materialResult = materialManager->createOrRetrieve( uuid, path, type );

            auto resource = materialResult.first;
            if( bLoadResource && resource && !resource->isLoaded() )
            {
                graphicsSystem->loadObject( resource );
            }

            return materialResult.first;
        }
        if( type == "Mesh" || StringUtil::contains( type, "MeshResource" ) )
        {
            auto meshManager = applicationManager->getMeshManager();
            if( !meshManager || ( bLoadResource && !graphicsSystem ) )
            {
                return nullptr;
            }
            auto meshResult = meshManager->createOrRetrieve( uuid, path, type );

            auto resource = meshResult.first;
            if( bLoadResource && resource && !resource->isLoaded() )
            {
                graphicsSystem->loadObject( resource );
            }

            return meshResult.first;
        }
        if( type == "Texture" )
        {
            if( !textureManager )
            {
                return nullptr;
            }
            auto textureResult = textureManager->createOrRetrieve( uuid, path, type );

            auto resource = textureResult.first;
            if( bLoadResource )
            {
                graphicsSystem->loadObject( resource );
            }

            return textureResult.first;
        }
        if( type == "Font" )
        {
            auto manager = graphicsSystem ? graphicsSystem->getFontManager() : nullptr;
            if( !manager )
            {
                return nullptr;
            }
            auto result = manager->createOrRetrieve( uuid, path, type ).first;
            if( bLoadResource && result )
            {
                graphicsSystem->loadObject( result );
            }
            return result;
        }
        if( StringUtil::contains( type, "Sound" ) )
        {
            if( !soundManager )
            {
                return nullptr;
            }
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
        if( id.is_nil() )
        {
            return nullptr;
        }

        auto it = m_objects.find( id );
        if( it != m_objects.end() )
        {
            return it->second;
        }

        AssetDatabaseManager::EntrySnapshot entry;
        auto catalog = catalogForUuid( *this, StringUtil::toString( id ), entry );
        if( !catalog || entry.kind != AssetDatabaseManager::EntryKind::File )
        {
            return nullptr;
        }

        auto application = core::IApplicationManager::instance();
        if( !application )
        {
            return nullptr;
        }

        auto graphics = application->getGraphicsSystem();
        auto path = catalog->getProjectRoot() + "/" + entry.path;

        SmartPtr<ISharedObject> result;
        if( entry.type == "Material" )
        {
            auto manager = graphics ? graphics->getMaterialManager() : nullptr;
            if( manager )
            {
                result = manager->createOrRetrieve( entry.uuid, path, entry.type ).first;
            }
        }
        else if( entry.type == "Texture" )
        {
            auto manager = graphics ? graphics->getTextureManager() : nullptr;
            if( manager )
            {
                result = manager->createOrRetrieve( entry.uuid, path, entry.type ).first;
            }
        }
        else if( entry.type == "script" )
        {
            auto script = make_ptr<ScriptAsset>();
            script->getHandle()->setUUID( entry.uuid );
            script->loadFromFile( path );
            result = script;
        }
        else if( entry.type == "Font" )
        {
            auto manager = graphics ? graphics->getFontManager() : nullptr;
            if( manager )
            {
                result = manager->createOrRetrieve( entry.uuid, path, entry.type ).first;
            }
        }
        else if( entry.type == "Mesh" )
        {
            auto manager = application->getMeshManager();
            if( manager )
            {
                result = manager->createOrRetrieve( entry.uuid, path, entry.type ).first;
            }
        }
        else if( StringUtil::contains( entry.type, "Sound" ) )
        {
            if( auto manager = application->getSoundManager() )
            {
                result = manager->createOrRetrieve( entry.uuid, path, entry.type ).first;
            }
        }
        else if( auto factory = application->getFactoryManager() )
        {
            auto resource = factory->createObjectFromType<IResource>( entry.type );
            if( resource && resource->getHandle() )
            {
                resource->getHandle()->setUUID( entry.uuid );
                resource->loadFromFile( path );
                result = resource;
            }
        }
        return catalog->isEntryCurrent( entry ) ? result : nullptr;
    }

    bool ResourceDatabase::setMediaCatalog( SmartPtr<AssetDatabaseManager> catalog )
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        if( catalog )
        {
            if( !catalog->isReadOnly() )
            {
                return false;
            }
            auto marker = queryResourceValues(
                catalog, "SELECT namespace,version FROM wp_media_catalog WHERE id=1", {} );
            if( !marker || marker->eof() ||
                marker->getFieldValue( "namespace" ) != "workphone.builtin.media.v1" ||
                marker->getFieldValue( "version" ) != "1" )
            {
                return false;
            }
        }
        m_mediaCatalog = catalog;
        return true;
    }

    SmartPtr<AssetDatabaseManager> ResourceDatabase::getMediaCatalog() const
    {
        RecursiveMutex::ScopedLock lock( m_mutex );
        return m_mediaCatalog;
    }

    namespace
    {
        std::set<String> domainColumns( SmartPtr<IDatabaseManager> db, const String &table )
        {
            // Only fixed internal table names reach this helper.
            auto rows = queryResourceValues( db, "PRAGMA table_info(" + table + ")", {} );
            std::set<String> columns;
            if( !rows )
            {
                throw std::runtime_error( "Cannot inspect domain schema." );
            }
            while( !rows->eof() )
            {
                columns.insert( rows->getFieldValue( "name" ) );
                rows->nextRow();
            }
            return columns;
        }

        SmartPtr<Properties> readDomainTree( SmartPtr<IDatabaseManager> db, const UUID &uuid,
                                             SmartPtr<Properties> flattened )
        {
            if( !db || uuid.is_nil() )
            {
                return nullptr;
            }
            auto connection = db->getDatabase();
            DatabaseConnectionLock guard(
                connection ? dynamic_cast<ISerializedDatabase *>( connection.get() ) : nullptr );
            if( !queryResourceValues( db, "SAVEPOINT wp_domain_read", {} ) )
            {
                return nullptr;
            }
            try
            {
                const auto columns = domainColumns( db, "configured_actors" );
                if( !columns.count( "id" ) || !columns.count( "parent_id" ) )
                {
                    throw std::runtime_error( "Domain actor schema is unsupported." );
                }
                const auto attributes = domainColumns( db, "attribs" );
                const String identityColumn = columns.count( "uuid" ) ? "uuid" : "id";
                auto identity = queryResourceValues(
                    db, "SELECT id FROM configured_actors WHERE " + identityColumn + "=? LIMIT 2",
                    { StringUtil::toString( uuid ) } );
                if( !identity || identity->eof() )
                {
                    throw std::runtime_error(
                        "Domain UUID is missing; numeric actors require an explicit UUID column." );
                }
                const auto rootId = identity->getFieldValue( "id" );
                identity->nextRow();
                if( !identity->eof() )
                {
                    throw std::runtime_error( "Domain UUID is ambiguous." );
                }
                String attributeOwner;
                if( !attributes.empty() )
                {
                    if( !attributes.count( "title" ) || !attributes.count( "value" ) )
                    {
                        throw std::runtime_error( "Domain attribute schema is unsupported." );
                    }
                    attributeOwner = attributes.count( "configured_actor_id" ) ? "configured_actor_id"
                                     : attributes.count( "actor_id" )          ? "actor_id"
                                                                               : "";
                    if( attributeOwner.empty() )
                    {
                        throw std::runtime_error( "Domain attributes have no supported owner column." );
                    }
                }
                std::set<String> visited;
                size_t bytes = 0, attributeCount = 0;
                auto account = [&]( const String &text ) {
                    if( text.size() > 65536 || text.size() > 4u * 1024u * 1024u - bytes )
                    {
                        throw std::runtime_error( "Domain property byte budget exceeded." );
                    }
                    bytes += text.size();
                };
                std::function<SmartPtr<Properties>( const String &, size_t )> visit;
                visit = [&]( const String &id, size_t depth ) -> SmartPtr<Properties> {
                    if( depth > 64 || visited.size() >= 4096 || !visited.insert( id ).second )
                    {
                        throw std::runtime_error(
                            "Domain hierarchy contains a cycle or exceeds its traversal budget." );
                    }
                    auto row = queryResourceValues(
                        db, "SELECT * FROM configured_actors WHERE id=? LIMIT 2", { id } );
                    if( !row || row->eof() || row->getNumFields() > 256 )
                    {
                        throw std::runtime_error(
                            "Domain actor is missing or exceeds its field budget." );
                    }
                    auto result = make_ptr<Properties>();
                    for( u32 field = 0; field < row->getNumFields(); ++field )
                    {
                        const auto name = row->getFieldName( field );
                        const auto value = row->getFieldValue( field );
                        account( name );
                        account( value );
                        result->setProperty( name, value );
                    }
                    row->nextRow();
                    if( !row->eof() )
                    {
                        throw std::runtime_error( "Domain actor ID is ambiguous." );
                    }
                    if( !attributeOwner.empty() )
                    {
                        auto values =
                            queryResourceValues( db,
                                                 "SELECT title,value FROM attribs WHERE " +
                                                     attributeOwner + "=? ORDER BY title LIMIT 4097",
                                                 { id } );
                        if( !values )
                        {
                            throw std::runtime_error( "Cannot read domain attributes." );
                        }
                        std::set<String> names;
                        while( !values->eof() )
                        {
                            const auto name = values->getFieldValue( "title" );
                            const auto value = values->getFieldValue( "value" );
                            if( ++attributeCount > 4096 || name.empty() || name.size() > 256 ||
                                !names.insert( name ).second )
                            {
                                throw std::runtime_error(
                                    "Domain attributes are ambiguous or exceed their budget." );
                            }
                            account( name );
                            account( value );
                            result->setProperty( name, value );
                            if( depth == 0 && flattened )
                            {
                                flattened->setProperty( name, value );
                            }
                            values->nextRow();
                        }
                    }
                    auto children = queryResourceValues(
                        db, "SELECT id FROM configured_actors WHERE parent_id=? ORDER BY id LIMIT 4097",
                        { id } );
                    if( !children )
                    {
                        throw std::runtime_error( "Cannot read domain children." );
                    }
                    while( !children->eof() )
                    {
                        result->addChild( visit( children->getFieldValue( "id" ), depth + 1 ) );
                        children->nextRow();
                    }
                    return result;
                };
                auto tree = visit( rootId, 0 );
                if( !queryResourceValues( db, "RELEASE wp_domain_read", {} ) )
                {
                    throw std::runtime_error( "Cannot finish domain read snapshot." );
                }
                return tree;
            }
            catch( const std::exception &failure )
            {
                queryResourceValues( db, "ROLLBACK TO wp_domain_read", {} );
                queryResourceValues( db, "RELEASE wp_domain_read", {} );
                WP_LOG_ERROR( String( "Domain asset read failed: " ) + failure.what() );
                return nullptr;
            }
        }
    }  // namespace

    auto ResourceDatabase::loadAssetNode( SmartPtr<IDatabaseManager> db, SmartPtr<Properties> parent,
                                          const UUID &id ) -> SmartPtr<Properties>
    {
        auto result = readDomainTree( db, id, nullptr );
        if( result && parent )
        {
            parent->addChild( result );
        }
        return result;
    }

    auto ResourceDatabase::loadAssetProperties( const UUID &id ) -> SmartPtr<Properties>
    {
        auto application = core::IApplicationManager::instancePtr();
        if( !application || id.is_nil() )
        {
            return nullptr;
        }
        const auto path = application->getMediaPath() + "/AssetDatabase.db";
        std::error_code error;
        if( !std::filesystem::is_regular_file( std::filesystem::u8path( path.c_str() ), error ) ||
            error )
        {
            return nullptr;
        }
        auto factory = application->getFactoryManager();
        auto backend = factory ? factory->make_object<IDatabase>() : nullptr;
        auto reader = backend ? dynamic_cast<IReadOnlyDatabase *>( backend.get() ) : nullptr;
        String diagnostic;
        if( !reader || !reader->openReadOnly( path, diagnostic ) )
        {
            return nullptr;
        }
        auto domain = make_ptr<DatabaseManager>();
        domain->setDatabase( backend );
        auto properties = make_ptr<Properties>();
        properties->setName( "asset" );
        properties->setProperty( "asset_type", "model" );
        auto tree = readDomainTree( domain, id, properties );
        domain->close();
        if( !tree )
        {
            return nullptr;
        }
        properties->setProperty( "uuid", StringUtil::toString( id ) );
        properties->addChild( tree );
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
        {
            return objects;
        }
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

        for( auto &[id, object] : m_objects )
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

    Array<SmartPtr<IResource>> ResourceDatabase::getInstances() const
    {
        return m_instances.snapshot();
    }

    void ResourceDatabase::setInstances( const Array<SmartPtr<IResource>> &instances )
    {
        m_instances = { instances.begin(), instances.end() };
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

            if( activeImport != resourceDatabase )
            {
                resourceDatabase->importFile( filePath, getOverwrite() );
                return;
            }
            if( !currentImportSession || !currentImportSession() )
            {
                return;
            }
            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManagerPtr();
            WP_ASSERT( factoryManager );

            auto soundManager = applicationManager->getSoundManagerPtr();

            auto sceneManager = applicationManager->getGameManagerPtr();

            auto prefabManager = applicationManager->getPrefabManager();
            WP_ASSERT( prefabManager );

            auto cachePath = importCachePath;

            auto dbFilePath = resourceDatabase->getFilePath();
            auto databasePath =
                Path::isPathAbsolute( dbFilePath ) ? dbFilePath : cachePath + "/" + dbFilePath;
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

            auto projectPath = importProjectPath;
            auto settingsCachePath = importSettingsPath;

            auto basePath = importProjectPath;
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
                    auto materialManager =
                        graphicsSystem ? graphicsSystem->getMaterialManager() : nullptr;
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

            // Runtime eviction is an explicit primary-thread maintenance operation.
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
