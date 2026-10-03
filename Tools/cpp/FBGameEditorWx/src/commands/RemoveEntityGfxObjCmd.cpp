#include <GameEditorPCH.hpp>
#include "RemoveEntityGfxObjCmd.hpp"
#include "ui/ProjectTreeData.hpp"
#include "ui/UIManager.hpp"
#include <editor/Project.hpp>
#include "editor/EditorManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/GfxObjectTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		
		
		//--------------------------------------------
		RemoveEntityGfxObjCmd::RemoveEntityGfxObjCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityGfxObjCmd::~RemoveEntityGfxObjCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityGfxObjCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			SmartPtr<GfxObjectTemplate> gfxObjectTemplate(new GfxObjectTemplate);
		
			String name;
			String type;
			String label;
			String mesh;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Type", type);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("Mesh", mesh);
		
			gfxObjectTemplate->setName(name);
			gfxObjectTemplate->setGfxType(type);
			gfxObjectTemplate->setLabel(label);
			gfxObjectTemplate->setMeshName(mesh);
		
			// if we already have the gfx object we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isGfxObject(name))
				m_parentEnt->getResourceSetTemplate()->removeGfxObject(name);
		
			// add gfx object to entity
			m_parentEnt->getResourceSetTemplate()->addGfxObject(gfxObjectTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityGfxObjCmd::redo()
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
			m_parentEnt->getResourceSetTemplate()->removeGfxObject(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityGfxObjCmd::execute()
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
		String RemoveEntityGfxObjCmd::getCommandId() const
		{
			return "RemoveEntityGfxObjCmd";
		}
		
		
	}
}
