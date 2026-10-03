#include <EditorPCH.hpp>
#include "jobs/JobRestoreTree.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include <editor/EditorManager.hpp>
#include "editor/Project.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/UIManager.hpp"

#include <Workphone/Workphone.hpp>
#include <fstream>

namespace workphone::editor
{
    JobRestoreTree::JobRestoreTree() = default;

    JobRestoreTree::~JobRestoreTree() = default;

    void JobRestoreTree::execute()
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

        // projectWindow->buildTree();
        // projectWindow->restoreTreeState();
    }
}  // namespace workphone::editor
