#ifndef AddGfxObjDialog_h__
#define AddGfxObjDialog_h__




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
		class AddGfxObjDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			AddGfxObjDialog(wxWindow *parent);
			~AddGfxObjDialog();
	
			String getLabel() const;
			String getName() const;
			String getMesh() const;
			String getType() const;
	
			void populateTypeCombo();
	
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_nameTxt;
			wxTextCtrl* m_meshTxt;
			wxComboBox* m_typeTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
		};
	
	
	
	} // end namespace editor
}



#endif // AddGfxObjDialog_h__