#include <GameEditorPCH.hpp>
#include "ui/TimeLineConfigDialog.hpp"
#include <wx/wx.hpp>


namespace fb
{	
	namespace editor
	{


		
		TimeLineConfigDialog::TimeLineConfigDialog(const wxString & title)
			: wxDialog(NULL, -1, title, wxDefaultPosition, wxSize(190, 135))
		{
		
			wxPanel *panel = new wxPanel(this, -1);
		
			wxBoxSizer *hbox1 = new wxBoxSizer(wxHORIZONTAL);
			wxBoxSizer *hbox2 = new wxBoxSizer(wxHORIZONTAL);
			wxBoxSizer *hbox3 = new wxBoxSizer(wxHORIZONTAL);
			wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
		
			wxStaticText *st1 = new wxStaticText(panel, wxID_ANY, 
				wxT("Number of frames"));
			m_txtCtrlNrFrames = new wxTextCtrl(panel, -1, wxT(""), wxPoint(-1, -1),
				wxSize(50, -1));
		
			wxStaticText *st2 = new wxStaticText(panel, wxID_ANY, 
				wxT("Step"));
			m_txtCtrlStep = new wxTextCtrl(panel, -1, wxT(""), wxPoint(-1, -1),
				wxSize(50, -1));
		
			hbox1->Add(st1, 1, wxLEFT, 10);
			hbox1->Add(m_txtCtrlNrFrames, 0, wxLEFT, 10);
			vbox->Add(hbox1, 1, wxALIGN_LEFT | wxLEFT | wxTOP, 10);
		
			hbox2->Add(st2, 1, wxLEFT, 10);
			hbox2->Add(m_txtCtrlStep,  0, wxLEFT, 73);
			vbox->Add(hbox2, 1, wxALIGN_LEFT | wxLEFT | wxTOP, 10);
		
			wxButton *okButton = new wxButton(panel, wxID_OK, wxT("Ok"), 
				wxDefaultPosition, wxSize(70, -1));
			wxButton *closeButton = new wxButton(panel, wxID_CANCEL, wxT("Close"), 
				wxDefaultPosition, wxSize(70, -1));
			hbox3->Add(okButton, 1, wxLEFT, 10);
			hbox3->Add(closeButton, 1, wxLEFT, 10);
			vbox->Add(hbox3, 1, wxALIGN_LEFT | wxLEFT | wxTOP, 10);
		
		
			panel->SetSizer(vbox);
		
			Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, 
				wxCommandEventHandler(TimeLineConfigDialog::OnOK));
		
			Centre();
		
		}
		
		int TimeLineConfigDialog::getNrFrames()
		{
			return m_nrFrames;
		}

		void TimeLineConfigDialog::setNrFrames(int frames)
		{
			m_nrFrames = frames;
		}

		int TimeLineConfigDialog::getStep()
		{
			return m_step;
		}

		void TimeLineConfigDialog::setStep(int step)
		{
			m_step = step;
		}

		void TimeLineConfigDialog::OnOK(wxCommandEvent& WXUNUSED(event))
		{
			// get number of frames
			m_nrFrames = atoi(m_txtCtrlNrFrames->GetLineText(0).c_str());
		
			// get step
			m_step = atoi(m_txtCtrlStep->GetLineText(0).c_str());
		
			Close();
		
			SetReturnCode(wxID_OK);
		}



	}
}