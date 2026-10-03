#ifndef NewComponentDialog_h__
#define NewComponentDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>
#include "core/IMessageListener.hpp"
#include "ui/PropertiesWindow.hpp"
#include <FBObjectTemplates/FBObjectTemplatesPrerequisites.hpp>



namespace fb
{
	
	
	namespace editor
	{
	
	
		
		//--------------------------------------------
		class EditComponentDialog : public wxDialog
		{
		public:
			enum 
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};
	
		public:
			EditComponentDialog(wxWindow *parent, bool edit = true);
			~EditComponentDialog();
	
			String getLabel() const;
			String getType() const;
	
			SmartPtr<ComponentTemplate> getComponentTemplate() const;
			void setComponentTemplate(SmartPtr<ComponentTemplate> val);
	
			void populateComponentCombo();
			void onSelect(wxCommandEvent& event);
	
			void populateWithComponentTemplate( SmartPtr<ComponentTemplate> &componentTemplate );
	
			void onAddNewProperty(wxCommandEvent& event);
			void onRemoveProperty(wxCommandEvent& event);
			void onAddNewEvent(wxCommandEvent& event);
			void onRemoveEvent(wxCommandEvent& event);
			void onEventListDblCLick(wxCommandEvent& event);
			void RemoveEvent(String eventName);
			void OnPropertyGridItemRightClick( wxPropertyGridEvent& event );
			void onPopMenuAddProperty(wxCommandEvent & event);
			void onPopMenuRemoveProperty(wxCommandEvent & event);
			void onListboxRightMouseBtnUp(wxMouseEvent& event);
			void onPopMenuAddEvent(wxCommandEvent & event);
			void onPopMenuRemoveEvent(wxCommandEvent & event);
	
			DECLARE_EVENT_TABLE()
	
		protected:
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_typeTxt;
			wxListBox* m_events;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			PropertiesWindow *m_propertiesWindow;
			SmartPtr<ComponentTemplate> m_lastSelectedComponentTemplate;
			SmartPtr<ComponentTemplate> m_componentTemplate;
		};
	
	
	
	} // end namespace editor
	
}


#endif // NewComponentDialog_h__