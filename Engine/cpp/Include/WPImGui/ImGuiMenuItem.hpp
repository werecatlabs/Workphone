#ifndef __ImGuiMenuItem_h__
#define __ImGuiMenuItem_h__

/**
 * \file ImGuiMenuItem.hpp
 * \brief ImGui wrapper for a workphone UI menu item interface.
 *
 * This file declares the `ImGuiMenuItem` class which implements the
 * `IUIMenuItem` interface using ImGui-driven elements. It stores common
 * menu item properties such as type, text and help string.
 */

#include <WPImGui/WPImGuiPrerequisites.hpp>
#include <WPImGui/ImGuiElement.hpp>
#include <Workphone/Interface/UI/IUIMenuItem.hpp>

namespace workphone
{
    namespace ui
    {
        /**
         * \brief Concrete ImGui implementation of a menu item.
         *
         * `ImGuiMenuItem` adapts the generic `IUIMenuItem` interface to an
         * ImGui-based element by inheriting from `ImGuiElement<IUIMenuItem>`.
         * It stores the menu item type, display text and an optional help
         * string.
         */
        class ImGuiMenuItem : public ImGuiElement<IUIMenuItem>
        {
        public:
            /**
             * \brief Construct a default ImGuiMenuItem.
             *
             * The default constructed item will have Type::Normal and empty
             * text/help strings.
             */
            ImGuiMenuItem();

            /**
             * \brief Virtual destructor.
             */
            ~ImGuiMenuItem() override;

            /**
             * \brief Get the menu item's type.
             * \return The stored menu item Type (e.g. Normal, Check, Radio).
             */
            Type getMenuItemType() const override;

            /**
             * \brief Set the menu item's type.
             * \param type The new Type to apply to this menu item.
             */
            void setMenuItemType( Type type ) override;

            /**
             * \brief Get the menu item's display text.
             * \return The text shown for this menu item.
             */
            String getText() const override;

            /**
             * \brief Set the menu item's display text.
             * \param text A string to display for this menu item.
             */
            void setText( const String &text ) override;

            /**
             * \brief Get the help/tooltip text for this menu item.
             * \return The help string associated with the item.
             */
            String getHelp() const override;

            /**
             * \brief Set the help/tooltip text for this menu item.
             * \param help The help string to associate with this menu item.
             */
            void setHelp( const String &help ) override;

            /**
             * \brief Macro used by the workphone type registration system.
             *
             * Expands to declarations required for runtime type registration
             * and reflection used by the framework.
             */
            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * \brief The menu item type (Normal, Check, Radio, etc.).
             */
            Type m_type = Type::Normal;

            /**
             * \brief The visible text for the menu item.
             */
            FixedString<128> m_text;

            /**
             * \brief Optional help / tooltip string for the item.
             */
            FixedString<128> m_help;
        };
    }  // end namespace ui
}  // namespace workphone

#endif  // CEGUIText_h__
