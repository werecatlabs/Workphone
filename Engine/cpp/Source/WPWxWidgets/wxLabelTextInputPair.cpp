#include <WPWxWidgets/WPWxWidgetsPCH.hpp>
#include <WPWxWidgets/wxLabelTextInputPair.hpp>
#include <Workphone/Workphone.hpp>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace workphone
{
    namespace ui
    {

        wxLabelTextInputPair::wxLabelTextInputPair()
        {
        }

        wxLabelTextInputPair::~wxLabelTextInputPair()
        {
        }

        void wxLabelTextInputPair::load( SmartPtr<ISharedObject> data )
        {
            auto parent = getParent();
            auto panel = new wxPanel( parent, -1 );
            setWindow( panel );
            panel->SetBackgroundColour( wxColour( *wxGREEN ) );

            auto parentSizer = parent->GetSizer();
            if( parentSizer )
            {
                parentSizer->Add( panel, wxSizerFlags().Expand() );
            }

            auto sizer = new wxBoxSizer( wxHORIZONTAL );
            panel->SetSizer( sizer );

            auto label = getLabel();
            auto text = new wxStaticText( panel, -1, label.c_str() );
            setLabelText( text );
            sizer->Add( text, wxSizerFlags().Expand().Proportion( 30 ) );

            auto value = getValue();
            auto valueText = new wxTextCtrl( panel, -1, value.c_str() );
            setInputText( valueText );
            sizer->Add( valueText, wxSizerFlags().Expand().Proportion( 70 ) );

            //valueText->Bind(wxEVT_TEXT, &wxLabelTextInputPair::OnText, this, -1);
            //valueText->Bind(wxEVT_TEXT_ENTER, &wxLabelTextInputPair::OnTextEnter, this, -1);
            //valueText->Bind(wxEVT_TEXT_COPY, &wxLabelTextInputPair::OnTextCopy, this, -1);
            //valueText->Bind(wxEVT_TEXT_PASTE, &wxLabelTextInputPair::OnTextPaste, this, -1);
        }

        String wxLabelTextInputPair::getLabel() const
        {
            return m_label;
        }

        void wxLabelTextInputPair::setLabel( const String &label )
        {
            m_label = label;

            auto text = getLabelText();
            if( text )
            {
                text->SetLabelText( m_label.c_str() );
            }
        }

        String wxLabelTextInputPair::getValue() const
        {
            return m_value;
        }

        void wxLabelTextInputPair::setValue( const String &value )
        {
            m_value = value;

            auto text = getInputText();
            if( text )
            {
                text->SetLabelText( m_value.c_str() );
            }
        }

        wxStaticText *wxLabelTextInputPair::getLabelText() const
        {
            return m_labelText;
        }

        void wxLabelTextInputPair::setLabelText( wxStaticText *labelText )
        {
            m_labelText = labelText;
        }

        wxTextCtrl *wxLabelTextInputPair::getInputText() const
        {
            return m_inputText;
        }

        void wxLabelTextInputPair::setInputText( wxTextCtrl *inputText )
        {
            m_inputText = inputText;
        }

        void wxLabelTextInputPair::OnText( wxCommandEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextEnter( wxCommandEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextURL( wxTextUrlEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextMaxLen( wxCommandEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextCut( wxClipboardTextEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextCopy( wxClipboardTextEvent &event )
        {
        }

        void wxLabelTextInputPair::OnTextPaste( wxClipboardTextEvent &event )
        {
        }

    }  // end namespace ui
}  // namespace workphone
