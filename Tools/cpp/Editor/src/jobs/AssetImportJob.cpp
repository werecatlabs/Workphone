#include <EditorPCH.hpp>
#include "jobs/AssetImportJob.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    AssetImportJob::AssetImportJob( bool reimport ) : m_reimport( reimport )
    {
        auto app = core::IApplicationManager::instance();
        if( !app )
        {
            return;
        }
        m_resources = app->getResourceDatabase();
        m_projectPath = app->getProjectPath();
        m_cachePath = app->getCachePath();
        m_catalog = m_resources
                        ? dynamic_pointer_cast<AssetDatabaseManager>( m_resources->getDatabaseManager() )
                        : nullptr;
        if( m_catalog && m_catalog->getDatabase() && m_catalog->getDatabase()->isLoaded() )
        {
            m_bound = m_session.capture( m_catalog );
        }
    }

    AssetImportJob::~AssetImportJob() = default;

    void AssetImportJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( !m_resources || resourceDatabase != m_resources ||
            applicationManager->getProjectPath() != m_projectPath ||
            applicationManager->getCachePath() != m_cachePath ||
            resourceDatabase->getDatabaseManager() != m_catalog ||
            ( m_bound && !m_session.isCurrent( m_catalog ) ) )
        {
            WP_LOG_ERROR( "Skipped import/reimport requested by an expired Editor project session." );
            return;
        }

        if( m_reimport )
        {
            resourceDatabase->reimportAssets();
        }
        else
        {
            resourceDatabase->importAssets();
        }
    }
}  // namespace workphone::editor
