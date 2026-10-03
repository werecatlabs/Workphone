#ifndef UIToggle_h__
#define UIToggle_h__

#include <WPGraphicsOgreNext/UI/Colibri/UIElementColibri.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>
#include "Workphone/UI/UIElement.hpp"

namespace workphone
{
    namespace core
    {
        extern template class WPCore_API Prototype<ui::IUIToggle>;
    }

    namespace ui
    {
        extern template class WPCore_API UIElement<IUIToggle>;

        /**
         * @brief UI toggle component implementation using OgreNext rendering backend.
         *
         * This class provides a toggle/checkbox UI element with customizable appearance
         * and behavior. It supports both checkbox and radio button toggle types, with
         * optional text labels and configurable visual states.
         */
        class UIToggleColibri : public UIElementColibri<UIElement<IUIToggle>>
        {
        public:
            /**
             * @brief Constructs a new UIToggleOgreNext instance.
             */
            UIToggleColibri();

            /**
             * @brief Destroys the UIToggleOgreNext instance.
             */
            ~UIToggleColibri() override;

            /**
             * @brief Loads the toggle component with the specified data.
             * @param data Shared object containing initialization data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unloads the toggle component and releases resources.
             * @param data Shared object containing unload data.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Sets the text label displayed next to the toggle.
             * @param text The label text to display.
             */
            void setLabel( const String &text ) override;

            /**
             * @brief Gets the current label text.
             * @return The label text currently displayed.
             */
            String getLabel() const override;

            /**
             * @brief Sets the toggled state of the checkbox.
             * @param checked True to check the toggle, false to uncheck it.
             */
            void setToggled( bool checked ) override;

            /**
             * @brief Checks if the toggle is currently in the checked state.
             * @return True if the toggle is checked, false otherwise.
             */
            bool isToggled() const override;

            /**
             * @brief Handles state change events for the toggle component.
             * @param state The state object containing the state change information.
             * @return True if the state change was handled successfully, false otherwise.
             */
            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @brief Sets the font size for the label text.
             * @param textSize The text size in points.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Gets the current font size of the label text.
             * @return The current text size in points.
             */
            f32 getTextSize() const override;

            /**
             * @brief Gets the type of toggle control (checkbox or radio button).
             * @return The current toggle type.
             */
            ToggleType getToggleType() const;

            /**
             * @brief Sets the type of toggle control (checkbox or radio button).
             * @param toggleType The toggle type to set.
             */
            void setToggleType( ToggleType toggleType );

            /**
             * @brief Gets the current visual state of the toggle.
             * @return The current toggle state (On, Off, Indeterminate, etc.).
             */
            ToggleState getToggleState() const;

            /**
             * @brief Sets the visual state of the toggle.
             * @param toggleState The toggle state to set.
             */
            void setToggleState( ToggleState toggleState );

            /**
             * @brief Checks if the label is currently visible.
             * @return True if the label is shown, false if hidden.
             */
            bool getShowLabel() const;

            /**
             * @brief Sets the visibility of the text label.
             * @param showLabel True to show the label, false to hide it.
             */
            void setShowLabel( bool showLabel );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Text label element displayed alongside the toggle.
            SmartPtr<IUIText> m_label;

            /// Background image element for the toggle control.
            SmartPtr<IUIImage> m_bgImage;

            /// Image element representing the toggle indicator.
            SmartPtr<IUIImage> m_toggleImage;

            /// Pointer to the underlying Colibri checkbox widget.
            Colibri::Checkbox *m_checkbox = nullptr;

            /// The label text content.
            String m_text;

            /// Current checked state of the toggle.
            bool m_checked = false;

            /// Flag indicating whether the label should be displayed.
            bool m_showLabel = true;

            /// The type of toggle control (checkbox or radio button).
            ToggleType m_toggleType = ToggleType::CheckBox;

            /// The current visual state of the toggle.
            ToggleState m_toggleState = ToggleState::Off;
        };
    }  // namespace ui
}  // namespace workphone

#endif  // UIToggle_h__
