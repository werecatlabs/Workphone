#include <GameEditorPCH.hpp>
#include "RemoveFSMStateCmd.hpp"
#include "editor/EditorManager.hpp"
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
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
		RemoveFSMStateCmd::RemoveFSMStateCmd(const Properties& propertyGroup, SmartPtr<FSMTemplate> parentFsm, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentFsm(parentFsm),
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_fsmName = parentFsm->getName();
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveFSMStateCmd::~RemoveFSMStateCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveFSMStateCmd::undo()
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
		void RemoveFSMStateCmd::redo()
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
			m_parentFsm->deleteState(label);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveFSMStateCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentFsm )
		
			String label;
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentFsm->deleteState(label);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveFSMStateCmd::getCommandId() const
		{
			return "RemoveFSMStateCmd";
		}
		
		

	} // end namespace editor
} // end namespace fb



