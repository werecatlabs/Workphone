#ifndef AddBodyDialog_h__
#define AddBodyDialog_h__



#include <GameEditorPrerequisites.hpp>
#include "core/IMessageListener.hpp"
#include "ui/PropertiesWindow.hpp"
#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>
#include <wx/wx.hpp>



namespace fb
{	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class AddBodyDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
			AddBodyDialog(wxWindow *parent);
			~AddBodyDialog();
	
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


