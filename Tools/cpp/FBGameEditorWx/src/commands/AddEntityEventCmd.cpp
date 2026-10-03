#include <GameEditorPCH.hpp>
#include "AddEntityEventCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>



namespace fb
{	
	namespace editor
	{

		
		
		//--------------------------------------------
		AddEntityEventCmd::AddEntityEventCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntityEventCmd::~AddEntityEventCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntityEventCmd::undo()
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
		
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityEventCmd::redo()
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
		void AddEntityEventCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
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
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntityEventCmd::getCommandId() const
		{
			return "AddEntityEventCmd";
		}
		

		
	} // end namespace editor	
} // end namespace fb	


