#ifndef AddEntityDialog_h__
#define AddEntityDialog_h__




#include <GameEditorPrerequisites.hpp>
#include <wx/event.hpp>
#include <wx/dialog.hpp>
#include <wx/propgrid/propgrid.hpp>



namespace fb
{	
	namespace editor
	{
	
	
		
		//--------------------------------------------
		class AddEntityDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
				USE_DEFAULTS_CHK,
				PropertiesId,
			};
	
			AddEntityDialog(wxWindow *parent);
			~AddEntityDialog();
	
			String getLabel() const;
			bool getUseDefaults() const;
	
			void setProperties(const Properties& properties);
			void getProperties(Properties& properties) const;
	
		protected:
			void OnCancelBtn(wxCommandEvent& event);
			void OnPropertyChange( wxPropertyGridEvent& event );
	
			wxPropertyGrid*	m_properties;
	
			wxCheckBox* m_useDefaultsChkBox;
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			Properties m_propGrp;
	
			DECLARE_EVENT_TABLE()
			
		};
	
	
	
	} // end namespace editor	
} // end namespace fb	



#endif // AddEntityDialog_h__


