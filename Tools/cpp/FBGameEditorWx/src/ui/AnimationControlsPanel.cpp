#include <GameEditorPCH.hpp>
#include "AnimationControlsPanel.hpp"
#include "ui/TimeLineWidget.hpp"
#include "ui/TimeLineConfigDialog.hpp"
#include <wx/wx.hpp>



int ID_SLIDER = 1;
int ID_BTN_ST = 2;
int ID_BTN_FB = 3;
int ID_BTN_B = 4;
int ID_BTN_F = 5;
int ID_BTN_FF = 6;
int ID_BTN_END = 7;
int ID_BTN_ADD_MARKER = 8;
int ID_BTN_CLEAR = 9;
int ID_BTN_CONFIG = 10;

#define ID_POPMENU_CONFIG wxID_HIGHEST + 1
#define ID_POPMENU_DELETE_MARKERS ID_POPMENU_CONFIG + 1



namespace fb
{
	namespace editor
	{

	
				
		AnimationControlsPanel::AnimationControlsPanel(wxWindow *parent, int id)
			: wxPanel(parent, id, wxDefaultPosition, wxSize(-1, 100), wxSUNKEN_BORDER)
		{
			m_parent = parent;
		
			m_nrFrames = 133;
			m_steps = 5;
		
			auto vbox = new wxBoxSizer(wxVERTICAL);
			auto hbox1 = new wxBoxSizer(wxHORIZONTAL);
			auto hbox2 = new wxBoxSizer(wxHORIZONTAL);
		
			m_slider = new wxSlider(this, ID_SLIDER, 10, 0, m_nrFrames, wxPoint(-1, -1), 
				wxSize(100, -1), wxSL_LABELS);
		
			m_mainTimelinePos = 10;
		
			m_txtCtrlPos = new wxTextCtrl(this, -1, wxT(""), wxPoint(-1, -1), wxSize(40, -1));
			m_txtCtrlPos->SetEditable(false);
		
			m_cWid = new wxTimeline(this, wxID_ANY);
			m_cWid->setNumberOfFrames(m_nrFrames);
			m_cWid->setNumberOfSteps(m_steps);	
		
			m_gotoStart = new wxButton(this, ID_BTN_ST, wxT("|<<"), wxDefaultPosition, wxSize(25, -1));
			m_fastBackward = new wxButton(this, ID_BTN_FB, wxT("<<"), wxDefaultPosition, wxSize(25, -1));
			m_backward = new wxButton(this, ID_BTN_B, wxT("<"), wxDefaultPosition, wxSize(25, -1));
			m_forward = new wxButton(this, ID_BTN_F, wxT(">"), wxDefaultPosition, wxSize(25, -1));
			m_fastForwards = new wxButton(this, ID_BTN_FF, wxT(">>"), wxDefaultPosition, wxSize(25, -1));
			m_gotoEnd = new wxButton(this, ID_BTN_END, wxT(">>|"), wxDefaultPosition, wxSize(25, -1));
			m_addMarker = new wxButton(this, ID_BTN_ADD_MARKER, wxT("Add marker"), wxDefaultPosition, wxSize(100, -1));
			m_clearMarkers = new wxButton(this, ID_BTN_CLEAR, wxT("Delete all markers"), wxDefaultPosition, wxSize(100, -1));
			m_configDialog = new wxButton(this, ID_BTN_CONFIG, wxT("Config timeline"), wxDefaultPosition, wxSize(100, -1));
		
			hbox2->Add(m_cWid);
			hbox2->Add(m_txtCtrlPos, 0, wxLEFT, 10);
			hbox2->Add(m_gotoStart, 1, wxLEFT, 10);
			hbox2->Add(m_fastBackward);
			hbox2->Add(m_backward);
			hbox2->Add(m_forward);
			hbox2->Add(m_fastForwards);
			hbox2->Add(m_gotoEnd);
		
			vbox->Add(hbox2, 1, wxALIGN_LEFT | wxLEFT | wxTOP, 10);
		
			//hbox1->Add(new wxPanel(this, -1, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER));
			hbox1->Add(m_addMarker);
			hbox1->Add(m_clearMarkers);
			hbox1->Add(m_slider, 1, wxLEFT, 10);
			hbox1->Add(m_configDialog, 1, wxLEFT, 10);
			vbox->Add(hbox1, 1, wxALIGN_LEFT | wxLEFT | wxTOP, 10);
		
			this->SetSizer(vbox);
		
			Connect(ID_SLIDER, wxEVT_COMMAND_SLIDER_UPDATED, 
				wxScrollEventHandler(AnimationControlsPanel::OnScroll)); 
		
			Connect(ID_BTN_ST, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnGotoStart));
			Connect(ID_BTN_FB, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnFastBackward)); 
			Connect(ID_BTN_B, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnBackward)); 
			Connect(ID_BTN_F, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnForward)); 
			Connect(ID_BTN_FF, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnFastForward)); 
			Connect(ID_BTN_END, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnGotoEnd)); 
			Connect(ID_BTN_ADD_MARKER, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnAddMarker));
			Connect(ID_BTN_CLEAR, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnClearMarkers));
			Connect(ID_BTN_CONFIG, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(AnimationControlsPanel::OnShowConfigDialog));
	
			Connect(wxEVT_RIGHT_DOWN,
				wxMouseEventHandler(AnimationControlsPanel::onRightMouseBtnUp));
	
			Connect(ID_POPMENU_CONFIG, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(AnimationControlsPanel::onPopMenuConfigDialog));
	
			Connect(ID_POPMENU_DELETE_MARKERS, wxEVT_COMMAND_MENU_SELECTED, 
				wxCommandEventHandler(AnimationControlsPanel::onPopMenuDeleteAllMarkers));		
		}
		


		AnimationControlsPanel::~AnimationControlsPanel()
		{
			if (m_cWid)
			{
				delete m_cWid;
				m_cWid = nullptr;
			}

			if (m_slider)
			{
				delete m_slider;
				m_slider = nullptr;
			}

			if (m_txtCtrlPos)
			{
				delete m_txtCtrlPos;
				m_txtCtrlPos = nullptr;
			}

			if (m_gotoStart)
			{
				delete m_gotoStart;
				m_gotoStart = nullptr;
			}

			if (m_fastBackward)
			{
				delete m_fastBackward;
				m_fastBackward = nullptr;
			}

			if (m_backward)
			{
				delete m_backward;
				m_backward = nullptr;
			}

			if (m_forward)
			{
				delete m_forward;
				m_forward = nullptr;
			}

			if (m_fastForwards)
			{
				delete m_fastForwards;
				m_fastForwards = nullptr;
			}

			if (m_gotoEnd)
			{
				delete m_gotoEnd;
				m_gotoEnd = nullptr;
			}
		}
		
		int AnimationControlsPanel::GetCurWidth() 
		{
			return m_mainTimelinePos;
		}
		
		void AnimationControlsPanel::setMainTimeLinePos(int pos)
		{
			m_mainTimelinePos = pos;
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//if(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnScroll(wxScrollEvent& WXUNUSED(event))
		{
			m_mainTimelinePos = m_slider->GetValue();
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//if(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnGotoStart(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos = 0;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//i/f(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnFastBackward(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos -= 4;
			if(m_mainTimelinePos < 0)
				m_mainTimelinePos = 0;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//if(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnBackward(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos -= 1;
			if(m_mainTimelinePos < 0)
				m_mainTimelinePos = 0;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//if(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnForward(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos += 1;
			if(m_mainTimelinePos > m_nrFrames)
				m_mainTimelinePos = m_nrFrames;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		
			//GUIEditorWindow *edWin = (GUIEditorWindow *) m_parent->GetParent();
			//if(edWin)
			//	edWin->WalkButton(m_mainTimelinePos);
		}
		
		void AnimationControlsPanel::OnFastForward(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos += 4;
			if(m_mainTimelinePos > m_nrFrames)
				m_mainTimelinePos = m_nrFrames;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		}
		
		void AnimationControlsPanel::OnGotoEnd(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos = m_nrFrames;
			m_slider->SetValue(m_mainTimelinePos);
			m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
			m_cWid->Refresh();
		}
		
		void AnimationControlsPanel::OnAddMarker(wxCommandEvent & WXUNUSED(event))
		{
			m_mainTimelinePos = m_slider->GetValue();
		
			//add marker
			_TimelineMarker marker;
			marker.m_position = m_mainTimelinePos;
			marker.m_showMarker = true;
			AddMarker(marker);
			m_cWid->Refresh();
		}
		
		void AnimationControlsPanel::OnClearMarkers(wxCommandEvent & WXUNUSED(event))
		{
			//clear markers
			ClearMarkers();
			m_cWid->Refresh();
		}
		
		
		// timeline markers functions
		void AnimationControlsPanel::AddMarker(_TimelineMarker marker)
		{
			if(!IsMarker(marker.m_position))
				m_markers.push_back(marker);
		}
		
		bool AnimationControlsPanel::IsMarker(int pos)
		{
			MarkersArrray::iterator it;
			for(it = m_markers.begin(); it != m_markers.end(); it ++)
			{
				_TimelineMarker marker = (*it);
				if(marker.m_position == pos)
					return true;
			}
		
			return false;
		}
		
		bool AnimationControlsPanel::ModifyMarker(int oldPos, int newPos)
		{
			MarkersArrray::iterator it;
			for(it = m_markers.begin(); it != m_markers.end(); it ++)
			{
				_TimelineMarker *marker = &(*it);
				if(marker->m_position == oldPos)
				{
					//marker->m_showMarker = true;
					marker->m_position = newPos;
					return true;
				}
			}
		
			return false;
		}
		
		void AnimationControlsPanel::ShowMarker(int pos, bool show /*= true*/)
		{
			MarkersArrray::iterator it;
			for(it = m_markers.begin(); it != m_markers.end(); it ++)
			{
				_TimelineMarker *marker = &(*it);
				if(marker->m_position == pos)
				{
					marker->m_showMarker = show;
					break;
				}
			}
		}
		
		bool AnimationControlsPanel::DeleteMarker(int pos)
		{
			MarkersArrray::iterator it;
			for(it = m_markers.begin(); it != m_markers.end(); it ++)
			{
				_TimelineMarker marker = (*it);
				if(marker.m_position == pos)
				{
					m_markers.erase(it);
					return true;
				}
			}
		
			return false;
		}
		
		void AnimationControlsPanel::ClearMarkers()
		{
			m_markers.clear();
		}
		
		MarkersArrray* AnimationControlsPanel::GetMarkers()
		{
			return &m_markers;
		}
		
		void AnimationControlsPanel::OnShowConfigDialog(wxCommandEvent & WXUNUSED(event))
		{
			TimeLineConfigDialog *dlg = new TimeLineConfigDialog(wxT("TimeLine Config Dialog"));
			//dlg->ShowModal();
			//dlg->Show(true);
			if(dlg->ShowModal() == wxID_OK)
			{
				m_nrFrames = dlg->getNrFrames();
				m_steps = dlg->getStep();
	
				m_cWid->setNumberOfFrames(m_nrFrames);
				m_cWid->setNumberOfSteps(m_steps);
	
				m_mainTimelinePos = 0;
				m_slider->SetMax(m_nrFrames);
				m_slider->SetValue(m_mainTimelinePos);
				m_txtCtrlPos->SetLabelText(wxString::Format(wxT("%d"), m_mainTimelinePos));
				m_cWid->Refresh();
			}
			dlg->Destroy();
		}
	
	
		void AnimationControlsPanel::onPopMenuConfigDialog(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			OnShowConfigDialog(event);
		}
	
	
		void AnimationControlsPanel::onPopMenuDeleteAllMarkers(wxCommandEvent& WXUNUSED(event))
		{
			wxCommandEvent event;
			OnClearMarkers(event);
		}
	
	
		void AnimationControlsPanel::onRightMouseBtnUp(wxMouseEvent& event)
		{
			event.Skip();
	
			wxMenu *menu;
			wxPoint point;
	
			// get mouse position
			point.x = wxGetMousePosition().x;
			point.y = wxGetMousePosition().y;
			point = this->ScreenToClient(point);
	
			// to get a window location if required use
			//int id = wxFindWindowAtPoint(point);
	
			// create menu 
			menu = new wxMenu();
	
			// add stuff
			menu->Append(ID_POPMENU_DELETE_MARKERS, wxT("Delete all markers"), wxT(""));
			menu->AppendSeparator();
			menu->Append(ID_POPMENU_CONFIG, wxT("Show timeline configuration dialog"), wxT(""));
			menu->AppendSeparator();
			menu->Append(-1, wxT("Cancel"), wxT(""));
	
			// and then display
			PopupMenu(menu, point.x, point.y);
	
			if(menu)
				delete menu;
		}
	
	
	
	} //namespace editor
}