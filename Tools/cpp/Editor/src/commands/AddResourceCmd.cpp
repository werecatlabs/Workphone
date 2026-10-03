#include <EditorPCH.hpp>
#include <commands/AddResourceCmd.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, AddResourceCmd, Command );

    AddResourceCmd::AddResourceCmd() = default;

    AddResourceCmd::~AddResourceCmd() = default;

    void AddResourceCmd::redo()
    {
        execute();
    }

    void AddResourceCmd::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto application = applicationManager->getApplication();

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto graphicsSystem = applicationManager->getGraphicsSystem();
            auto materialManager = graphicsSystem->getMaterialManager();

            auto resourceDatabase = applicationManager->getResourceDatabase();

            auto filePath = getFilePath();

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

                fileSystem->writeAllText( newFilePath, dataStr );
                createdFilePath = newFilePath;
            }

            m_createdFilePath = createdFilePath;

            if( resource )
            {
                resource->setFilePath( createdFilePath );
                setResource( resource );
                resourceDatabase->addResource( resource );
            }

            auto refreshPath = Path::getFilePath( createdFilePath );
            fileSystem->refreshPath( refreshPath, true );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AddResourceCmd::undo()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        auto filePath = String( m_createdFilePath );
        if( StringUtil::isNullOrEmpty( filePath ) )
        {
            filePath = getFilePath();
        }

        if( auto resource = workphone::dynamic_pointer_cast<IResource>( getResource() ) )
        {
            resourceDatabase->removeResource( resource );
        }
        else
        {
            resourceDatabase->removeResourceFromPath( filePath );
        }

        if( fileSystem->isExistingFile( filePath ) )
        {
            fileSystem->deleteFile( filePath );
        }

        auto refreshPath = Path::getFilePath( filePath );
        fileSystem->refreshPath( refreshPath, true );
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
