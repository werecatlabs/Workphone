#include <GameEditorPCH.hpp>

#include "ui/ComponentsWindow.hpp"


#include "ui/NewComponentDialog.hpp"

#include "editor/EditorManager.hpp"
#include "editor/ComponentTemplateMgr.hpp"
#include <FBObjectTemplates/ComponentTemplate.hpp>
#include <FBCore/Interface/System/IMessage.hpp>
#include "wx/treectrl.hpp"


#define TREE_ITEM_LABEL	0	
#define TREE_ITEM_COMPONENT	1
#define TREE_ITEM_EVENT	2



namespace fb
{
	namespace editor
	{
	
	
	
		//-------------------------------------------------
		ComponentsWindow::ComponentsWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
		{
			auto window = new wxScrolledWindow(parent, id, pos, size, style);
			setWindow(window);

			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			window->SetSizer(baseSizer);
	
			m_tree = new wxTreeCtrl(window, -1);
			baseSizer->Add(m_tree, 1, wxEXPAND);
	
			//wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			//baseSizer->Add(btnBox);
	
			//m_newBtn = new wxButton(this, NEW_COMP_BTN_ID, "Add/Modify");
			//btnBox->Add(m_newBtn, 1, wxEXPAND);
	
			//m_deleteBtn = new wxButton(this, DELETE_COMP_BTN_ID, "Delete");
			//btnBox->Add(m_deleteBtn, 1, wxEXPAND);
	
			//m_tree->Connect(wxEVT_RIGHT_DOWN,
			//	wxMouseEventHandler(ComponentsWindow::onTreeRightMouseBtnUp), NULL, this);
	
			//Connect(ID_POPMENU_ADD_COMPONENT, wxEVT_COMMAND_MENU_SELECTED, 
			//	wxCommandEventHandler(ComponentsWindow::OnAddComponent));
			//
			//Connect(ID_POPMENU_EDIT_COMPONENT, wxEVT_COMMAND_MENU_SELECTED, 
			//	wxCommandEventHandler(ComponentsWindow::OnEditComponent));
	
			//Connect(ID_POPMENU_REMOVE_COMPONENT, wxEVT_COMMAND_MENU_SELECTED, 
			//	wxCommandEventHandler(ComponentsWindow::OnDeleteComponent));
	
			populateComponentTree();
		}
	
	
	
		//-------------------------------------------------
		ComponentsWindow::~ComponentsWindow()
		{
		}
	
	
	
		//-------------------------------------------------
		void ComponentsWindow::populateComponentTree()
		{
			m_tree->DeleteAllItems();
	
			wxTreeItemId componentsId = m_tree->AddRoot("Components");
			
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			Array<SmartPtr<ComponentTemplate>> componentTempates = appRoot->getComponentTemplateMgr()->getComponents();
			for(u32 i=0; i<componentTempates.size(); ++i)
			{
				SmartPtr<ComponentTemplate> componentTemplate = componentTempates[i];
				String componentType = componentTemplate->getLabel();
				if(componentType.length() != 0)
				{
					ComponentTreeData* data = new ComponentTreeData;
					data->templateType = componentTemplate->getType();
					wxTreeItemId componentId = m_tree->AppendItem(componentsId, componentType.c_str(), -1, -1, data);
	
					const Array<SmartPtr<EventTemplate>>& events = componentTemplate->getEventTemplates();
					for(u32 eventIdx=0; eventIdx<events.size(); ++eventIdx)
					{
						const SmartPtr<EventTemplate>& eventTemplate = events[eventIdx];
						m_tree->AppendItem(componentId, eventTemplate->getLabel().c_str());
					}
				}
			}
	
			m_tree->SortChildren(componentsId);
	
			// expand tree
			m_tree->Expand(m_tree->GetRootItem());
		}
	
	
	
