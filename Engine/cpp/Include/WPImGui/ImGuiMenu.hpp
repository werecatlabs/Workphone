#ifndef __ImGuiMenu_h__
#define __ImGuiMenu_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIMenu.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class ImGuiMenu
         * @brief ImGui-based implementation of a user interface menu component
         *
         * This class provides a concrete implementation of the IUIMenu interface using
         * the Dear ImGui library. It manages a collection of menu items with cursor
         * navigation support, allowing users to navigate through menu options using
         * keyboard or other input methods.
         *
         * The menu maintains state information including:
         * - Current cursor position for navigation
         * - Collection of menu items
         * - Current selected item index
         * - Display label
         *
         * @see IUIMenu
         * @see CImGuiElement
         *
         * @author Workphone Engine Team
         * @version 1.0
         * @since Engine v1.0
         */
        class ImGuiMenu : public ImGuiElement<IUIMenu>
        {
        public:
            /**
             * @brief Default constructor
             *
             * Initializes the ImGui menu with default values:
             * - Current item index: 0
             * - Cursor position: 0
             * - Number of list items: 0
             * - Empty label and menu items collection
             */
            ImGuiMenu();

            /**
             * @brief Virtual destructor
             *
             * Ensures proper cleanup of menu resources and menu items.
             */
            ~ImGuiMenu() override;

            /**
             * @brief Loads menu data and initializes the menu component
             *
             * This method is called during the menu's initialization phase to
             * load any required data or configuration settings.
             *
             * @param data Shared pointer to initialization data
             *
             * @see ISharedObject::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads menu data and performs cleanup
             *
             * This method is called during the menu's destruction phase to
             * unload data and perform necessary cleanup operations.
             *
             * @param data Shared pointer to cleanup data
             *
             * @see ISharedObject::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Updates the menu's state and rendering
             *
             * Called each frame to update the menu's visual representation,
             * handle input events, and maintain proper state consistency.
             * This method should be called regularly as part of the main update loop.
             */
            void update() override;

            /**
             * @brief Adds a menu item to the menu
             *
             * Appends a new menu item to the end of the menu items collection.
             * The item will be displayed in the menu and can be navigated to
             * using cursor movement functions.
             *
             * @param item Smart pointer to the UI element to add as a menu item
             *
             * @pre item must not be null
             * @post Menu items count increases by 1
             * @post Item is accessible via cursor navigation
             *
             * @see removeMenuItem
             * @see getMenuItems
             */
            void addMenuItem( SmartPtr<IUIElement> item ) override;

            /**
             * @brief Removes a menu item from the menu
             *
             * Removes the specified menu item from the menu items collection.
             * If the removed item was the current selection, the cursor position
             * may be adjusted to remain valid.
             *
             * @param item Smart pointer to the UI element to remove from the menu
             *
             * @pre item must exist in the menu items collection
             * @post Menu items count decreases by 1 if item was found
             * @post Cursor position is adjusted if necessary to remain valid
             *
             * @see addMenuItem
             * @see getMenuItems
             */
            void removeMenuItem( SmartPtr<IUIElement> item ) override;

            /**
             * @brief Gets all menu items in the menu
             *
             * Returns a copy of the current menu items collection, allowing
             * iteration over all items in the menu.
             *
             * @return Array containing smart pointers to all menu items
             *
             * @note The returned array is a copy, modifications won't affect the menu
             *
             * @see setMenuItems
             * @see addMenuItem
             * @see getNumMenuItems
             */
            Array<SmartPtr<IUIElement>> getMenuItems() const override;

            /**
             * @brief Sets the complete collection of menu items
             *
             * Replaces the entire menu items collection with the provided array.
             * This operation will reset the cursor position to 0 and update
             * the current item index accordingly.
             *
             * @param menuItems Array of smart pointers to UI elements to set as menu items
             *
             * @post All previous menu items are replaced
             * @post Cursor position is reset to 0
             * @post Current item index is reset to 0
             *
             * @see getMenuItems
             * @see addMenuItem
             * @see removeMenuItem
             */
            void setMenuItems( const Array<SmartPtr<IUIElement>> &menuItems ) override;

            /**
             * @brief Sets the cursor position within the menu
             *
             * Moves the cursor to the specified menu item index. The cursor
             * indicates which menu item is currently highlighted or selected
             * for user interaction.
             *
             * @param cursorIdx Zero-based index of the menu item to position cursor at
             *
             * @pre cursorIdx must be less than the number of menu items
             * @post Cursor is positioned at the specified index
             * @post Current item index is updated to match cursor position
             *
             * @see getCursorPosition
             * @see incrementCursor
             * @see decrementCursor
             */
            void setCursorPosition( u32 cursorIdx ) override;

            /**
             * @brief Gets the current cursor position
             *
             * Returns the zero-based index of the menu item where the cursor
             * is currently positioned.
             *
             * @return Zero-based index of the current cursor position
             *
             * @see setCursorPosition
             * @see getCurrentItemIndex
             */
            u32 getCursorPosition() const override;

            /**
             * @brief Sets the number of visible list items in the menu
             *
             * Configures how many menu items should be visible at once when
             * the menu is displayed. This is useful for scrollable menus with
             * many items.
             *
             * @param numListItems Maximum number of items to display simultaneously
             *
             * @post Number of visible list items is updated
             *
             * @see getNumListItems
             */
            void setNumListItems( u32 numListItems ) override;

            /**
             * @brief Gets the number of visible list items in the menu
             *
             * Returns the maximum number of menu items that can be displayed
             * simultaneously in the menu interface.
             *
             * @return Maximum number of visible list items
             *
             * @see setNumListItems
             */
            u32 getNumListItems() const override;

            /**
             * @brief Sets the index of the currently selected item
             *
             * Updates which menu item is considered the current selection.
             * This may differ from the cursor position in some menu implementations.
             *
             * @param index Zero-based index of the item to set as current
             *
             * @pre index must be less than the number of menu items
             * @post Current item index is updated to the specified value
             *
             * @see getCurrentItemIndex
             * @see setCursorPosition
             */
            void setCurrentItemIndex( u32 index ) override;

            /**
             * @brief Gets the index of the currently selected item
             *
             * Returns the zero-based index of the menu item that is currently
             * selected or active.
             *
             * @return Zero-based index of the current item
             *
             * @see setCurrentItemIndex
             * @see getCursorPosition
             */
            u32 getCurrentItemIndex() const override;

            /**
             * @brief Moves the cursor to the next menu item
             *
             * Advances the cursor position by one, wrapping to the beginning
             * if the cursor is already at the last item. This provides
             * convenient navigation through menu items.
             *
             * @post Cursor position is incremented by 1 (with wraparound)
             * @post Current item index is updated to match new cursor position
             *
             * @see decrementCursor
             * @see setCursorPosition
             */
            void incrementCursor() override;

            /**
             * @brief Moves the cursor to the previous menu item
             *
             * Moves the cursor position back by one, wrapping to the end
             * if the cursor is already at the first item. This provides
             * convenient backward navigation through menu items.
             *
             * @post Cursor position is decremented by 1 (with wraparound)
             * @post Current item index is updated to match new cursor position
             *
             * @see incrementCursor
             * @see setCursorPosition
             */
            void decrementCursor() override;

            /**
             * @brief Gets the total number of menu items
             *
             * Returns the count of all menu items currently in the menu,
             * regardless of how many are visible at once.
             *
             * @return Total number of menu items in the collection
             *
             * @see getMenuItems
             * @see getNumListItems
             */
            s32 getNumMenuItems() const override;

            /**
             * @brief Gets the display label of the menu
             *
             * Returns the text label that identifies or describes this menu.
             * This label may be displayed as a title or header for the menu.
             *
             * @return String containing the menu's display label
             *
             * @see setLabel
             */
            String getLabel() const override;

            /**
             * @brief Sets the display label of the menu
             *
             * Updates the text label that identifies or describes this menu.
             * The label is typically displayed as a title or header.
             *
             * @param label String containing the new label to display
             *
             * @post Menu label is updated to the specified value
             *
             * @see getLabel
             */
            void setLabel( const String &label ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Current selected item index (signed for comparison operations)
            atomic_s32 m_currentItemIndex = 0;

            /// Current cursor position for navigation (signed for comparison operations)
            atomic_s32 m_cursorPosition = 0;

            /// Maximum number of items to display simultaneously
            atomic_u32 m_numListItems = 0;

            /// Display label for the menu
            String m_label;

            /// Collection of menu items managed by this menu
            ConcurrentArray<SmartPtr<IUIElement>> m_menuItems;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // CEGUIText_h__
