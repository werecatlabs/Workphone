#ifndef ProjectSettingsDialog_h__
#define ProjectSettingsDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/dialog.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class ProjectSettingsDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
			ProjectSettingsDialog(wxWindow *parent);
			~ProjectSettingsDialog();
	
			String getLabel() const;
			String getName() const;
			String getType() const;
	
			void populateTypeCombo();	
	
		protected:
			wxTextCtrl* m_labelTxt = nullptr;
			wxTextCtrl* m_nameTxt = nullptr;
			wxComboBox* m_typeTxt = nullptr;
	
			wxButton* m_okBtn = nullptr;
			wxButton* m_cancelBtn = nullptr;
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // AddBodyDialog_h__


