#include <EditorPCH.hpp>
#include <jobs/CopyEngineFilesJob.hpp>
#include <editor/EditorManager.hpp>
#include <ui/UIManager.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    CopyEngineFilesJob::CopyEngineFilesJob() = default;

    CopyEngineFilesJob::~CopyEngineFilesJob() = default;

    void CopyEngineFilesJob::execute()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            auto editorManager = EditorManager::getSingletonPtr();
            auto projectManager = editorManager->getProjectManager();

            auto srcFolder = applicationManager->getMediaPath() + "/Engine";
            auto dstFolder = applicationManager->getProjectPath() + "/Engine";

            if( Path::isExistingFolder( dstFolder ) )
            {
                Path::deleteFolder( dstFolder );
            }

            Path::copyFolder( srcFolder, dstFolder );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }
}  // namespace workphone::editor
