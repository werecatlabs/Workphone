#include <GameEditorPCH.hpp>
#include "ui/AddEntityDialog.hpp"
#include "ui/wxGUIUtil.hpp"
#include "editor/Project.hpp"
#include "editor/EditorManager.hpp"
#include <wx/wx.hpp>

#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>

using namespace fb;

BEGIN_EVENT_TABLE(editor::AddEntityDialog, wxDialog)
	EVT_BUTTON(AddEntityDialog::CANCEL_BTN_ID, editor::AddEntityDialog::OnCancelBtn)
	EVT_PG_CHANGED(PropertiesId,  AddEntityDialog::OnPropertyChange)	
END_EVENT_TABLE()



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		AddEntityDialog::AddEntityDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Add new Entity"),
	               wxDefaultPosition, wxDefaultSize,
	               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			wxPropertyGridManager* pgman = new wxPropertyGridManager(this,
				PropertiesId,
				wxDefaultPosition,
				wxDefaultSize,
				wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED );
			m_properties = pgman->GetGrid();
			baseSizer->Add( pgman, 1, wxEXPAND );
			
			wxBoxSizer *optionsBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(optionsBox, 1, wxEXPAND);
	
			m_useDefaultsChkBox = new wxCheckBox(this, USE_DEFAULTS_CHK, "Use Defaults");
			m_useDefaultsChkBox->SetValue(false);
			optionsBox->Add(m_useDefaultsChkBox, 0, wxALIGN_CENTER_HORIZONTAL);
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox, 0, wxALIGN_CENTER_HORIZONTAL);
	
			m_okBtn = new wxButton(this, wxID_OK, "Ok");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, CANCEL_BTN_ID, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
		}
	
	
	
		AddEntityDialog::~AddEntityDialog()
		{

		}

		//--------------------------------------------
		String AddEntityDialog::getLabel() const
		{
			const Array<Property>& properties = m_propGrp.getPropertiesAsArray();
			for(u32 i=0; i<properties.size(); ++i)
			{
				const Property& prop = properties[i];	
	
				String label = prop.getLabel();
				if(label.length() == 0)
					label = prop.getName();
	
				if(label == "Name")
					return prop.getValue();
			}
	
			return "";
		}
	
	
	
		//--------------------------------------------
		void AddEntityDialog::OnCancelBtn( wxCommandEvent& event )
		{
			this->Close();
		}
	
	
	
		//--------------------------------------------
		bool AddEntityDialog::getUseDefaults() const
		{
			return m_useDefaultsChkBox->IsChecked();
		}
	
	
	
		//--------------------------------------------
		void AddEntityDialog::setProperties( const Properties& properties )
		{
			m_propGrp = properties;
			wxGUIUtil::populateProperties(m_propGrp, m_properties);
		}
	
	
	
		//--------------------------------------------
		void AddEntityDialog::getProperties( Properties& properties ) const
		{
			properties = m_propGrp;
		}
	
	
	
		//--------------------------------------------
		void AddEntityDialog::OnPropertyChange( wxPropertyGridEvent& event )
		{
			wxPGProperty* p = event.GetProperty();
			String propName = String(p->GetName().c_str());
	
			if(propName==("path"))
			{	
				ProjectPtr project = EditorManager::getSingletonPtr()->getProject();
				String relPath = String(p->GetValueAsString().c_str());
				String value = Path::getRelativePath(project->getProjectDirectory(), relPath).c_str();
				m_propGrp.setProperty(propName, value);
				p->SetValueFromString(value.c_str());
				return;
			}
	
			wxGUIUtil::setPropertyValue(m_propGrp, m_properties, p);
		}
	
	
	
	} // end namespace editor
	
	
}
