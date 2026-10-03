#ifndef UIToggle_h__
#define UIToggle_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIToggle.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @class UIToggle
         * @brief Implementation of a toggleable user interface element.
         *
         * Provides functionality for a UI component that can be switched between states,
         * supporting custom labels, text sizing, and various toggle types and states.
         */
        class WPCore_API UIToggle : public UIElement<IUIToggle>
        {
        public:
            /**
             * @brief Constructs a new UIToggle instance.
             */
            UIToggle();

            /**
             * @brief Destroys the UIToggle instance.
             */
            ~UIToggle() override;

            /**
             * @brief Sets the toggled state of the element.
             * @param toggled True to set the element as toggled, false otherwise.
             */
            void setToggled( bool toggled ) override;

            /**
             * @brief Checks if the element is currently toggled.
             * @return True if toggled, false otherwise.
             */
            bool isToggled() const override;

            /**
             * @brief Retrieves the current toggle type.
             * @return The current ToggleType of the element.
             */
            ToggleType getToggleType() const override;

            /**
             * @brief Sets the type of toggle used by the element.
             * @param toggleType The desired ToggleType to apply.
             */
            void setToggleType( ToggleType toggleType ) override;

            /**
             * @brief Retrieves the current state of the toggle.
             * @return The current ToggleState of the element.
             */
            ToggleState getToggleState() const override;

            /**
             * @brief Sets the state of the toggle.
             * @param toggleState The desired ToggleState to apply.
             */
            void setToggleState( ToggleState toggleState ) override;

            /**
             * @brief Checks if the label is currently set to be visible.
             * @return True if the label is shown, false otherwise.
             */
            bool getShowLabel() const override;

            /**
             * @brief Determines whether the label should be displayed.
             * @param showLabel True to show the label, false to hide it.
             */
            void setShowLabel( bool showLabel ) override;

            /**
             * @brief Retrieves the text label of the toggle.
             * @return The label string.
             */
            String getLabel() const override;

            /**
             * @brief Sets the text label for the toggle.
             * @param label The string to use as the label.
             */
            void setLabel( const String &label ) override;

            /**
             * @brief Sets the size of the label text.
             * @param textSize The font size for the label text.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Retrieves the current size of the label text.
             * @return The current text size.
             */
            f32 getTextSize() const override;

            /**
             * @brief Forces the element to redraw or recalculate its layout.
             */
            void invalidate() override;

            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIToggle_h__
