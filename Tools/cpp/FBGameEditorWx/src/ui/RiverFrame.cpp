#include <GameEditorPCH.hpp>
#include "RiverFrame.hpp"
#include <wx//grid.hpp>
#include <wx//treectrl.hpp>
#include <wx//spinctrl.hpp>
#include <wx//artprov.hpp>
#include <wx//clipbrd.hpp>
#include <wx//image.hpp>
#include <wx//colordlg.hpp>
#include <wx//wxhtml.hpp>
#include <wx//tglbtn.hpp>
#include <wx//gbsizer.hpp>
#include <wx//sizer.hpp>
#include "wx/listctrl.hpp"
#include <wx//aui//framemanager.hpp>
#include "wxGUIUtil.hpp"
#include <FBCore/FBCoreHeaders.hpp>


namespace fb
{
	namespace editor
	{



		//------------------------------------------------
		class RiverListCtrl : public wxListCtrl
		{
		public:
			RiverListCtrl(RiverFrame* vegetationFrame, wxWindow *parent,
						  const wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
						  :
						  wxListCtrl(parent, id, pos, size, style),
						  m_attr(*wxBLUE, *wxLIGHT_GREY, wxNullFont),
						  m_vegetationFrame(vegetationFrame)
			{}

			void OnSelected(wxListEvent& event)
			{
				wxString string1;
				wxString string2;

				wxListItem info;
				info.m_itemId = event.m_itemIndex;
				info.m_col = 0;
				info.m_mask = wxLIST_MASK_TEXT;
				if ( GetItem(info) )
				{
					string1 = info.m_text.c_str();
				}
				else
				{
					wxFAIL_MSG(wxT("wxListCtrl::GetItem() failed"));
				}

				//RiverManager::getSingletonPtr()->selectRiver(string1.c_str());
			}

		private:
			wxLog *m_logOld;
			wxListItemAttr m_attr;
			RiverFrame* m_vegetationFrame;

			DECLARE_NO_COPY_CLASS(RiverListCtrl)
			//DECLARE_EVENT_TABLE()
		};


		//BEGIN_EVENT_TABLE(RiverListCtrl, wxListCtrl)
		//	EVT_LIST_ITEM_SELECTED(MyFrame::ID_RiverFrameRiverList, RiverListCtrl::OnSelected)
		//	END_EVENT_TABLE()

		//	BEGIN_EVENT_TABLE(RiverFrame, wxScrolledWindow)
		//	EVT_BUTTON(MyFrame::ID_RiverFrameCreateRiver, RiverFrame::OnCreateRiver)
		//	EVT_TOGGLEBUTTON(MyFrame::ID_RiverFrameAddRiverNode, RiverFrame::OnAddRiverNodeToggled)
		//	EVT_PG_CHANGED(MyFrame::ID_RiverFrameRiverProperties, RiverFrame::OnRiverPropertyGridChange)
		//	EVT_BUTTON(MyFrame::ID_RiverFrameCreateRiverEnt, RiverFrame::OnSwitchCamera)
		//	EVT_BUTTON(MyFrame::ID_CameraFrameNextNode, RiverFrame::OnNextCameraNode)
		//	EVT_BUTTON(MyFrame::ID_CameraFramePrevNode, RiverFrame::OnPrevCameraNode)
		//	END_EVENT_TABLE()


		//--------------------------------------------
		RiverFrame::RiverFrame(wxWindow* parent, wxWindowID id, const wxPoint& pos,
							   const wxSize& size, long style, const wxValidator& validator, const wxString& name)
							   : wxScrolledWindow(parent, id),
							   m_createRiverBtn(NULL),
							   m_lastCreatedEntity(nullptr),
							   m_isCreateRiverBtnToggled(false)
		{
			//m_pGridBagSizer = new wxGridSizer(0,0,0,0);
			wxBoxSizer* m_pButtonBox = new wxBoxSizer(wxVERTICAL);
			SetSizer(m_pButtonBox);

			m_createRiverBtn = new wxButton(this, ID_RiverFrameCreateRiver, wxString("Create River"));
			m_pButtonBox->Add(m_createRiverBtn);

			m_addRiverNodeBtn = new wxToggleButton(this, ID_RiverFrameAddRiverNode, wxString("Add River Node"));
			m_pButtonBox->Add(m_addRiverNodeBtn);

			m_insertRiverNodeBtn = new wxToggleButton(this, ID_RiverFrameAddRiverNode, wxString("Insert River Node"));
			m_pButtonBox->Add(m_insertRiverNodeBtn);

			m_riversList = new RiverListCtrl(this, this, ID_RiverFrameRiverList, wxDefaultPosition, wxDefaultSize,
											 wxLC_REPORT | wxSUNKEN_BORDER | wxLC_EDIT_LABELS);
			m_pButtonBox->Add(m_riversList, 1, wxEXPAND);

			m_riverPropertyGrid = new wxPropertyGrid(this, ID_RiverFrameRiverProperties, wxDefaultPosition, wxSize(400, 400),
													 wxPG_SPLITTER_AUTO_CENTER |
													 wxPG_BOLD_MODIFIED);
			m_pButtonBox->Add(m_riverPropertyGrid, 1, wxEXPAND);


			m_isCreateRiverBtnToggled = false;

			//RiverManager::getSingletonPtr()->addRiverManagerListener(this);
		}



