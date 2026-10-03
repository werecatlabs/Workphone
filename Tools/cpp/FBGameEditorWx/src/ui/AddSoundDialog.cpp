#include <GameEditorPCH.hpp>
#include "ui/AddSoundDialog.hpp"
#include "editor/EditorManager.hpp"
#include "editor/EditorMessages.hpp"
#include <FBCore/Interface/System/IEditableObject.hpp>


namespace fb
{
	
	namespace editor
	{
	
	
	
		AddSoundDialog::AddSoundDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Create Sound Object"),
			wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			wxBoxSizer *classBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(classBox, 0, wxEXPAND);
			m_nameTxt = new wxTextCtrl(this, -1);
			classBox->Add( new wxStaticText( this, -1, wxT("Name:") ));
			classBox->Add(m_nameTxt, 1, wxEXPAND);
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Label:") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
			wxBoxSizer *functionBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(functionBox, 0, wxEXPAND);
			m_SoundNameTxt = new wxTextCtrl(this, -1);
			functionBox->Add( new wxStaticText( this, -1, wxT("Sound Name:") ));
			functionBox->Add(m_SoundNameTxt, 1, wxEXPAND);
	
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
		}
	
	
	
	
		AddSoundDialog::~AddSoundDialog()
		{
			if(m_labelTxt)
				delete m_labelTxt;
			if(m_nameTxt)
				delete m_nameTxt;
			if(m_SoundNameTxt)
				delete m_SoundNameTxt;
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
		String AddSoundDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		String AddSoundDialog::getName() const
		{
			return String(m_nameTxt->GetValue().c_str());
		}
	
	
	
		String AddSoundDialog::getSoundName() const
		{
			return String(m_SoundNameTxt->GetValue().c_str());
		}
	
	
	
	
	
	} // end namespace editor
	
}

