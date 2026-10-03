#include <GameEditorPCH.hpp>
#include "RemoveEntityComponentCmd.hpp"
#include "ui/ProjectTreeData.hpp"
#include "ui/UIManager.hpp"
#include "editor/EditorManager.hpp"
#include <editor/Project.hpp>
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
		RemoveEntityComponentCmd::RemoveEntityComponentCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityComponentCmd::~RemoveEntityComponentCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityComponentCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<ComponentTemplate> compTemplate(new ComponentTemplate);
		
			String type;
			m_propertyGroup.getPropertyValue("Type", type);
		
			compTemplate->setType(type);
		
			if(m_parentEnt->findComponentTemplate(type) == nullptr)
		
			// add component to entity
			m_parentEnt->add(compTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityComponentCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String type;
			m_propertyGroup.getPropertyValue("Type", type);
			//m_parentEnt->remove(m_parentEnt->findComponentTemplate(type));
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityComponentCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String type;
			m_propertyGroup.getPropertyValue("Type", type);
			//m_parentEnt->remove(m_parentEnt->findComponentTemplate(type));
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveEntityComponentCmd::getCommandId() const
		{
			return "RemoveEntityComponentCmd";
		}
		
		
	}
}
