#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include "WPWxWidgets/WPWxDnDText.hpp"

namespace workphone
{
    namespace ui
    {

        bool DnDText::OnDropText( wxCoord, wxCoord, const wxString &text )
        {
            m_pOwner->Append( text );

            return true;
        }

    }  // namespace ui
}  // namespace workphone
