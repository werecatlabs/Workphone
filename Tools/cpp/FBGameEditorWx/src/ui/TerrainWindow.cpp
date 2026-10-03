#include <GameEditorPCH.hpp>
#include "TerrainWindow.hpp"

#include <wx/grid.hpp>
#include <wx/treectrl.hpp>
#include <wx/spinctrl.hpp>
#include <wx/artprov.hpp>
#include <wx/clipbrd.hpp>
#include <wx/image.hpp>
#include <wx/colordlg.hpp>
#include <wx/wxhtml.hpp>
#include <wx/tglbtn.hpp>
#include <wx/gbsizer.hpp>
#include <wx/sizer.hpp>

#include <wx/wx.hpp>
#include <wx/treectrl.hpp>
#include <wx/propgrid/propgrid.hpp>
#include <wx/bmpcbox.hpp>

#include <wx/aui/framemanager.hpp>
#include <wx/propgrid/manager.hpp>

#include "bitmaps/new.xpm"
#include "bitmaps/open.xpm"
#include "bitmaps/save.xpm"
#include "bitmaps/copy.xpm"
#include "bitmaps/cut.xpm"
#include "bitmaps/paste.xpm"
#include "bitmaps/print.xpm"
#include "bitmaps/preview.xpm"
#include "bitmaps/help.xpm"

#include "ApplicationFrame.hpp"
//#include "terrain\TerrainManager.hpp"
//#include "terrain\ITerrainTool.hpp"
#include "TerrainCreateDialog.hpp"
#include "ui/wxGUIUtil.hpp"
#include <FBCore/FBCoreHeaders.hpp>

//#include<IL/il.hpp>
#include<stdlib.hpp> /* because of malloc() etc. */


//BEGIN_EVENT_TABLE(fb::editor::TerrainWindow, wxScrolledWindow)
//	EVT_BUTTON(TerrainWindow::ID_TerrainCreateTerrainBtn, TerrainWindow::OnCreateTerrainBtn)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainRaiseBtn, TerrainWindow::OnRaiseBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainLowerBtn, TerrainWindow::OnLowerBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainMinimumBtn, TerrainWindow::OnMinimumBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainMaximumBtn, TerrainWindow::OnMaximumBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainsetHeightBtn, TerrainWindow::OnsetHeightBtnToggled)
//
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainPaintBtn, TerrainWindow::OnPaintBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainEraseBtn, TerrainWindow::OnEraseBtnToggled)
//	EVT_TOGGLEBUTTON(TerrainWindow::ID_TerrainBlendBtn, TerrainWindow::OnBlendBtnToggled)
//
//	EVT_COMBOBOX(TerrainWindow::ID_TerrainSplatImage, TerrainWindow::OnComboBox)
//
//	EVT_PG_CHANGED(TerrainWindow::ID_TerrainFrameToolProperties, TerrainWindow::OnPropertyGridChangeTool)
//
//	EVT_BUTTON(TerrainWindow::ID_AddTerrainSplatImage, TerrainWindow::OnAddTextureBtn)
//END_EVENT_TABLE()



namespace fb
{	
	namespace editor
	{



		//--------------------------------------------
		TerrainWindow::TerrainWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
			: 
			m_pButton(0)
		{
			auto parentWindow = new wxScrolledWindow(parent, id);
			setWindow(parentWindow);

			wxBoxSizer* m_pButtonBox = new wxBoxSizer( wxVERTICAL );
			parentWindow->SetSizer(m_pButtonBox);

			m_createTerrainBtn = new wxButton(parentWindow, TerrainWindow::ID_TerrainCreateTerrainBtn, wxString( "Create Terrain" ) );
			m_pButtonBox->Add(m_createTerrainBtn);

			m_raiseBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainRaiseBtn, wxString( "Raise" ) );
			m_pButtonBox->Add(m_raiseBtn);

			m_lowerBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainLowerBtn, wxString( "Lower" ) );
			m_pButtonBox->Add(m_lowerBtn);

