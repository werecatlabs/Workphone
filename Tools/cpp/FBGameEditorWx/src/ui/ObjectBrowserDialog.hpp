#ifndef ObjectBrowserDialog_h__
#define ObjectBrowserDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/dialog.hpp>
#include <wx/treectrl.hpp>


namespace fb
{
	namespace editor
	{



		//--------------------------------------------
		class ObjectBrowserDialog : public wxDialog
		{
		public:
			enum
			{
				CANCEL_BTN_ID = wxID_HIGHEST,
			};

			ObjectBrowserDialog(wxWindow* parent);
			~ObjectBrowserDialog();

			void populate();

			String getSelectedObject() const;
			void setSelectedObject(const String& val);

		protected:
			void handleTreeSelectionChanged(wxTreeEvent& event);
			void handleTreeSelectionActivated(wxTreeEvent& event);

			wxTreeCtrl* m_tree = nullptr;
			String m_selectedObject;
		};



	} // end namespace editor
} // end namespace fb



#endif // AddBodyDialog_h__


