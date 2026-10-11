#include <EditorPCH.hpp>
#include <commands/AddResourceCmd.hpp>
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddResourceCmd, Command );

    AddResourceCmd::AddResourceCmd() = default;

    AddResourceCmd::~AddResourceCmd() = default;

    SmartPtr<AssetDatabaseManager> AddResourceCmd::activeCreationCatalog() const
    {
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto catalog =
            resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                      : nullptr;
        String root, error;
        if( !catalog || !m_creationSession.isCurrent( catalog ) || app->getProjectPath().empty() ||
            !canonicalAssetCatalogRoot( app->getProjectPath(), root, error ) ||
            root != catalog->getProjectRoot() )
            return nullptr;
        return catalog;
    }
    void AddResourceCmd::redo()
    {
        if( m_undoOperationId.empty() )
            return;
        auto catalog = activeCreationCatalog();
        if( !catalog )
        {
            WP_LOG_ERROR( "Creation redo project session expired." );
            return;
        }
        const auto result = catalog->undoFileOperation( m_undoOperationId );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot restore created asset: " + result.error );
            return;
        }
        m_undoOperationId.clear();
        auto app = core::IApplicationManager::instancePtr();
        if( auto fs = app->getFileSystem() )
            fs->refreshPath( Path::getFilePath( String( m_createdFilePath ) ),
                             app->getJobQueue() != nullptr );
    }

    void AddResourceCmd::execute()
    {
        try
        {
            if( !m_undoOperationId.empty() )
            {
                redo();
                return;
            }
            if( !String( m_createdFilePath ).empty() )
                return;
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto application = applicationManager->getApplication();

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto resourceDatabase = applicationManager->getResourceDatabase();
            auto catalog = resourceDatabase ? dynamic_pointer_cast<AssetDatabaseManager>(
                                                  resourceDatabase->getDatabaseManager() )
                                            : nullptr;
            if( !catalog || catalog->isReadOnly() || !m_creationSession.capture( catalog ) ||
                !activeCreationCatalog() )
                throw std::runtime_error(
                    "Asset creation requires the active writable project catalog" );
            AssetCatalogPath source;
            String error;
            if( !canonicalAssetCatalogPath( catalog->getProjectRoot(), getFilePath(), source, error ) )
                throw std::runtime_error( error.c_str() );
            if( source.key == ".workphone" || source.key.find( ".workphone/" ) == 0 )
                throw std::runtime_error( "Asset operation storage is reserved" );
            auto filePath = catalog->getProjectRoot() + "/" + source.path;

            SmartPtr<IResource> resource;
            SmartPtr<Properties> data;
            String dataStr;

            auto resourceType = getResourceType();
            switch( resourceType )
            {
            case ResourceType::Script:
            {
                resource = nullptr;
                data = nullptr;
                dataStr = "";
            }
            break;
            case ResourceType::LightingPreset:
            {
                auto director = workphone::make_ptr<scene::LightingDirector>();
                WP_ASSERT( director );

                director->load( nullptr );

                data = director->toData();
                WP_ASSERT( data );

                dataStr = DataUtil::toString( data.get(), true );
                WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );
            }
            break;
            case ResourceType::Material:
            {
                if( !application )
                    throw std::runtime_error( "Material creation requires an application" );
                resource = application->createDefaultMaterial();
                WP_ASSERT( resource );

                data = resource->toData();
                WP_ASSERT( data );

                dataStr = DataUtil::toString( data.get(), true );
                WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );
            }
            break;
            case ResourceType::Scene:
            {
            }
            break;
            case ResourceType::Director:
            {
                auto director = workphone::make_ptr<Director>();
                WP_ASSERT( director );

                director->load( nullptr );

                data = director->toData();
                WP_ASSERT( data );

                dataStr = DataUtil::toString( data.get(), true );
                WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );
            }
            break;
            }

            auto createdFilePath = filePath;

            if( !fileSystem->isExistingFile( filePath ) )
            {
                fileSystem->writeAllText( filePath, dataStr );
            }
            else
            {
                auto fileIndex = 0;
                auto maxRetries = 1000;

                auto path = Path::getFilePath( filePath );
                auto fileName = Path::getFileNameWithoutExtension( filePath );
                auto fileExt = Path::getFileExtension( filePath );

                auto newFileName = fileName + StringUtil::toString( fileIndex ) + fileExt;
                auto newFilePath = Path::lexically_normal( path, newFileName );
                while( fileSystem->isExistingFile( newFilePath ) && fileIndex < maxRetries )
                {
                    ++fileIndex;

                    newFileName = fileName + StringUtil::toString( fileIndex ) + fileExt;
                    newFilePath = Path::lexically_normal( path, newFileName );
                }

                if( fileSystem->isExistingFile( newFilePath ) )
                    throw std::runtime_error( "Cannot find an unused asset filename" );
                fileSystem->writeAllText( newFilePath, dataStr );
                createdFilePath = newFilePath;
            }

            if( !fileSystem->isExistingFile( createdFilePath ) )
                throw std::runtime_error( "Created asset file was not written" );
            m_createdFilePath = createdFilePath;

            if( resource )
            {
                resource->setFilePath( createdFilePath );
                setResource( resource );
                resourceDatabase->addResource( resource );
            }

            auto refreshPath = Path::getFilePath( createdFilePath );
            fileSystem->refreshPath( refreshPath, applicationManager->getJobQueue() != nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AddResourceCmd::undo()
    {
        if( String( m_createdFilePath ).empty() || !m_undoOperationId.empty() )
            return;
        auto catalog = activeCreationCatalog();
        if( !catalog )
        {
            WP_LOG_ERROR( "Creation undo project session expired." );
            return;
        }
        const auto result = catalog->performFileOperation( AssetDatabaseManager::FileOperation::Delete,
                                                           String( m_createdFilePath ) );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot undo created asset: " + result.error );
            return;
        }
        m_undoOperationId = result.operationId;
        auto app = core::IApplicationManager::instancePtr();
        if( auto fs = app->getFileSystem() )
            fs->refreshPath( Path::getFilePath( String( m_createdFilePath ) ),
                             app->getJobQueue() != nullptr );
    }

    String AddResourceCmd::getFilePath() const
    {
        return m_filePath;
    }

    void AddResourceCmd::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    AddResourceCmd::ResourceType AddResourceCmd::getResourceType() const
    {
        return m_resourceType;
    }

    void AddResourceCmd::setResourceType( ResourceType resourceType )
    {
        m_resourceType = resourceType;
    }

    SmartPtr<ISharedObject> AddResourceCmd::getResource() const
    {
        return m_resource;
    }

    void AddResourceCmd::setResource( SmartPtr<ISharedObject> resource )
    {
        m_resource = resource;
    }
}  // namespace workphone::editor