		//--------------------------------------------
		RiverFrame::~RiverFrame()
		{
			//bool result = RiverManager::getSingletonPtr()->removeRiverManagerListener(this);
		}



		//--------------------------------------------
		void RiverFrame::OnSwitchCamera(wxCommandEvent& event)
		{}



		//--------------------------------------------
		void RiverFrame::OnNextCameraNode(wxCommandEvent& event)
		{
			//FollowSplineCameraPtr camera = IApplicationManager::instance()->getCameraManager()->findCamera("FollowSplineCamera");
			//camera->nextNode();
		}



		//--------------------------------------------
		void RiverFrame::OnPrevCameraNode(wxCommandEvent& event)
		{
			//FollowSplineCameraPtr camera = IApplicationManager::instance()->getCameraManager()->findCamera("FollowSplineCamera");
			//camera->prevNode();
		}



		//--------------------------------------------
		void RiverFrame::OnRiverPropertyGridChange(wxPropertyGridEvent& event)
		{
			//wxPGProperty* p = event.GetProperty();

			//SmartPtr<IActor> curRiver = RiverManager::getSingletonPtr()->getSelectedRiver();
			//CompPropertiesPtr compProperties = curRiver->GetComponent(CompProperties::Type);
			//if ( !compProperties.isNull() )
			//{
			//	SmartPtr<Properties> propertyGroup = compProperties->getPropertyGroup();
			//	String name = p->GetName().c_str();
			//	String value = m_riverPropertyGrid->GetPropertyValueAsString(p).c_str();
			//	propertyGroup->setPropertyValue(name, value);
			//}
		}



		//--------------------------------------------
		void RiverFrame::setCreateRiverBtnToggled(bool status)
		{
			m_isCreateRiverBtnToggled = status;
		}



		//--------------------------------------------
		bool RiverFrame::isCreateRiverBtnToggled()
		{
			return m_isCreateRiverBtnToggled;
		}



		//--------------------------------------------
		void RiverFrame::OnCreateRiver(wxCommandEvent& event)
		{
			//RiverManager::getSingletonPtr()->createRiver();
		}



		//--------------------------------------------
		void RiverFrame::OnAddRiverNodeToggled(wxCommandEvent& event)
		{
			if ( event.GetInt() )
			{
				m_isCreateRiverBtnToggled = true;
			}
			else
			{
				m_isCreateRiverBtnToggled = false;
			}
		}



		//--------------------------------------------
		void RiverFrame::setCurrentRiver(SmartPtr<IActor> riverNode)
		{
			m_lastCreatedEntity = riverNode;
		}



		//--------------------------------------------
		SmartPtr<IActor> RiverFrame::getCurrentRiver() const
		{
			return m_lastCreatedEntity;
		}



		//--------------------------------------------
		void RiverFrame::OnCreateRiver()
		{
			populateFoliageLayers();
			populateLayerProperties();
		}



		//--------------------------------------------
		void RiverFrame::OnDestroyRiver()
		{
			populateFoliageLayers();
			populateLayerProperties();
		}



		//--------------------------------------------
		void RiverFrame::OnSelectRiver()
		{
			//m_lastCreatedEntity = RiverManager::getSingletonPtr()->getSelectedRiver();
			//populateLayerProperties();
		}



		//--------------------------------------------
		void RiverFrame::populateFoliageLayers()
		{
			//m_riversList->ClearAll();

			//wxListItem itemCol;
			//itemCol.setText(_T("Name"));
			//itemCol.setWidth(500);
			//m_riversList->InsertColumn(0, itemCol);

			//Array<SmartPtr<IActor>> layers = RiverManager::getSingletonPtr()->getRivers();
			//for ( u32 i = 0; i < layers.size(); ++i )
			//{
			//	SmartPtr<IActor> layer = layers[i];
			//	String name = layer->GetName();
			//	wxString buf(name.c_str());
			//	long tmp = m_riversList->InsertItem(i, buf, 0);
			//	//m_listCtrl->setItemData(tmp, i);

			//	//buf = wxString(property->getValue().c_str());
			//	//m_listCtrl->setItem(i, 1, buf);
			//}
		}



		//--------------------------------------------
		void RiverFrame::populateLayerProperties()
		{
			//m_riverPropertyGrid->Clear();

			//EditorEntityPtr layer = RiverManager::getSingletonPtr()->getSelectedRiver();
			//if ( !layer.isNull() )
			//{
			//	const Properties& propertyGroup = *layer->getPropertyGroup();
			//	wxGUIUtil::populateProperties(propertyGroup, m_riverPropertyGrid);
			//}
		}


	} // end namespace editor	
} // end namespace fb