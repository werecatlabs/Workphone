#include <GameEditorPCH.hpp>
#include <wx/wx.hpp>
#include "TimeLineWidget.hpp"
#include "AnimationControlsPanel.hpp"



namespace fb
{	
	namespace editor
	{



		wxTimeline::wxTimeline(wxPanel *parent, int id)
			: wxPanel(parent, id, wxDefaultPosition, wxSize(785, 30), wxSUNKEN_BORDER)
		{
		
			m_parent = parent;
			m_leftButtonPushed = false;
			m_mainPosGrabbed = false;
			m_markerGrabbed = false;
		
			m_timer = new wxTimer(this, 1);
		
			m_numberOfFrames = 100;
			m_step = 5;
		
			Connect(wxEVT_PAINT, wxPaintEventHandler(wxTimeline::OnPaint)); 
			Connect(wxEVT_SIZE, wxSizeEventHandler(wxTimeline::OnSize)); 
			Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(wxTimeline::OnLeftMouseDown)); 
			Connect(wxEVT_LEFT_UP, wxMouseEventHandler(wxTimeline::OnLeftMouseUp));
			Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(wxTimeline::OnRightMouseDown)); 
			Connect(wxEVT_TIMER, wxCommandEventHandler(wxTimeline::OnTimer));
		
			m_timer->Start(200);
		}
		
		wxTimeline::~wxTimeline()
		{
			m_timer->Stop();
			if(m_timer)
				delete m_timer;
		}
		
		void wxTimeline::OnPaint(wxPaintEvent& event)
		{
		
		
			wxFont font(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
				wxFONTWEIGHT_NORMAL, false, wxT("Courier 10 Pitch"));
		
			wxPaintDC dc(this);
			dc.SetFont(font);
			wxSize size = GetSize();
			int width = size.GetWidth() - 5;
		
			AnimationControlsPanel *animPanel = (AnimationControlsPanel *) m_parent/*->GetParent()*/;
			if(!animPanel)
				return;
		
			int cur_width = animPanel->GetCurWidth();
			MarkersArrray *markers = animPanel->GetMarkers();
		
			float nrFramesTemp = m_numberOfFrames;
			int asize = m_numberOfFrames / m_step;
			float step = width / (nrFramesTemp / m_step);
		
			if(m_leftButtonPushed && m_mainPosGrabbed)
			{
				// get mouse position
				int mx = wxGetMousePosition().x - this->GetScreenPosition().x;
				int my = wxGetMousePosition().y - this->GetScreenPosition().y;
		
				// make marker
				cur_width = (int)(mx / (width / nrFramesTemp));
				if(cur_width < 0)
					cur_width = 0;
				if(cur_width > m_numberOfFrames)
					cur_width = m_numberOfFrames;
		
				animPanel->setMainTimeLinePos(cur_width);
			}
		
			float till = (width / nrFramesTemp) * cur_width;
			//int full = (int) ((width / m_numberOfFrames) * 23);
		
		
			/*if (cur_width >= 23) {
		
				dc.setPen(wxPen(wxColour(255, 255, 184))); 
				dc.setBrush(wxBrush(wxColour(255, 255, 184)));
				dc.DrawRectangle(0, 0, full, 30);
				dc.setPen(wxPen(wxColour(255, 175, 175)));
				dc.setBrush(wxBrush(wxColour(255, 175, 175)));
				dc.DrawRectangle(full, 0, till-full, 30);
		
			} else*/ { 
		
				dc.SetPen(wxPen(wxColour(255, 255, 184)));
				dc.SetBrush(wxBrush(wxColour(255, 255, 184)));
				dc.DrawRectangle(0, 0, floor(till), 30);
	
				// draw main line
				dc.SetPen(wxPen(wxColour(128, 128, 128), 6, wxSOLID));
				dc.DrawLine(floor(till), 0, floor(till), 30);
			}
	
			dc.SetPen(wxPen(wxColour(90, 80, 60)));
			for ( int i=1; i <= asize; i++ ) {
	
				dc.DrawLine(floor(i*step), 0, floor(i*step), 6);
				int timelineValue = m_step * i;
				wxSize size = dc.GetTextExtent(wxString::Format(wxT("%d"), timelineValue));
				dc.DrawText(wxString::Format(wxT("%d"), timelineValue), 
					floor(i*step-size.GetWidth()/2), 8);
		
		
			}
		
			// add markers
			if(markers)
			{
				dc.SetPen(wxPen(wxColour(255, 0, 0), 2, wxSOLID));
				MarkersArrray::iterator it;
				for(it = markers->begin(); it != markers->end(); it ++)
				{
					_TimelineMarker marker = (*it);
					if(!marker.m_showMarker)
						continue;
					int markerPos = (int) ((width / nrFramesTemp) * marker.m_position);
					dc.DrawLine(markerPos, 0, markerPos, 30);
				}
			}
		
			if(m_leftButtonPushed && m_markerGrabbed)
			{
				// get mouse position
				int mx = wxGetMousePosition().x - this->GetScreenPosition().x;
				int my = wxGetMousePosition().y - this->GetScreenPosition().y;
		
				// make marker
				int markerPos = (int)(mx / (width / nrFramesTemp));
				if(markerPos < 0)
					markerPos = 0;
				if(markerPos > m_numberOfFrames)
					markerPos = m_numberOfFrames;
		
				m_tempMarkerEndPos = markerPos;
				markerPos = (int) ((width / nrFramesTemp) * markerPos);
				dc.SetPen(wxPen(wxColour(255, 0, 0), 2, wxSOLID));
				dc.DrawLine(markerPos, 0, markerPos, 30);
			}
		}
		
		int wxTimeline::GetNumberOfFrames()
		{
			return m_numberOfFrames;
		}

		void wxTimeline::setNumberOfFrames(int frames)
		{
			m_numberOfFrames = frames;
		}

		int wxTimeline::GetNumberOfSteps()
		{
			return m_step;
		}

		void wxTimeline::setNumberOfSteps(int step)
		{
			m_step = step;
		}

		void wxTimeline::OnSize(wxSizeEvent& event)
		{
			Refresh();
		}
		
		void wxTimeline::OnLeftMouseDown(wxMouseEvent & WXUNUSED(event))
		{
			// get size
			wxSize size = GetSize();
			int width = size.GetWidth() - 5;
		
			// get mouse position
			int mx = wxGetMousePosition().x - this->GetScreenPosition().x;
			int my = wxGetMousePosition().y - this->GetScreenPosition().y;
		
			// make marker
			float nrFramesTemp = m_numberOfFrames;
			int markerPos = (int)(mx / (width / nrFramesTemp));
			if(markerPos < 0)
				markerPos = 0;
			if(markerPos > m_numberOfFrames)
				markerPos = m_numberOfFrames;
		
			AnimationControlsPanel *animPanel = (AnimationControlsPanel *) m_parent/*->GetParent()*/;
			if(animPanel)
			{
				// check if we had main position selected
				if(markerPos == animPanel->m_mainTimelinePos)
				{
					m_leftButtonPushed = true;
					m_mainPosGrabbed = true;
				}
		
				// check if we had a marker
				if(!m_mainPosGrabbed)
				{
					if(animPanel->IsMarker(markerPos))
					{
						m_tempMarkerStartPos = markerPos;
						animPanel->ShowMarker(markerPos, false);
						m_leftButtonPushed = true;
						m_markerGrabbed = true;
					}
				}
		
				// add marker
				if(!m_mainPosGrabbed && !m_markerGrabbed)
				{
					_TimelineMarker marker;
					marker.m_position = markerPos;
					marker.m_showMarker = true;
					animPanel->AddMarker(marker);
				}
			}
		
			Refresh();
		}
		
		void wxTimeline::OnLeftMouseUp(wxMouseEvent & WXUNUSED(event))
		{
			AnimationControlsPanel *animPanel = (AnimationControlsPanel *) m_parent/*->GetParent()*/;
			if(animPanel)
			{
				if(animPanel->m_slider)
					animPanel->m_slider->SetValue(animPanel->m_mainTimelinePos);
		
				if(m_markerGrabbed)
				{
					// check if we already have a marker to this position
					if(!animPanel->IsMarker(m_tempMarkerEndPos))
					{
						animPanel->ModifyMarker(m_tempMarkerStartPos, m_tempMarkerEndPos);
						animPanel->ShowMarker(m_tempMarkerEndPos);
					}
					else
						animPanel->ShowMarker(m_tempMarkerStartPos);
				}
			}
		
			m_leftButtonPushed = false;
			m_mainPosGrabbed = false;
			m_markerGrabbed = false;
		
			Refresh();
		}
		
		void wxTimeline::OnRightMouseDown(wxMouseEvent & WXUNUSED(event))
		{
			// get size
			wxSize size = GetSize();
			int width = size.GetWidth() - 5;
		
			// get mouse position
			int mx = wxGetMousePosition().x - this->GetScreenPosition().x;
			int my = wxGetMousePosition().y - this->GetScreenPosition().y;
		
			// make marker
			float nrFramesTemp = m_numberOfFrames;
			int markerPos = (int)(mx / (width / nrFramesTemp));
			if(markerPos < 0)
				markerPos = 0;
			if(markerPos > m_numberOfFrames)
				markerPos = m_numberOfFrames;
		
			AnimationControlsPanel *animPanel = (AnimationControlsPanel *) m_parent/*->GetParent()*/;
			if(animPanel)
				animPanel->DeleteMarker(markerPos);
		
			Refresh();
		}
		
		void wxTimeline::OnTimer(wxCommandEvent& event)
		{
			if(m_leftButtonPushed)
			{
				Refresh();
			}
		}
		
		
	} // end namespace editor
} // end namespace fb
