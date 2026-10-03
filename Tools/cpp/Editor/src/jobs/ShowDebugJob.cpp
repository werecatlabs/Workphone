#include <EditorPCH.hpp>
#include <jobs/ShowDebugJob.hpp>
#include <editor/EditorManager.hpp>
#include <ui/ProjectWindow.hpp>
#include <ui/SceneWindow.hpp>
#include <ui/UIManager.hpp>

namespace workphone::editor
{

    WP_CLASS_REGISTER_DERIVED( workphone::editor, ShowDebugJob, Job );

    ShowDebugJob::ShowDebugJob()
    {
    }

    ShowDebugJob::~ShowDebugJob()
    {
    }

    void ShowDebugJob::execute()
    {
        auto editorManager = EditorManager::getSingletonPtr();
        WP_ASSERT( editorManager );

        auto ui = editorManager->getUI();

        if( auto sceneWindow = ui->getSceneWindow() )
        {
            sceneWindow->buildTree();
        }

        if( auto projectWindow = ui->getProjectWindow() )
        {
            projectWindow->buildTree();
        }
    }

}  // namespace workphone::editor
