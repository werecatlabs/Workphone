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
        if( !m_operationCatalog || m_operationId.empty() )
            return;
        auto app = core::IApplicationManager::instancePtr();
        auto resources = app ? app->getResourceDatabase() : nullptr;
        auto active = resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() ) : nullptr;
        if( active != m_operationCatalog || m_operationCatalog->getProjectRoot() != m_operationRoot )
        {
            WP_LOG_ERROR( "Cannot undo asset deletion in a different project." );
            return;
        }
        const auto result = m_operationCatalog->undoFileOperation( m_operationId );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot undo asset deletion: " + result.error );
            m_operationCatalog->recoverFileOperations();
            return;
        }
        if( app && app->getFileSystem() )
            app->getFileSystem()->refreshPath( Path::getFilePath( m_filePath ), true );
    }

    void RemoveResourceCmd::redo()
    {
        execute();
    }

    void RemoveResourceCmd::execute()
    {
        auto app = core::IApplicationManager::instancePtr();
        if( !app )
            return;
        auto resources = app->getResourceDatabase();
        auto database = resources ? resources->getDatabaseManager() : nullptr;
        auto catalog = dynamic_pointer_cast<AssetDatabaseManager>( database );
        if( !catalog )
        {
            WP_LOG_ERROR( "Asset deletion requires an open project catalog." );
            return;
        }
        if( m_operationCatalog && ( catalog != m_operationCatalog ||
                                   catalog->getProjectRoot() != m_operationRoot ) )
        {
            WP_LOG_ERROR( "Cannot redo asset deletion in a different project." );
            return;
        }
        const auto result = catalog->performFileOperation(
            AssetDatabaseManager::FileOperation::Delete, m_filePath );
        if( !result.succeeded )
        {
            WP_LOG_ERROR( "Cannot delete asset: " + result.error );
            catalog->recoverFileOperations();
            return;
        }
        m_operationCatalog = catalog;
        m_operationId = result.operationId;
        m_operationRoot = catalog->getProjectRoot();
        if( auto fileSystem = app->getFileSystem() )
            fileSystem->refreshPath( Path::getFilePath( m_filePath ), true );
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
