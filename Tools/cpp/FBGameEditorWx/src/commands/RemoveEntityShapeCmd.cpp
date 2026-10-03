#include <GameEditorPCH.hpp>
#include "RemoveEntityShapeCmd.hpp"
#include "editor/EditorManager.hpp"
#include <editor/Project.hpp>
#include <ui/UIManager.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplateContainer.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/FSMTemplateContainer.hpp>
#include <FBObjectTemplates/CollisionShapeTemplate.hpp>


namespace fb
{	
	namespace editor
	{
		

		
		//--------------------------------------------
		RemoveEntityShapeCmd::RemoveEntityShapeCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: 
		m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		RemoveEntityShapeCmd::~RemoveEntityShapeCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityShapeCmd::undo()
		{
			// get entity, it may be changed
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			editor::ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
		
			CollisionShapeTemplatePtr shapeTemplate(new CollisionShapeTemplate);
		
			String name;
			String label;
			String type;
		
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_propertyGroup.getPropertyValue("Type", type);
		
			shapeTemplate->setName(name);
			shapeTemplate->setLabel(label);
			shapeTemplate->setShapeType(type);
		
			// if we already have the shape we delete it before added again
			if(m_parentEnt->getResourceSetTemplate()->isShape(name))
				m_parentEnt->getResourceSetTemplate()->removeShape(name);
		
			// add shape to entity
			m_parentEnt->getResourceSetTemplate()->addShape(shapeTemplate);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void RemoveEntityShapeCmd::redo()
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
			m_parentEnt->getResourceSetTemplate()->removeShape(name);
		
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//-----\\\----------------------------------------------------------------------------------------------
		void RemoveEntityShapeCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String name;
			String label;
			m_propertyGroup.getPropertyValue("Name", name);
			m_propertyGroup.getPropertyValue("Label", label);
			m_parentEnt->getResourceSetTemplate()->removeShape(name);
		
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String RemoveEntityShapeCmd::getCommandId() const
		{
			return "RemoveEntityShapeCmd";
		}
		

		
	} // end namespace editor
} // end namespace fb


