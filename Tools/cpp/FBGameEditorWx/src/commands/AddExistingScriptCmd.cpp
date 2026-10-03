#include <GameEditorPCH.hpp>
#include "AddExistingScriptCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/ScriptTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		
	
		
		//--------------------------------------------
		AddExistingScriptCmd::AddExistingScriptCmd(const Properties& properties)
		{
			m_properties = properties;
		}
		
		
		
		//--------------------------------------------
		AddExistingScriptCmd::~AddExistingScriptCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddExistingScriptCmd::undo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			project->removeScriptTemplate(m_scriptTemplate);
	
		
			//String fileName;
			//m_properties.getPropertyValue("FileName", fileName);
			//m_parentEnt->getResourceSetTemplate()->removeScript(fileName);
		
			//AppRoot* appRoot = AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddExistingScriptCmd::redo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
		
			m_scriptTemplate = SmartPtr<ScriptTemplate>(new ScriptTemplate);		
			m_scriptTemplate->setProperties(m_properties);
	
			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IFileSystem>& fileSystem = applicationManager->getFileSystem();
			//SmartPtr<IStream> stream = fileSystem->open(m_scriptTemplate->getFileName(), m_scriptTemplate->getFilePath());
			//if(stream)
			//{
			//	String data = stream->getAsString();
			//	m_scriptTemplate->setData(data);
			//}
			
			// if we already have the script we delete it before added again
			//if(m_parentEnt->getResourceSetTemplate()->isScript(fileName))
			//	m_parentEnt->getResourceSetTemplate()->removeScript(fileName);
		
			// add script to entity
			project->addScriptTemplate(m_scriptTemplate);
		
			//AppRoot* appRoot = AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddExistingScriptCmd::execute()
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
		
			m_scriptTemplate = SmartPtr<ScriptTemplate>(new ScriptTemplate);		
			m_scriptTemplate->setProperties(m_properties);
	
			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IFileSystem>& fileSystem = applicationManager->getFileSystem();
			//SmartPtr<IStream> stream = fileSystem->open(m_scriptTemplate->getFileName(), m_scriptTemplate->getFilePath());
			//if(stream)
			//{
			//	String data = stream->getAsString();
			//	m_scriptTemplate->setData(data);
			//}

			project->addScriptTemplate(m_scriptTemplate);
		}
		
		
		
		//--------------------------------------------
		String AddExistingScriptCmd::getCommandId() const
		{
			return "AddExistingScriptCmd";
		}
		
		
		
	} // end namespace editor
} // end namespace fb
