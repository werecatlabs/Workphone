#include <GameEditorPCH.hpp>
#include "AddEntityBodyCmd.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>


namespace fb
{
	namespace editor
	{
		

		
		//--------------------------------------------
		AddEntityBodyCmd::AddEntityBodyCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntityBodyCmd::~AddEntityBodyCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntityBodyCmd::undo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			String name;
			String label;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getResourceSetTemplate()->removeBody(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityBodyCmd::redo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<PhysicsBodyTemplate> bodyTemplate(new PhysicsBodyTemplate);
		
			String name;
			String label;
			String type;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("Type", type);
		
			bodyTemplate->setName(name);
			bodyTemplate->setLabel(label);
			bodyTemplate->setPhysicsBodyType(type);
		
			// if we already have the body we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isBody(name))
				m_parentEnt->getResourceSetTemplate()->removeBody(name);
		
			// add body to entity
			m_parentEnt->getResourceSetTemplate()->addBody(bodyTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityBodyCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			SmartPtr<PhysicsBodyTemplate> bodyTemplate(new PhysicsBodyTemplate);
		
			String name;
			String label;
			String type;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("Type", type);
		
			bodyTemplate->setName(name);
			bodyTemplate->setLabel(label);
			bodyTemplate->setPhysicsBodyType(type);
		
			// if we already have the body we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isBody(name))
				m_parentEnt->getResourceSetTemplate()->removeBody(name);
		
			// add body to entity
			m_parentEnt->getResourceSetTemplate()->addBody(bodyTemplate);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntityBodyCmd::getCommandId() const
		{
			return "AddEntityBodyCmd";
		}
		
		
		
		
	}
}
