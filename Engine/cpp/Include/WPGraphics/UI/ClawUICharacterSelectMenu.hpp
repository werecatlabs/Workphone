#ifndef _ClawUICharacterSelectMenu_H
#define _ClawUICharacterSelectMenu_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIElement.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUICharacterSelectMenu : public ClawUIElement<IUIElement>
        {
        public:
            ClawUICharacterSelectMenu();
            ~ClawUICharacterSelectMenu() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            //! Sets the position of the cursor
            //! param - idx - the index of the menu item
            void setCursorPosition( u32 cursorIdx, u32 cursorPosition );
            u32 getCursorPosition( u32 cursorIdx ) const;

            void setNumListItems( u32 numListItems );
            u32 getNumListItems() const;

            void setCurrentItemIndex( u32 index );
            u32 getCurrentItemIndex() const;

            void increamentCursor( s32 cursorIndex );  // move the cursor to the next menu item
            void decrementCursor( s32 cursorIndex );
            // move the cursor to the previous menu item

            int getNumMenuItems() const;

            u32 getCurrentCursorIndex() const;

            void setPosition( const Vector2F &position ) override;

            void draw( struct wp_context *ctx ) override;

        private:
            void onAddChild( IUIElement *child );
            void onChildChangedState( IUIElement *child );
            void onToggleVisibility();
            void onGainFocus() override;
            void onLostFocus() override;

            void populateMenuItemList();

            void WrapCursor( s32 cursorIndex );  //! ensure the cursor values is within bounds

            //
            // Callbacks
            //
            void OnSelectPrevItem();
            void OnSelectNextItem();
            void OnWrapSelectionStart();
            void OnWrapSelectionEnd();

            bool inputEvent( const SmartPtr<IInputEvent> &event );

            SmartPtr<IUILayoutWindow> m_layout;

            Array<SmartPtr<IUIElement>> m_menuItems;  //

            u32 m_curCursorIndex;

            s32 m_uiNumMenuItems;
            s32 m_uiNumListItems;
            s32 m_iItemIdx;  // store the index of an item in a list

            Array<s32> m_cursorIdx;      // the index of the menu item which the cursor is on
            Array<s32> m_prevCursorIdx;  // the menu item the Cursor was previously on
        };
    }  // end namespace ui
}  // namespace workphone

#endif
