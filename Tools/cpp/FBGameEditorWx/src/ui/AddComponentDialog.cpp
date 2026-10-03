#include <GameEditorPCH.hpp>
#include "editor/EditorManager.hpp"
#include "ui/AddComponentDialog.hpp"
#include "NewComponentDialog.hpp"
#include "ui/ProjectTreeData.hpp"
#include "editor/ComponentTemplateMgr.hpp"
#include "editor/EditorManager.hpp"
#include <FBCore/FBCoreHeaders.hpp>

#include <wx/treectrl.hpp>



BEGIN_EVENT_TABLE(fb::editor::AddComponentDialog, wxDialog)
	EVT_TREE_SEL_CHANGED(fb::editor::AddComponentDialog::COMPONENT_TREE_ID, editor::AddComponentDialog::OnSelChanged)
	EVT_TREE_ITEM_MENU(fb::editor::AddComponentDialog::COMPONENT_TREE_ID, editor::AddComponentDialog::OnContextMenu)
END_EVENT_TABLE()

namespace fb
{
	namespace editor
	{
	
	
	
		//--------------------------------------------
		AddComponentDialog::AddComponentDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Add Component"),
	               wxDefaultPosition, wxDefaultSize,
	               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			m_tree = new wxTreeCtrl(this, COMPONENT_TREE_ID, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_MULTIPLE|wxTR_TWIST_BUTTONS);
			baseSizer->Add(m_tree, 1, wxEXPAND);
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Ok");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			m_componentMenu = new wxMenu();
			m_componentMenu->Append( PW_COMP_ADD_MODIF, "Modify Component" );
			m_componentMenu->AppendSeparator();
			m_componentMenu->Append( PW_COMP_CANCEL, "Cancel" );
	
			m_labelComponentMenu = new wxMenu();
			m_labelComponentMenu->Append( PW_COMP_ADD_MODIF, "Add Component" );
			m_labelComponentMenu->AppendSeparator();
			m_labelComponentMenu->Append( PW_COMP_CANCEL, "Cancel" );
	
			m_defaultMenu = new wxMenu();
			m_defaultMenu->Append( PW_COMP_CANCEL, "Cancel"  );
	
			Connect(PW_COMP_ADD_MODIF, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(AddComponentDialog::onAddModifyComponent));
	
			populateComponentTree();
		}
	
	
	
		AddComponentDialog::~AddComponentDialog()
		{

		}

		//--------------------------------------------
		String AddComponentDialog::getType() const
		{
			return m_componentType;
		}
	
	
	
		fb::Array<fb::String> AddComponentDialog::getComponents() const
		{
			return m_components;
		}

		//--------------------------------------------
		void AddComponentDialog::populateComponentTree()
		{
			m_tree->DeleteAllItems();
	
			ProjectTreeData labelData(m_labelComponentMenu, "label", "label", NULL, NULL);
			wxTreeItemId rootId = m_tree->AddRoot("Components", -1, -1, new ProjectTreeData(labelData));
			
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			Array<SmartPtr<ComponentTemplate>> componentTempates = appRoot->getComponentTemplateMgr()->getComponents();
			for(u32 i=0; i<componentTempates.size(); ++i)
			{
				SmartPtr<ComponentTemplate> componentTemplate = componentTempates[i];
				ProjectTreeData data(m_componentMenu, "component", "component", componentTemplate, componentTemplate);
				m_tree->AppendItem(rootId, componentTemplate->getLabel().c_str(), -1, -1, new ProjectTreeData(data));
			}
	
			m_tree->SortChildren(rootId);
		}
	
	
	
		//--------------------------------------------
		void AddComponentDialog::OnSelChanged( wxTreeEvent& event )
		{
			wxArrayTreeItemIds selection;
			u32 numSelected = m_tree->GetSelections(selection);
			if(numSelected == 0)
				return;
	
			m_components.clear();
	
			for(u32 i=0; i<numSelected; ++i)
			{
				ProjectTreeData* data = (ProjectTreeData*)m_tree->GetItemData(selection[i]);
				if(data)
				{
					if(data->getObjectType()==("component"))
					{
						SmartPtr<ComponentTemplate> componentTemplate;// = (ComponentTemplate*)data->getObjectData();
						m_components.push_back(componentTemplate->getType());
					}
				}
			}
		}
	
	
	
		//--------------------------------------------
		void AddComponentDialog::OnContextMenu( wxTreeEvent &event )
		{		
			wxTreeItemId itemId = event.GetItem();
			ProjectTreeData* data = (ProjectTreeData*)m_tree->GetItemData(itemId);
			if(data != NULL)
			{
				wxMenu* menu = data->getContextMenu();
				if(menu)
					m_tree->PopupMenu(menu);
				else
					m_tree->PopupMenu(m_defaultMenu);
			}
			else
			{
				m_tree->PopupMenu(m_defaultMenu);
			}
	
			event.Skip();
		}
	
	
	
		//--------------------------------------------
		void AddComponentDialog::onAddModifyComponent(wxCommandEvent& WXUNUSED(event))
		{
			EditComponentDialog dlg(this);
			if(dlg.ShowModal() == wxID_OK)
			{
				SmartPtr<ComponentTemplate> componentTemplate(new ComponentTemplate);
				componentTemplate->setLabel(dlg.getLabel());
				componentTemplate->setType(dlg.getType());
	
				EditorManager* appRoot = EditorManager::getSingletonPtr();
				if(!appRoot->getComponentTemplateMgr()->isExistingComponent(dlg.getLabel()))
					appRoot->getComponentTemplateMgr()->addComponent(componentTemplate);
				/*else
				{
					wxMessageDialog *dial = new wxMessageDialog(NULL, 
						wxT("A component with this label already exist"), wxT("Duplicate"), wxOK | wxICON_INFORMATION);
					dial->ShowModal();
				}*/
	
				populateComponentTree();
			}
		}
	
	
	
	} // end namespace editor
	
	
}
