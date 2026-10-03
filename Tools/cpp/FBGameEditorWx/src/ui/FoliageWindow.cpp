#include <GameEditorPCH.hpp>

#include "FoliageWindow.hpp"

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

#include "wx/imaglist.hpp"
#include "wx/listctrl.hpp"
#include "wx/timer.hpp"           // for wxStopWatch
#include "wx/colordlg.hpp"        // for wxGetColourFromUser
#include "wx/settings.hpp"

#include "ui/AddNamedDialog.hpp"

//#include "Command\CsetSelectedEntity.hpp"
//#include "terrain\FoliageManager.hpp"
//#include "terrain\FoliageTool.hpp"
#include <wx/aui/framemanager.hpp>
//#include "entity/FoliageEnt.hpp"
//#include "entity/FoliageLayer.hpp"
#include "wxGUIUtil.hpp"
#include "editor/EditorManager.hpp"
#include <FBCore/FBCoreHeaders.hpp>




namespace fb
{
	namespace editor
	{



		//------------------------------------------------
		class FoliageListCtrl: public wxListCtrl
		{
		public:
			FoliageListCtrl(FoliageWindow* vegetationFrame, wxWindow *parent,
					const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
			: 
			wxListCtrl(parent, id, pos, size, style),
			m_attr(*wxBLUE, *wxLIGHT_GREY, wxNullFont),
			m_vegetationFrame(vegetationFrame)
			{
			}

			void OnSelected(wxListEvent& event)
			{	
				//FoliageLayerPtr foliageLayer;

				wxString string1;
				wxString string2;

				wxListItem info;
				info.m_itemId = event.m_itemIndex;
				info.m_col = 0;
				info.m_mask = wxLIST_MASK_TEXT | wxLIST_MASK_DATA;
				//if ( GetItem(info) )
				//{
				//	string1 = info.m_text.c_str();
				//	foliageLayer = (FoliageLayer*)info.GetData();
				//}
				//else
				//{
				//	wxFAIL_MSG("wxListCtrl::GetItem() failed");
				//}

				String layerName = String(string1.c_str());

				//ApplicationManager* appRoot = IApplicationManager::instance();
				//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();
				//if(foliageLayer)
				//	foliageManager->selectLayer(foliageLayer->getId());
			}

		private:
			wxLog *m_logOld;
			wxListItemAttr m_attr;
			FoliageWindow* m_vegetationFrame;

			DECLARE_NO_COPY_CLASS(FoliageListCtrl)
			DECLARE_EVENT_TABLE()
		};

		BEGIN_EVENT_TABLE(FoliageListCtrl, wxListCtrl)
			EVT_LIST_ITEM_SELECTED(FoliageWindow::ID_FoliageFrameFoliageList, FoliageListCtrl::OnSelected)
		END_EVENT_TABLE()



		//------------------------------------------------
		class FoliagePropertiesListCtrl: public wxListCtrl
		{
		public:
			FoliagePropertiesListCtrl(FoliageWindow* vegetationFrame, wxWindow *parent,
					const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
			: 
			wxListCtrl(parent, id, pos, size, style),
			m_attr(*wxBLUE, *wxLIGHT_GREY, wxNullFont),
			m_vegetationFrame(vegetationFrame)
			{
			}

		private:
			wxLog *m_logOld;
			wxListItemAttr m_attr;
			FoliageWindow* m_vegetationFrame;
		};

		FoliageWindow* FoliageWindow::m_pFoliageFrame = 0;







		//--------------------------------------------
		FoliageWindow::FoliageWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos, 
			const wxSize& size, long style, const wxValidator& validator, const wxString& name)
			: 
			
