#include <EditorPCH.hpp>
#include <jobs/SaveProjectJob.hpp>
#include <editor/EditorManager.hpp>
#include <editor/Project.hpp>
#include <Workphone/Workphone.hpp>

namespace workphone::editor
{
    WP_CLASS_REGISTER_DERIVED( workphone, SaveProjectJob, Job );

    SaveProjectJob::SaveProjectJob() = default;

    SaveProjectJob::~SaveProjectJob() = default;

    void SaveProjectJob::execute()
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        auto stateManager = applicationManager->getStateManagerPtr();
        auto fileSystem = applicationManager->getFileSystemPtr();

        auto application = applicationManager->getApplication();

        auto editorManager = EditorManager::getSingletonPtr();

        if( auto project = editorManager->getProject() )
        {
            auto filePath = project->getFilePath();
            if( !StringUtil::isNullOrEmpty( filePath ) )
            {
                project->saveToFile( filePath );
            }
            else
            {
                WP_LOG_ERROR( "Project file path empty" );
            }
        }
        else
        {
            WP_LOG_ERROR( "Project null" );
        }
    }

}  // namespace workphone::editor
