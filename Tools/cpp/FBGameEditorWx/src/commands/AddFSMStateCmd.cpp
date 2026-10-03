#include <GameEditorPCH.hpp>
#include "AddFSMStateCmd.hpp"
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
		AddFSMStateCmd::AddFSMStateCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt)
			: m_parentFsm(parentFsm),
			m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_fsmName = parentFsm->getName();
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddFSMStateCmd::~AddFSMStateCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddFSMStateCmd::undo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			// get FSM, it may be changed
			m_parentFsm = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(m_fsmName);
			if(m_parentFsm == nullptr)
				return;
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentFsm->deleteState(label);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddFSMStateCmd::redo()
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
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			eventTemplate->setLabel(label);
		
			// add state to entity
			m_parentFsm->addState(label);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddFSMStateCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentFsm )
		
			SmartPtr<EventTemplate> eventTemplate(new EventTemplate);
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			eventTemplate->setLabel(label);
		
			// add state to entity
			m_parentFsm->addState(label);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddFSMStateCmd::getCommandId() const
		{
			return "AddFSMStateCmd";
		}
		
		
	}
}
