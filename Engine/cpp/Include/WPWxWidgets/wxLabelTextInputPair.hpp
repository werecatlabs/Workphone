#ifndef wxLabelTextInputPair_h__
#define wxLabelTextInputPair_h__

#include <WPWxWidgets/WPWxWidgetsPrerequisites.hpp>
#include <WPWxWidgets/WPWxApplicationWindow.hpp>
#include <wx/textctrl.h>

namespace workphone
{
    namespace ui
    {

        class wxLabelTextInputPair : public wxApplicationWindow
        {
        public:
            wxLabelTextInputPair();
            ~wxLabelTextInputPair();

            void load( SmartPtr<ISharedObject> data ) override;

            String getLabel() const;
            void setLabel( const String &label );

            String getValue() const;
            void setValue( const String &value );

            wxStaticText *getLabelText() const;
            void setLabelText( wxStaticText *labelText );

            wxTextCtrl *getInputText() const;
            void setInputText( wxTextCtrl *inputText );

        protected:
            void OnText( wxCommandEvent &event );
            void OnTextEnter( wxCommandEvent &event );
            void OnTextURL( wxTextUrlEvent &event );
            void OnTextMaxLen( wxCommandEvent &event );

            void OnTextCut( wxClipboardTextEvent &event );
            void OnTextCopy( wxClipboardTextEvent &event );
            void OnTextPaste( wxClipboardTextEvent &event );

            String m_label;
            String m_value;

            wxStaticText *m_labelText = nullptr;
            wxTextCtrl *m_inputText = nullptr;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // wxLabelTextInputPair_h__