		//-------------------------------------------------
		void ComponentsWindow::OnAddComponent( wxCommandEvent& event )
		{
			//EditComponentDialog dlg(this, false);
			//if(dlg.ShowModal() == wxID_OK)
			//{
			//	SmartPtr<ComponentTemplate> componentTemplate = dlg.getComponentTemplate();
			//	if(!componentTemplate)
			//		return;
	
			//	componentTemplate->setLabel(dlg.getLabel());
			//	componentTemplate->setType(dlg.getType());
	
			//	EditorManager* appRoot = EditorManager::getSingletonPtr();
			//	ComponentTemplateMgrPtr componentTemplateMgr = appRoot->getComponentTemplateMgr();
	
			//	if(!componentTemplateMgr->isExistingComponent(dlg.getLabel()))
			//	{
			//		componentTemplateMgr->addComponent(componentTemplate);
			//	}
			//	else
			//	{
			//		wxMessageDialog *dial = new wxMessageDialog(NULL, 
			//			wxT("A component with this label already exist"), wxT("Duplicate"), wxOK | wxICON_INFORMATION);
			//		dial->ShowModal();
			//	}
	
			//	populateComponentTree();
			//}
		}
	
	
	
		//-------------------------------------------------
		void ComponentsWindow::OnEditComponent( wxCommandEvent& event )
		{
			//EditComponentDialog dlg(this);
	
			//EditorManager* appRoot = EditorManager::getSingletonPtr();
			//ComponentTemplateMgrPtr componentTemplateMgr = appRoot->getComponentTemplateMgr();
	
			//wxTreeItemId selection = m_tree->GetSelection();
			//ComponentTreeData* data = (ComponentTreeData*)m_tree->GetItemData(selection);
			//SmartPtr<ComponentTemplate> componentTemplate = componentTemplateMgr->getTemplateByType(data->templateType);
			//if(!componentTemplate)
			//	return;
	
			//dlg.setComponentTemplate(componentTemplate);
	
			//if(dlg.ShowModal() == wxID_OK)
			//{
			//	componentTemplateMgr->componentModified(componentTemplate);
			//}
		}
	
	
	
		//-------------------------------------------------
		void ComponentsWindow::OnDeleteComponent( wxCommandEvent& event )
		{
			// get selected item
			wxTreeItemId item = m_tree->GetSelection();
			if(item.IsOk())
			{
				// get item name
				String componentName = String(m_tree->GetItemText(item));
	
				// delete component
				EditorManager* appRoot = EditorManager::getSingletonPtr();
				appRoot->getComponentTemplateMgr()->deleteComponent(componentName);
	
				// refresh tree
				populateComponentTree();
			}
		}
	
	
	
		//-------------------------------------------------
		void ComponentsWindow::onTreeRightMouseBtnUp(wxMouseEvent& event)
		{
			wxTreeItemId item = m_tree->HitTest(event.GetPosition());
	
			// select tree item
			if(item.IsOk())
				m_tree->SelectItem(item);
			/*else
				m_tree->Select(-1);*/
	
			event.Skip();
	
			// show menu only for components or for root item
			if(item.IsOk())
			{
				// get item name
				String componentName = String(m_tree->GetItemText(item));
	
				// delete component
				EditorManager* appRoot = EditorManager::getSingletonPtr();
				if(item == m_tree->GetRootItem() || appRoot->getComponentTemplateMgr()->isExistingComponent(componentName))
				{
					wxMenu *menu;
					wxPoint point;
	
					// get mouse position
					point.x = wxGetMousePosition().x;
					point.y = wxGetMousePosition().y;
					//point = this->ScreenToClient(point);
	
					// to get a window location if required use
					//int id = wxFindWindowAtPoint(point);
	
					// create menu 
					menu = new wxMenu();
	
					// add stuff
					menu->Append(ID_POPMENU_ADD_COMPONENT, wxT("Add Component"), wxT(""));
					menu->Append(ID_POPMENU_EDIT_COMPONENT, wxT("Edit Component"), wxT(""));
					menu->Append(ID_POPMENU_REMOVE_COMPONENT, wxT("Remove Component"), wxT(""));
					menu->AppendSeparator();
					menu->Append(-1, wxT("Cancel"), wxT(""));
	
					// and then display
					//PopupMenu(menu, point.x, point.y);
	
					if(menu)
						delete menu;
				}
	
			}
		}
	
	
	
	} // end namespace editor
	
	
}
