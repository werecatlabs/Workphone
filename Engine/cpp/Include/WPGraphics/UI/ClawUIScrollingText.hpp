#ifndef _ClawUIScrollingText_H
#define _ClawUIScrollingText_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIScrollingText.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIScrollingText : public ClawUIElement<IUIScrollingText>
        {
        public:
            enum ScrollingTextState
            {
                STS_START,
                STS_SCROLLING,
                STS_FADEOUT,

                STS_COUNT
            };

            ClawUIScrollingText();
            ~ClawUIScrollingText() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            //! Sets the position of the cursor
            //! param - idx - the index of the menu item
            void setCursorPosition( u32 cursorIdx );
            u32 getCursorPosition() const;

            void setNumListItems( u32 numListItems );
            u32 getNumListItems() const;

            void setCurrentItemIndex( u32 index );
            u32 getCurrentItemIndex() const;

            void increamentCursor();  // move the cursor to the next menu item
            void decrementCursor();   // move the cursor to the previous menu item

            int getNumMenuItems() const;

            void draw( struct wp_context *ctx ) override;

        private:
            void populateMenuItemList();

            void WrapCursor();  //! ensure the cursor values is within bounds

            Array<SmartPtr<IUIElement>> m_elements;

            s32 m_uiNumMenuItems;
            s32 m_uiNumListItems;
            s32 m_iItemIdx;       // store the index of an item in a list
            s32 m_cursorIdx;      // the index of the menu item which the cursor is on
            s32 m_prevCursorIdx;  // the menu item the Cursor was previously on
        };
    }  // end namespace ui
}  // namespace workphone

#endif
