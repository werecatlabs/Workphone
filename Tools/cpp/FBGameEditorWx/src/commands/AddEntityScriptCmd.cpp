#include <GameEditorPCH.hpp>
#include "AddEntityScriptCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>



namespace fb
{	
	namespace editor
	{
		
		
	
		//--------------------------------------------
		AddEntityScriptCmd::AddEntityScriptCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntityScriptCmd::~AddEntityScriptCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntityScriptCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String fileName;
			m_propertyGroup.getPropertyValue("FileName", fileName);
			m_parentEnt->getResourceSetTemplate()->removeScript(fileName);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityScriptCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<ScriptTemplate> scriptTemplate(new ScriptTemplate);
		
			String fileName;
		
			m_propertyGroup.getPropertyValue("FileName", fileName);
		
			scriptTemplate->setFileName(fileName);
			
			// if we already have the script we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isScript(fileName))
				m_parentEnt->getResourceSetTemplate()->removeScript(fileName);
		
			// add script to entity
			m_parentEnt->getResourceSetTemplate()->add(scriptTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityScriptCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			SmartPtr<ScriptTemplate> scriptTemplate(new ScriptTemplate);
		
			String fileName;
		
			m_propertyGroup.getPropertyValue("FileName", fileName);
		
			scriptTemplate->setFileName(fileName);
		
			// if we already have the script we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isScript(fileName))
				m_parentEnt->getResourceSetTemplate()->removeScript(fileName);
		
			// add script to entity
			m_parentEnt->getResourceSetTemplate()->add(scriptTemplate);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntityScriptCmd::getCommandId() const
		{
			return "AddEntityScriptCmd";
		}
		
		
	}
}
