#ifndef UIButton_h__
#define UIButton_h__

#include <Workphone/UI/UIElement.hpp>
#include <Workphone/Interface/UI/IUIButton.hpp>

namespace workphone
{
    namespace ui
    {

        /**
         * @file UIButton.hpp
         * @brief Declaration of the UIButton UI element.
         *
         * This file defines the `UIButton` class which implements the
         * `IUIButton` interface and provides a simple button element
         * for the UI system. The class stores a label and a text size
         * used when rendering the button.
         */

        /**
         * @class UIButton
         * @brief Concrete UI button implementation.
         *
         * `UIButton` is a concrete UI element implementing the `IUIButton`
         * interface. It encapsulates button-specific state such as the
         * displayed label and the text size used to render the label.
         *
         * It inherits from `UIElement<IUIButton>` so it participates in the
         * engine's UI element hierarchy and lifecycle.
         *
         * @see IUIButton, UIElement
         */
        class WPCore_API UIButton : public UIElement<IUIButton>
        {
        public:
            /**
             * @brief Construct a new UIButton.
             *
             * Initializes the button with default state. Concrete renderers
             * or callers should call `setLabel` and `setTextSize` as required.
             */
            UIButton();

            /**
             * @brief Destroy the UIButton.
             *
             * Virtual destructor override ensures proper cleanup in derived
             * classes and when handled via base pointers.
             */
            ~UIButton() override;

            /**
             * @brief Set the text size used to render the label.
             *
             * Controls the font size (in engine units) used by renderers for
             * the button label. Typical implementations will clamp or map
             * this value to available font sizes.
             *
             * @param textSize The desired text size.
             */
            void setTextSize( f32 textSize ) override;

            /**
             * @brief Get the current text size for the label.
             *
             * Returns the size previously set via `setTextSize`. The meaning
             * of the value is renderer-dependent (for example pixels, points,
             * or a scaled UI unit).
             *
             * @return f32 The current text size.
             */
            f32 getTextSize() const override;

            /**
             * @brief Engine registration macro.
             *
             * Expands to declarations required by the engine for class
             * registration, reflection or serialization. Keep this macro in
             * place to ensure the class is discoverable by engine systems.
             */
            WP_CLASS_REGISTER_DECL;
        };

    }  // namespace ui
}  // namespace workphone

#endif  // UIButton_h__
