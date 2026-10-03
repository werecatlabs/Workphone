#ifndef NewFSMStateDialog_h__
#define NewFSMStateDialog_h__




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
		class NewFSMStateDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
			NewFSMStateDialog(wxWindow *parent);
			~NewFSMStateDialog();
	
			// put the values in controls
			void setLabelValue(String val);
			/*inline void setClassValue(String val){m_class = val; m_classTxt->setValue(m_class.c_str());}
			inline void setFunctionValue(String val){m_function = val; m_functionTxt->setValue(m_function.c_str());}
			inline void setTypeValue(String val){m_type = val; m_typeTxt->setValue(m_type.c_str());}*/
	
			String getLabel() const;
			/*String getClass() const;
			String getFunction() const;
			String getType() const;*/
	
	
		protected:
			wxTextCtrl* m_labelTxt;
			/*wxTextCtrl* m_classTxt;
			wxTextCtrl* m_functionTxt;
			wxTextCtrl* m_typeTxt;*/
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			String m_label;
			/*String m_class;
			String m_function;
			String m_type;*/
		};
	
	
	
	} // end namespace editor
}



#endif // NewFSMStateDialog_h__