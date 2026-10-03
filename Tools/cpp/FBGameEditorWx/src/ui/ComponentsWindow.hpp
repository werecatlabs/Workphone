#ifndef ComponentsWindow_h__
#define ComponentsWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <FBWxWidgets/FBWxApplicationWindow.hpp>
#include <wx/treebase.hpp>


namespace fb
{
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class ComponentsWindow : public ui::wxApplicationWindow
		{
		public:
			enum
			{
				ID_POPMENU_ADD_COMPONENT = wxID_HIGHEST,
				ID_POPMENU_EDIT_COMPONENT,
				ID_POPMENU_REMOVE_COMPONENT,
				DELETE_COMP_BTN_ID,
			}; 
	
			ComponentsWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition, 
				const wxSize& size = wxDefaultSize, long style = wxFULL_REPAINT_ON_RESIZE | wxVSCROLL | wxHSCROLL, 
				const wxValidator& validator = wxDefaultValidator, const wxString& name = "ComponentsWindow");
			~ComponentsWindow();
	
		protected:
			class ComponentTreeData : public wxTreeItemData
			{
			public:
				String templateType;
			};
	
			void populateComponentTree();
	
			void OnAddComponent(wxCommandEvent& event);
			void OnEditComponent( wxCommandEvent& event );
			void OnDeleteComponent(wxCommandEvent& event);
			void onTreeRightMouseBtnUp(wxMouseEvent& event);		
	
			wxTreeCtrl* m_tree = nullptr;
			wxButton* m_newBtn = nullptr;
			wxButton* m_deleteBtn = nullptr;
		};
	
	
	
	} // end namespace editor
} // end namespace fb



#endif // ComponentsWindow_h__


