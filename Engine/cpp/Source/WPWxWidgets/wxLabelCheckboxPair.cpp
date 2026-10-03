#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/wxLabelCheckboxPair.hpp>
#include <Workphone/Workphone.hpp>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace workphone
{
    namespace ui
    {

        wxLabelCheckboxPair::wxLabelCheckboxPair()
        {
        }

        wxLabelCheckboxPair::~wxLabelCheckboxPair()
        {
        }

        void wxLabelCheckboxPair::load( SmartPtr<ISharedObject> data )
        {
            auto parent = getParent();
            auto panel = new wxPanel( parent, -1 );
            setWindow( panel );

            auto parentSizer = parent->GetSizer();
            if( parentSizer )
            {
                parentSizer->Add( panel, wxEXPAND );
            }

            auto sizer = new wxBoxSizer( wxHORIZONTAL );
            panel->SetSizer( sizer );

            auto text = new wxStaticText( panel, -1, "Mesh Scale" );
            sizer->Add( text, wxEXPAND );

            auto valueText = new wxTextCtrl( panel, -1, "1.0" );
            sizer->Add( valueText, wxEXPAND );
            //sizer->Add(actorNameText, 0, wxEXPAND);
        }

    }  // end namespace ui
}  // namespace workphone
