#include <GameEditorPCH.hpp>
#include "AddEntityComponentCmd.hpp"
#include "editor/ComponentTemplateMgr.hpp"
#include "editor/EditorManager.hpp"
#include "editor/Project.hpp"
#include "editor/ProjectManager.hpp"
#include "ui/UIManager.hpp"
#include <FBObjectTemplates/EntityTemplate.hpp>



namespace fb
{
	
	namespace editor
	{
		
		
		
		//--------------------------------------------
		AddEntityComponentCmd::AddEntityComponentCmd(const Properties& propertyGroup, SmartPtr<EntityTemplate> parentEnt)
			: 
			m_parentEnt(parentEnt)
		{
			m_propertyGroup = propertyGroup;
			m_entityName = parentEnt->getName();
		}
		
		
		
		//--------------------------------------------
		AddEntityComponentCmd::~AddEntityComponentCmd()
		{
		}
		
		
		
		//--------------------------------------------
		void AddEntityComponentCmd::undo()
		{
			// get entity, it may be changed
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			ProjectPtr project = appRoot->getProject();
			m_parentEnt = project->getEntity(m_entityName);
			if(m_parentEnt == nullptr)
				return;
	
			String type;
			m_propertyGroup.getPropertyValue("Type", type);
			//m_parentEnt->remove(m_parentEnt->findComponentTemplate(type));
	
			//editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityComponentCmd::redo()
		{
			//// get entity, it may be changed
			//editor::ApplicationManager* appRoot = editor::IApplicationManager::instance();
			//editor::ProjectPtr project = appRoot->getProject();
			//m_parentEnt = project->getEntity(m_entityName);
			//if(m_parentEnt == nullptr)
			//	return;
		
			//String type;
			//m_propertyGroup.getPropertyValue("Type", type);
		
			//ComponentTemplateMgrPtr componentTemplateMgr = ApplicationManager::getSingletonPtr()->getComponentTemplateMgr();
			//SmartPtr<ComponentTemplate> parentCompTemplate = componentTemplateMgr->getTemplateByType(type);	
			//SmartPtr<ComponentTemplate> compTemplate = parentCompTemplate->clone();
			//
			//if(m_parentEnt->findComponentTemplate(type) == nullptr)
			//{
			//	// add component to entity
			//	m_parentEnt->add(compTemplate);
			//}
	
			////editor::AppRoot* appRoot = editor::AppRoot::getSingletonPtr();
			//appRoot->getGUIManager()->rebuildProjectTree();
		}
		
		
		
		//--------------------------------------------
		void AddEntityComponentCmd::execute()
		{
			FB_ASSERT_TRUE( !m_parentEnt )
		
			String type;
			m_propertyGroup.getPropertyValue("Type", type);
		
			ComponentTemplateMgrPtr componentTemplateMgr = EditorManager::getSingletonPtr()->getComponentTemplateMgr();
			SmartPtr<ComponentTemplate> parentCompTemplate = componentTemplateMgr->getTemplateByType(type);	
			SmartPtr<ComponentTemplate> compTemplate;// = parentCompTemplate->clone();
		
			if(m_parentEnt->findComponentTemplate(type) == nullptr)
			{
				// add component to entity
				m_parentEnt->add(compTemplate);
			}
	
			editor::EditorManager* appRoot = editor::EditorManager::getSingletonPtr();
			appRoot->getUI()->rebuildSceneTree();
		}
		
		
		
		//--------------------------------------------
		String AddEntityComponentCmd::getCommandId() const
		{
			return "AddEntityComponentCmd";
		}
		
		
	}
}
