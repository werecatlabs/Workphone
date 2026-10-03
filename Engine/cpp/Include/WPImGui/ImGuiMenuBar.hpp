#ifndef __ImGuiMenuBar_h__
#define __ImGuiMenuBar_h__

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIMenubar.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * @brief ImGui-backed implementation of an IUIMenubar.
         *
         * This class adapts the generic IUIMenubar interface to an
         * ImGui-based implementation used by the UI rendering system.
         * It manages a collection of IUIMenu objects, provides keyboard
         * navigation helpers and exposes options such as auto-close and
         * orientation.
         */
        class ImGuiMenuBar : public ImGuiElement<IUIMenubar>
        {
        public:
            /**
             * @brief Construct a new ImGuiMenuBar.
             *
             * Initializes internal state to sensible defaults.
             */
            ImGuiMenuBar();

            /**
             * @brief Destroy the ImGuiMenuBar.
             */
            ~ImGuiMenuBar() override;

            /**
             * @copydoc ISharedObject::load
             *
             * Loads any platform or renderer specific resources required by
             * the menu bar. The provided @p data pointer can be used to pass
             * context-specific configuration.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc ISharedObject::unload
             *
             * Releases resources acquired in load().
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Called each frame to update internal state and draw the menu bar.
             */
            void update() override;

            /**
             * @copydoc IUIMenubar::addMenu
             *
             * Adds a menu instance to the end of the internal menu list.
             */
            void addMenu( SmartPtr<IUIMenu> menu ) override;

            /**
             * @copydoc IUIMenubar::removeMenu
             *
             * Removes the first matching menu from the internal list.
             */
            void removeMenu( SmartPtr<IUIMenu> menu ) override;

            /**
             * @copydoc IUIMenubar::getMenus
             *
             * Returns a copy of the current menu list.
             */
            Array<SmartPtr<IUIMenu>> getMenus() const override;

            /**
             * @copydoc IUIMenubar::setMenus
             *
             * Replaces the internal menu list with @p menus.
             */
            void setMenus( const Array<SmartPtr<IUIMenu>> &menus ) override;

            /**
             * @copydoc IUIMenubar::findMenuByLabel
             *
             * Searches menus for one with a matching label and returns it
             * or a null smart pointer if not found.
             */
            SmartPtr<IUIMenu> findMenuByLabel( const String &label ) const override;

            /**
             * @copydoc IUIMenubar::getActiveMenu
             */
            SmartPtr<IUIMenu> getActiveMenu() const override;

            /**
             * @copydoc IUIMenubar::setActiveMenu
             */
            void setActiveMenu( SmartPtr<IUIMenu> menu ) override;

            /**
             * @copydoc IUIMenubar::getHighlightedMenu
             */
            SmartPtr<IUIMenu> getHighlightedMenu() const override;

            /**
             * @copydoc IUIMenubar::setHighlightedMenu
             */
            void setHighlightedMenu( SmartPtr<IUIMenu> menu ) override;

            /**
             * @copydoc IUIMenubar::navigateNext
             *
             * Moves highlight to the next menu entry (wraps at end).
             */
            void navigateNext() override;

            /**
             * @copydoc IUIMenubar::navigatePrevious
             *
             * Moves highlight to the previous menu entry (wraps at start).
             */
            void navigatePrevious() override;

            /**
             * @copydoc IUIMenubar::activateHighlightedMenu
             *
             * Opens/activates the currently highlighted menu if any.
             */
            void activateHighlightedMenu() override;

            /**
             * @copydoc IUIMenubar::closeAllMenus
             *
             * Closes any open menu and resets active/highlighted state.
             */
            void closeAllMenus() override;

            /**
             * @copydoc IUIMenubar::isKeyboardNavigationEnabled
             */
            bool isKeyboardNavigationEnabled() const override;

            /**
             * @copydoc IUIMenubar::setKeyboardNavigationEnabled
             */
            void setKeyboardNavigationEnabled( bool enabled ) override;

            /**
             * @copydoc IUIMenubar::isAutoCloseEnabled
             */
            bool isAutoCloseEnabled() const override;

            /**
             * @copydoc IUIMenubar::setAutoCloseEnabled
             */
            void setAutoCloseEnabled( bool enabled ) override;

            /**
             * @copydoc IUIMenubar::getOrientation
             */
            Direction getOrientation() const override;

            /**
             * @copydoc IUIMenubar::setOrientation
             */
            void setOrientation( Direction orientation ) override;

            /**
             * @copydoc IUIMenubar::handleKeyboardInput
             *
             * Handles raw key presses and modifier flags. Returns true if the
             * input was consumed by the menu bar.
             */
            bool handleKeyboardInput( u32 key, u32 modifiers ) override;

            /**
             * @copydoc IUIMenubar::hasOpenMenu
             */
            bool hasOpenMenu() const override;

            /**
             * @copydoc IUIMenubar::getHighlightedMenuIndex
             */
            s32 getHighlightedMenuIndex() const override;

            /**
             * @copydoc IUIMenubar::setHighlightedMenuIndex
             */
            void setHighlightedMenuIndex( s32 index ) override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Internal helper that processes keyboard navigation commands.
             *
             * This updates m_highlightedMenu / m_highlightedMenuIndex and may
             * open/close menus depending on the current state.
             */
            void handleKeyboardNavigation();

            /**
             * @brief The collection of menus in this menu bar.
             *
             * Owned references to the IUIMenu objects displayed by this bar.
             */
            ConcurrentArray<SmartPtr<IUIMenu>> m_menus;

            /**
             * @brief The currently active (opened) menu, or null if none are open.
             */
            AtomicSmartPtr<IUIMenu> m_activeMenu;

            /**
             * @brief The currently highlighted menu (for keyboard navigation).
             */
            AtomicSmartPtr<IUIMenu> m_highlightedMenu;

            /**
             * @brief Index of the currently highlighted menu, or -1 if none.
             */
            atomic_s32 m_highlightedMenuIndex = -1;

            /**
             * @brief Menu bar orientation (horizontal or vertical).
             */
            AtomicValue<Direction> m_orientation = Direction::Horizontal;

            /**
             * @brief Whether keyboard navigation is enabled.
             */
            atomic_bool m_keyboardNavigationEnabled = true;

            /**
             * @brief Whether menus auto-close when focus is lost.
             */
            atomic_bool m_autoCloseEnabled = true;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // __ImGuiMenuBar_h__
