#include <EditorPCH.hpp>
#include "jobs/JobSaveTree.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include <editor/EditorManager.hpp>
#include "editor/Project.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>
#include <fstream>

namespace workphone::editor
{
    JobSaveTree::JobSaveTree() = default;

    JobSaveTree::~JobSaveTree() = default;

    void JobSaveTree::execute()
    {
        // auto applicationManager = core::IApplicationManager::instance();
        // SmartPtr<IFileSystem>& fileSystem = applicationManager->getFileSystem();

        // EditorManager* appRoot = EditorManager::getSingletonPtr();
        // SmartPtr<Project> project = appRoot->getProject();
        // SmartPtr<UIManager> guiMgr = appRoot->getUI();

        // ProjectWindow* projectWindow = guiMgr->getProjectWindow();
        // if(!projectWindow)
        //{
        //	WP_EXCEPTION("Error: projectWindow null! ");
        //	return;
        // }

        // projectWindow->saveTreeState();
    }
}  // namespace workphone::editor
