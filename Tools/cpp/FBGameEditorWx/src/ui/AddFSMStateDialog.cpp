#include <GameEditorPCH.hpp>
#include "ui/AddFSMStateDialog.hpp"
#include "editor/EditorManager.hpp"
#include "editor/EditorMessages.hpp"
#include "editor/EditorManager.hpp"
#include <FBCore/FBCoreHeaders.hpp>


namespace fb
{
	
	namespace editor
	{
	
	
	
		NewFSMStateDialog::NewFSMStateDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Create State"),
			wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			m_label = "";
			/*m_class = "";
			m_function = "";
			m_type = "";*/
	
	
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			/*wxBoxSizer *classBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(classBox, 0, wxEXPAND);
			m_classTxt = new wxTextCtrl(this, -1);
			classBox->Add( new wxStaticText( this, -1, wxT("Class:") ));
			classBox->Add(m_classTxt, 1, wxEXPAND);
	
			wxBoxSizer *typesBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(typesBox, 0, wxEXPAND);
			m_typeTxt = new wxTextCtrl(this, -1);
			typesBox->Add( new wxStaticText( this, -1, wxT("Type:") ));
			typesBox->Add(m_typeTxt, 1, wxEXPAND);*/
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Label:") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
			/*wxBoxSizer *functionBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(functionBox, 0, wxEXPAND);
			m_functionTxt = new wxTextCtrl(this, -1);
			functionBox->Add( new wxStaticText( this, -1, wxT("Function:") ));
			functionBox->Add(m_functionTxt, 1, wxEXPAND);*/
	
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			// put the values in controls
			m_labelTxt->SetValue(m_label.c_str());
			/*m_classTxt->setValue(m_class.c_str());
			m_functionTxt->setValue(m_function.c_str());
			m_typeTxt->setValue(m_type.c_str());*/
		}
	
	
	
	
		NewFSMStateDialog::~NewFSMStateDialog()
		{
			if(m_labelTxt)
				delete m_labelTxt;
			/*if(m_classTxt)
				delete m_classTxt;
			if(m_functionTxt)
				delete m_functionTxt;
			if(m_typeTxt)
				delete m_typeTxt;*/
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
		String NewFSMStateDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}

		void NewFSMStateDialog::setLabelValue(String val)
		{
			m_label = val; 
			m_labelTxt->SetValue(m_label.c_str());
		}


	
	
		/*fb::String EditEventDialog::getClass() const
		{
			return m_classTxt->GetValue().c_str();
		}
	
	
	
		fb::String EditEventDialog::getFunction() const
		{
			return m_functionTxt->GetValue().c_str();
		}
	
	
	
		fb::String EditEventDialog::getType() const
		{
			return m_typeTxt->GetValue().c_str();
		}*/
	
	
	
	} // end namespace editor
	
	
}
