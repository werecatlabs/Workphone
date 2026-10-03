#include <GameEditorPCH.hpp>

#include "ui/ApplicationWindow.hpp"
#include "wx/treectrl.hpp"

#include "ui/NewComponentDialog.hpp"

#include "editor/EditorManager.hpp"
#include <editor/EntityTemplateMgr.hpp>
#include <editor/ComponentTemplateMgr.hpp>
#include <FBCore/FBCoreHeaders.hpp>




BEGIN_EVENT_TABLE(fb::editor::ApplicationWindow, wxScrolledWindow)
	EVT_BUTTON(fb::editor::ApplicationWindow::NEW_COMP_BTN_ID, fb::editor::ApplicationWindow::OnNewComponent)
END_EVENT_TABLE()

namespace fb
{
	
	namespace editor
	{
	
	
	
		ApplicationWindow::ApplicationWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
			: wxScrolledWindow(parent, id, pos, size, style)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			m_tree = new wxTreeCtrl(this, -1);
			baseSizer->Add(m_tree, 1, wxEXPAND);
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_newBtn = new wxButton(this, NEW_COMP_BTN_ID, "New");
			btnBox->Add(m_newBtn, 1, wxEXPAND);
	
			m_deleteBtn = new wxButton(this, DELETE_COMP_BTN_ID, "Delete");
			btnBox->Add(m_deleteBtn, 1, wxEXPAND);
	
			populateComponentTree();
		}
	
		ApplicationWindow::~ApplicationWindow()
		{
	
		}
	
		void ApplicationWindow::populateComponentTree()
		{
			m_tree->DeleteAllItems();
	
			wxTreeItemId rootId = m_tree->AddRoot("Components");
			
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			Array<SmartPtr<ComponentTemplate>> componentTempates = appRoot->getComponentTemplateMgr()->getComponents();
			for(u32 i=0; i<componentTempates.size(); ++i)
			{
				SmartPtr<ComponentTemplate> componentTemplate = componentTempates[i];
				m_tree->AppendItem(rootId, componentTemplate->getLabel().c_str());
			}
	
			m_tree->SortChildren(rootId);
		}
	
		void ApplicationWindow::OnNewComponent( wxCommandEvent& event )
		{
			EditComponentDialog dlg(this);
			if(dlg.ShowModal() == wxID_OK)
			{
				SmartPtr<ComponentTemplate> componentTemplate(new ComponentTemplate);
				componentTemplate->setLabel(dlg.getLabel());
				componentTemplate->setType(dlg.getType());
	
				auto editorManager = EditorManager::getSingletonPtr();
				auto componentTemplateMgr = editorManager->getComponentTemplateMgr();
				componentTemplateMgr->addComponent(componentTemplate);
	
				populateComponentTree();
			}
		}
	
		void ApplicationWindow::OnDeleteComponent( wxCommandEvent& event )
		{
	
		}
	
	
	
	} // end namespace editor
	
	
}
