#ifndef _ClawUIDIALOGBOX_H
#define _ClawUIDIALOGBOX_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIDialogBox : public ClawUIElement<IUIElement>
        {
        public:
            ClawUIDialogBox();
            ~ClawUIDialogBox() override;

            //! initializes the dialog box
            void initialise( SmartPtr<IUIElement> &parent, const TiXmlNode *pNode );

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            //! Set scroll speed
            void setScrollSpeed( s32 scroll_speed );

            //! returns the string in the dialog box
            String getString();

            void draw( struct wp_context *ctx ) override;

        private:
            //
            // Callbacks
            //
            void OnScroll();
            void OnClose();

            String m_Text[2];  // array to store the string that is to be displayed and the full string.

            u32 m_scroll_speed = 0;  // store the number of nano second it takes to move onto the next
            // character
            u32 m_time_to_next_char = 0;  // store time the next character should be displayed

            bool m_bPauseDialog = false;  //
            bool m_bIsFinished = false;   // let us know if the dialog has finished been shown
        };

        // dialog box with character icon
        class IconDialogBox
        {
        };

        using ClawUIDialogBoxPtr = SmartPtr<ClawUIDialogBox>;
    }  // end namespace ui
}  // namespace workphone

#endif
