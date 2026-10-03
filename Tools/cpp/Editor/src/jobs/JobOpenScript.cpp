#include <EditorPCH.hpp>
#include "jobs/JobOpenScript.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/UIManager.hpp"
#include <Workphone/Workphone.hpp>
#include <fstream>

namespace workphone::editor
{
    JobOpenScript::JobOpenScript() = default;

    JobOpenScript::~JobOpenScript() = default;

    void JobOpenScript::execute()
    {
        /*
        auto applicationManager = core::IApplicationManager::instance();

        auto appRoot = EditorManager::getSingletonPtr();
        SmartPtr<Project> project = appRoot->getProject();
        SmartPtr<UIManager> guiMgr = appRoot->getUI();

        ApplicationFrame *appFrame = guiMgr->getApplicationFrame();

        appFrame->openScript( getScriptTemplate() );
         */
    }
}  // namespace workphone::editor
