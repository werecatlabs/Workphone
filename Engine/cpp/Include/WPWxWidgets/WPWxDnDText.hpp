#ifndef WPWxDnDFile_h__
#define WPWxDnDFile_h__

#include "wx/wxprec.h"
#include <wx/dnd.h>
#include <wx/listbox.h>

namespace workphone
{
    namespace ui
    {
        class DnDText : public wxTextDropTarget
        {
        public:
            DnDText( wxListBox *pOwner )
            {
                m_pOwner = pOwner;
            }

            virtual bool OnDropText( wxCoord x, wxCoord y, const wxString &text ) wxOVERRIDE;

        private:
            wxListBox *m_pOwner;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // WPWxDnDFile_h__
