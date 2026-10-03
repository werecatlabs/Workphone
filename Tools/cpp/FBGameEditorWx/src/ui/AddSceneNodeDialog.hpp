#ifndef AddSceneNodeDialog_h__
#define AddSceneNodeDialog_h__



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
		class AddSceneNodeDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			AddSceneNodeDialog(wxWindow *parent);
			~AddSceneNodeDialog();
	
			String getLabel() const;
			String getName() const;
	
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_nameTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
		};
	
	
	
	} // end namespace editor
	
}


#endif // AddSceneNodeDialog_h__