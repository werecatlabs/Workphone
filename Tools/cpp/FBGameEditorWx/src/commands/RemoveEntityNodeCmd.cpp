#include <GameEditorPCH.hpp>
#include <commands/RemoveEntityNodeCmd.hpp>
#include <editor/EditorManager.hpp>
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
		RemoveEntityNodeCmd::RemoveEntityNodeCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityNodeCmd::~RemoveEntityNodeCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityNodeCmd::undo()
		{
			// get entity, it may be changed
			auto appRoot = editor::EditorManager::getSingletonPtr();
			auto project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<SceneNodeTemplate> sceneNodeTemplate(new SceneNodeTemplate);
		
			String name;
			String type;
			String label;
			String mesh;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
		
			sceneNodeTemplate->setName(name);
			sceneNodeTemplate->setLabel(label);
		
			// if we already have the node we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isNode(name))
				m_parentEnt->getResourceSetTemplate()->removeNode(name);
		
			// add node to entity
			m_parentEnt->getResourceSetTemplate()->addNode(sceneNodeTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityNodeCmd::redo()
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
		void RemoveEntityNodeCmd::execute()
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
		String RemoveEntityNodeCmd::getCommandId() const
		{
			return "RemoveEntityNodeCmd";
		}
		
		
	}
}
