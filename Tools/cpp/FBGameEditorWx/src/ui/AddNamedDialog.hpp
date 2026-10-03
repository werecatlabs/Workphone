#ifndef AddNamedDialog_h__
#define AddNamedDialog_h__


#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>
#include <FBCore/Base/StringTypes.hpp>




namespace fb
{	
	namespace editor
	{
	
	
		
		//--------------------------------------------
		class AddNamedDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
				USE_DEFAULTS_CHK,
			};
	
		public:
			AddNamedDialog(wxWindow *parent);
			~AddNamedDialog(){}
	
			String getLabel() const;
			bool getUseDefaults() const;

			String getObjectType() const;
			void setObjectType(const String& val);
	
		protected:
			void OnCancelBtn(wxCommandEvent& event);
	
			wxTextCtrl* m_labelTxt;
			wxCheckBox* m_useDefaultsChkBox;
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
			String m_objectType;


			DECLARE_EVENT_TABLE()
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // AddNamedDialog_h__



