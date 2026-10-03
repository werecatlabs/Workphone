#include <GameEditorPCH.hpp>
#include <ui/TextureWindow.hpp>
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
#include "wx/listctrl.hpp"
#include <wx/aui/framemanager.hpp>
#include "wxGUIUtil.hpp"
#include "editor/EditorManager.hpp"
#include <FBCore/Interface/Actor/IActor.hpp>
//#include "terrain/RoadManager.hpp"



namespace fb
{
	namespace editor
	{




		//--------------------------------------------
		TextureWindow::TextureWindow(wxWindow* parent, wxWindowID id, const wxPoint& pos,
							 const wxSize& size, long style, const wxValidator& validator, const wxString& name)
							 :
							 m_createRoadBtn(NULL),
							 m_lastCreatedEntity(nullptr),
							 m_isCreateRoadBtnToggled(false)
		{
			auto window = new wxScrolledWindow(parent, id, pos, size, style);
			setWindow(window);

			//m_pGridBagSizer = new wxGridSizer(0,0,0,0);
			wxBoxSizer* m_pButtonBox = new wxBoxSizer(wxVERTICAL);
			window->SetSizer(m_pButtonBox);

			m_createRoadBtn = new wxButton(window, ID_RoadFrameCreateRoad, wxString("Create Road"));
			m_pButtonBox->Add(m_createRoadBtn);

			m_addRoadNodeBtn = new wxToggleButton(window, ID_RoadFrameAddRoadNode, wxString("Add Road Node"));
			m_pButtonBox->Add(m_addRoadNodeBtn);

			m_roadPropertyGrid = new wxPropertyGrid(window, ID_RoadFrameProperties, wxDefaultPosition, wxSize(400, 400),
													wxPG_SPLITTER_AUTO_CENTER |
													wxPG_BOLD_MODIFIED);
			m_pButtonBox->Add(m_roadPropertyGrid, 1, wxEXPAND);

			//m_roadManagerListener = new RoadManagerListener(this);
		}



		//--------------------------------------------
		TextureWindow::~TextureWindow()
		{
			//ApplicationManager* applicationManager = IApplicationManager::instance();
			//RoadManagerPtr roadManager = applicationManager->getRoadManager();
			//if ( roadManager )
			//{
			//	roadManager->removeRoadManagerListener(m_roadManagerListener.get());
			//}
		}



		//--------------------------------------------
		void TextureWindow::setCreateRoadBtnToggled(bool status)
		{
			m_isCreateRoadBtnToggled = status;
		}



		//--------------------------------------------
		bool TextureWindow::isCreateRoadBtnToggled()
		{
			return m_isCreateRoadBtnToggled;
		}



		//--------------------------------------------
		void TextureWindow::setLastRoadNodeCreated(SmartPtr<IActor> riverNode)
		{
			m_lastCreatedEntity = riverNode;
		}



		//--------------------------------------------
		SmartPtr<IActor> TextureWindow::getLastRoadNodeCreated() const
		{
			return m_lastCreatedEntity;
		}



		//--------------------------------------------
		void TextureWindow::OnCreateRoad(wxCommandEvent& event)
		{
			//ApplicationManager* applicationManager = IApplicationManager::instance();
			//RoadManagerPtr roadManager = applicationManager->getRoadManager();
			//if ( roadManager )
			//{
			//	roadManager->createRoad();
			//}
		}



		//--------------------------------------------
		void TextureWindow::OnAddRoadNodeToggled(wxCommandEvent& event)
		{
			EditorManager* applicationManager = EditorManager::getSingletonPtr();
			//RoadManagerPtr roadManager = applicationManager->getRoadManager();

			//if ( event.GetInt() )
			//{
			//	m_isCreateRoadBtnToggled = true;

			//	if ( roadManager )
			//	{
			//		roadManager->setCreateRoadNode(m_isCreateRoadBtnToggled);
			//	}
			//}
			//else
			//{
			//	m_isCreateRoadBtnToggled = false;

			//	if ( roadManager )
			//	{
			//		roadManager->setCreateRoadNode(m_isCreateRoadBtnToggled);
			//	}
			//}
		}



		//--------------------------------------------
		void TextureWindow::OnPropertyGridChange(wxPropertyGridEvent& event)
		{
			wxPGProperty* p = event.GetProperty();

			//SmartPtr<IActor> road = RoadManager::getSingletonPtr()->getSelectedRoad();
			//if ( !road.isNull() )
			//{
			//	String name = p->GetName().c_str();
			//	String value = m_roadPropertyGrid->GetPropertyValueAsString(p).c_str();
			//	road->setPropertyValue(name, value);
			//}
		}



		//--------------------------------------------
		void TextureWindow::OnCreateRoad()
		{
			populateLayerProperties();
			populateFoliageLayers();
		}



		//--------------------------------------------
		void TextureWindow::populateFoliageLayers()
		{
			//m_riversList->ClearAll();

			//wxListItem itemCol;
			//itemCol.setText(_T("Name"));
			//itemCol.setWidth(500);
			//m_riversList->InsertColumn(0, itemCol);

			//Array<SmartPtr<IActor>> layers = RoadManager::getSingletonPtr()->getRoads();
			//for ( u32 i = 0; i < layers.size(); ++i )
			//{
			//	SmartPtr<IActor> layer = layers[i];
			//	String name = layer->GetName();
			//	wxString buf(name.c_str());
			//	long tmp = m_riversList->InsertItem(i, buf, 0);
			//}
		}



		//--------------------------------------------
		void TextureWindow::populateLayerProperties()
		{
			m_roadPropertyGrid->Clear();

			//EditorEntityPtr layer = RoadManager::getSingletonPtr()->getSelectedRoad();
			//if ( !layer.isNull() )
			//{
			//	Properties propertyGroup;
			//	layer->getProperties(propertyGroup);
			//	wxGUIUtil::populateProperties(propertyGroup, m_roadPropertyGrid);
			//}
		}



		//--------------------------------------------
		void TextureWindow::OnDestroyRoad()
		{
			populateLayerProperties();
			populateFoliageLayers();
		}



		//--------------------------------------------
		void TextureWindow::OnSelectRoad()
		{
			populateLayerProperties();
		}



		//RoadFrame::RoadManagerListener::~RoadManagerListener()
		//{

		//}

		//RoadFrame::RoadManagerListener::RoadManagerListener(RoadFrame* roadFrame) 
		//	: m_roadFrame(roadFrame)
		//{

		//}

	} // end namespace editor	
} // end namespace fb