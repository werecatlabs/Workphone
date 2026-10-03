#ifndef TimeLineWidget_h__
#define TimeLineWidget_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/panel.hpp>


namespace fb
{
	namespace editor
	{



		class wxTimeline : public wxPanel
		{
		public:
			wxTimeline(wxPanel *parent, int id );
			~wxTimeline();

			// methods
		public:
			int GetNumberOfFrames();
			void setNumberOfFrames(int frames);
			int GetNumberOfSteps();
			void setNumberOfSteps(int step);

			void OnSize(wxSizeEvent& event);
			void OnPaint(wxPaintEvent& event);
			void OnLeftMouseDown(wxMouseEvent& WXUNUSED(event));
			void OnLeftMouseUp(wxMouseEvent& WXUNUSED(event));
			void OnRightMouseDown(wxMouseEvent& WXUNUSED(event));
			void OnTimer(wxCommandEvent& event);
	
			// members
		protected:
			wxPanel *m_parent;
			bool m_leftButtonPushed;
			bool m_mainPosGrabbed;
			bool m_markerGrabbed;
	
			int m_tempMarkerStartPos;
			int m_tempMarkerEndPos;
	
			wxTimer *m_timer;
	
			int m_numberOfFrames;
			int m_step;
		};



	} // end namespace editor
} // end namespace fb



#endif // TimeLineWidget_h__


