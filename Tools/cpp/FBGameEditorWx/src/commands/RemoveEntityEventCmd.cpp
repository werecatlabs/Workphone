#include <GameEditorPCH.hpp>
#include "RemoveEntityEventCmd.hpp"
#include "editor/EditorManager.hpp"
#include <ui/UIManager.hpp>
#include <editor/Project.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/GfxObjectTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		
		
		//--------------------------------------------
		RemoveEntityEventCmd::RemoveEntityEventCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityEventCmd::~RemoveEntityEventCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityEventCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<EventTemplate> eventTemplate(new EventTemplate);
		
			String className;
			String type;
			String label;
			String functionName;
		
			m_propertyGroup.getPropertyValue("ClassName", className);
			m_propertyGroup.getPropertyValue("Type", type);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("Function", functionName);
		
			eventTemplate->setClassName(className);
			eventTemplate->setType(type);
			eventTemplate->setLabel(label);
			eventTemplate->setFunction(functionName);
		
			// if we already have the event we delete it before added again
			if(m_parentEnt->getEventTemplateContainer()->hasEventTemplate(label))
				m_parentEnt->getEventTemplateContainer()->removeEventTemplate(label);
		
			// add event to entity
			m_parentEnt->getEventTemplateContainer()->addEventTemplate( eventTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityEventCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getEventTemplateContainer()->removeEventTemplate(label);
			//populateTree();
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityEventCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
				String label;
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getEventTemplateContainer()->removeEventTemplate(label);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveEntityEventCmd::getCommandId() const
		{
			return "RemoveEntityEventCmd";
		}
		
		
	}
}
