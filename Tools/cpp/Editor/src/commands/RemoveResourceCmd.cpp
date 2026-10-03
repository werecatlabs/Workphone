#include <EditorPCH.hpp>
#include "RemoveResourceCmd.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, RemoveResourceCmd, Command );

    RemoveResourceCmd::RemoveResourceCmd() = default;

    RemoveResourceCmd::~RemoveResourceCmd() = default;

    void RemoveResourceCmd::undo()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto fileSystem = applicationManager->getFileSystem();

        auto path = getFilePath();
        auto folder = Path::isFolder( path );

        if ( folder )
        {
            // Restore folder and its contents
            Path::createDirectories( path );

            for( const auto &fileData : m_removedFileData )
            {
                auto filePath = fileData.first;
                auto folderPath = Path::getFilePath( filePath );
                if( !StringUtil::isNullOrEmpty( folderPath ) )
                {
                    Path::createDirectories( folderPath );
                }

                fileSystem->writeAllText( filePath, fileData.second );
            }

            for( const auto &resource : m_removedResources )
            {
                if( resource )
                {
                    resourceDatabase->addResource( resource );
                }
            }
        }
        else
        {
            // Restore single file
            if( !m_removedFileData.empty() )
            {
                auto fileData = m_removedFileData.front();
                auto folderPath = Path::getFilePath( fileData.first );
                if( !StringUtil::isNullOrEmpty( folderPath ) )
                {
                    Path::createDirectories( folderPath );
                }

                fileSystem->writeAllText( fileData.first, fileData.second );
            }

            if( m_resource )
            {
                resourceDatabase->addResource( m_resource );
            }
        }

        fileSystem->refreshPath( Path::getFilePath( path ), true );
    }

    void RemoveResourceCmd::redo()
    {
        execute();
    }

    void RemoveResourceCmd::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        auto fileSystem = applicationManager->getFileSystem();

        m_removedResources.clear();
        m_removedFileData.clear();

        auto path = getFilePath();
        auto folder = Path::isFolder( path );
        if( folder )
        {
            auto files = fileSystem->getFiles( path );
            for( auto file : files )
            {
                if( fileSystem->isExistingFile( file ) )
                {
                    m_removedFileData.push_back(
                        Pair<String, String>( file, fileSystem->readAllText( file ) ) );
                }

                auto resource = resourceDatabase->loadResource( file );
                if( resource )
                {
                    m_removedResources.push_back( resource );
                    resourceDatabase->removeResource( resource );
                }
            }

            Path::deleteFolder( path );
        }
        else
        {
            if( fileSystem->isExistingFile( path ) )
            {
                m_removedFileData.push_back(
                    Pair<String, String>( path, fileSystem->readAllText( path ) ) );
            }

            auto resource = resourceDatabase->loadResource( path );
            setResource( resource );
            if( resource )
            {
                resourceDatabase->removeResource( resource );
            }

            fileSystem->deleteFile( m_filePath );
        }

        fileSystem->refreshPath( Path::getFilePath( path ), true );
    }

    String RemoveResourceCmd::getFilePath() const
    {
        return m_filePath;
    }

    void RemoveResourceCmd::setFilePath( const String &filePath )
    {
        m_filePath = filePath;
    }

    void RemoveResourceCmd::setResource( SmartPtr<IResource> resource )
    {
        m_resource = resource;
    }

    SmartPtr<IResource> RemoveResourceCmd::getResource() const
    {
        return m_resource;
    }

}  // namespace workphone::editor
