#ifndef NewEventDialog_h__
#define NewEventDialog_h__



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
		class EditEventDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
			EditEventDialog(wxWindow *parent);
			~EditEventDialog();
	
			// put the values in controls
			void setLabelValue(String val);
			void setClassValue(String val);
			void setFunctionValue(String val);
			void setTypeValue(String val);
	
			String getLabel() const;
			String getClass() const;
			String getFunction() const;
			String getType() const;
	
		
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_classTxt;
			wxTextCtrl* m_functionTxt;
			wxTextCtrl* m_typeTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			String m_label;
			String m_class;
			String m_function;
			String m_type;
		};
	
	
	
	} // end namespace editor	
}



#endif // NewEventDialog_h__