			m_minimumBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainMinimumBtn, wxString( "Minimum" ) );
			m_pButtonBox->Add(m_minimumBtn);

			m_maximumBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainMaximumBtn, wxString( "Maximum" ) );
			m_pButtonBox->Add(m_maximumBtn);

			m_setHeightBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainsetHeightBtn, wxString( "set Height" ) );
			m_pButtonBox->Add(m_setHeightBtn);

			m_paintBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainPaintBtn, wxString( "Paint" ) );
			m_pButtonBox->Add(m_paintBtn);

			m_eraseBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainEraseBtn, wxString( "Erase" ) );
			m_pButtonBox->Add(m_eraseBtn);

			m_blendBtn = new wxToggleButton(parentWindow, TerrainWindow::ID_TerrainBlendBtn, wxString( "Blend" ) );
			m_pButtonBox->Add(m_blendBtn);

			m_addTextureBtn = new wxButton(parentWindow, TerrainWindow::ID_AddTerrainSplatImage, wxString( "Add Texture" ) );
			m_pButtonBox->Add(m_addTextureBtn);

			m_brushesCombobox = new wxBitmapComboBox();
			m_brushesCombobox->Create(parentWindow, TerrainWindow::ID_TerrainBrushImage, wxEmptyString,
				wxDefaultPosition, wxDefaultSize,
				0, NULL,
				wxCB_SORT);

			m_splatMapsCombobox = new wxBitmapComboBox();
			m_splatMapsCombobox->Create(parentWindow, TerrainWindow::ID_TerrainSplatImage, wxEmptyString,
				wxDefaultPosition, wxDefaultSize,
				0, NULL,
				wxCB_SORT);

			String brushesFolderName = "../media/ET/Brushes/";
			String splatMapsFolderName = "../media/ET/SplatMaps/";

			auto fileSystem = IApplicationManager::instance()->getFileSystem();
			fileSystem->getFileNamesInFolder(brushesFolderName, m_brushes);
			fileSystem->getFileNamesInFolder(splatMapsFolderName, m_splatMaps);

			loadImages(brushesFolderName, m_brushes, m_brushesCombobox);
			loadImages(splatMapsFolderName, m_splatMaps, m_splatMapsCombobox);

			//m_combobox->setPopupMaxHeight(64);
			//m_combobox->setSize(wxDefaultCoord, 64);

			m_pButtonBox->Add(m_brushesCombobox);
			m_pButtonBox->Add(m_splatMapsCombobox);

			wxPropertyGridManager* pgman = new wxPropertyGridManager(parentWindow,
				TerrainWindow::ID_TerrainFrameToolProperties,
				wxDefaultPosition,
				wxDefaultSize,
				style );
			m_toolPropertyGrid = pgman->GetGrid();
			m_pButtonBox->Add(pgman , 1, wxEXPAND);

			//TerrainManager::getSingletonPtr()->addTerrainManagerListener(m_terrainManagerListener);
		}



		//--------------------------------------------
		TerrainWindow::~TerrainWindow()
		{
			//bool result = TerrainManager::getSingletonPtr()->removeTerrainManagerListener(m_terrainManagerListener);
		}



		//--------------------------------------------
		void TerrainWindow::loadImages(const String& folderName, Array<String>& images, wxBitmapComboBox* comboBox)
		{
			for(u32 i=0; i<images.size(); ++i)
			{
				String filePath = folderName + images[i];
				addComboBoxBitmap(filePath, comboBox);

			}	
		}



		//--------------------------------------------
		void TerrainWindow::addComboBoxBitmap(const String& filePath, wxBitmapComboBox* comboBox) 
		{
			//String ext = StringUtil::getFileNameExtension(filePath);
			//wxBitmap* baseMap = NULL;

			//if(ext==(".jpg"))
			//{
			//	baseMap = new wxBitmap(filePath.c_str(), wxBITMAP_TYPE_JPEG);
			//}
			//else if(ext==(".png"))
			//{
			//	baseMap = new wxBitmap(filePath.c_str(), wxBITMAP_TYPE_PNG);
			//}
			//else if(ext == (".dds"))
			//{		
			//	baseMap = createTexture(filePath);
			//}
			//else 
			//{
			//	return;
			//}

			//baseMap->SetSize(64,64);

			//String fileName = Path::getFileName(filePath);
			//comboBox->Append(fileName.c_str(), *baseMap);
			//m_splatMaps.push_back(fileName);
		}




		wxBitmap* TerrainWindow::createTexture(const String& texturePath)
		{
			//auto applicationManager = IApplicationManager::instance();
			//SmartPtr<IFileSystem>& fileSystem = engine->getFileSystem();
			//SmartPtr<IStream> stream = fileSystem->open(texturePath);

			//ILuint handle, w, h;

			//ILenum	Error;

			//// Generate the main image name to use.
			//ilGenImages(1, &handle);

			//// Bind this image name.
			//ilBindImage(handle);

			////ILboolean loaded = ilLoadImage(filePath.c_str());
			//ILboolean loaded = ilLoadL(IL_DDS, stream->getData(), stream->getSize());
			//if (loaded == IL_FALSE)
			//	return 0; /* error encountered during loading */
			///* Let�s spy on it a little bit */
			//w = ilGetInteger(IL_IMAGE_WIDTH); // getting image width
			//h = ilGetInteger(IL_IMAGE_HEIGHT); // and height
			//printf("Our image resolution: %dx%d\n", w, h);
			///* how much memory will we need? */
			//int memory_needed = w * h * 3 * sizeof(unsigned char);
			///* We multiply by 3 here because we want 3 components per pixel */
			//ILubyte * data = (ILubyte *)malloc(memory_needed);
			///* finally get the image data */
			//ilCopyPixels(0, 0, 0, w, h, 1, IL_RGB, IL_UNSIGNED_BYTE, data);

			///* Finally, clean the mess! */
			//ilDeleteImages(1, & handle);
			////free(data); data = NULL;

			//wxImage image(w, h, data);
			//return new wxBitmap(image, 24);

			return nullptr;
		}



		//--------------------------------------------
		void TerrainWindow::OnButtonToggled(wxCommandEvent& event)
		{
		}



		//--------------------------------------------
		void TerrainWindow::OnCreateTerrainBtn( wxCommandEvent& event )
		{
			//TerrainCreateDialog dlg(this);
		}



		//--------------------------------------------
		void TerrainWindow::OnRaiseBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Raise");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Raise");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnLowerBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Lower");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Lower");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnMinimumBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Minimum");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Minimum");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnMaximumBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Maximum");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Maximum");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnsetHeightBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("set Height");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("set Height");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnPaintBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Paint");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Paint");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnEraseBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Erase");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Erase");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnBlendBtnToggled( wxCommandEvent& event )
		{
			//if( event.GetInt() )
			//{
			//	TerrainManager::getSingletonPtr()->selectTool("Blend");
			//}
			//else
			//{
			//	TerrainManager::getSingletonPtr()->deselectTool("Blend");
			//}
		}



		//--------------------------------------------
		void TerrainWindow::OnAddTextureBtn( wxCommandEvent& event )
		{
			//wxFileDialog dialog(this,
			//	_T("Open"),
			//	wxEmptyString,
			//	wxEmptyString,
			//	_T("DDS (*.dds)|*.dds|Png (*.png)|*.png|Jpg (*.jpg)|*.jpg"),
			//	wxFD_OPEN | wxFD_MULTIPLE 
			//	);

			//dialog.SetFilterIndex(0);

			//if (dialog.ShowModal() == wxID_OK)
			//{
			//	wxArrayString paths;
			//	dialog.GetPaths(paths);

			//	for(u32 i=0; i<paths.size(); ++i)
			//	{
			//		String filePath = paths[i].c_str();	
			//		addComboBoxBitmap(filePath, m_splatMapsCombobox);
			//	}			
			//}	
		}



		//--------------------------------------------
		void TerrainWindow::OnComboBox(wxCommandEvent& event)
		{
			long sel = event.GetInt();
			String imageName = m_splatMaps[sel];
			//TerrainManager::getSingletonPtr()->setTexture(imageName.c_str());
		}



		//--------------------------------------------
		void TerrainWindow::OnPropertyGridChangeTool(wxPropertyGridEvent& event)
		{
			wxPGProperty* p = event.GetProperty();

			//ITerrainTool* curTerrainTool = TerrainManager::getSingletonPtr()->getSelectedTool();
			//if(!curTerrainTool)
			//	return;

			//SmartPtr<Properties> propertyGroup = curTerrainTool->getPropertyGroup();
			//propertyGroup->setProperty(String(p->GetName().c_str()), String(m_toolPropertyGrid->GetPropertyValueAsString(p).c_str()));
		}



		////--------------------------------------------
		//void TerrainWindow::OnSelectTool(TerrainToolPtr tool)
		//{
		//	populateToolProperties();
		//}



		//--------------------------------------------
		void TerrainWindow::populateToolProperties()
		{
			//m_toolPropertyGrid->Clear();

			//ITerrainTool* curTerrainTool = TerrainManager::getSingletonPtr()->getSelectedTool();
			//if(!curTerrainTool)
			//	return;

			//const SmartPtr<Properties> properties = curTerrainTool->getPropertyGroup();
			//wxGUIUtil::populateProperties(*properties, m_toolPropertyGrid);
		}



	} // end namespace editor	
} // end namespace fb

