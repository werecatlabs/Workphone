#include <GameEditorPCH.hpp>
#include "ui/AddNamedDialog.hpp"



BEGIN_EVENT_TABLE(fb::editor::AddNamedDialog, wxDialog)
	EVT_BUTTON(AddNamedDialog::CANCEL_BTN_ID, editor::AddNamedDialog::OnCancelBtn)
END_EVENT_TABLE()




namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		AddNamedDialog::AddNamedDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T(""),
	               wxDefaultPosition, wxDefaultSize,
	               wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Name: ") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
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
	
	
	
		//--------------------------------------------
		String AddNamedDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		//--------------------------------------------
		void AddNamedDialog::OnCancelBtn( wxCommandEvent& event )
		{
			this->Close();
		}
	
	
	
		//--------------------------------------------
		bool AddNamedDialog::getUseDefaults() const
		{
			return m_useDefaultsChkBox->IsChecked();
		}



		//--------------------------------------------
		String AddNamedDialog::getObjectType() const
		{
			return m_objectType;
		}



		//--------------------------------------------
		void AddNamedDialog::setObjectType( const String& val )
		{
			m_objectType = val;
		}


		
	} // end namespace editor
} // end namespace fb


