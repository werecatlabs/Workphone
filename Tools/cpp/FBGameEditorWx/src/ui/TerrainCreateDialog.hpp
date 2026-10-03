#ifndef __TerrainCreateDialog_H__
#define __TerrainCreateDialog_H__



#include <GameEditorPrerequisites.hpp>
#include "wx/dialog.hpp"
#include "wx/artprov.hpp"
#include <wx/propgrid/propgrid.hpp>
#include "FBCore/Base/Properties.hpp"



class WXDLLIMPEXP_FWD_CORE wxListCtrl;
class WXDLLIMPEXP_FWD_CORE wxListEvent;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;



namespace fb
{	
	namespace editor
	{



		//--------------------------------------------
		class TerrainCreateDialog : public wxDialog
		{
		public:
			TerrainCreateDialog(wxWindow *parent);

		private:
			void OnCreateTerrainBtn( wxCommandEvent& event );
			void OnPropertyGridChange(wxPropertyGridEvent& event); 

			Properties	m_propertyGroup;
    
			wxPropertyGrid*		m_terrainCreateGrid;

			wxButton* m_createBtn;

			wxListCtrl *m_list;
			wxStaticBitmap *m_canvas;
			wxStaticText *m_text;
			wxString m_client;

			int m_controlValue;

			DECLARE_EVENT_TABLE()
		};



	} // end namespace editor	
} // end namespace fb



#endif // __ARTBROWS_H__



