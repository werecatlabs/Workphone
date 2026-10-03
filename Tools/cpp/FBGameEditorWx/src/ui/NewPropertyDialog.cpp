#include <GameEditorPCH.hpp>
#include "ui/NewPropertyDialog.hpp"

#include "editor/EditorManager.hpp"
#include "editor/EditorMessages.hpp"
#include "editor/EditorManager.hpp"
#include <FBCore/FBCoreHeaders.hpp>



namespace fb
{
	
	namespace editor
	{
	
	
	
		EditPropertyDialog::EditPropertyDialog( wxWindow *parent )
			: wxDialog(parent, wxID_ANY, _T("Create Property"),
			wxDefaultPosition, wxDefaultSize,
			wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxVERTICAL );
			SetSizer(baseSizer);
	
			wxBoxSizer *compDataBox = new wxBoxSizer( wxVERTICAL );
			baseSizer->Add(compDataBox, 1, wxEXPAND);
	
			wxBoxSizer *nameBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(nameBox, 0, wxEXPAND);
			m_nameTxt = new wxTextCtrl(this, -1);
			nameBox->Add( new wxStaticText( this, -1, wxT("Name:") ));
			nameBox->Add(m_nameTxt, 1, wxEXPAND);
	
			wxBoxSizer *typesBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(typesBox, 0, wxEXPAND);
			m_type = new wxComboBox(this, -1);
			typesBox->Add( new wxStaticText( this, -1, wxT("Type:") ));
			typesBox->Add(m_type, 1, wxEXPAND);
	
			wxBoxSizer *labelBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(labelBox, 0, wxEXPAND);
			m_labelTxt = new wxTextCtrl(this, -1);
			labelBox->Add( new wxStaticText( this, -1, wxT("Label:") ));
			labelBox->Add(m_labelTxt, 1, wxEXPAND);
	
			wxBoxSizer *valueBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(valueBox, 0, wxEXPAND);
			m_valueTxt = new wxTextCtrl(this, -1);
			valueBox->Add( new wxStaticText( this, -1, wxT("Value:") ));
			valueBox->Add(m_valueTxt, 1, wxEXPAND);
	
			wxBoxSizer *readOnlyBox = new wxBoxSizer( wxHORIZONTAL );
			compDataBox->Add(readOnlyBox, 0, wxEXPAND);
			m_readOnly = new wxCheckBox(this, -1, wxT("Read only"));
			//readOnlyBox->Add( new wxStaticText( this, -1, wxT("Read only:") ));
			readOnlyBox->Add(m_readOnly, 1, wxEXPAND);
	
	
			wxBoxSizer *btnBox = new wxBoxSizer( wxHORIZONTAL );
			baseSizer->Add(btnBox);
	
			m_okBtn = new wxButton(this, wxID_OK, "Save");
			btnBox->Add(m_okBtn, 1, wxEXPAND);
	
			m_cancelBtn = new wxButton(this, wxID_CANCEL, "Cancel");
			btnBox->Add(m_cancelBtn, 1, wxEXPAND);
	
			//Connect(wxEVT_COMMAND_COMBOBOX_SELECTED, 
			//	wxCommandEventHandler(EditComponentDialog::onSelect));
	
	
			// add existing components in combo box
			populateTypeCombo();
		}
	
	
	
	
		EditPropertyDialog::~EditPropertyDialog()
		{
			if(m_readOnly)
				delete m_readOnly;
			if(m_labelTxt)
				delete m_labelTxt;
			if(m_nameTxt)
				delete m_nameTxt;
			if(m_valueTxt)
				delete m_valueTxt;
			if(m_type)
				delete m_type;
			if(m_cancelBtn)
				delete m_cancelBtn;
			if(m_okBtn)
				delete m_okBtn;
		}
	
	
	
	
		//void EditComponentDialog::onSelect(wxCommandEvent& event) 
		//{
		//	int sel = m_components->GetSelection();
		//	if(sel == wxNOT_FOUND)
		//		return;
	
		//	// get pointer relative to this record
		//	SmartPtr<ComponentTemplate> componentTemplate = (ComponentTemplate*)(m_components->GetClientData(sel));
	
		//	if(!componentTemplate)
		//		return;
	
		//	String componentType = componentTemplate->getType();
		//	String componentLabel = componentTemplate->getLabel();
	
		//	// get properties
		//	ComponentItemSelectedPtr msg(new ComponentItemSelected);
		//	SmartPtr<IEditableObject> obj = (IEditableObject*)componentTemplate.get();
		//	msg->setSelectedObject(obj);
		//	m_propertiesWindow->handleMessage(msg);
	
		//	// set the label text control
		//	m_labelTxt->setValue(componentLabel.c_str());
	
		//	// set the type text control
		//	m_typeTxt->setValue(componentType.c_str());
		//}
	
	
	
		bool EditPropertyDialog::getReadOnly() const
		{
			return m_readOnly->GetValue();
		}
	
		String EditPropertyDialog::getLabel() const
		{
			return String(m_labelTxt->GetValue().c_str());
		}
	
	
	
		String EditPropertyDialog::getName() const
		{
			return String(m_nameTxt->GetValue().c_str());
		}
	
	
	
		String EditPropertyDialog::getValue() const
		{
			return String(m_valueTxt->GetValue().c_str());
		}
	
	
	
		String EditPropertyDialog::getType() const
		{
			return String(m_type->GetValue().c_str());
		}
	
	
	
	
		void EditPropertyDialog::populateTypeCombo()
		{
			m_type->Clear();
	
			m_type->Append("string");
			m_type->Append("file");
			m_type->Append("folder");
			m_type->Append("integer");
			m_type->Append("float");
			m_type->Append("bool");
			m_type->Append("enum");
			m_type->Append("choices");
			m_type->Append("list");
			m_type->Append("Array");
			m_type->Append("dirs");
			m_type->Append("folders");
			m_type->Append("MultiChoice");
			m_type->Append("vector2");
			m_type->Append("vector2d");
			m_type->Append("vector3");
			m_type->Append("vector3d");
			m_type->Append("colourf");
			m_type->Append("colour");
			m_type->Append("colouri");
	
			// select first item
			m_type->Select(0);
			
		}
	
	
	
	} // end namespace editor
	
	
}
