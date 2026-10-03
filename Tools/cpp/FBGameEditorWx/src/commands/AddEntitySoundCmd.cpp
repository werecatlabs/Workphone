#include <GameEditorPCH.hpp>
#include "AddEntitySoundCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/SoundTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		AddEntitySoundCmd::AddEntitySoundCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntitySoundCmd::~AddEntitySoundCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntitySoundCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String name;
			String label;
			String soundname;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("SoundName", soundname);
			m_parentEnt->getResourceSetTemplate()->removeSound(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntitySoundCmd::redo()
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
		void AddEntitySoundCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
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
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntitySoundCmd::getCommandId() const
		{
			return "AddEntitySoundCmd";
		}
		
		
	}
}
