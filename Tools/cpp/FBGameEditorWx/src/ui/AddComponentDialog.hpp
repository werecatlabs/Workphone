#ifndef AddComponentDialog_h__
#define AddComponentDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>
#include <wx/treebase.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/propgrid/advprops.hpp>
#include <wx/propgrid/manager.hpp>
#include "core/IMessageListener.hpp"



namespace fb
{
	
	namespace editor
	{
	
	
		
		//--------------------------------------------
		class AddComponentDialog : public wxDialog
		{
		public:
			enum 
			{
				COMPONENT_TREE_ID = wxID_HIGHEST,
				PW_COMP_ADD_MODIF,
				PW_COMP_CANCEL,
			};
	
			AddComponentDialog(wxWindow *parent);
			~AddComponentDialog();
	
			String getType() const;
			Array<String> getComponents() const;
	
		protected:
			void populateComponentTree();
	
			void OnSelChanged(wxTreeEvent& event);
			void OnContextMenu( wxTreeEvent &event );
			void onAddModifyComponent(wxCommandEvent & event);
	
			wxTreeCtrl* m_tree;
	
			wxTextCtrl* m_labelTxt;
			wxTextCtrl* m_typeTxt;
	
			wxButton* m_okBtn;
			wxButton* m_cancelBtn;
	
			String m_componentType;
			Array<String> m_components;
	
			wxMenu*	m_labelComponentMenu;
			wxMenu*	m_componentMenu;
			wxMenu*	m_defaultMenu;
	
			DECLARE_EVENT_TABLE()
		};
	
	
	
	} // end namespace editor
}



#endif // AddComponentDialog_h__