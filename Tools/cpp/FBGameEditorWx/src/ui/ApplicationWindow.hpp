#ifndef ApplicationWindow_h__
#define ApplicationWindow_h__




#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>






namespace fb
{
	
	namespace editor
	{
	
	
	
		class ApplicationWindow :  public wxScrolledWindow
		{
		public:
			enum
			{
				NEW_COMP_BTN_ID = wxID_HIGHEST,
				DELETE_COMP_BTN_ID,
			}; 
	
		public:
			ApplicationWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = 0, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "listCtrl");
			~ApplicationWindow();
	
		protected:
			void populateComponentTree();
	
			void OnNewComponent(wxCommandEvent& event);
			void OnDeleteComponent(wxCommandEvent& event);
	
			wxTreeCtrl* m_tree;
	
			wxButton* m_newBtn;
			wxButton* m_deleteBtn;
	
			DECLARE_EVENT_TABLE()
		};
	
	
	
	}// end namespace editor
}



#endif // ApplicationWindow_h__


