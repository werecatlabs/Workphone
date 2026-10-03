#include <GameEditorPCH.hpp>
#include "jobs/JobRestoreTree.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include <editor/EditorManager.hpp>
#include "editor/Project.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/ApplicationFrame.hpp"
#include "ui/UIManager.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <FBCore/FBCoreHeaders.hpp>
#include <fstream>



namespace fb
{
	namespace editor
	{



		JobRestoreTree::JobRestoreTree()
		{

		}

		JobRestoreTree::~JobRestoreTree()
		{

		}

		void JobRestoreTree::execute()
		{
			auto applicationManager = IApplicationManager::instance();
			auto fileSystem = applicationManager->getFileSystem();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			SmartPtr<UIManager> guiMgr = appRoot->getUI();


			ProjectWindow* projectWindow = guiMgr->getProjectWindow();
			if(!projectWindow)
			{
				FB_EXCEPTION("Error: projectWindow null! ");
				return;
			}

			projectWindow->buildTree();
			projectWindow->restoreTreeState();
		}



	} // end namespace editor	
} // end namespace fb