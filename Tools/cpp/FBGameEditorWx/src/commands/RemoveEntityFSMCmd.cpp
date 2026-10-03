#include <GameEditorPCH.hpp>
#include "RemoveEntityFSMCmd.hpp"
#include "ui/ProjectTreeData.hpp"
#include "RemoveFSMEventCmd.hpp"
#include "RemoveFSMStateCmd.hpp"
#include "editor/EditorManager.hpp"
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
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
		RemoveEntityFSMCmd::RemoveEntityFSMCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityFSMCmd::~RemoveEntityFSMCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityFSMCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<FSMTemplate> fsmTemplate(new FSMTemplate);
		
			String name;
		
			m_propertyGroup.getPropertyValue("Name", name);
		
			fsmTemplate->setName(name);
		
			// if we already have the FSM, we delete it before added again
			SmartPtr<FSMTemplate> existingFSMTemplate = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(name);
			if(existingFSMTemplate != nullptr)
				m_parentEnt->getFSMTemplateContainer()->remove(existingFSMTemplate);
		
			// add FSM to entity
			m_parentEnt->getFSMTemplateContainer()->add(fsmTemplate);
		
			for(u32 eventIdx=0; eventIdx<m_commandsEvents.size(); ++eventIdx)
			{
				SmartPtr<ICommand> eventCmd = m_commandsEvents[eventIdx];
				eventCmd->undo();
			}
		
			for(u32 stateIdx=0; stateIdx<m_commandsStates.size(); ++stateIdx)
			{
				SmartPtr<ICommand> stateCmd = m_commandsStates[stateIdx];
				stateCmd->undo();
			}
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityFSMCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String name;
			m_propertyGroup.getPropertyValue("Name", name);
			SmartPtr<FSMTemplate> fsmTemplate = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(name);
			if(fsmTemplate != nullptr)
			{
				// clear Arrays
				m_commandsEvents.clear();
				m_commandsStates.clear();
		
				Array<SmartPtr<EventTemplate>> eventTemplates = fsmTemplate->getEventTemplates();
				for(u32 eventIdx=0; eventIdx<eventTemplates.size(); ++eventIdx)
				{
					SmartPtr<EventTemplate> eventTemplate = eventTemplates[eventIdx];
					if(eventTemplate != nullptr)
					{
						Properties propertyGroup;
						propertyGroup.setProperty("ClassName", eventTemplate->getClassName(), String("string"));
						propertyGroup.setProperty("Type", eventTemplate->getType(), String("string"));
						propertyGroup.setProperty("Label", eventTemplate->getLabel(), String("string"));
						propertyGroup.setProperty("Function", eventTemplate->getFunction(), String("string"));
		
						RemoveFSMEventCmdPtr cmd(new RemoveFSMEventCmd(propertyGroup, fsmTemplate, m_parentEnt));
		
						// execute command
						cmd->execute();
		
						// add command to Array
						m_commandsEvents.push_back(cmd);
					}
				}
		
				Array<String> states = fsmTemplate->getStates();
				for(u32 stateIdx=0; stateIdx<states.size(); ++stateIdx)
				{
					String stateName = states[stateIdx];
					if(stateName != "")
					{
						Properties propertyGroup;
						propertyGroup.setProperty("Label", stateName, String("string"));
		
						RemoveFSMStateCmdPtr cmd(new RemoveFSMStateCmd(propertyGroup, fsmTemplate, m_parentEnt));
		
						/*auto applicationManager = IApplicationManager::instance();
						applicationManager->getCommandManager()->add(cmd);*/
		
						cmd->execute();
		
						// add command to Array
						m_commandsStates.push_back(cmd);
					}
				}
		
				// remove FSM
				m_parentEnt->getFSMTemplateContainer()->remove(fsmTemplate);
		
				//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
				appRoot->getUI()->rebuildSceneTree();
			}
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityFSMCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String name;
			m_propertyGroup.getPropertyValue("Name", name);
			SmartPtr<FSMTemplate> fsmTemplate = m_parentEnt->getFSMTemplateContainer()->findFSMTemplate(name);
			if(fsmTemplate != nullptr)
			{
				// clear Arrays
				m_commandsEvents.clear();
				m_commandsStates.clear();
		
				Array<SmartPtr<EventTemplate>> eventTemplates = fsmTemplate->getEventTemplates();
				for(u32 eventIdx=0; eventIdx<eventTemplates.size(); ++eventIdx)
				{
					SmartPtr<EventTemplate> eventTemplate = eventTemplates[eventIdx];
					if(eventTemplate != nullptr)
					{
						Properties propertyGroup;
						propertyGroup.setProperty("ClassName", eventTemplate->getClassName(), String("string"));
						propertyGroup.setProperty("Type", eventTemplate->getType(), String("string"));
						propertyGroup.setProperty("Label", eventTemplate->getLabel(), String("string"));
						propertyGroup.setProperty("Function", eventTemplate->getFunction(), String("string"));
		
						RemoveFSMEventCmdPtr cmd(new RemoveFSMEventCmd(propertyGroup, fsmTemplate, m_parentEnt));
		
						// execute command
						cmd->execute();
		
						// add command to Array
						m_commandsEvents.push_back(cmd);
					}
				}
		
				Array<String> states = fsmTemplate->getStates();
				for(u32 stateIdx=0; stateIdx<states.size(); ++stateIdx)
				{
					String stateName = states[stateIdx];
					if(stateName != "")
					{
						Properties propertyGroup;
						propertyGroup.setProperty("Label", stateName, String("string"));
		
						RemoveFSMStateCmdPtr cmd(new RemoveFSMStateCmd(propertyGroup, fsmTemplate, m_parentEnt));
		
						/*auto applicationManager = IApplicationManager::instance();
						applicationManager->getCommandManager()->add(cmd);*/
		
						cmd->execute();
		
						// add command to Array
						m_commandsStates.push_back(cmd);
					}
				}
		
				// remove FSM
				m_parentEnt->getFSMTemplateContainer()->remove(fsmTemplate);
		
				editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
				appRoot->getUI()->rebuildSceneTree();
			}
		}
		
		
		
		//--------------------------------------------
		String RemoveEntityFSMCmd::getCommandId() const
		{
			return "RemoveEntityFSMCmd";
		}
		
		
	}
}
