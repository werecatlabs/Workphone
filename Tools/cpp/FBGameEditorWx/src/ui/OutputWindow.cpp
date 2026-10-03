#include <GameEditorPCH.hpp>
#include "OutputWindow.hpp"
#include <wx/richtext/richtextctrl.hpp>
#include "editor/EditorManager.hpp"



namespace fb
{
	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		OutputWindow::OutputWindow( wxWindow* parent )
			: wxScrolledWindow(parent)
		{
			wxBoxSizer *baseSizer = new wxBoxSizer( wxHORIZONTAL );
			SetSizer(baseSizer);
	
			m_textCtrl = new wxRichTextCtrl(this);
	
			baseSizer->Add(m_textCtrl, 1, wxEXPAND);
		}
	
	
	
		//--------------------------------------------
		OutputWindow::~OutputWindow()
		{
	
		}
	
	
	
		//--------------------------------------------
		void OutputWindow::append( const String& text )
		{
			m_textCtrl->AppendText(text.c_str());
		}
	
	
	
	} // end namespace editor
	
	
}
