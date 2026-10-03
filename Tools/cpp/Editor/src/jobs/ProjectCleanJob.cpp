#include <EditorPCH.hpp>
#include "jobs/ProjectCleanJob.hpp"
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

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

            auto editorManager = EditorManager::getSingletonPtr();
            auto uiManager = editorManager->getUI();

            auto metaFiles = fileSystem->getFileNamesWithExtension( ".meta" );
            for( auto &file : metaFiles )
            {
                fileSystem->deleteFile( file );
            }

            auto lightingFiles = fileSystem->getFileNamesWithExtension( ".lighting" );
            for( auto &file : lightingFiles )
            {
                fileSystem->deleteFile( file );
            }

            auto unityFiles = fileSystem->getFileNamesWithExtension( ".unity" );
            for( auto &file : unityFiles )
            {
                fileSystem->deleteFile( file );
            }

            uiManager->rebuildResourceTree();
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
