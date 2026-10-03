#include <GameEditorPCH.hpp>
#include "jobs/JobOpenScript.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include "editor/EditorManager.hpp"
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


		JobOpenScript::JobOpenScript()
		{

		}

		JobOpenScript::~JobOpenScript()
		{

		}

		void JobOpenScript::execute()
		{
			auto applicationManager = IApplicationManager::instance();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			SmartPtr<UIManager> guiMgr = appRoot->getUI();

			ApplicationFrame* appFrame = guiMgr->getApplicationFrame();

			appFrame->openScript(getScriptTemplate());
		}

		SmartPtr<ScriptTemplate> JobOpenScript::getScriptTemplate() const
		{
			return m_scriptTemplate;
		}

		void JobOpenScript::setScriptTemplate( SmartPtr<ScriptTemplate> val )
		{
			m_scriptTemplate = val;
		}



	} // end namespace editor	
} // end namespace fb



