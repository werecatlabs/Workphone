#ifndef AddSoundDialog_h__
#define AddSoundDialog_h__




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
		class AddSoundDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			AddSoundDialog(wxWindow *parent);
			~AddSoundDialog();
	
			String getLabel() const;
			String getName() const;
			String getSoundName() const;
	
	
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_nameTxt;
			wxTextCtrl* m_SoundNameTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
		};
	
	
	
	} // end namespace editor
	
}


#endif // AddSoundDialog_h__