			m_paintBtn(0),
			m_eraseBtn(0),
			m_toolPropertyGrid(0),
			m_vegetationLayers(0),
			m_layerPropertyGrid(0)
		{
			m_pFoliageFrame = this;

			auto window = new wxScrolledWindow(parent, id);
			setWindow(window);

			wxBoxSizer *topBox = new wxBoxSizer( wxVERTICAL );
			window->SetSizer(topBox);

			wxBoxSizer *horizontalBox0 = new wxBoxSizer( wxHORIZONTAL );
			topBox->Add(horizontalBox0);

			wxBoxSizer *horizontalBox1 = new wxBoxSizer( wxHORIZONTAL );
			topBox->Add(horizontalBox1);

			wxBoxSizer *horizontalBox2 = new wxBoxSizer( wxHORIZONTAL );
			topBox->Add(horizontalBox2);

			wxBoxSizer *horizontalBox3 = new wxBoxSizer( wxHORIZONTAL );
			topBox->Add(horizontalBox3);

			

			//m_addBtn = new wxToggleButton( this, FoliageFrame::ID_FoliageFrameAdd, wxString( "Add" ) );
			//topBox->Add(m_addBtn);

			m_paintBtn = new wxToggleButton(window, FoliageWindow::ID_FoliageFramePaint, wxString( "Paint" ) );
			horizontalBox0->Add(m_paintBtn);

			m_eraseBtn = new wxToggleButton(window, FoliageWindow::ID_FoliageFrameErase, wxString( "Erase" ) );
			horizontalBox0->Add(m_eraseBtn);

			m_newLayerBtn = new wxButton(window, FoliageWindow::ID_FoliageFrameCreateLayer, wxString( "New" ) );
			horizontalBox1->Add(m_newLayerBtn);

			m_removeLayerBtn = new wxButton(window, FoliageWindow::ID_FoliageFrameDestroyLayer, wxString( "Delete" ) );
			horizontalBox1->Add(m_removeLayerBtn);
			m_removeLayerBtn->Enable(false);

			m_generateBtn = new wxButton(window, FoliageWindow::ID_FoliageFrameGenerate, wxString( "Generate" ) );
			horizontalBox2->Add(m_generateBtn);

			m_autoGenChkBox = new wxCheckBox(window, FoliageWindow::ID_FoliageFrameAutoGenerateChkBox, _T("&Auto Generate"));
			horizontalBox2->Add(m_autoGenChkBox);
			
			m_addFoliage = new wxButton(window, FoliageWindow::ID_FoliageFrameAddFoliage, wxString( "Add Foliage" ) );
			horizontalBox3->Add(m_addFoliage);

			m_notebook = new wxNotebook(window, -1);
			topBox->Add(m_notebook, 1, wxEXPAND );

			m_foliageMeshes = new wxListCtrl(m_notebook, -1);
			m_notebook->AddPage(m_foliageMeshes, "Foliage Meshes");

			m_toolPropertyGrid = new wxPropertyGrid(m_notebook, FoliageWindow::ID_FoliageFrameToolProperties,
								wxDefaultPosition,wxSize(400,400),
								wxPG_SPLITTER_AUTO_CENTER |
								wxPG_BOLD_MODIFIED );
			m_notebook->AddPage(m_toolPropertyGrid, "Tool Properties");

	
			//m_toolPropertyGrid->setSize(400, 600);
			//for ( int i=0; i< 20; i++ )
			//    m_toolPropertyGrid->Append(new wxStringProperty(wxString::Format("Item %i"),i), wxPG_LABEL));



			//m_vegetationLayers = new FoliageListCtrl(window, m_notebook, FoliageWindow::ID_FoliageFrameFoliageList,
			//	wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxSUNKEN_BORDER | wxLC_EDIT_LABELS);
			//m_notebook->AddPage(m_vegetationLayers, "Layers");

			m_layerPropertyGrid = new wxPropertyGrid(m_notebook,FoliageWindow::ID_FoliageFrameLayerProperties,
				wxDefaultPosition, wxSize(400,400), wxPG_SPLITTER_AUTO_CENTER | wxPG_BOLD_MODIFIED );			
			m_notebook->AddPage(m_layerPropertyGrid, "Layers Properties");

			m_paintBtn->Enable(false);
			m_eraseBtn->Enable(false);

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//m_listener = new FoliageWindowListener(this);

			//if(foliageManager)
			//	foliageManager->addFoliageManagerListener(m_listener);
		}



		//--------------------------------------------
		FoliageWindow::~FoliageWindow()
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();
			//foliageManager->removeFoliageManagerListener(this);

			//FB_SAFE_DELETE(m_listener);
		}



		//--------------------------------------------
		FoliageWindow* FoliageWindow::GetInstance( wxWindow *parent_window )
		{
			return m_pFoliageFrame;
		}



