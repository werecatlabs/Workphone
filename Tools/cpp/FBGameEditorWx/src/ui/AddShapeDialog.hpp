#ifndef AddShapeDialog_h__
#define AddShapeDialog_h__



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
		class AddShapeDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			AddShapeDialog(wxWindow *parent);
			~AddShapeDialog();
	
			String getLabel() const;
			String getName() const;
			String getType() const;
	
			void populateTypeCombo();
	
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_nameTxt;
			wxComboBox* m_typeTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
		};
	
	
	
	} // end namespace editor
	
}


#endif // AddShapeDialog_h__