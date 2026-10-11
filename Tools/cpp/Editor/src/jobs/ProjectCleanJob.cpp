#include <EditorPCH.hpp>
#include "jobs/ProjectCleanJob.hpp"
#include <Workphone/Workphone.hpp>
#include <Workphone/Database/AssetCatalogPath.hpp>

namespace workphone::editor
{
    ProjectCleanJob::ProjectCleanJob() = default;

    ProjectCleanJob::~ProjectCleanJob() = default;

    void ProjectCleanJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto resources = applicationManager->getResourceDatabase();
            auto catalog =
                resources ? dynamic_pointer_cast<AssetDatabaseManager>( resources->getDatabaseManager() )
                          : nullptr;
            String root, error;
            if( !catalog || catalog->isReadOnly() || applicationManager->getProjectPath().empty() ||
                !canonicalAssetCatalogRoot( applicationManager->getProjectPath(), root, error ) ||
                root != catalog->getProjectRoot() )
                throw std::runtime_error( "Project clean requires the active writable project catalog" );
            // Metadata, lighting and imported scene files are authored content.
            // Artifact pruning requires the generation retention/pin policy.
            catalog->optimise();
            fileSystem->refreshPath( root, applicationManager->getJobQueue() != nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
