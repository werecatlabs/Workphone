#ifndef WPWxDnDFile_h__
#define WPWxDnDFile_h__

#include "wx/wxprec.h"
#include <wx/dnd.h>
#include <wx/listbox.h>

namespace workphone
{
    namespace ui
    {

        class DnDFile : public wxFileDropTarget
        {
        public:
            DnDFile( wxListBox *pOwner = NULL )
            {
                m_pOwner = pOwner;
            }

            virtual bool OnDropFiles( wxCoord x, wxCoord y, const wxArrayString &filenames ) wxOVERRIDE;

        private:
            wxListBox *m_pOwner;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // WPWxDnDFile_h__
