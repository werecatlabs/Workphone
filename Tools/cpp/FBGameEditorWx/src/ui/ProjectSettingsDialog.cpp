#include <GameEditorPCH.hpp>
#include <ui/ProjectSettingsDialog.hpp>
#include <editor/EditorManager.hpp>
#include <editor/EditorMessages.hpp>
#include <FBObjectTemplates/EntityTemplate.hpp>
#include <FBObjectTemplates/EventTemplate.hpp>
#include <FBObjectTemplates/FSMTemplate.hpp>
#include <FBObjectTemplates/PhysicsBodyTemplate.hpp>
#include <wx/wx.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		ProjectSettingsDialog::ProjectSettingsDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Create Physics Body"),
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
	
			wxBoxSizer *typesBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(typesBox, 0, wxEXPAND);
			m_typeTxt = new wxComboBox(this, -1);
			typesBox->Add( new wxStaticText( this, -1, wxT("Type:") ));
			typesBox->Add(m_typeTxt, 1, wxEXPAND);
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Label:") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			// add existing components in combo box
			populateTypeCombo();
		}
	
	
	
	
		ProjectSettingsDialog::~ProjectSettingsDialog()
		{
			if(m_labelTxt)
				delete m_labelTxt;
			if(m_nameTxt)
				delete m_nameTxt;
			if(m_typeTxt)
				delete m_typeTxt;
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
		String ProjectSettingsDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		String ProjectSettingsDialog::getName() const
		{
			return String(m_nameTxt->GetValue().c_str());
		}
	
	
	
		String ProjectSettingsDialog::getType() const
		{
			return String(m_typeTxt->GetValue().c_str());
		}
	
	
	
		void ProjectSettingsDialog::populateTypeCombo()
		{
			m_typeTxt->Clear();
	
			m_typeTxt->Append(PhysicsBodyTemplate::RIGID_BODY_2_TYPE.c_str());
			m_typeTxt->Append(PhysicsBodyTemplate::RIGID_BODY_3_TYPE.c_str());
			m_typeTxt->Append(PhysicsBodyTemplate::SOFT_BODY_2_TYPE.c_str());
			m_typeTxt->Append(PhysicsBodyTemplate::SOFT_BODY_3_TYPE.c_str());
	
			// select first item
			m_typeTxt->Select(0);
	
		}
	
	
	
	} // end namespace editor	
} // end namespace fb


