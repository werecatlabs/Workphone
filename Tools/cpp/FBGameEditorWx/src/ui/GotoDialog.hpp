#ifndef GotoDialog_h__
#define GotoDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>

#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>



namespace fb
{
	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class GotoDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
				USE_DEFAULTS_CHK,
			};
	
		public:
			GotoDialog(wxWindow *parent);
			~GotoDialog(){}
	
			int getLineNumber() const;
			String getLabel() const;
			bool getUseDefaults() const;
	
		protected:
			void OnCancelBtn(wxCommandEvent& event);
	
			wxTextCtrl* m_labelTxt;
			wxCheckBox* m_useDefaultsChkBox;
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			DECLARE_EVENT_TABLE()
		};
	
	
	
	} // end namespace editor
} // end namespace fb


#endif // GotoDialog_h__


