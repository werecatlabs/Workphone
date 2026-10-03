#include <GameEditorPCH.hpp>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.hpp"
    #include "wx/choice.hpp"
#endif

#include "wx/listctrl.hpp"
#include "wx/sizer.hpp"
#include "wx/imaglist.hpp"
#include "wx/listctrl.hpp"
#include <wx/propgrid/manager.hpp>
#include "bitmaps/open.xpm"

#include "ui/TerrainCreateDialog.hpp"
#include "ui/TerrainWindow.hpp"
#include "ui/wxGUIUtil.hpp"

#define ART_CLIENT(id) \
    choice->Append(_T(#id), (void*)id);
#define ART_ICON(id) \
    { \
        int ind; \
        wxIcon icon = wxArtProvider::GetIcon(id, client, size); \
        if ( icon.Ok() ) \
            ind = images->Add(icon); \
        else \
            ind = 0; \
        list->InsertItem(index, _T(#id), ind); \
        list->SetItemData(index, (long)id); \
        index++; \
    }

// ----------------------------------------------------------------------------
// Functions to fill-in all supported art IDs
// ----------------------------------------------------------------------------

static void FillClients(wxChoice *choice)
{
    //ART_CLIENT(wxART_OTHER)
    //ART_CLIENT(wxART_TOOLBAR)
    //ART_CLIENT(wxART_MENU)
    //ART_CLIENT(wxART_FRAME_ICON)
    //ART_CLIENT(wxART_CMN_DIALOG)
    //ART_CLIENT(wxART_HELP_BROWSER)
    //ART_CLIENT(wxART_MESSAGE_BOX)
}

static void FillBitmaps(wxImageList *images, wxListCtrl *list,
                        int& index,
                        const wxArtClient& client, const wxSize& size)
{
    //ART_ICON(wxART_ERROR)
    //ART_ICON(wxART_QUESTION)
    //ART_ICON(wxART_WARNING)
    //ART_ICON(wxART_INFORMATION)
    //ART_ICON(wxART_ADD_BOOKMARK)
    //ART_ICON(wxART_DEL_BOOKMARK)
    //ART_ICON(wxART_HELP_SIDE_PANEL)
    //ART_ICON(wxART_HELP_SETTINGS)
    //ART_ICON(wxART_HELP_BOOK)
    //ART_ICON(wxART_HELP_FOLDER)
    //ART_ICON(wxART_HELP_PAGE)
    //ART_ICON(wxART_GO_BACK)
    //ART_ICON(wxART_GO_FORWARD)
    //ART_ICON(wxART_GO_UP)
    //ART_ICON(wxART_GO_DOWN)
    //ART_ICON(wxART_GO_TO_PARENT)
    //ART_ICON(wxART_GO_HOME)
    //ART_ICON(wxART_FILE_OPEN)
    //ART_ICON(wxART_PRINT)
    //ART_ICON(wxART_HELP)
    //ART_ICON(wxART_TIP)
    //ART_ICON(wxART_REPORT_VIEW)
    //ART_ICON(wxART_LIST_VIEW)
    //ART_ICON(wxART_NEW_DIR)
    //ART_ICON(wxART_FOLDER)
    //ART_ICON(wxART_GO_DIR_UP)
    //ART_ICON(wxART_EXECUTABLE_FILE)
    //ART_ICON(wxART_NORMAL_FILE)
    //ART_ICON(wxART_TICK_MARK)
    //ART_ICON(wxART_CROSS_MARK)
    //ART_ICON(wxART_MISSING_IMAGE)
    //ART_ICON(wxART_FILE_SAVE)
    //ART_ICON(wxART_FILE_SAVE_AS)
    //ART_ICON(wxART_COPY)
    //ART_ICON(wxART_CUT)
    //ART_ICON(wxART_PASTE)
    //ART_ICON(wxART_UNDO)
    //ART_ICON(wxART_REDO)
    //ART_ICON(wxART_QUIT)
    //ART_ICON(wxART_FIND)
    //ART_ICON(wxART_FIND_AND_REPLACE)

}



const int ID_CREATE_TERRAIN = wxID_ANY;
const int ID_CREATE_TERRAIN_PROPERTIES = wxID_ANY;

BEGIN_EVENT_TABLE(fb::editor::TerrainCreateDialog, wxDialog)
	EVT_BUTTON(ID_CREATE_TERRAIN, TerrainCreateDialog::OnCreateTerrainBtn)
	EVT_PG_CHANGED(ID_CREATE_TERRAIN_PROPERTIES, TerrainCreateDialog::OnPropertyGridChange)
END_EVENT_TABLE()



namespace fb
{	
	namespace editor
	{



		//--------------------------------------------
		TerrainCreateDialog::TerrainCreateDialog(wxWindow *parent)
			: wxDialog(parent, wxID_ANY, _T("Create Terrain"),
					   wxDefaultPosition, wxDefaultSize,
					   wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
		{
			wxSizer *sizer = new wxBoxSizer(wxVERTICAL);

			wxPropertyGridManager* pgman = new wxPropertyGridManager(this,
				ID_CREATE_TERRAIN_PROPERTIES,
				wxDefaultPosition,
				wxDefaultSize);
			m_terrainCreateGrid = pgman->GetGrid();
			sizer->Add( pgman, 1, wxEXPAND );

			wxSizer *buttonBox = new wxBoxSizer(wxHORIZONTAL);

			m_createBtn = new wxButton(this, wxID_ANY, _T("Create"));
			m_createBtn->SetDefault();
			buttonBox->Add(m_createBtn, 0, wxALIGN_LEFT | wxALL, 10);

			wxButton *ok = new wxButton(this, wxID_OK, _T("Close"));
			//ok->setDefault();
			buttonBox->Add(ok, 0, wxALIGN_RIGHT | wxALL, 10);

			sizer->Add(buttonBox);

			SetSizer(sizer);
   

			//if(!m_propertyGroup.Load("../Media/settings/TerrainCreateDefault.xml"))
			{
				wxMessageBox("Could not load default terrain settings. Using hardcoded defaults.",
						 "Message",
						 wxOK, NULL);

				m_propertyGroup.addProperty("File", "...", "file");
				m_propertyGroup.addProperty("TileSize", "65", "int");
				m_propertyGroup.addProperty("TerrainExtents", "16000.0, 1000.0, 16000.0", "vector3"); 
			}

			wxGUIUtil::populateProperties(m_propertyGroup, m_terrainCreateGrid);

			 //sizer->Fit(this);

			m_controlValue = ShowModal();
		}



		//--------------------------------------------
		void TerrainCreateDialog::OnCreateTerrainBtn( wxCommandEvent& event )
		{
			Properties	terrainPropertiesGroup;
			//if(terrainPropertiesGroup.Load("../media/settings/TerrainDefault.xml"))
			{
				String heightMapFile;
				m_propertyGroup.getPropertyValue("File", heightMapFile);
				String fileName = Path::getFileName(heightMapFile);

				String terrainExtents;
				m_propertyGroup.getPropertyValue("TerrainExtents", terrainExtents);
								
				String tileSize;
				m_propertyGroup.getPropertyValue("TileSize", tileSize);

				terrainPropertiesGroup.setProperty("FilePath", fileName);
				terrainPropertiesGroup.setProperty("TerrainExtents", terrainExtents);
				terrainPropertiesGroup.setProperty("TileSize", tileSize);

				//IApplicationManager::instance()->getEntityManager()->CreateEntity(terrainPropertiesGroup);
			}

			EndModal(m_controlValue);
		}



		//--------------------------------------------
		void TerrainCreateDialog::OnPropertyGridChange(wxPropertyGridEvent& event)
		{
			wxPGProperty* p = event.GetProperty();

			auto name = p->GetName().c_str();
			auto value = m_terrainCreateGrid->GetPropertyValueAsString(p).c_str();
			//m_propertyGroup.setProperty(name, value);
		}



	} // end namespace editor	
} // end namespace fb


