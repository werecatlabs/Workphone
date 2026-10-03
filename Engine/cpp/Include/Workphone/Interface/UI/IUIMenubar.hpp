#ifndef __IUIMenuBar_h__
#define __IUIMenuBar_h__

#include <Workphone/Interface/UI/IUIElement.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @class IUIMenubar
         * @brief Interface for a menu bar, responsible for managing menus within the user interface
         */
        class WPCore_API IUIMenubar : public IUIElement
        {
        public:
            IUIMenubar() : IUIElement( IUIMenubar::typeInfo() )
            {
            }

            IUIMenubar( u32 poolTypeId ) : IUIElement( poolTypeId )
            {
            }

            /**
             * @brief Destructor
             */
            ~IUIMenubar() override;

            /**
             * @brief Adds a menu to the menu bar
             * @param menu The menu to add
             */
            virtual void addMenu( SmartPtr<IUIMenu> menu ) = 0;

            /**
             * @brief Removes a menu from the menu bar
             * @param menu The menu to remove
             */
            virtual void removeMenu( SmartPtr<IUIMenu> menu ) = 0;

            /**
             * @brief Gets the menus in the menu bar
             * @return An array of smart pointers to the menus
             */
            virtual Array<SmartPtr<IUIMenu>> getMenus() const = 0;

            /**
             * @brief Sets the menus in the menu bar
             * @param menus An array of smart pointers to the menus
             */
            virtual void setMenus( const Array<SmartPtr<IUIMenu>> &menus ) = 0;

            /**
             * @brief Finds a menu by its label
             * @param label The label of the menu to find
             * @return Smart pointer to the menu, or nullptr if not found
             */
            virtual SmartPtr<IUIMenu> findMenuByLabel( const String &label ) const = 0;

            /**
             * @brief Gets the currently active/focused menu
             * @return Smart pointer to the active menu, or nullptr if none is active
             */
            virtual SmartPtr<IUIMenu> getActiveMenu() const = 0;

            /**
             * @brief Sets the active/focused menu
             * @param menu The menu to set as active
             */
            virtual void setActiveMenu( SmartPtr<IUIMenu> menu ) = 0;

            /**
             * @brief Gets the currently highlighted menu (for keyboard navigation)
             * @return Smart pointer to the highlighted menu, or nullptr if none is highlighted
             */
            virtual SmartPtr<IUIMenu> getHighlightedMenu() const = 0;

            /**
             * @brief Sets the highlighted menu (for keyboard navigation)
             * @param menu The menu to highlight
             */
            virtual void setHighlightedMenu( SmartPtr<IUIMenu> menu ) = 0;

            /**
             * @brief Navigates to the next menu (keyboard navigation)
             */
            virtual void navigateNext() = 0;

            /**
             * @brief Navigates to the previous menu (keyboard navigation)
             */
            virtual void navigatePrevious() = 0;

            /**
             * @brief Activates the currently highlighted menu
             */
            virtual void activateHighlightedMenu() = 0;

            /**
             * @brief Closes all open menus in the menu bar
             */
            virtual void closeAllMenus() = 0;

            /**
             * @brief Gets whether keyboard navigation is enabled
             * @return True if keyboard navigation is enabled
             */
            virtual bool isKeyboardNavigationEnabled() const = 0;

            /**
             * @brief Sets whether keyboard navigation is enabled
             * @param enabled True to enable keyboard navigation
             */
            virtual void setKeyboardNavigationEnabled( bool enabled ) = 0;

            /**
             * @brief Gets whether the menu bar automatically closes menus when focus is lost
             * @return True if auto-close is enabled
             */
            virtual bool isAutoCloseEnabled() const = 0;

            /**
             * @brief Sets whether the menu bar automatically closes menus when focus is lost
             * @param enabled True to enable auto-close
             */
            virtual void setAutoCloseEnabled( bool enabled ) = 0;

            /**
             * @brief Gets the menu bar's orientation
             * @return The orientation (horizontal or vertical)
             */
            virtual Direction getOrientation() const = 0;

            /**
             * @brief Sets the menu bar's orientation
             * @param orientation The orientation to set
             */
            virtual void setOrientation( Direction orientation ) = 0;

            /**
             * @brief Handles keyboard input for menu navigation
             * @param key The key code that was pressed
             * @param modifiers Any modifier keys (Ctrl, Alt, Shift)
             * @return True if the key was handled, false otherwise
             */
            virtual bool handleKeyboardInput( u32 key, u32 modifiers ) = 0;

            /**
             * @brief Gets whether a menu is currently open
             * @return True if any menu is open
             */
            virtual bool hasOpenMenu() const = 0;

            /**
             * @brief Gets the index of the currently highlighted menu
             * @return The index, or -1 if no menu is highlighted
             */
            virtual s32 getHighlightedMenuIndex() const = 0;

            /**
             * @brief Sets the highlighted menu by index
             * @param index The index of the menu to highlight
             */
            virtual void setHighlightedMenuIndex( s32 index ) = 0;

            WP_CLASS_REGISTER_DECL;
        };

    }  // end namespace ui
}  // namespace workphone

#endif  // IUIMenuBar_h__
