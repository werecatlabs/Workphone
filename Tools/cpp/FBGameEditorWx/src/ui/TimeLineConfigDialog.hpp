#ifndef TimeLineConfigDialog_h__
#define TimeLineConfigDialog_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/dialog.hpp>



namespace fb
{	
	namespace editor
	{



		class TimeLineConfigDialog : public wxDialog
		{
		public:
			TimeLineConfigDialog(const wxString& title);

			int getNrFrames();
			void setNrFrames(int frames);
			int getStep();
			void setStep(int step);

			void OnOK(wxCommandEvent& WXUNUSED(event));

		protected:
			int m_nrFrames = 0;
			int m_step = 0;

			wxTextCtrl* m_txtCtrlNrFrames = nullptr;
			wxTextCtrl* m_txtCtrlStep = nullptr;
		};



	} // end namespace editor
} // end namespace fb


#endif // TimeLineConfigDialog_h__


