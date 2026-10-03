#ifndef OutputWindow_h__
#define OutputWindow_h__



#include <GameEditorPrerequisites.hpp>
#include <wx/wx.hpp>


class wxRichTextCtrl;



namespace fb
{
	
	namespace editor
	{
	
	
	
		//--------------------------------------------
		class OutputWindow : public wxScrolledWindow
		{
		public:
			OutputWindow(wxWindow* parent);
			~OutputWindow();
	
			void append(const String& text);
	
		protected:
			wxRichTextCtrl * m_textCtrl;
		};
	
	
	
	} // end namespace editor
	
}


#endif // OutputWindow_h__


