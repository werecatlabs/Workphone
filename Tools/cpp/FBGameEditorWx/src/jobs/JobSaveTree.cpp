#include <GameEditorPCH.hpp>
#include "jobs/JobSaveTree.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include <editor/EditorManager.hpp>
#include "editor/Project.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/ApplicationFrame.hpp"
#include "ui/UIManager.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <FBCore/FBCore.hpp>
#include <fstream>



namespace fb
{
	namespace editor
	{


		JobSaveTree::JobSaveTree()
		{

		}

		JobSaveTree::~JobSaveTree()
		{

		}

		void JobSaveTree::execute()
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

			projectWindow->saveTreeState();
		}

	} // end namespace editor	
} // end namespace fb



