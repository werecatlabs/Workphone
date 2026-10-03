#include <GameEditorPCH.hpp>
#include "ui/AddGfxObjDialog.hpp"

#include "editor/EditorManager.hpp"
#include "editor/EditorMessages.hpp"
#include <FBCore/FBCoreHeaders.hpp>



namespace fb
{
	
	namespace editor
	{
	
	
	
		AddGfxObjDialog::AddGfxObjDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Create Graphical Object"),
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
	
			wxBoxSizer *functionBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(functionBox, 0, wxEXPAND);
			m_meshTxt = new wxTextCtrl(this, -1);
			functionBox->Add( new wxStaticText( this, -1, wxT("Mesh:") ));
			functionBox->Add(m_meshTxt, 1, wxEXPAND);
	
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			// add existing components in combo box
			populateTypeCombo();
		}
	
	
	
	
		AddGfxObjDialog::~AddGfxObjDialog()
		{
			if(m_labelTxt)
				delete m_labelTxt;
			if(m_nameTxt)
				delete m_nameTxt;
			if(m_meshTxt)
				delete m_meshTxt;
			if(m_typeTxt)
				delete m_typeTxt;
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
		String AddGfxObjDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		String AddGfxObjDialog::getName() const
		{
			return String(m_nameTxt->GetValue().c_str());
		}
	
	
	
		String AddGfxObjDialog::getMesh() const
		{
			return String(m_meshTxt->GetValue().c_str());
		}
	
	
	
		String AddGfxObjDialog::getType() const
		{
			return String(m_typeTxt->GetValue().c_str());
		}
	
	
	
		void AddGfxObjDialog::populateTypeCombo()
		{
			m_typeTxt->Clear();
	
			m_typeTxt->Append(GfxObjectTemplate::GFX_OBJ_TYPE_MESH.c_str());
			m_typeTxt->Append(GfxObjectTemplate::GFX_OBJ_TYPE_LIGHT.c_str());
			m_typeTxt->Append(GfxObjectTemplate::GFX_OBJ_TYPE_DYNAMIC_MESH.c_str());
			m_typeTxt->Append(GfxObjectTemplate::GFX_OBJ_TYPE_LINES.c_str());
			m_typeTxt->Append(GfxObjectTemplate::GFX_OBJ_TYPE_QUADS.c_str());
	
			// select first item
			m_typeTxt->Select(0);
	
		}
	
	
	
	} // end namespace editor
	
}

