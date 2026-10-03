#ifndef _ClawUIMENU_H
#define _ClawUIMENU_H

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <WPGraphics/UI/ClawUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        class ClawUIMenu : public ClawUIElement<IUIMenu>
        {
        public:
            ClawUIMenu();
            ~ClawUIMenu() override;

            bool handleEvent( const SmartPtr<IInputEvent> &event ) override;

            //! Sets the position of the cursor
            //! param - idx - the index of the menu item
            void setCursorPosition( u32 cursorIdx ) override;
            u32 getCursorPosition() const override;

            void setNumListItems( u32 numListItems ) override;
            u32 getNumListItems() const override;

            void setCurrentItemIndex( u32 index ) override;
            u32 getCurrentItemIndex() const override;

            void incrementCursor() override;  // move the cursor to the next menu item
            void decrementCursor() override;  // move the cursor to the previous menu item

            int getNumMenuItems() const override;

            void setPosition( const Vector2F &position ) override;

            void addMenuItem( SmartPtr<IUIElement> item ) override;

            void removeMenuItem( SmartPtr<IUIElement> item ) override;

            Array<SmartPtr<IUIElement>> getMenuItems() const override;

            void setMenuItems( const Array<SmartPtr<IUIElement>> &menuItems ) override;

            String getLabel() const override;

            void setLabel( const String &label ) override;

            void draw( struct wp_context *ctx ) override;

        private:
            void populateMenuItemList();

            void WrapCursor();  //! ensure the cursor values is within bounds

            Array<SmartPtr<IUIElement>> m_aMenuItemList;

            String m_label;

            s32 m_uiNumMenuItems;
            s32 m_uiNumListItems;
            s32 m_iItemIdx;       // store the index of an item in a list
            s32 m_cursorIdx;      // the index of the menu item which the cursor is on
            s32 m_prevCursorIdx;  // the menu item the Cursor was previously on
        };
    }  // end namespace ui
}  // namespace workphone

#endif
