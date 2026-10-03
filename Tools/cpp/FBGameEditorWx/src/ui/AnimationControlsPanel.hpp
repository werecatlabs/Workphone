#ifndef AnimationControlsPanel_h__
#define AnimationControlsPanel_h__


#include <GameEditorPrerequisites.hpp>
#include <wx/panel.hpp>
#include "TimeLineWidget.hpp"
#include <vector>



namespace fb
{
	namespace editor
	{
	
	
	

	
	
	
		struct _TimelineMarker
		{
			int m_position;
			bool m_showMarker;
		};
	
	
	
		typedef Array<_TimelineMarker> MarkersArrray;
	
	
	
		class AnimationControlsPanel : public wxPanel
		{
		public:
			AnimationControlsPanel(wxWindow *parent, int id);
			~AnimationControlsPanel();
	
			void OnScroll(wxScrollEvent& event);
			int GetCurWidth();
			void OnGotoStart(wxCommandEvent & WXUNUSED(event));
			void OnFastBackward(wxCommandEvent & WXUNUSED(event));
			void OnBackward(wxCommandEvent & WXUNUSED(event));
			void OnForward(wxCommandEvent & WXUNUSED(event));
			void OnFastForward(wxCommandEvent & WXUNUSED(event));
			void OnGotoEnd(wxCommandEvent & WXUNUSED(event));
			void OnAddMarker(wxCommandEvent & WXUNUSED(event));
			void OnClearMarkers(wxCommandEvent & WXUNUSED(event));
			void OnShowConfigDialog(wxCommandEvent & WXUNUSED(event));
			void onPopMenuConfigDialog(wxCommandEvent & event);
			void onPopMenuDeleteAllMarkers(wxCommandEvent & event);
			void onRightMouseBtnUp(wxMouseEvent& event);
	
			// main timeline position functions
			void setMainTimeLinePos(int pos);
	
			// timeline markers functions
			void AddMarker(_TimelineMarker marker);
			bool IsMarker(int pos);
			bool ModifyMarker(int oldPos, int newPos);
			MarkersArrray* GetMarkers(); 
			void ClearMarkers();
			bool DeleteMarker(int pos);
			void ShowMarker(int pos, bool show = true);
	
			// members
			wxWindow *m_parent;
			wxTimeline *m_cWid;
			wxSlider *m_slider;
			wxTextCtrl *m_txtCtrlPos;
			wxButton *m_gotoStart;
			wxButton *m_fastBackward;
			wxButton *m_backward;
			wxButton *m_forward;
			wxButton *m_fastForwards;
			wxButton *m_gotoEnd;
			wxButton *m_addMarker;
			wxButton *m_clearMarkers;
			wxButton *m_configDialog;
	
			int m_nrFrames;
			int m_steps;
	
			// main timeline position
			int m_mainTimelinePos;
	
			// timeline marker
			MarkersArrray m_markers;
		};
	
	
	} // end namespace editor
} // end namespace fb



#endif // AnimationControlsPanel_h__