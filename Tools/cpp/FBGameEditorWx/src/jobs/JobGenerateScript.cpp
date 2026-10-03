#include <GameEditorPCH.hpp>
#include "jobs/JobGenerateScript.hpp"
#include "commands/AddNewScriptCmd.hpp"
#include "ui/ProjectWindow.hpp"
#include "ui/ApplicationFrame.hpp"
#include <FBApplication/Script/ScriptGenerator.hpp>
#include <fstream>

#include "editor/EditorManager.hpp"
#include <FBCore/FBCoreHeaders.hpp>




namespace fb
{
	namespace editor
	{



		JobGenerateScript::JobGenerateScript()
			: m_isExistingScript(false)
		{

		}

		JobGenerateScript::~JobGenerateScript()
		{

		}

		void JobGenerateScript::execute()
		{
			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IFileSystem>& fileSystem = engine->getFileSystem();

			//ApplicationManager* appRoot = IApplicationManager::instance();
			//ProjectPtr project = appRoot->getProject();
			//SmartPtr<UIManager> guiMgr = appRoot->getGUIManager();

			//AppFrame* appFrame = guiMgr->getAppFrame();
			//if(!appFrame)
			//{
			//	FB_EXCEPTION("Error: App frame null! ");
			//	return;
			//}


			//ProjectWindow* projectWindow = guiMgr->getProjectWindow();
			//if(!projectWindow)
			//{
			//	FB_EXCEPTION("Error: projectWindow null! ");
			//	return;
			//}

			//SmartPtr<EventTemplate> event = getEvent();
			//EventTemplateContainer* eventTemplateContainer = event->getOwner();
			//SmartPtr<ITemplate> objectTemplate = eventTemplateContainer->getOwner();

			//Properties templateProperties;
			//objectTemplate->getProperties(templateProperties);

			//String className;

			//String templateType = objectTemplate->getEditableType();
			//if(templateType==("EntityTemplate"))
			//{
			//	templateProperties.getPropertyValue("name", className);
			//}
			//else 
			//{
			//	templateProperties.getPropertyValue("className", className);
			//}

			//// build script db

			//// search for existing function

			//// add function to source file

			//// find source file

			//// check if script exists
			//bool isExistingScript = false;
			//String fileName;
			//SmartPtr<ScriptTemplate> existingScriptTemplate;

			//Array<SmartPtr<ScriptTemplate>> scriptTemplates = project->getScriptTemplates();
			//for(u32 i=0; i<scriptTemplates.size(); ++i)
			//{
			//	SmartPtr<ScriptTemplate> scriptTemplate = scriptTemplates[i];

			//	// temp code assume to the class name equals the file name
			//	fileName = scriptTemplate->getFileName();
			//	String templateClassName = StringUtil::cutFilenameExtension(fileName);

			//	if(className==(templateClassName))
			//	{
			//		existingScriptTemplate = scriptTemplate;
			//		isExistingScript = true;
			//		break;
			//	}
			//}

			//// check class exists

			//// generate script
			//String filePath; 

			//if(!isExistingScript)
			//{
			//	filePath = guiMgr->saveScript(className);
			//	if(filePath == StringUtil::EmptyString)
			//		return;
			//}
			//else 
			//{
			//	if(existingScriptTemplate)
			//	{
			//		filePath = existingScriptTemplate->getFilePath();
			//		filePath += "/";
			//		filePath += existingScriptTemplate->getFileName();
			//	}
			//	else
			//	{
			//		FB_EXCEPTION("Error: Unexpected null. ");
			//	}
			//}

			//String path = Path::getFilePath(filePath);
			//path += String("/");
			//fileSystem->addFileArchive(path, true, true, FB_ARCH_TYPE_FOLDER);

			//String scriptFullFilePath = Path::getFullPath(project->getProjectDirectory(), path);

			//if(!isExistingScript)
			//{
			//	Properties properties;
			//	properties.setProperty("filePath", path, String("string"));
			//	properties.setProperty("className", className, String("string"));
			//	properties.setProperty("fileName", Path::getFileName(filePath), String("string"));

			//	SmartPtr<AddNewScriptCmd> cmd(new AddNewScriptCmd(properties));
			//	engine->getCommandManager()->add(cmd);
			//	cmd->execute();

			//	existingScriptTemplate = cmd->getScriptTemplate();
			//}

			//// generate script
			//if(!isExistingScript)
			//{
			//	ScriptGenerator generator;
			//	String classScriptStr = generator.createClass(className);

			//	std::ofstream scriptStream;
			//	scriptStream.open( scriptFullFilePath.c_str() );

			//	scriptStream << classScriptStr.c_str() << std::endl;

			//	// generate function
			//	String functionScriptStr = generator.createFunction(className, event);
			//	scriptStream << functionScriptStr.c_str() << std::endl;

			//	// close the stream
			//	scriptStream.close();
			//}
			//else
			//{
			//	ScriptGenerator generator;

			//	std::ofstream scriptStream;
			//	scriptStream.open( (scriptFullFilePath + String("/") + existingScriptTemplate->getFileName()).c_str(), std::ofstream::app );

			//	// generate function
			//	String functionScriptStr = generator.createFunction(className, event);
			//	scriptStream << functionScriptStr.c_str() << std::endl;

			//	// close the stream
			//	scriptStream.close();
			//}

			//setScriptTemplate(existingScriptTemplate);
			//setExistingScript(isExistingScript);
		}

		fb::SmartPtr<ScriptTemplate> JobGenerateScript::getScriptTemplate() const
		{
			return m_scriptTemplate;
		}

		void JobGenerateScript::setScriptTemplate(SmartPtr<ScriptTemplate> val)
		{
			m_scriptTemplate = val;
		}

		fb::SmartPtr<EventTemplate> JobGenerateScript::getEvent() const
		{
			return m_event;
		}

		void JobGenerateScript::setEvent(SmartPtr<EventTemplate> val)
		{
			m_event = val;
		}

		bool JobGenerateScript::isExistingScript() const
		{
			return m_isExistingScript;
		}

		void JobGenerateScript::setExistingScript(bool val)
		{
			m_isExistingScript = val;
		}



	} // end namespace editor	
} // end namespace fb


