#include <GameEditorPCH.hpp>
#include "AddFSMEventCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/SoundTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		

		
		//--------------------------------------------
		AddFSMEventCmd::AddFSMEventCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentFsm(parentFsm),
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_fsmName = parentFsm->getName();
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddFSMEventCmd::~AddFSMEventCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddFSMEventCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			// get FSM, it may be changed
			m_parentFsm = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(m_fsmName);
			if(m_parentFsm == nullptr)
				return;
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentFsm->removeEventTemplate(label);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddFSMEventCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			// get FSM, it may be changed
			m_parentFsm = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(m_fsmName);
			if(m_parentFsm == nullptr)
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
			if(m_parentFsm->hasEventTemplate(label))
				m_parentFsm->removeEventTemplate(label);
		
			// add event to entity
			m_parentFsm->addEventTemplate( eventTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddFSMEventCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentFsm )
		
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
			if(m_parentFsm->hasEventTemplate(label))
				m_parentFsm->removeEventTemplate(label);
		
			// add event to entity
			m_parentFsm->addEventTemplate( eventTemplate);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddFSMEventCmd::getCommandId() const
		{
			return "AddFSMEventCmd";
		}
		
		
	}
}
