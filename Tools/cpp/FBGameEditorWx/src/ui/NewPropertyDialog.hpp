#ifndef NewPropertyDialog_h__
#define NewPropertyDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>

#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>

#include "core/IMessageListener.hpp"

#include "ui/PropertiesWindow.hpp"



namespace fb
{
	
	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class EditPropertyDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			EditPropertyDialog(wxWindow *parent);
			~EditPropertyDialog();
	
			String getLabel() const;
			String getName() const;
			String getValue() const;
			String getType() const;
			bool getReadOnly() const;
	
			void populateTypeCombo();
			//void onSelect(wxCommandEvent& event);
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_valueTxt;
			wxTextCtrl* m_nameTxt;
			wxComboBox* m_type;
			wxCheckBox* m_readOnly;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
		};
	
	
	
	} // end namespace editor
	
}


#endif // NewPropertyDialog_h__