		//--------------------------------------------
		void FoliageWindow::OnAddBtnToggled( wxCommandEvent& event )
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//if( event.GetInt() )
			//{
			//	foliageManager->selectTool("Add");
			//}
			//else
			//{
			//	foliageManager->deselectTool("Add");
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnAddFoliageBtn( wxCommandEvent& event )
		{
			//wxFileDialog dialog(this,
			//	_T("Open"),
			//	wxEmptyString,
			//	wxEmptyString,
			//	_T("Mesh (*.mesh)|*.mesh"),
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

			//		auto applicationManager = IApplicationManager::instance();
			//		SmartPtr<IFileSystem>& fileSystem = applicationManager->getFileSystem();

			//		String path = Path::getFilePath(filePath);
			//		path += String("/");
			//		//fileSystem->addFileArchive(path, true, true, FB_ARCH_TYPE_FOLDER);

			//		wxListItem info;
			//		info.SetText(filePath.c_str());
			//		m_foliageMeshes->InsertItem(info);
			//	}
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnPaintBtnToggled( wxCommandEvent& event )
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//if( event.GetInt() )
			//{
			//	foliageManager->selectTool("Paint");

			//	appRoot->setEditFoliage(true);
			//}
			//else
			//{
			//	foliageManager->deselectTool("Paint");

			//	appRoot->setEditFoliage(false);
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnEraseBtnToggled( wxCommandEvent& event )
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//if( event.GetInt() )
			//{
			//	foliageManager->selectTool("Erase");
			//}
			//else
			//{
			//	foliageManager->deselectTool("Erase");
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnCreateLayerBtn( wxCommandEvent& event )
		{
			//AddNamedDialog dialog(this);
			//if(dialog.ShowModal() == wxID_OK)
			//{
			//	String label = dialog.getLabel();

			//	ApplicationManager* appRoot = IApplicationManager::instance();
			//	FoliageManagerPtr foliageManager = appRoot->getFoliageManager();
			//	foliageManager->createFoliageLayer(label);
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnDestroyLayerBtn( wxCommandEvent& event )
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();
			//foliageManager->destroySelectedFoliageLayer();
		}



		//--------------------------------------------
		void FoliageWindow::OnGenerateBtn( wxCommandEvent& event )
		{
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();
			//foliageManager->generateFoliage();
		}



		//--------------------------------------------
		void FoliageWindow::OnCheckBox(wxCommandEvent& event)
		{
			//wxLogMessage(_T("Test checkbox %schecked (value = %d)."),
			//             event.IsChecked() ? _T("") : _T("un"),
			//             (int)m_checkbox->Get3StateValue());
			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//foliageManager->setAutoGenerate(event.IsChecked());
		}



		//--------------------------------------------
		void FoliageWindow::OnPropertyGridChangeTool(wxPropertyGridEvent& event)
		{
			wxPGProperty* p = event.GetProperty();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//FoliageToolPtr curFoliageTool = foliageManager->getSelectedTool();
			//if(curFoliageTool)
			//{
			//	//SmartPtr<Properties> propertyGroup = curFoliageTool->getPropertyGroup();
			//	//propertyGroup->setPropertyValue(p->GetName().c_str(), m_toolPropertyGrid->GetPropertyValueAsString(p).c_str());
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnPropertyGridChangeVegProps(wxPropertyGridEvent& event)
		{
			wxPGProperty* p = event.GetProperty();

			/*
			AppRoot* appRoot = AppRoot::getSingletonPtr();
			FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			SmartPtr<IActor> curLayer = foliageManager->getSelectedLayer();
			CompPropertiesPtr compProperties = curLayer->GetComponent(CompProperties::Type);
			if(!compProperties)
				return;

			SmartPtr<Properties> propertyGroup = compProperties->getPropertyGroup();
			String name = p->GetName().c_str();
			String value = m_toolPropertyGrid->GetPropertyValueAsString(p).c_str();
			propertyGroup->setPropertyValue(name, value);
			*/
		}



		//--------------------------------------------
		void FoliageWindow::OnCreateLayer()
		{
			populateFoliageLayers();
		}



		//--------------------------------------------
		void FoliageWindow::OnDestroyLayer()
		{
			populateFoliageLayers();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//FoliageLayerPtr layer = foliageManager->getSelectedLayer();
			//if(layer.isNull())
			//{
			//	m_paintBtn->Enable(false);
			//	m_eraseBtn->Enable(false);
			//	m_removeLayerBtn->Enable(false);
			//}
		}



		//--------------------------------------------
		void FoliageWindow::OnSelectLayer()
		{
			populateLayerProperties();

			EditorManager* appRoot = EditorManager::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//FoliageLayerPtr layer = foliageManager->getSelectedLayer();
			//if(!layer.isNull())
			//{
			//	m_paintBtn->Enable(true);
			//	m_eraseBtn->Enable(true);
			//	m_removeLayerBtn->Enable(true);
			//}
		}



		////--------------------------------------------
		//void FoliageWindow::OnSelectTool(FoliageToolPtr tool)
		//{
		//	populateToolProperties();
		//}



		//--------------------------------------------
		void FoliageWindow::setAddFoliageToggled( bool status )
		{
			m_bIsPortalAddToggled = status;
		}



		//--------------------------------------------
		bool FoliageWindow::GetAddFoliageToggled()
		{
			return m_bIsPortalAddToggled;
		}



		//--------------------------------------------
		int FoliageWindow::GetFoliageIdx()
		{
			return 0;
		}



		//--------------------------------------------
		void FoliageWindow::populateToolProperties()
		{
			clearToolProperties();

			//ApplicationManager* appRoot = IApplicationManager::instance();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//FoliageToolPtr curFoliageTool = foliageManager->getSelectedTool();
			//SmartPtr<Properties> properties = curFoliageTool->getProperties();
			//wxGUIUtil::populateProperties(*properties, m_toolPropertyGrid);
		}



		//--------------------------------------------
		void FoliageWindow::populateFoliageLayers()
		{
			//clearFoliageLayers();

			//wxListItem itemCol;
			//itemCol.setText(_T("Name"));
			//itemCol.setWidth(500);
			//m_vegetationLayers->InsertColumn(0, itemCol);

			//EditorSystem* appRoot = EditorSystem::getSingletonPtr();
			//FoliageManagerPtr foliageManager = appRoot->getFoliageManager();

			//Array<FoliageLayerPtr> layers = foliageManager->getLayers();
			//for(u32 i=0; i<layers.size(); ++i)
			//{
			//	FoliageLayerPtr layer = layers[i];

			//	String name = layer->getLabel();
			//	wxString label(name.c_str());

			//	wxListItem itemData;
			//	itemData.setText(label);
			//	itemData.setData(layer.get());
			//	itemData.setId(i);
			//	long tmp = m_vegetationLayers->InsertItem(itemData);
			//	if(tmp == -1)
			//	{
			//		LOG_MESSAGE("FoliageWindow", "Could not add layer to list. ");
			//	}
			//}
		}



		//--------------------------------------------
		void FoliageWindow::populateLayerProperties()
		{
			clearLayerProperties();
			/*

			SmartPtr<IActor> layer = FoliageManager::getSingletonPtr()->getSelectedLayer();

			CompPropertiesPtr compProperties = layer->GetComponent(CompProperties::Type);
			if(!compProperties)
				return;

			const Properties& propertyGroup = *compProperties->getPropertyGroup();
			wxGUIUtil::populateProperties(propertyGroup, m_layerPropertyGrid);
			*/
		}



		//--------------------------------------------
		void FoliageWindow::clearToolProperties()
		{
			m_toolPropertyGrid->Clear();
		}



		//--------------------------------------------
		void FoliageWindow::clearFoliageLayers()
		{
			m_vegetationLayers->ClearAll();
		}



		//--------------------------------------------
		void FoliageWindow::clearLayerProperties()
		{
			m_layerPropertyGrid->Clear();
		}



		//FoliageWindow::FoliageWindowListener::FoliageWindowListener( FoliageWindow* foliageWindow ) : m_foliageWindow(foliageWindow)
		//{
		//}

		//FoliageWindow::FoliageWindowListener::~FoliageWindowListener()
		//{
		//}

		//void FoliageWindow::FoliageWindowListener::OnCreateLayer()
		//{
		//	m_foliageWindow->populateFoliageLayers();
		//}

		//void FoliageWindow::FoliageWindowListener::OnDestroyLayer()
		//{

		//}

		//void FoliageWindow::FoliageWindowListener::OnSelectLayer()
		//{
		//	m_foliageWindow->OnSelectLayer();
		//}

		//void FoliageWindow::FoliageWindowListener::OnSelectTool( FoliageTool* tool )
		//{

		//}

		//void FoliageWindow::FoliageWindowListener::OnDeselectTool( FoliageTool* tool )
		//{

		//}



	} // end namespace editor
} // end namespace fb


