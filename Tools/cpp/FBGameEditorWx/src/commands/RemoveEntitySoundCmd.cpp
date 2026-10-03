#include <GameEditorPCH.hpp>
#include <commands/RemoveEntitySoundCmd.hpp>
#include <editor/EditorManager.hpp>
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
		RemoveEntitySoundCmd::RemoveEntitySoundCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntitySoundCmd::~RemoveEntitySoundCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntitySoundCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SoundTemplatePtr soundTemplate(new SoundTemplate);
		
			String name;
			String label;
			String soundName;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("SoundName", soundName);
		
			soundTemplate->setName(name);
			soundTemplate->setLabel(label);
			soundTemplate->setSoundName(soundName);
		
			// if we already have the sound we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isSound(name))
				m_parentEnt->getResourceSetTemplate()->removeSound(name);
		
			// add sound to entity
			m_parentEnt->getResourceSetTemplate()->addSound(soundTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntitySoundCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String name;
			String label;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getResourceSetTemplate()->removeNode(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntitySoundCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String name;
			String label;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getResourceSetTemplate()->removeGfxObject(name);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveEntitySoundCmd::getCommandId() const
		{
			return "RemoveEntitySoundCmd";
		}
		
		
	}
}
