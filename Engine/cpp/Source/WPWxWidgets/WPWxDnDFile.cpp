#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxDnDFile.hpp"

namespace workphone
{
    namespace ui
    {

        bool DnDFile::OnDropFiles( wxCoord, wxCoord, const wxArrayString &filenames )
        {
            size_t nFiles = filenames.GetCount();
            wxString str;
            str.Printf( "%d files dropped", (int)nFiles );

            if( m_pOwner != NULL )
            {
                m_pOwner->Append( str );
                for( size_t n = 0; n < nFiles; n++ )
                    m_pOwner->Append( filenames[n] );
            }

            return true;
        }

    }  // namespace ui
}  // namespace workphone
