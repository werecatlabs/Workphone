#include <GameEditorPCH.hpp>
#include "AddEntityNodeCmd.hpp"
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
		AddEntityNodeCmd::AddEntityNodeCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntityNodeCmd::~AddEntityNodeCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntityNodeCmd::undo()
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
		void AddEntityNodeCmd::redo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<SceneNodeTemplate> sceneNodeTemplate(new SceneNodeTemplate);
		
			String name;
			String label;
		
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
		void AddEntityNodeCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			SmartPtr<SceneNodeTemplate> sceneNodeTemplate(new SceneNodeTemplate);
		
			String name;
			String label;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
		
			sceneNodeTemplate->setName(name);
			sceneNodeTemplate->setLabel(label);
		
			// if we already have the gfx object we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isNode(name))
				m_parentEnt->getResourceSetTemplate()->removeNode(name);
		
			// add gfx object to entity
			m_parentEnt->getResourceSetTemplate()->addNode(sceneNodeTemplate);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntityNodeCmd::getCommandId() const
		{
			return "AddEntityNodeCmd";
		}
		
		
	}
}
