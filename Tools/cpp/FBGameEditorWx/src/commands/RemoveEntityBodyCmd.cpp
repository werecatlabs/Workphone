#include <GameEditorPCH.hpp>
#include "RemoveEntityBodyCmd.hpp"
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
		RemoveEntityBodyCmd::RemoveEntityBodyCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityBodyCmd::~RemoveEntityBodyCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityBodyCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
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
		void RemoveEntityBodyCmd::redo()
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
			m_parentEnt->getResourceSetTemplate()->removeBody(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityBodyCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String name;
			String label;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getResourceSetTemplate()->removeBody(name);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveEntityBodyCmd::getCommandId() const
		{
			return "RemoveEntityBodyCmd";
		}
		
		
	}
}
